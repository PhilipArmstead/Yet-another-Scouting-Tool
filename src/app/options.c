// SPDX-FileCopyrightText: © 2026 Phil Armstead <philarmstead@mailbox.org>
// SPDX-License-Identifier: GPL-3.0-or-later

#include "options.h"
#include "app/player.h"
#include "app/types.h"
#include "app/helpers/file-watcher.h"
#include "app/helpers/vector.h"
#include "core/logger.h"
#include "platform/platform.h"
#include "types/position-code-names.h"

#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <math.h>


#define OPTIONS_FILE_NAME "options.yaml"
#define OPTIONS_PATH_BUFFER_SIZE (4096 - 12)
#define OPTIONS_LINE_BUFFER_SIZE 512

static const char *attributeNames[ATTRIBUTE_COUNT] = {
	"Crossing",
	"Dribbling",
	"Finishing",
	"Heading",
	"LongShots",
	"Marking",
	"OffTheBall",
	"Passing",
	"PenaltyTaking",
	"Tackling",
	"Vision",
	"Handling",
	"AerialReach",
	"CommandOfArea",
	"Communication",
	"Kicking",
	"Throwing",
	"Anticipation",
	"Decisions",
	"OneOnOnes",
	"Positioning",
	"Reflexes",
	"FirstTouch",
	"Technique",
	"LeftFoot",
	"RightFoot",
	"Flair",
	"CornerTaking",
	"Teamwork",
	"WorkRate",
	"LongThrows",
	"Eccentricity",
	"RushingOut",
	"Punching",
	"Acceleration",
	"FreeKickTaking",
	"Strength",
	"Stamina",
	"Pace",
	"JumpingReach",
	"Leadership",
	"Dirtiness",
	"Balance",
	"Bravery",
	"Consistency",
	"Aggression",
	"Agility",
	"ImportantMatches",
	"InjuryProneness",
	"Versatility",
	"NaturalFitness",
	"Determination",
	"Composure",
	"Concentration",
	"Adaptability",
	"Ambition",
	"Loyalty",
	"Pressure",
	"Professionalism",
	"Sportsmanship",
	"Temperament",
	"Controversy",
};

static const char *positionGroupedCodes[POSITION_GROUPED_COUNT + 1] = {
	"GK",
	"FB",
	"CB",
	"WB",
	"DM",
	"MC",
	"W",
	"AM",
	"ST",
	"General",
};

extern GameContext gameContext;

// Resolved once at startup and reused by the watcher; empty if it could not be resolved.
static char optionsPath[4096];
static FileWatcher *optionsWatcher;
static char optionsError[256];

static void parseOptionsFile(FILE *file);
static void watchOptionsFile(void);

typedef struct {
	Formation current;
	bool haveCurrent;
	bool positionsSeen;
	bool positionsValid;
	uint8_t positionCount;
} FormationParser;

typedef struct {
	PositionWeights current;
	bool haveCurrent;
	bool inWeights;
} RatingsParser;


static char *trimLeadingWithIndentCount(char *text, uint8_t *indentLevel) {
	while (true) {
		if (*text == ' ') {
			++*indentLevel;
			++text;
		} else if (*text == '\t') {
			*indentLevel = *indentLevel + 2;
			++text;
		} else {
			break;
		}
	}
	return text;
}

static char *trimLeading(char *text) {
	while (*text == ' ' || *text == '\t') {
		++text;
	}
	return text;
}

static void trimTrailing(char *text) {
	size_t length = strlen(text);
	while (length > 0) {
		const char c = text[length - 1];
		if (c != ' ' && c != '\t' && c != '\r' && c != '\n') {
			break;
		}
		text[--length] = '\0';
	}
}

static bool equalsIgnoreCase(const char *a, const char *b) {
	while (*a && *b) {
		int ca = (unsigned char)*a;
		int cb = (unsigned char)*b;
		if (ca >= 'A' && ca <= 'Z') {
			ca += 'a' - 'A';
		}
		if (cb >= 'A' && cb <= 'Z') {
			cb += 'a' - 'A';
		}
		if (ca != cb) {
			return false;
		}
		++a;
		++b;
	}
	return *a == *b;
}

static bool parseBool(const char *value, bool *out) {
	if (
		equalsIgnoreCase(value, "true") ||
		equalsIgnoreCase(value, "yes") ||
		equalsIgnoreCase(value, "1")
	) {
		*out = true;
		return true;
	}

	if (
		equalsIgnoreCase(value, "false") ||
		equalsIgnoreCase(value, "no") ||
		equalsIgnoreCase(value, "0")
	) {
		*out = false;
		return true;
	}

	return false;
}

// Position codes are matched case-insensitively ("GK" and "gk" both work).
static bool positionCodeFromString(const char *token, PositionCode *out) {
	for (int i = 0; i < POSITION_CODE_COUNT; ++i) {
		if (equalsIgnoreCase(token, positionCodeNames[i])) {
			*out = (PositionCode)i;
			return true;
		}
	}
	return false;
}

// Copies a formation name, stripping a single pair of surrounding double quotes if present.
static void copyFormationName(char *destination, const char *value) {
	const size_t length = strlen(value);
	if (length >= 2 && value[0] == '"' && value[length - 1] == '"') {
		snprintf(destination, FORMATION_NAME_LENGTH, "%.*s", (int)(length - 2), value + 1);
	} else {
		snprintf(destination, FORMATION_NAME_LENGTH, "%s", value);
	}
}

// Validates the accumulated formation and, if valid, appends it to the options.
static void formationParser_finalise(FormationParser *parser) {
	if (!parser->haveCurrent) {
		return;
	}

	Options *options = &gameContext.options;
	if (parser->current.name[0] == '\0') {
		LOG_WARN("Skipping formation with no name");
	} else if (!parser->positionsSeen || !parser->positionsValid ||
		parser->positionCount != FORMATION_POSITION_COUNT) {
		LOG_WARN(
			"Skipping formation '%s': expected %d valid positions",
			parser->current.name,
			FORMATION_POSITION_COUNT
		);
	} else {
		vector_push(options->formations, parser->current);
	}

	*parser = (FormationParser){0};
}

static void ratingsParser_finalise(RatingsParser *parser) {
	if (!parser->haveCurrent) {
		return;
	}

	Options *options = &gameContext.options;
	vector_push(options->weights, parser->current);
	*parser = (RatingsParser){0};
}

static void formationParser_begin(FormationParser *parser) {
	formationParser_finalise(parser);
	*parser = (FormationParser){0};
	parser->haveCurrent = true;
}

static void ratingsParser_begin(RatingsParser *parser) {
	ratingsParser_finalise(parser);
	*parser = (RatingsParser){0};
	parser->haveCurrent = true;
}

static void formationParser_positions(FormationParser *parser, char *value) {
	parser->positionsSeen = true;
	parser->positionsValid = true;
	parser->positionCount = 0;

	char *token = strtok(value, " \t");
	while (token) {
		PositionCode code = POSITION_CODE_GK;
		if (!positionCodeFromString(token, &code)) {
			LOG_WARN("Unknown position code '%s' in formation '%s'", token, parser->current.name);
			parser->positionsValid = false;
		} else if (parser->positionCount < FORMATION_POSITION_COUNT) {
			parser->current.positions[parser->positionCount] = code;
		}

		if (parser->positionCount < UINT8_MAX) {
			++parser->positionCount;
		}
		token = strtok(NULL, " \t");
	}
}

static void formationParser_weight(RatingsParser *parser, const char *key, const char *value) {
	for (uint8_t i = 0; i < ATTRIBUTE_COUNT; ++i) {
		if (equalsIgnoreCase(key, attributeNames[i])) {
			char *end;
			const float weight = strtof(value, &end);
			if (end == value || *end || !isfinite(weight)) {
				LOG_WARN("Invalid weight '%s' for %s", value, key);
				return;
			}
			vector_push(parser->current.weights, ((RatingWeight){.attribute = i, .weight = weight}));
			return;
		}
	}
}

static inline PositionGrouped readRatingsPosition(const char *value) {
	for (PositionGrouped i = 0; i < POSITION_GROUPED_COUNT; ++i) {
		if (equalsIgnoreCase(value, positionGroupedCodes[i])) {
			return i;
		}
	}
	return POSITION_GROUPED_COUNT;
}

static void formationParser_keyValue(FormationParser *parser, const char *key, char *value) {
	if (!parser->haveCurrent) {
		LOG_WARN("Ignoring formation field '%s' outside a list item", key);
		return;
	}

	if (!strcmp(key, "name")) {
		copyFormationName(parser->current.name, value);
		return;
	}

	if (!strcmp(key, "positions")) {
		formationParser_positions(parser, value);
		return;
	}

	LOG_WARN("Unrecognised formation field '%s'", key);
}

static void ratingsParser_keyValue(RatingsParser *parser, const char *key, char *value) {
	if (!parser->haveCurrent) {
		LOG_WARN("Ignoring ratings field '%s' outside a list item", key);
		return;
	}

	if (!strcmp(key, "position")) {
		parser->current.position = readRatingsPosition(value);
		parser->inWeights = false;
		return;
	}

	if (!strcmp(key, "scale")) {
		char *end;
		errno = 0;
		float scale = strtof(value, &end);
		if (errno || end == value || *end || !isfinite(scale)) LOG_WARN("Invalid rating scale '%s'", value);
		else parser->current.scale = scale;
		parser->inWeights = false;
		return;
	}

	if (parser->inWeights) {
		formationParser_weight(parser, key, value);
		return;
	}

	if (!strcmp(key, "weights")) {
		parser->inWeights = true;
		return;
	}

	LOG_WARN("Unrecognised ratings field '%s'", key);
}

// Handles one indented line inside a `formations:` block.
static void formationParser_line(FormationParser *parser, char *cursor) {
	if (cursor[0] == '-') {
		formationParser_begin(parser);
		cursor = trimLeading(cursor + 1);
		if (*cursor == '\0') {
			return;
		}
	}

	char *separator = strchr(cursor, ':');
	if (!separator) {
		LOG_WARN("Ignoring malformed formation entry '%s'", cursor);
		return;
	}

	*separator = '\0';
	char *key = cursor;
	char *value = trimLeading(separator + 1);
	trimTrailing(key);
	trimTrailing(value);
	formationParser_keyValue(parser, key, value);
}


// Handles one indented line inside a `ratings:` block.
static void ratingsParser_line(RatingsParser *parser, char *cursor) {
	if (cursor[0] == '-') {
		if (!parser->inWeights) {
			ratingsParser_begin(parser);
		}
		cursor = trimLeading(cursor + 1);
		if (*cursor == '\0') {
			return;
		}
	}

	char *separator = strchr(cursor, ':');
	if (!separator) {
		LOG_WARN("Ignoring malformed ratings entry '%s'", cursor);
		return;
	}

	*separator = '\0';
	char *key = cursor;
	char *value = trimLeading(separator + 1);
	trimTrailing(key);
	trimTrailing(value);
	ratingsParser_keyValue(parser, key, value);
}

static void applyOption(const char *key, const char *value) {
	if (!strcmp(key, "dark-mode")) {
		bool darkMode = false;
		if (parseBool(value, &darkMode)) {
			gameContext.options.darkMode = darkMode;
		} else {
			LOG_WARN("Invalid boolean value '%s' for option '%s'", value, key);
		}
		return;
	}

	LOG_WARN("Unrecognised option key '%s'", key);
}

static void freeWeights(void) {
	Options *options = &gameContext.options;
	for (uint64_t i = 0; i < vector_length(options->weights); ++i) {
		vector_free(options->weights[i].weights);
	}
	vector_free(options->weights);
}

void options_defaults(void) {
	Options *options = &gameContext.options;
	freeWeights();
	vector_free(options->formations);
	memset(options, 0, sizeof(*options));
	options->darkMode = false;

	vector_push(
		options->formations,
		((Formation){
			.name = "4-4-2",
			.positions = {
			POSITION_CODE_GK,
			POSITION_CODE_DL,
			POSITION_CODE_DC,
			POSITION_CODE_DC,
			POSITION_CODE_DR,
			POSITION_CODE_ML,
			POSITION_CODE_MC,
			POSITION_CODE_MC,
			POSITION_CODE_MR,
			POSITION_CODE_ST,
			POSITION_CODE_ST,
			},
			})
	);
	vector_push(
		options->formations,
		((Formation){
			.name = "4-3-3",
			.positions = {
			POSITION_CODE_GK,
			POSITION_CODE_DL,
			POSITION_CODE_DC,
			POSITION_CODE_DC,
			POSITION_CODE_DR,
			POSITION_CODE_MC,
			POSITION_CODE_MC,
			POSITION_CODE_MC,
			POSITION_CODE_AML,
			POSITION_CODE_AMR,
			POSITION_CODE_ST,
			},
			})
	);
	vector_push(
		options->formations,
		((Formation){
			.name = "4-2-3-1",
			.positions = {
			POSITION_CODE_GK,
			POSITION_CODE_DL,
			POSITION_CODE_DC,
			POSITION_CODE_DC,
			POSITION_CODE_DR,
			POSITION_CODE_DM,
			POSITION_CODE_DM,
			POSITION_CODE_AML,
			POSITION_CODE_AMC,
			POSITION_CODE_AMR,
			POSITION_CODE_ST,
			},
			})
	);
	vector_push(
		options->formations,
		((Formation){
			.name = "4-2-4 IF",
			.positions = {
			POSITION_CODE_GK,
			POSITION_CODE_DL,
			POSITION_CODE_DC,
			POSITION_CODE_DC,
			POSITION_CODE_DR,
			POSITION_CODE_DM,
			POSITION_CODE_DM,
			POSITION_CODE_AML,
			POSITION_CODE_AMR,
			POSITION_CODE_ST,
			POSITION_CODE_ST,
			},
			})
	);

	vector_push(options->weights, ((PositionWeights){.position = POSITION_GROUPED_GK, .scale = 1.1f, .weights = NULL}));
	vector_push(
		options->weights,
		((PositionWeights){.position = POSITION_GROUPED_COUNT, .scale = 1.1f, .weights = NULL})
	);

	// Ref: https://fm-arena.com/find-comment/53835/
	vector_push(options->weights[0].weights, ((RatingWeight){.attribute = ATTR_DET, .weight = 20}));
	vector_push(options->weights[0].weights, ((RatingWeight){.attribute = ATTR_CON, .weight = 18.53f}));
	vector_push(options->weights[0].weights, ((RatingWeight){.attribute = ATTR_REF, .weight = 19.35f}));
	vector_push(options->weights[0].weights, ((RatingWeight){.attribute = ATTR_AER, .weight = 6.45f}));
	vector_push(options->weights[0].weights, ((RatingWeight){.attribute = ATTR_AGI, .weight = 7.35f}));
	vector_push(options->weights[0].weights, ((RatingWeight){.attribute = ATTR_PAC, .weight = 6.37f}));
	vector_push(options->weights[0].weights, ((RatingWeight){.attribute = ATTR_ACC, .weight = 3.38f}));
	vector_push(options->weights[0].weights, ((RatingWeight){.attribute = ATTR_INJ, .weight = -11.03f}));
	vector_push(options->weights[0].weights, ((RatingWeight){.attribute = ATTR_DIR, .weight = -3.68f}));
	vector_push(options->weights[0].weights, ((RatingWeight){.attribute = ATTR_FIR, .weight = 3.83f}));
	vector_push(options->weights[0].weights, ((RatingWeight){.attribute = ATTR_WOR, .weight = 8.7f}));
	vector_push(options->weights[0].weights, ((RatingWeight){.attribute = ATTR_JUM, .weight = 3.22f}));
	vector_push(options->weights[0].weights, ((RatingWeight){.attribute = ATTR_STA, .weight = 7.35f}));
	vector_push(options->weights[0].weights, ((RatingWeight){.attribute = ATTR_TEC, .weight = 3.53f}));
	vector_push(options->weights[0].weights, ((RatingWeight){.attribute = ATTR_FLA, .weight = 11.03f}));
	vector_push(options->weights[0].weights, ((RatingWeight){.attribute = ATTR_COM, .weight = 3.53f}));
	vector_push(options->weights[0].weights, ((RatingWeight){.attribute = ATTR_PRE, .weight = 3.38f}));
	vector_push(options->weights[0].weights, ((RatingWeight){.attribute = ATTR_PRO, .weight = 2.2f}));
	vector_push(options->weights[0].weights, ((RatingWeight){.attribute = ATTR_NAT, .weight = 2.35f}));

	vector_push(options->weights[1].weights, ((RatingWeight){.attribute = ATTR_PAC, .weight = 20}));
	vector_push(options->weights[1].weights, ((RatingWeight){.attribute = ATTR_ACC, .weight = 19.26f}));
	vector_push(options->weights[1].weights, ((RatingWeight){.attribute = ATTR_JUM, .weight = 9.15f}));
	vector_push(options->weights[1].weights, ((RatingWeight){.attribute = ATTR_DRI, .weight = 3.04f}));
	vector_push(options->weights[1].weights, ((RatingWeight){.attribute = ATTR_BAL, .weight = 3.03f}));
	vector_push(options->weights[1].weights, ((RatingWeight){.attribute = ATTR_CON, .weight = 9.57f}));
	vector_push(options->weights[1].weights, ((RatingWeight){.attribute = ATTR_ANT, .weight = 8.51f}));
	vector_push(options->weights[1].weights, ((RatingWeight){.attribute = ATTR_DET, .weight = 9.25f}));
	vector_push(options->weights[1].weights, ((RatingWeight){.attribute = ATTR_AGI, .weight = 3.40f}));
	vector_push(options->weights[1].weights, ((RatingWeight){.attribute = ATTR_STA, .weight = 10.64f}));
	vector_push(options->weights[1].weights, ((RatingWeight){.attribute = ATTR_DIR, .weight = 6.8f}));
	vector_push(options->weights[1].weights, ((RatingWeight){.attribute = ATTR_CMP, .weight = 5.53f}));
	vector_push(options->weights[1].weights, ((RatingWeight){.attribute = ATTR_WOR, .weight = 14.15f}));
	vector_push(options->weights[1].weights, ((RatingWeight){.attribute = ATTR_PRE, .weight = 3.62f}));
	vector_push(options->weights[1].weights, ((RatingWeight){.attribute = ATTR_INJ, .weight = -4.8f}));
}

// Writes the current in-memory defaults to a new options file so the user has one to edit.
// The output is intentionally in the same form the parser reads back, so it round-trips.
static bool writeDefaultOptions(const char *path) {
	FILE *file = platform_fopen(path, "wx");
	if (!file) {
		LOG_WARN("Could not create default options file '%s'; continuing with in-memory defaults", path);
		return false;
	}

	const Options *options = &gameContext.options;

	fputs("# Yet another Scouting Tool options\n", file);
	fprintf(file, "dark-mode: %s\n", options->darkMode ? "true" : "false");

	fputs("formations:\n", file);
	for (uint64_t i = 0; i < vector_length(options->formations); ++i) {
		fprintf(file, "  - name: \"%s\"\n", options->formations[i].name);
		fputs("    positions:", file);
		for (uint8_t j = 0; j < FORMATION_POSITION_COUNT; ++j) {
			fprintf(file, " %s", positionCodeNames[options->formations[i].positions[j]]);
		}
		fputc('\n', file);
	}

	fputs("ratings:\n", file);
	for (uint64_t i = 0; i < vector_length(options->weights); ++i) {
		fprintf(file, "  - position: %s\n", positionGroupedCodes[options->weights[i].position]);
		fprintf(file, "    scale: %.2f\n", options->weights[i].scale);
		fputs("    weights:\n", file);
		bool isFirstAttribute = true;
		for (uint64_t j = 0; j < vector_length(options->weights[i].weights); ++j) {
			const float weight = options->weights[i].weights[j].weight;
			const uint8_t attribute = options->weights[i].weights[j].attribute;
			if (weight != 0) {
				if (isFirstAttribute) {
					fprintf(file, "      - %s: %.2f\n", attributeNames[attribute], weight);
					isFirstAttribute = false;
				} else {
					fprintf(file, "        %s: %.2f\n", attributeNames[attribute], weight);
				}
			}
		}
	}

	bool written = !ferror(file);
	if (fclose(file) != 0) written = false;
	if (!written) {
		LOG_ERROR("Could not finish default options file '%s': %s", path, strerror(errno));
		if (!platform_removeFile(path)) LOG_ERROR("Cannot remove incomplete options file '%s'", path);
		return false;
	}

	LOG_INFO("Created default options file '%s'", path);
	return true;
}

bool options_init(void) {
	options_defaults();
	snprintf(optionsError, sizeof(optionsError), "Options could not be loaded or watched. See the log.");

	char directory[OPTIONS_PATH_BUFFER_SIZE];
	if (!platform_getUserDirectory(directory, sizeof(directory), false)) return false;

	snprintf(optionsPath, sizeof(optionsPath), "%s%s", directory, OPTIONS_FILE_NAME);

	FILE *file = platform_fopen(optionsPath, "r");
	if (!file) {
		if (errno != ENOENT) {
			LOG_ERROR("Cannot open options file '%s': %s", optionsPath, strerror(errno));
			return false;
		}
		char legacy[4096];
		platform_getExecutableDirectory(directory, sizeof(directory));
		snprintf(legacy, sizeof(legacy), "%s%s", directory, OPTIONS_FILE_NAME);
		FILE *old = directory[0] ? platform_fopen(legacy, "rb") : NULL;
		if (old) {
			FILE *copy = platform_fopen(optionsPath, "wbx");
			if (!copy) {
				fclose(old);
				LOG_ERROR("Cannot import legacy options to '%s'", optionsPath);
				return false;
			}
			char bytes[4096];
			size_t count;
			bool copied = true;
			while ((count = fread(bytes, 1, sizeof(bytes), old)) != 0) {
				if (fwrite(bytes, 1, count, copy) != count) { copied = false; break; }
			}
			copied = !ferror(old) && copied;
			if (fclose(copy) != 0) copied = false;
			fclose(old);
			if (!copied) {
				if (!platform_removeFile(optionsPath)) LOG_ERROR("Cannot remove incomplete import '%s'", optionsPath);
				LOG_ERROR("Legacy options import failed; original '%s' is unchanged", legacy);
				return false;
			}
			LOG_INFO("Imported options from '%s'", legacy);
		} else if (directory[0] && errno != ENOENT) {
			LOG_ERROR("Cannot read legacy options '%s': %s", legacy, strerror(errno));
			return false;
		} else {
			if (!writeDefaultOptions(optionsPath)) return false;
		}
		file = platform_fopen(optionsPath, "r");
		if (!file) return false;
	}

	parseOptionsFile(file);
	bool read = !ferror(file);
	if (fclose(file) != 0) read = false;
	if (!read) {
		LOG_ERROR("Could not read options file '%s'", optionsPath);
		options_defaults();
		return false;
	}

	LOG_DEBUG("Loaded options from '%s'", optionsPath);

	watchOptionsFile();
	if (optionsWatcher) optionsError[0] = '\0';
	return optionsWatcher != NULL;
}

// Re-reads the options file from scratch, falling back to defaults for anything it no longer
// defines, then refreshes every open view.
static void reloadOptions(void *userData) {
	(void)userData;

	FILE *file = platform_fopen(optionsPath, "r");
	if (!file) {
		LOG_WARN("Cannot read options file '%s': %s; keeping current options", optionsPath, strerror(errno));
		snprintf(optionsError, sizeof(optionsError), "Options could not be read; keeping current settings. See the log.");
		return;
	}

	Options previous = gameContext.options;
	gameContext.options = (Options){0};
	options_defaults();
	parseOptionsFile(file);
	bool read = !ferror(file);
	if (fclose(file) != 0) read = false;
	if (!read) {
		freeWeights();
		vector_free(gameContext.options.formations);
		gameContext.options = previous;
		LOG_ERROR("Could not finish reading options '%s'; keeping current settings", optionsPath);
		snprintf(optionsError, sizeof(optionsError), "Options read failed; keeping current settings. See the log.");
		return;
	}
	for (size_t i = 0; i < vector_length(previous.weights); ++i) vector_free(previous.weights[i].weights);
	vector_free(previous.weights);
	vector_free(previous.formations);
	optionsError[0] = '\0';

	LOG_INFO("Reloaded options from '%s'", optionsPath);

	// Recalculate player ratings in case the option file's weights have changed
	for (uint64_t i = 0; i < gameContext.playerCount; ++i) {
		getSortedPositionRatings(&gameContext.players[i]);
	}

	++gameContext.optionsRevision;
}

void options_update(void) {
	if (optionsWatcher && !fileWatcher_poll(optionsWatcher))
		snprintf(optionsError, sizeof(optionsError), "Options hot reload stopped. Restart the app; see the log.");
}

const char *options_error(void) {
	return optionsError;
}
const char *options_getPath(void) {
	return optionsPath;
}
static void watchOptionsFile(void) {
	if (optionsWatcher) {
		return;
	}
	optionsWatcher = fileWatcher_create(optionsPath, reloadOptions, NULL);
}

void options_shutdown(void) {
	fileWatcher_destroy(optionsWatcher);
	optionsWatcher = NULL;
	freeWeights();
	vector_free(gameContext.options.formations);
}

// Parses an already-opened options file into `gameContext.options`, overriding the defaults.
static void parseOptionsFile(FILE *file) {
	FormationParser formationParser = {0};
	RatingsParser ratingsParser = {0};
	bool inFormations = false;
	bool inRatings = false;

	char line[OPTIONS_LINE_BUFFER_SIZE];
	while (fgets(line, sizeof(line), file)) {
		const bool indented = line[0] == ' ' || line[0] == '\t';
		uint8_t indentLevel = 0;
		char *cursor = trimLeadingWithIndentCount(line, &indentLevel);
		if (inRatings && indentLevel < 6) {
			ratingsParser.inWeights = false;
		}
		trimTrailing(cursor);

		// Skip blank lines and comments without ending an open formations block.
		if (*cursor == '\0' || *cursor == '#') {
			continue;
		}

		// TODO: Change format from space-separated string to YAML array
		if (inFormations) {
			if (indented) {
				formationParser_line(&formationParser, cursor);
				continue;
			}

			// A non-indented line ends the block; fall through to handle it as a top-level key.
			formationParser_finalise(&formationParser);
			inFormations = false;
		}

		if (inRatings) {
			if (indented) {
				ratingsParser_line(&ratingsParser, cursor);
				continue;
			}

			// A non-indented line ends the block; fall through to handle it as a top-level key.
			ratingsParser_finalise(&ratingsParser);
			inRatings = false;
		}

		char *separator = strchr(cursor, ':');
		if (!separator) {
			LOG_WARN("Ignoring malformed options line '%s'", cursor);
			continue;
		}

		*separator = '\0';
		char *key = cursor;
		char *value = trimLeading(separator + 1);
		trimTrailing(key);
		trimTrailing(value);

		if (*key == '\0') {
			continue;
		}

		if (!strcmp(key, "formations")) {
			vector_free(gameContext.options.formations);
			formationParser = (FormationParser){0};
			inFormations = true;
			if (*value != '\0') {
				LOG_WARN("Ignoring inline value for 'formations'");
			}
			continue;
		}

		if (!strcmp(key, "ratings")) {
			freeWeights();
			inRatings = true;
			if (*value != '\0') {
				LOG_WARN("Ignoring inline value for 'ratings'");
			}
			continue;
		}

		applyOption(key, value);
	}

	formationParser_finalise(&formationParser);
	ratingsParser_finalise(&ratingsParser);
}
