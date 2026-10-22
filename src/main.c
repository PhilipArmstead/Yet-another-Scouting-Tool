// SPDX-License-Identifier: GPL-3.0-or-later
#include "app/cache.h"
#include "app/configuration.h"
#include "app/helpers/game.h"
#include "app/injury-names.h"
#include "app/options.h"
#include "app/player-table.h"
#include "app/ui.h"
#include "core/logger.h"
#include "platform/platform.h"
#include "raylib.h"
#include <string.h>

ProcessContext processContext;
GameContext gameContext = {.gameKey = 1};
static double cacheAt;

#ifndef MOCKS_MODE
static void disconnect(void) {
	cacheAt = 0;
	cache_shutdown();
	platform_closeProcess(&processContext);
	gameContext.gameKey = 1;
	gameContext.currentDate = (DateTime){0};
	gameContext.gameVersion[0] = '\0';
	ui_setCurrentStatus("Disconnected; waiting for FM24");
}
#endif

static void pollGame(void) {
#ifndef MOCKS_MODE
	if (!processContext.handle) {
		platform_openProcess(&processContext);
		if (!processContext.handle) {
			ui_setCurrentStatus("Cannot find a supported running process 'fm.exe'");
			return;
		}
		LOG_INFO("Connected to process %u", processContext.pid);
	}
#endif
	const DateTime date = game_getDateTime(&processContext);
	if (!date.days || !date.year) {
#ifndef MOCKS_MODE
		disconnect();
#endif
		return;
	}
	if (date.days != gameContext.currentDate.days || date.year != gameContext.currentDate.year || date.time != gameContext.currentDate.time) {
		gameContext.currentDate = date;
		gameContext.gameKey = 1;
	}
	game_getVersion(&processContext, gameContext.gameVersion, sizeof(gameContext.gameVersion));
	GameKeyStatus status;
	const uint64_t key = game_getKey(&processContext, &status);
	if (gameContext.gameKey != key) {
		gameContext.gameKey = key;
		if (status == GAME_KEY_FOUND && date.year > 1970) {
			cache_clear();
			cacheAt = GetTime() + 1;
			ui_setCurrentStatus("Caching data");
		} else {
			cache_reset();
			cacheAt = 0;
			ui_setCurrentStatus("Cannot read save data");
		}
	}
}

int main(int argc, char **argv) {
	bool smoke = argc >= 2 && !strcmp(argv[1], "--smoke-test");
	int result = 0;
	const bool loggerReady = logger_init();
	LOG_INFO("Starting " APP_NAME " " APP_VERSION);
	if (!injuryNames_init()) {
		logger_shutdown();
		return 1;
	}
	const bool optionsReady = options_init();
	playerTable_init();
	if (!ui_init()) {
		LOG_ERROR("The Raylib frontend could not be initialized");
		ui_shutdown();
		playerTable_shutdown();
		options_shutdown();
		injuryNames_destroy();
		logger_shutdown();
		return 1;
	}
	if (!optionsReady)
		ui_setCurrentStatus("Options could not be loaded or watched. See the log.");
	double nextPoll = 0;
	int frameRate = 60;
	const double start = GetTime();
	while (!ui_shouldClose()) {
		const double now = GetTime();
		if (now >= nextPoll) {
			pollGame();
			nextPoll = now + 1;
		}
		if (cacheAt > 0 && now >= cacheAt) {
			cacheAt = 0;
			cache_run();
		}
		if (cache_poll()) {
			if (cache_error()[0])
				ui_setCurrentStatus(cache_error());
			else if (!cache_isRunning())
				ui_setCurrentStatus("Ready");
		} else if (cache_error()[0])
			ui_setCurrentStatus(cache_error());
		options_update();
		if (!loggerReady)
			ui_setCurrentStatus("Log file is unavailable. Check per-user directory permissions.");
		ui_update();
		const int target = IsWindowMinimized() ? 5 : IsWindowFocused() ? 60
																																	 : 15;
		if (frameRate != target) {
			SetTargetFPS(target);
			frameRate = target;
		}
		ui_draw();
		if (smoke) {
#ifdef MOCKS_MODE
			const int status = ui_smokeTest(argc > 2 ? argv[2] : NULL);
			if (status) {
				if (status < 0)
					result = 1;
				break;
			}
			if (now - start > 15) {
				LOG_ERROR("UI smoke test timed out");
				result = 1;
				break;
			}
#else
			if (now - start > 3)
				break;
#endif
		}
	}
	cache_shutdown();
	ui_shutdown();
	platform_closeProcess(&processContext);
	playerTable_shutdown();
	options_shutdown();
	injuryNames_destroy();
	LOG_INFO("Application stopped");
	logger_shutdown();
	return result;
}
