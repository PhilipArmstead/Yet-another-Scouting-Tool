// SPDX-FileCopyrightText: © 2026 Phil Armstead <philarmstead@mailbox.org>
// SPDX-License-Identifier: GPL-3.0-or-later

#include "options.h"
#include "app/types.h"
#include "app/helpers/vector.h"
#include "core/logger.h"
#include "platform/platform.h"

#include <stdio.h>
#include <string.h>


#define OPTIONS_FILE_NAME "options.yaml"
#define OPTIONS_PATH_BUFFER_SIZE 4096
#define OPTIONS_LINE_BUFFER_SIZE 512

extern GameContext gameContext;

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
		if (!strcasecmp(key, attributeNames[i])) {
			vector_push(parser->current.weights, ((RatingWeight){.attribute = i, .weight = strtof(value, NULL)}));
			return;
		}
	}
}

static inline PositionGrouped readRatingsPosition(const char *value) {
	for (PositionGrouped i = 0; i < POSITION_GROUPED_COUNT; ++i) {
		if (!strcasecmp(value, positionGroupedCodes[i])) {
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

static void setDefaults(void) {
	Options *options = &gameContext.options;
	memset(options, 0, sizeof(*options));
	options->darkMode = true;

	vector_push(options->formations, ((Formation){
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
	}));
	vector_push(options->formations, ((Formation){
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
	}));
	vector_push(options->formations, ((Formation){
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
	}));
	vector_push(options->formations, ((Formation){
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
	}));

	vector_push(options->weights, ((PositionWeights){.position = POSITION_GROUPED_GK, .scale = 1, .weights = NULL}));
	vector_push(options->weights, ((PositionWeights){.position = POSITION_GROUPED_COUNT, .scale = 1.05f, .weights = NULL}));

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
static void writeDefaultOptions(const char *path) {
	FILE *file = fopen(path, "w");
	if (!file) {
		LOG_WARN("Could not create default options file '%s'; continuing with in-memory defaults", path);
		return;
	}

	const Options *options = &gameContext.options;

	fputs("# FM Player Rater options\n", file);
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
	for (uint64_t i = 0; i <  vector_length(options->weights); ++i) {
		fprintf(file, "  - position: %s\n", positionGroupedCodes[options->weights[i].position]);
		fputs("    weights:\n", file);
		bool isFirstAttribute = true;
		for (uint64_t j = 0; j < vector_length(options->weights[i].weights); ++j) {
			const float weight = options->weights[i].weights[j].weight;
			const uint8_t attribute = options->weights[i].weights[j].attribute;
			if (weight > 0) {
				if (isFirstAttribute) {
					fprintf(file, "      - %s: %.2f\n", attributeNames[attribute], weight);
					isFirstAttribute = false;
				} else {
					fprintf(file, "        %s: %.2f\n", attributeNames[attribute], weight);
				}
			}
		}
	}

	fclose(file);

	LOG_INFO("Created default options file '%s'", path);
}

void options_init(void) {
	setDefaults();

	char directory[OPTIONS_PATH_BUFFER_SIZE];
	platform_getExecutableDirectory(directory, sizeof(directory));
	if (directory[0] == '\0') {
		LOG_WARN("Could not resolve executable directory; using default options");
		return;
	}

	char path[OPTIONS_PATH_BUFFER_SIZE];
	snprintf(path, sizeof(path), "%s%s", directory, OPTIONS_FILE_NAME);

	FILE *file = fopen(path, "r");
	if (!file) {
		LOG_INFO("Options file '%s' not found; writing defaults", path);
		writeDefaultOptions(path);
		return;
	}

	FormationParser formationParser = {0};
	RatingsParser ratingsParser = {0};
	bool inFormations = false;
	bool inRatings = false;

	char line[OPTIONS_LINE_BUFFER_SIZE];
	while (fgets(line, sizeof(line), file)) {
		const bool indented = line[0] == ' ' || line[0] == '\t';
		char *cursor = trimLeading(line);
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
			vector_free(gameContext.options.weights);
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

	fclose(file);

	LOG_DEBUG("Loaded options from '%s'", path);
}
