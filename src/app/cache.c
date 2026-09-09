// SPDX-FileCopyrightText: © 2026 Phil Armstead <philarmstead@mailbox.org>
// SPDX-License-Identifier: GPL-3.0-or-later

#include "cache.h"
#include "app/callbacks.h"
#include "app/config.h"
#include "app/maths.h"
#include "app/mocks.h"
#include "app/player.h"
#include "app/helpers/formatter.h"
#include "core/logger.h"
#include "platform/platform.h"

#include <stdlib.h>
#include <string.h>

#include "ui.h"


extern ProcessContext processContext;
extern GameContext gameContext;

static GThread *threads[THREAD_COUNT];
static gint cacheInProgress = false;
static uint8_t completedPlayerThreads = 0;
static gboolean onThreadComplete(gpointer userData);
static void cachePlayers(uint8_t workerIndex);
static void cacheClubs(void);
static void cacheNations(void);

static gpointer threadFunction(gpointer arg) {
	const uint8_t functionIndex = (uint8_t)(intptr_t)arg;

	if (functionIndex == 1) {
		cacheNations();
	} else if (functionIndex == 2) {
		cacheClubs();
	} else {
		cachePlayers(functionIndex - (NON_PLAYERS_THREAD_COUNT + 1));
		g_idle_add(onThreadComplete, arg);
	}

	return NULL;
}

void cache_clear(void) {
	g_atomic_int_set(&cacheInProgress, false);

	for (uint8_t i = 0; i < THREAD_COUNT; i++) {
		if (threads[i] != NULL) {
			g_thread_join(threads[i]);
			threads[i] = NULL;
		}
	}

	completedPlayerThreads = 0;

	if (gameContext.clubs != NULL) {
		free(gameContext.clubs);
		gameContext.clubs = NULL;
		gameContext.clubCount = 0;
	}
	if (gameContext.nations != NULL) {
		free(gameContext.nations);
		gameContext.nations = NULL;
		gameContext.nationCount = 0;
	}
	if (gameContext.players != NULL) {
		free(gameContext.players);
		gameContext.players = NULL;
		gameContext.playerCount = 0;
	}
}


static void cacheNations(void) {
	const int64_t timeStart = platform_getMicroseconds();

#ifndef MOCKS_MODE
	uint8_t bytes[8];
	readFromMemory(processContext.handle, processContext.moduleBaseAddress + NATION_LIST_PTR_BASE, 8, bytes);
	readFromMemory(processContext.handle, hexBytesToInt(bytes, 8) + NATION_LIST_PTR_BASE_OFFSET, 8, bytes);

	uint8_t nationStartBuffer[8];
	uint8_t nationEndBuffer[8];
	readFromMemory(processContext.handle, hexBytesToInt(bytes, 8) + NATION_LIST_START, 8, nationStartBuffer);
	readFromMemory(processContext.handle, hexBytesToInt(bytes, 8) + NATION_LIST_END, 8, nationEndBuffer);
	const uint64_t nationStart = hexBytesToInt(nationStartBuffer, 8);
	const uint64_t nationEnd = hexBytesToInt(nationEndBuffer, 8);
	const uint64_t nationCount = (nationEnd - nationStart) / NATION_LIST_STRIDE;
	gameContext.nationCount = nationCount;
	gameContext.nations = calloc(nationCount, sizeof(Nation));
	for (uint64_t i = 0; i < nationCount; i++) {
		if (!g_atomic_int_get(&cacheInProgress)) {
			LOG_DEBUG("Ending nations cache early");
			return;
		}

		uint8_t nationBuffer[8];
		readFromMemory(processContext.handle, nationStart + i * NATION_LIST_STRIDE, 8, nationBuffer);
		readFromMemory(processContext.handle, hexBytesToInt(nationBuffer, 8) + NATION_OFFSET_NAME, 8, bytes);
		const uint64_t nameAddress = hexBytesToInt(bytes, 8);
		if (!nameAddress) {
			LOG_WARN("Nation %llu has no name pointer", i);
			return;
		}
		readFromMemory(
			processContext.handle,
			nameAddress + STRING_OFFSET_VALUE,
			MAX_NATION_STRING_LENGTH,
			(uint8_t*)gameContext.nations[i].name
		);
		readFromMemory(processContext.handle, hexBytesToInt(nationBuffer, 8) + NATION_OFFSET_NAME_CODE, 8, bytes);
		readFromMemory(
			processContext.handle,
			hexBytesToInt(bytes, 8) + STRING_OFFSET_VALUE,
			4,
			(uint8_t*)gameContext.nations[i].code
		);
	}
#else
	const uint8_t nationCount = 251;
	gameContext.nationCount = nationCount;
	gameContext.nations = calloc(nationCount, sizeof(Nation));
	gameContext.nations[189] = PLAYER_BY_ID_NATION_1;
	gameContext.nations[170] = PLAYER_BY_ID_NATION_2;
#endif

	const int64_t timeEnd = platform_getMicroseconds();
	LOG_DEBUG("Cached %d nations in %zu microseconds", nationCount, timeEnd - timeStart);
}

static void cacheClubs(void) {
	const int64_t timeStart = platform_getMicroseconds();

#ifndef MOCKS_MODE
	uint8_t bytes[8];
	readFromMemory(processContext.handle, processContext.moduleBaseAddress + CLUB_LIST_PTR_BASE, 4, bytes);
	readFromMemory(processContext.handle, hexBytesToInt(bytes, 4) + CLUB_LIST_PTR_BASE_OFFSET, 4, bytes);

	uint8_t clubStartBuffer[8];
	uint8_t clubEndBuffer[8];
	readFromMemory(processContext.handle, hexBytesToInt(bytes, 8) + CLUB_LIST_START, 8, clubStartBuffer);
	readFromMemory(processContext.handle, hexBytesToInt(bytes, 8) + CLUB_LIST_END, 8, clubEndBuffer);
	const uint64_t clubStart = hexBytesToInt(clubStartBuffer, 8);
	const uint64_t clubEnd = hexBytesToInt(clubEndBuffer, 8);
	const uint64_t clubCount = (clubEnd - clubStart) / CLUB_LIST_STRIDE;
	gameContext.clubCount = clubCount;
	gameContext.clubs = malloc(clubCount * sizeof(Club));
	uint64_t missed = 0;
	for (uint64_t i = 0; i < clubCount; i++) {
		if (!g_atomic_int_get(&cacheInProgress)) {
			LOG_DEBUG("Ending clubs cache early");
			return;
		}

		uint8_t clubBuffer[8];
		readFromMemory(processContext.handle, clubStart + i * CLUB_LIST_STRIDE, 8, clubBuffer);
		readFromMemory(processContext.handle, hexBytesToInt(clubBuffer, 8) + CLUB_OFFSET_NAME, 8, bytes);
		uint64_t namePointer = (uint32_t)hexBytesToInt(bytes, 8);
		if (!namePointer || !readFromMemory(
			processContext.handle,
			hexBytesToInt(bytes, 8) + STRING_OFFSET_VALUE,
			CLUB_LONG_NAME_LENGTH,
			(uint8_t*)gameContext.clubs[i - missed].name
		)) {
			missed++;
			continue;
		}

		readFromMemory(processContext.handle, hexBytesToInt(clubBuffer, 8) + CLUB_OFFSET_NAME_SHORT, 8, bytes);
		namePointer = (uint32_t)hexBytesToInt(bytes, 8);
		readFromMemory(
			processContext.handle,
			hexBytesToInt(bytes, 8) + STRING_OFFSET_VALUE,
			CLUB_SHORT_NAME_LENGTH,
			(uint8_t*)gameContext.clubs[i - missed].shortName
		);
	}

	gameContext.clubCount -= missed;
#else
	const uint32_t clubCount = 36289;
	gameContext.clubCount = clubCount;
	gameContext.clubs = malloc(clubCount * sizeof(Club));
	gameContext.clubs[1125] = PLAYER_BY_ID_CLUB;
#endif

	const int64_t timeEnd = platform_getMicroseconds();
	LOG_DEBUG("Cached %d clubs in %zu microseconds", gameContext.clubCount, timeEnd - timeStart);
}

static void cachePlayers(const uint8_t workerIndex) {
	const int64_t timeStart = platform_getMicroseconds();
	uint64_t cached = 0;

#ifndef MOCKS_MODE
	const uint64_t start = gameContext.playerCount * workerIndex / PLAYERS_THREAD_COUNT;
	const uint64_t end = gameContext.playerCount * (workerIndex + 1) / PLAYERS_THREAD_COUNT;

	uint8_t bytes[8];
	readFromMemory(processContext.handle, processContext.moduleBaseAddress + PLAYER_LIST_PTR_BASE, 8, bytes);
	const uint64_t playerStart = hexBytesToInt(bytes, 8);
	for (uint64_t i = start; i < end; i++) {
		if (!g_atomic_int_get(&cacheInProgress)) {
			LOG_DEBUG("Ending players cache early");
			return;
		}

		readFromMemory(processContext.handle, playerStart + i * PLAYER_LIST_STRIDE, 8, bytes);
		const uint64_t playerAddress = hexBytesToInt(bytes, 8);
		const uint64_t personAddress = getPersonAddressFromPlayerAddress(processContext.handle, playerAddress);
		const Player player = getPlayer(processContext.handle, true, personAddress, playerAddress);
		if (!player.uid) {
			continue;
		}
		gameContext.players[i] = player;
		++cached;
	}
#else
	const Player playerVini = PLAYER_VINI;
	const Player playerJeff = PLAYER_JEFF;
	const uint64_t start = (gameContext.playerCount * workerIndex) / THREAD_COUNT;
	const uint64_t end = (gameContext.playerCount * (workerIndex + 1)) / THREAD_COUNT;
	for (uint64_t i = start; i < end; i++) {
		memcpy(&gameContext.players[i], i & 1 ? &playerVini : &playerJeff, sizeof(Player));
	}
#endif

	const int64_t timeEnd = platform_getMicroseconds();
	LOG_DEBUG(
		"Cached %llu players in %llu microseconds",
		(unsigned long long)cached,
		(unsigned long long)(timeEnd - timeStart)
	);
}

void cache_run(void) {
	if (g_atomic_int_get(&cacheInProgress)) {
		return;
	}

	// Prepare player array for multithreaded writing
#ifndef MOCKS_MODE
	uint8_t bytes[8];
	readFromMemory(processContext.handle, processContext.moduleBaseAddress + PLAYER_LIST_PTR_BASE, 8, bytes);
	const uint64_t playerStart = hexBytesToInt(bytes, 8);
	readFromMemory(processContext.handle, processContext.moduleBaseAddress + PLAYER_LIST_PTR_BASE + 0x08, 8, bytes);
	const uint64_t playerEnd = hexBytesToInt(bytes, 8);
	const uint64_t playerCount = (playerEnd - playerStart) / 8;
#else
	const uint64_t playerCount = 900;
#endif

	gameContext.playerCount = playerCount;
	// calloc so that slots the workers skip stay zeroed (uid == 0) and are
	// recognisable to compactPlayers().
	gameContext.players = calloc(playerCount, sizeof(Player));
	if (gameContext.players == NULL) {
		LOG_ERROR("Failed to allocate memory for %llu players", (unsigned long long)playerCount);
		gameContext.playerCount = 0;
		return;
	}

	g_atomic_int_set(&cacheInProgress, true);
	completedPlayerThreads = 0;
	for (uint8_t i = 0; i < THREAD_COUNT; i++) {
		char buffer[12] = {0};
		snprintf(buffer, sizeof(buffer), "worker-%d", i);
		threads[i] = g_thread_new(buffer, threadFunction, (void*)(uintptr_t)(i + 1));
	}
}

static gboolean onThreadComplete(gpointer userData) {
	const uint8_t threadIndex = (uint8_t)userData;
	if (threadIndex >= (NON_PLAYERS_THREAD_COUNT + 1)) {
		++completedPlayerThreads;
	}

	// Back in main thread, update the UI after every player worker completes.
	if (completedPlayerThreads == PLAYERS_THREAD_COUNT) {
		g_atomic_int_set(&cacheInProgress, false);

		char buffer[8];
		snprintf(buffer, 8, "%llu", gameContext.playerCount);
		formatter_printNumber(buffer);
		char bufferStatus[32];
		snprintf(bufferStatus, sizeof(bufferStatus), "%s players cached", buffer);
		ui_setCurrentStatus(bufferStatus);
	}

	return G_SOURCE_REMOVE;
}
