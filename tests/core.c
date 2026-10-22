// SPDX-License-Identifier: GPL-3.0-or-later
#include "app/analysis.h"
#include "app/autocomplete.h"
#include "app/cache.h"
#include "app/helpers/file-watcher.h"
#include "app/helpers/formatter.h"
#include "app/helpers/ratings.h"
#include "app/helpers/text.h"
#include "app/helpers/vector.h"
#include "app/injury-names.h"
#include "app/options.h"
#include "app/player-snapshot.h"
#include "app/player-table.h"
#include "app/player.h"
#include "app/search-handler.h"
#include "app/search.h"
#include "platform/platform.h"
#include "platform/thread.h"
#include <assert.h>
#include <math.h>
#include <stdatomic.h>
#include <stdio.h>

GameContext gameContext;
ProcessContext processContext;

static bool match(const char *query, const char *candidate) {
	char *q = text_fold(query, false), *normal = text_fold(candidate, false), *alternate = text_fold(candidate, true);
	assert(q && normal && alternate);
	bool result = text_matches(q, normal, alternate);
	free(q);
	free(normal);
	free(alternate);
	return result;
}

static void unicode(void) {
	assert(match("real", "Real Madrid Club de Fútbol"));
	assert(match("fut", "Real Madrid Club de Fútbol"));
	assert(match("madrid real", "Real Madrid Club de Fútbol"));
	assert(!match("drid", "Real Madrid"));
	assert(match("MUNCH", "Bayern München"));
	assert(!match("münch", "Bayern Munch"));
	assert(match("lodz", "Łódź"));
	assert(match("AE", "Ægir"));
	assert(match("STRASSE", "Straße"));
	assert(match("νικ", "Νίκος"));
	assert(match("алек", "Алексей"));
	const char *text = "a\xc3\xa9\xce\xbd";
	assert(text_next(text, 1) == 3 && text_previous(text, 3) == 1);
	assert(text_next(text, 3) == 5 && text_previous(text, 5) == 3);
	assert(text_next(text, 5) == 5 && text_previous(text, 0) == 0);
}

static void baseline(void) {
	options_defaults();
	playerTable_init();
	gameContext.playerCount = 100000;
	gameContext.players = calloc((size_t)gameContext.playerCount, sizeof(Player));
	assert(gameContext.players);
	for (uint32_t i = 0; i < gameContext.playerCount; ++i) {
		Player *p = &gameContext.players[i];
		p->uid = i + 1;
		p->age = (uint8_t)(16 + i % 30);
		p->ca = (uint8_t)(60 + i % 141);
		p->pa = 200;
		p->condition = 9000;
		p->nationality[0] = (uint8_t)(i % 4);
		p->nationality[1] = p->nationality[2] = p->nationality[3] = 255;
		snprintf(p->surname, sizeof(p->surname), "%06u", 100000 - i);
		memset(p->positions, 20, sizeof(p->positions));
		memset(p->attributes, 50 + (int)(i % 51), sizeof(p->attributes));
		getSortedPositionRatings(p);
	}
	gameContext.filterOptions = (FilterOptions){
		.filterMask = FILTER_HAS_MIN_AGE | FILTER_HAS_MAX_AGE | FILTER_HAS_MIN_CA | FILTER_HAS_NATIONALITY,
		.minAge = 18,
		.maxAge = 23,
		.minCA = 150,
		.nationalityIndex = 2};
	int64_t start = platform_getMicroseconds();
	search_findPlayers();
	printf("search_count=%zu search_us=%" PRId64 "\n", playerTable_count(), platform_getMicroseconds() - start);
	assert(playerTable_count() == 1809);
	BestElevenRow rows[11];
	BestElevenFilters filters = {.minAge = 14, .maxAge = 50, .maxCondition = 10000};
	start = platform_getMicroseconds();
	assert(analysis_bestEleven(gameContext.players, (uint32_t)gameContext.playerCount, gameContext.options.formations[0].positions, filters, rows));
	float total = 0;
	for (unsigned i = 0; i < 11; ++i) {
		assert(rows[i].player);
		for (unsigned j = 0; j < i; ++j)
			assert(rows[i].player != rows[j].player);
		total += rows[i].rating;
	}
	printf("best_xi_total=%.6f solver_us=%" PRId64 "\n", (double)total, platform_getMicroseconds() - start);
	assert(fabsf(total - 1206.672607f) < .002f);
	assert(fabsf(getRatingForPosition(&gameContext.players[50], POSITION_CODE_GK) - 105.309074f) < .001f);
	assert(fabsf(getRatingForPosition(&gameContext.players[50], POSITION_CODE_MC) - 110.136368f) < .001f);
	Player raw = gameContext.players[50];
	for (unsigned i = TRUE_ATTRIBUTE_COUNT; i < ATTRIBUTE_COUNT; ++i)
		raw.attributes[i] = 15;
	getSortedPositionRatings(&raw);
	const Rating expected = raw.ratings[0];
	player_finishRead(&raw);
	assert(raw.ratings[0].value == expected.value && raw.attributes[ATTR_PRO] == 75);
}

static float key(const Player *player, Column column) {
	switch (column) {
		case COLUMN_AGE: return player->age;
		case COLUMN_CA: return player->ca;
		case COLUMN_PA: return player->pa;
		case COLUMN_CA_PA_DELTA: return (float)player->pa - player->ca;
		default: return player->ratings[0].value;
	}
}

static void sorting(void) {
	const Column columns[] = {COLUMN_NAME, COLUMN_AGE, COLUMN_CA, COLUMN_PA, COLUMN_CA_PA_DELTA, COLUMN_RATING};
	int64_t start = platform_getMicroseconds();
	for (size_t c = 0; c < sizeof(columns) / sizeof(*columns); ++c) {
		for (unsigned direction = 0; direction < 2; ++direction) {
			playerTable_sort(columns[c], false);
			const int order = playerTable_direction(columns[c]);
			for (size_t i = 1; i < playerTable_count(); ++i) {
				const Player *a = playerTable_player(i - 1), *b = playerTable_player(i);
				if (columns[c] == COLUMN_NAME)
					assert(order * strcmp(a->surname, b->surname) <= 0);
				else
					assert((float)order * (key(a, columns[c]) - key(b, columns[c])) <= 0);
			}
		}
	}
	playerTable_sort(COLUMN_AGE, false);
	playerTable_sort(COLUMN_CA, true);
	assert(playerTable_direction(COLUMN_AGE) && playerTable_direction(COLUMN_CA));
	printf("sort_1809_all_keys_us=%" PRId64 "\n", platform_getMicroseconds() - start);
}

static void filters(void) {
	char error[128];
	gameContext.filterBuffer = (FilterBuffer){0};
	snprintf(gameContext.filterBuffer.minAge, 16, "18");
	snprintf(gameContext.filterBuffer.maxAge, 16, "23");
	assert(searchHandler_cacheFilters(error));
	FilterOptions valid = gameContext.filterOptions;
	const char *invalid[] = {"256", "-1", "2x", "99999999999999", "nan"};
	for (size_t i = 0; i < sizeof(invalid) / sizeof(*invalid); ++i) {
		snprintf(gameContext.filterBuffer.minAge, 16, "%s", invalid[i]);
		assert(!searchHandler_cacheFilters(error) && error[0]);
		assert(memcmp(&valid, &gameContext.filterOptions, sizeof(valid)) == 0);
	}
	snprintf(gameContext.filterBuffer.minAge, 16, "24");
	assert(!searchHandler_cacheFilters(error));
	gameContext.filterBuffer = (FilterBuffer){0};
	snprintf(gameContext.filterBuffer.minRating, 16, "inf");
	assert(!searchHandler_cacheFilters(error));
	snprintf(gameContext.filterBuffer.minRating, 16, "10.5");
	snprintf(gameContext.filterBuffer.maxRating, 16, "10");
	assert(!searchHandler_cacheFilters(error));
	snprintf(gameContext.filterBuffer.maxRating, 16, "10.5");
	assert(searchHandler_cacheFilters(error));
	assert(gameContext.filterOptions.minRating == gameContext.filterOptions.maxRating);
	searchHandler_clearFilters();
	assert(!gameContext.searchActive && !playerTable_count());
}

static void snapshots(void) {
	Player original = gameContext.players[0];
	PlayerSnapshot *snapshot = playerSnapshot_createOne(&original);
	assert(snapshot);
	PlayerLookup lookup = {0};
	gameContext.players[0].ca = 201;
	assert(playerLookup_build(&lookup));
	playerSnapshot_refresh(snapshot, &lookup);
	assert(snapshot->players[0].ca == 201);
	playerLookup_destroy(&lookup);
	gameContext.players[0].uid = 2000000;
	assert(playerLookup_build(&lookup));
	playerSnapshot_refresh(snapshot, &lookup);
	assert(snapshot->players[0].uid == original.uid && snapshot->players[0].ca == 201);
	playerLookup_destroy(&lookup);
	gameContext.players[0] = original;
	playerSnapshot_free(snapshot);
}

static void assignment(void) {
	Player players[30] = {0};
	for (unsigned i = 0; i < 30; ++i) {
		players[i] = gameContext.players[0];
		players[i].uid = 30 - i;
		memset(players[i].positions, 0, sizeof(players[i].positions));
		players[i].positions[POSITION_CODE_MC] = 20;
	}
	BestElevenRow depth[DEPTH_LIMIT];
	assert(analysis_depth(players, 30, POSITION_CODE_MC, depth) == 20);
	for (unsigned i = 0; i < 20; ++i)
		assert(depth[i].player->uid == i + 1);
	memset(players[0].attributes, 100, sizeof(players[0].attributes));
	getSortedPositionRatings(&players[0]);
	players[0].positions[POSITION_CODE_ST] = 20;
	getSortedPositionRatings(&players[0]);
	PositionCode positions[11] = {POSITION_CODE_MC, POSITION_CODE_ST};
	BestElevenRow rows[11];
	BestElevenFilters filters = {.minAge = 14, .maxAge = 50, .maxCondition = 10000, .excludeInjured = true};
	assert(analysis_bestEleven(players, 2, positions, filters, rows));
	assert(rows[0].player == &players[1] && rows[1].player == &players[0]);
	for (unsigned i = 2; i < 11; ++i)
		assert(!rows[i].player);
	players[0].injury.duration = 1;
	assert(analysis_bestEleven(players, 2, positions, filters, rows));
	assert(!rows[1].player);
	assert(analysis_bestEleven(NULL, 0, positions, filters, rows));
	for (unsigned i = 0; i < 11; ++i)
		assert(!rows[i].player);
}

static void waitSearch(Autocomplete *source) {
	const int64_t deadline = platform_getMicroseconds() + 3000000;
	do {
		autocomplete_poll(source);
		if (!source->job && !source->pending)
			break;
		thread_sleep(1);
	} while (platform_getMicroseconds() < deadline);
	assert(!source->job && !source->pending && !source->error[0]);
}

static void autocomplete(void) {
	Club clubs[] = {{.name = "Real Madrid Club de Fútbol", .shortName = "Madrid"},
									{.name = "Fußball Club Bayern München", .shortName = "Bayern München"},
									{.name = "Red United", .shortName = "Blue City"}};
	gameContext.clubs = clubs;
	gameContext.clubCount = 3;
	Autocomplete source = {0};
	assert(autocomplete_rebuild(&source, false));
	autocomplete_query(&source, "fut");
	waitSearch(&source);
	assert(source.count == 1 && source.matches[0].index == 0);
	autocomplete_query(&source, "red city");
	waitSearch(&source);
	assert(!source.count);
	autocomplete_query(&source, "mun");
	autocomplete_poll(&source);
	autocomplete_query(&source, "madrid");
	assert(autocomplete_rebuild(&source, false));
	waitSearch(&source);
	assert(source.count == 1 && source.matches[0].index == 0);
	autocomplete_query(&source, "bay");
	autocomplete_poll(&source);
	autocomplete_destroy(&source);
	gameContext.clubs = NULL;
	gameContext.clubCount = 0;
}

static void writeFile(const char *path) {
	FILE *file = platform_fopen(path, "wb");
	assert(file && fputs("options\n", file) >= 0 && fclose(file) == 0);
}

static void fileChanged(void *data) {
	++*(unsigned *)data;
}
static void settle(FileWatcher *watcher) {
	const int64_t end = platform_getMicroseconds() + 300000;
	while (platform_getMicroseconds() < end) {
		fileWatcher_poll(watcher);
		thread_sleep(5);
	}
}

static void watcher(void) {
	const char *path = TEST_DIRECTORY "/watch-\xce\xb4.yaml";
	const char *replacement = TEST_DIRECTORY "/watch-replacement.yaml";
	const char *unrelated = TEST_DIRECTORY "/watch-unrelated.yaml";
	writeFile(path);
	unsigned changes = 0;
	FileWatcher *watcher = fileWatcher_create(path, fileChanged, &changes);
	assert(watcher);
	fileWatcher_poll(watcher);
	for (unsigned i = 0; i < 4; ++i) {
		writeFile(path);
		fileWatcher_poll(watcher);
		thread_sleep(15);
	}
	settle(watcher);
	assert(changes == 1);
	writeFile(unrelated);
	settle(watcher);
	assert(changes == 1);
	writeFile(replacement);
	assert(platform_replaceFile(replacement, path));
	settle(watcher);
	assert(changes == 2);
	writeFile(path);
	settle(watcher);
	assert(changes == 3);
	assert(platform_removeFile(path));
	fileWatcher_poll(watcher);
	writeFile(path);
	settle(watcher);
	assert(changes == 4);
	fileWatcher_destroy(watcher);
	assert(platform_removeFile(path) && platform_removeFile(unrelated));
}

static void intern(void *data) {
	(void)data;
	for (unsigned i = 0; i < 1000; ++i) {
		uint16_t index = injuryNames_intern("Knee injury", 11);
		assert(index && !strcmp(injuryNames_get(index), "Knee injury"));
	}
}

static void workers(void) {
	Thread threads[8] = {0};
	for (unsigned i = 0; i < 8; ++i)
		assert(thread_start(&threads[i], intern, NULL));
	for (unsigned i = 0; i < 8; ++i)
		thread_join(&threads[i]);
	assert(injuryNames_intern("Knee", 4) != injuryNames_intern("Knee injury", 11));
#ifdef MOCKS_MODE
	gameContext.currentDate = (DateTime){.days = 210, .year = 2026};
	cache_run();
	cache_poll();
	cache_clear();
	cache_run();
	const int64_t deadline = platform_getMicroseconds() + 5000000;
	while (cache_isRunning() && platform_getMicroseconds() < deadline) {
		cache_poll();
		thread_sleep(1);
	}
	assert(!cache_isRunning() && !cache_error()[0] && gameContext.playerCount == 900);
	for (uint32_t i = 0; i < gameContext.playerCount; ++i)
		assert(gameContext.players[i].uid == i + 1);
	cache_run();
	cache_poll();
	cache_shutdown();
	assert(!gameContext.players && !gameContext.clubs && !gameContext.nations && !cache_isRunning());
#endif
}

static void settleOptions(void) {
	const int64_t end = platform_getMicroseconds() + 350000;
	while (platform_getMicroseconds() < end) {
		options_update();
		thread_sleep(5);
	}
}

int main(int argc, char **argv) {
	if (argc > 1 && !strcmp(argv[1], "--probe-options")) {
		const bool loaded = options_init();
		printf("options_loaded=%d dark=%d\n", loaded, gameContext.options.darkMode);
		if (loaded && argc > 2) {
			const bool dark = gameContext.options.darkMode;
			char replacement[4128];
			snprintf(replacement, sizeof(replacement), "%s.new", options_getPath());
			FILE *file = platform_fopen(replacement, "w");
			assert(file && fprintf(file, "dark-mode: %s\n", dark ? "false" : "true") > 0);
			assert(fclose(file) == 0 && platform_replaceFile(replacement, options_getPath()));
			settleOptions();
			assert(gameContext.optionsRevision == 1 && gameContext.options.darkMode != dark);
			assert(platform_removeFile(options_getPath()));
			settleOptions();
			assert(options_error()[0] && gameContext.options.darkMode != dark);
			file = platform_fopen(options_getPath(), "w");
			assert(file && fprintf(file, "dark-mode: %s\nformations:\n", dark ? "true" : "false") > 0);
			assert(fclose(file) == 0);
			settleOptions();
			assert(gameContext.optionsRevision == 2 && !options_error()[0]);
			assert(gameContext.options.darkMode == dark && !vector_length(gameContext.options.formations));
		}
		options_shutdown();
		return loaded ? 0 : 1;
	}
	assert(injuryNames_init());
	unicode();
	baseline();
	sorting();
	filters();
	snapshots();
	assignment();
	autocomplete();
	free(gameContext.players);
	gameContext.players = NULL;
	gameContext.playerCount = 0;
	watcher();
	workers();
	playerTable_shutdown();
	options_shutdown();
	injuryNames_destroy();
	puts("Core checks passed");
	return 0;
}
