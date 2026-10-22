#include "cache.h"
#include "app/configuration.h"
#include "app/injury-names.h"
#include "app/maths.h"
#include "app/mocks.h"
#include "app/player-table.h"
#include "app/player.h"
#include "app/search.h"
#include "core/logger.h"
#include "platform/platform.h"
#include "platform/thread.h"
#include <stdatomic.h>
#include <stdlib.h>
#include <string.h>

#define PLAYER_WORKERS 8
#define WORKER_COUNT (PLAYER_WORKERS + 2)

extern ProcessContext processContext;
extern GameContext gameContext;

typedef struct CacheJob CacheJob;
typedef struct {
	CacheJob *job;
	unsigned index;
	atomic_bool done;
	bool failed;
	Thread thread;
} Worker;

struct CacheJob {
	ProcessContext process;
	DateTime date;
	atomic_bool cancel;
	Worker workers[WORKER_COUNT];
	Player *players;
	Club *clubs;
	Nation *nations;
	uint64_t playerCount, clubCount, nationCount;
	uintptr_t playerStart;
	bool playersPublished, clubsPublished, nationsPublished;
};

static CacheJob *job;
static bool requested;
static char error[128];

#ifndef MOCKS_MODE
static bool readList(CacheJob *self, uintptr_t base, uintptr_t offset, uintptr_t startOffset,
										 uintptr_t endOffset, uint64_t stride, uintptr_t *start, uint64_t *count) {
	uint8_t bytes[8];
	const ProcessContext *p = &self->process;
	if (!readFromMemory(p->handle, p->moduleBaseAddress + base, 8, bytes))
		return false;
	uintptr_t list = (uintptr_t)hexBytesToInt(bytes, 8);
	if (offset) {
		if (!list || !readFromMemory(p->handle, list + offset, 8, bytes))
			return false;
		list = (uintptr_t)hexBytesToInt(bytes, 8);
	}
	if (!list || !readFromMemory(p->handle, list + startOffset, 8, bytes))
		return false;
	*start = (uintptr_t)hexBytesToInt(bytes, 8);
	if (!readFromMemory(p->handle, list + endOffset, 8, bytes))
		return false;
	const uintptr_t end = (uintptr_t)hexBytesToInt(bytes, 8);
	if (end < *start || (end - *start) % stride || (end - *start) / stride > 2000000)
		return false;
	*count = (end - *start) / stride;
	return true;
}
#endif

static void cacheNations(Worker *worker) {
	CacheJob *self = worker->job;
#ifdef MOCKS_MODE
	self->nationCount = 251;
#else
	uintptr_t start;
	if (!readList(self, NATION_LIST_PTR_BASE, NATION_LIST_PTR_BASE_OFFSET, NATION_LIST_START,
								NATION_LIST_END, NATION_LIST_STRIDE, &start, &self->nationCount)) {
		worker->failed = true;
		return;
	}
#endif
	self->nations = calloc((size_t)self->nationCount, sizeof(Nation));
	if (!self->nations) {
		worker->failed = self->nationCount != 0;
		return;
	}
#ifdef MOCKS_MODE
	self->nations[189] = PLAYER_BY_ID_NATION_1;
	self->nations[170] = PLAYER_BY_ID_NATION_2;
	self->nations[1] = (Nation){.name = "Germany", .code = "GER"};
	self->nations[2] = (Nation){.name = "Greece", .code = "GRE"};
#else
	for (uint64_t i = 0; i < self->nationCount && !atomic_load(&self->cancel); ++i) {
		uint8_t bytes[8];
		if (!readFromMemory(self->process.handle, start + i * NATION_LIST_STRIDE, 8, bytes))
			continue;
		const uintptr_t nation = (uintptr_t)hexBytesToInt(bytes, 8);
		readFromMemory(self->process.handle, nation + NATION_OFFSET_NAME, 8, bytes);
		readFromMemory(self->process.handle, (uintptr_t)hexBytesToInt(bytes, 8) + STRING_OFFSET_VALUE,
									 sizeof(self->nations[i].name) - 1, (uint8_t *)self->nations[i].name);
		readFromMemory(self->process.handle, nation + NATION_OFFSET_NAME_CODE, 8, bytes);
		readFromMemory(self->process.handle, (uintptr_t)hexBytesToInt(bytes, 8) + STRING_OFFSET_VALUE,
									 3, (uint8_t *)self->nations[i].code);
	}
#endif
}

static void cacheClubs(Worker *worker) {
	CacheJob *self = worker->job;
#ifdef MOCKS_MODE
	self->clubCount = 36289;
#else
	uintptr_t start;
	if (!readList(self, CLUB_LIST_PTR_BASE, CLUB_LIST_PTR_BASE_OFFSET, CLUB_LIST_START, CLUB_LIST_END,
								CLUB_LIST_STRIDE, &start, &self->clubCount)) {
		worker->failed = true;
		return;
	}
#endif
	self->clubs = calloc((size_t)self->clubCount, sizeof(Club));
	if (!self->clubs) {
		worker->failed = self->clubCount != 0;
		return;
	}
#ifdef MOCKS_MODE
	self->clubs[1125] = PLAYER_BY_ID_CLUB;
	self->clubs[1] = (Club){.name = "Futebol Clube do Porto", .shortName = "Porto"};
	self->clubs[2] = (Club){.name = "Fußball Club Bayern München", .shortName = "Bayern München"};
#else
	for (uint64_t i = 0; i < self->clubCount && !atomic_load(&self->cancel); ++i) {
		uint8_t bytes[8];
		if (!readFromMemory(self->process.handle, start + i * CLUB_LIST_STRIDE, 8, bytes))
			continue;
		const uintptr_t club = (uintptr_t)hexBytesToInt(bytes, 8);
		self->clubs[i].address = club;
		readFromMemory(self->process.handle, club + CLUB_OFFSET_NAME, 8, bytes);
		readFromMemory(self->process.handle, (uintptr_t)hexBytesToInt(bytes, 8) + STRING_OFFSET_VALUE,
									 sizeof(self->clubs[i].name) - 1, (uint8_t *)self->clubs[i].name);
		readFromMemory(self->process.handle, club + CLUB_OFFSET_NAME_SHORT, 8, bytes);
		readFromMemory(self->process.handle, (uintptr_t)hexBytesToInt(bytes, 8) + STRING_OFFSET_VALUE,
									 sizeof(self->clubs[i].shortName) - 1, (uint8_t *)self->clubs[i].shortName);
	}
#endif
}

static void cachePlayers(Worker *worker) {
	CacheJob *self = worker->job;
	const unsigned index = worker->index - 2;
	const uint64_t start = self->playerCount * index / PLAYER_WORKERS;
	const uint64_t end = self->playerCount * (index + 1) / PLAYER_WORKERS;
	for (uint64_t i = start; i < end && !atomic_load(&self->cancel); ++i) {
#ifdef MOCKS_MODE
		Player player = i % 3 == 0 ? PLAYER_GK : (i % 2 ? PLAYER_VINI : PLAYER_JEFF);
		player.uid = (uint32_t)i + 1;
		player.clubIndex = i % 5 == 0 ? 1 : (i % 7 == 0 ? 2 : 1125);
		player.dateOfBirth = (Date){.year = (uint16_t)(self->date.year - player.age), .days = 1};
		player.height = 180;
		player.weight = 78;
		if (i >= 5) {
			memset(player.positions, 0, sizeof(player.positions));
			static const PositionCode positions[] = {POSITION_CODE_GK, POSITION_CODE_DL, POSITION_CODE_DC, POSITION_CODE_DR,
																							 POSITION_CODE_WBL, POSITION_CODE_DM, POSITION_CODE_WBR, POSITION_CODE_ML, POSITION_CODE_MC,
																							 POSITION_CODE_MR, POSITION_CODE_AML, POSITION_CODE_AMC, POSITION_CODE_AMR, POSITION_CODE_ST};
			player.positions[positions[i % 14]] = 20;
			player.age = (uint8_t)(18 + i % 18);
			player.condition = (uint16_t)(8500 + i % 1501);
			if (i % 5)
				player.injury.duration = 0;
		}
		if (i == 1)
			snprintf(player.commonName, sizeof(player.commonName), "%s", "Luka Modrić");
		if (i == 2)
			snprintf(player.commonName, sizeof(player.commonName), "%s", "Νίκος");
		if (i == 3)
			snprintf(player.commonName, sizeof(player.commonName), "%s", "Алексей");
		if (i == 4)
			snprintf(player.commonName, sizeof(player.commonName), "%s", "\xe4\xb8\xad\xe6\x9d\x91 \xe9\x9f\x93");
		if (player.injury.duration)
			player.injury.nameIndex = injuryNames_intern(MOCK_INJURY_NAME, sizeof(MOCK_INJURY_NAME) - 1);
#else
		uint8_t bytes[8];
		if (!readFromMemory(self->process.handle, self->playerStart + i * PLAYER_LIST_STRIDE, 8, bytes))
			continue;
		const uintptr_t address = (uintptr_t)hexBytesToInt(bytes, 8);
		const uint64_t person = getPersonAddressFromPlayerAddress(self->process.handle, address);
		const Player player = player_read(self->process.handle, true, person, address, self->date);
#endif
		self->players[i] = player;
	}
}

static void executeWorker(void *argument) {
	Worker *worker = argument;
	if (worker->index == 0)
		cacheNations(worker);
	else if (worker->index == 1)
		cacheClubs(worker);
	else
		cachePlayers(worker);
	atomic_store(&worker->done, true);
}

static void refreshResults(void) {
	if (gameContext.searchActive)
		search_findPlayers();
	else
		playerTable_clear();
	playerTable_populate();
	++gameContext.revision;
}

void cache_clear(void) {
	requested = false;
	if (job)
		atomic_store(&job->cancel, true);
	const bool hadPlayers = gameContext.players != NULL;
	free(gameContext.players);
	gameContext.players = NULL;
	gameContext.playerCount = 0;
	if (hadPlayers)
		refreshResults();
}

void cache_reset(void) {
	cache_clear();
	free(gameContext.clubs);
	free(gameContext.nations);
	gameContext.clubs = NULL;
	gameContext.nations = NULL;
	gameContext.clubCount = gameContext.nationCount = 0;
	++gameContext.revision;
}

static void startJob(void) {
	job = calloc(1, sizeof(*job));
	if (!job) {
		snprintf(error, sizeof(error), "Cannot allocate cache job");
		LOG_ERROR("%s", error);
		return;
	}
	job->process = processContext;
	job->date = gameContext.currentDate;
	atomic_init(&job->cancel, false);
#ifdef MOCKS_MODE
	job->playerCount = 900;
#else
	uint8_t bytes[16];
	if (!readFromMemory(job->process.handle, job->process.moduleBaseAddress + PLAYER_LIST_PTR_BASE, 16, bytes))
		goto fail;
	job->playerStart = (uintptr_t)hexBytesToInt(bytes, 8);
	const uintptr_t end = (uintptr_t)hexBytesToInt(bytes + 8, 8);
	if (end < job->playerStart || (end - job->playerStart) % PLAYER_LIST_STRIDE)
		goto fail;
	job->playerCount = (end - job->playerStart) / PLAYER_LIST_STRIDE;
	if (!job->playerCount || job->playerCount > 2000000)
		goto fail;
#endif
	job->players = calloc((size_t)job->playerCount, sizeof(Player));
	if (!job->players)
		goto fail;
	for (unsigned i = 0; i < WORKER_COUNT; ++i) {
		Worker *worker = &job->workers[i];
		worker->index = i;
		worker->job = job;
		atomic_init(&worker->done, false);
		if (!thread_start(&worker->thread, executeWorker, worker)) {
			worker->failed = true;
			atomic_store(&worker->done, true);
			atomic_store(&job->cancel, true);
			snprintf(error, sizeof(error), "Could not start cache worker");
		}
	}
	return;
fail:
	snprintf(error, sizeof(error), "Could not read or allocate the player database");
	LOG_ERROR("%s", error);
	free(job->players);
	free(job);
	job = NULL;
}

void cache_run(void) {
	error[0] = '\0';
	requested = true;
}

bool cache_poll(void) {
	bool changed = false;
	if (job) {
		bool allDone = true, playersDone = true;
		for (unsigned i = 0; i < WORKER_COUNT; ++i) {
			if (!atomic_load(&job->workers[i].done)) {
				allDone = false;
				if (i >= 2)
					playersDone = false;
			}
		}
		if (!atomic_load(&job->cancel)) {
			if (!job->nationsPublished && atomic_load(&job->workers[0].done)) {
				if (!job->workers[0].failed) {
					free(gameContext.nations);
					gameContext.nations = job->nations;
					gameContext.nationCount = job->nationCount;
					job->nations = NULL;
				} else {
					snprintf(error, sizeof(error), "Could not cache nations");
					LOG_ERROR("%s", error);
				}
				job->nationsPublished = true;
				changed = true;
			}
			if (!job->clubsPublished && atomic_load(&job->workers[1].done)) {
				if (!job->workers[1].failed) {
					free(gameContext.clubs);
					gameContext.clubs = job->clubs;
					gameContext.clubCount = job->clubCount;
					job->clubs = NULL;
				} else {
					snprintf(error, sizeof(error), "Could not cache clubs");
					LOG_ERROR("%s", error);
				}
				job->clubsPublished = true;
				changed = true;
			}
			if (!job->playersPublished && playersDone) {
				uint64_t count = 0;
				for (uint64_t i = 0; i < job->playerCount; ++i) {
					if (!job->players[i].uid)
						continue;
					player_finishRead(&job->players[i]);
					job->players[count++] = job->players[i];
				}
				free(gameContext.players);
				gameContext.players = job->players;
				gameContext.playerCount = count;
				job->players = NULL;
				job->playersPublished = true;
				refreshResults();
				changed = true;
			}
		}
		if (allDone) {
			for (unsigned i = 0; i < WORKER_COUNT; ++i)
				thread_join(&job->workers[i].thread);
			free(job->players);
			free(job->clubs);
			free(job->nations);
			free(job);
			job = NULL;
		}
	}
	if (requested && !job) {
		requested = false;
		startJob();
	}
	if (changed)
		++gameContext.revision;
	return changed;
}

bool cache_isRunning(void) {
	return job != NULL || requested;
}
const char *cache_error(void) {
	return error;
}

void cache_shutdown(void) {
	requested = false;
	if (job) {
		atomic_store(&job->cancel, true);
		for (unsigned i = 0; i < WORKER_COUNT; ++i)
			thread_join(&job->workers[i].thread);
		free(job->players);
		free(job->clubs);
		free(job->nations);
		free(job);
		job = NULL;
	}
	cache_reset();
}
