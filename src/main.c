// SPDX-FileCopyrightText: © 2026 Phil Armstead <philarmstead@mailbox.org>
// SPDX-License-Identifier: GPL-3.0-or-later

#include <gtk/gtk.h>

#include "app/callbacks.h"
#include "app/data.h"
#include "app/game-status.h"
#include "app/options.h"
#include "app/player-table.h"
#include "app/ui.h"
#include "app/helpers/vector-shared-pointer.h"
#include "core/logger.h"
#include "platform/platform.h"


ProcessContext processContext = {0};
GameContext gameContext = {0};

static void activate(GtkApplication *app);
static gboolean update(gpointer userData);
static inline void updateWhileDisconnected(void);
static void handleDisconnect(void);
static void handleConnect(void);
static inline void updateWhileConnected(void);
static inline void updateWhileDisconnected(void);

int main(const int argc, char **argv) {
	logger_init();

	LOG_INFO("App started");

	options_init();

	gameContext.searchResults = sharedPointer_new(NULL);

	GtkApplication *app = gtk_application_new("org.gtk.example", G_APPLICATION_DEFAULT_FLAGS);
	gameContext.app = app;
	g_signal_connect(app, "activate", G_CALLBACK(activate), NULL);
	const int status = g_application_run(G_APPLICATION(app), argc, argv);
	g_object_unref(app);

	logger_shutdown();

	return status;
}

static void activate(GtkApplication *app) {
	ui_init(app);
	callbacks_init();
	playerTable_init();

	g_timeout_add(1000, update, NULL);
	update(NULL);
}

static gboolean update(gpointer userData) {
	(void)userData;

	#ifndef MOCKS_MODE
	if (processContext.handle != NULL) {
		updateWhileConnected();
	} else {
		updateWhileDisconnected();
	}
	#else
	updateWhileConnected();
	#endif
	return G_SOURCE_CONTINUE;
}


static inline void updateWhileConnected(void) {
	// Get the current time/date
	DayMonthYear dayMonthYear = getDayMonthYear(&processContext);

	// Bail if the date is invalid and assume we're no longer connected
	if (dayMonthYear.day == 0 || dayMonthYear.year == 0) {
		LOG_INFO("Disconnecting because the date is blank");
		handleDisconnect();
		return;
	}

	// Cache the new date if it's different from the last one we saw
	if (
		dayMonthYear.day != gameContext.currentDate.day ||
		dayMonthYear.year != gameContext.currentDate.year ||
		strncmp(dayMonthYear.month, gameContext.currentDate.month, MONTH_NAME_LENGTH) != 0
	) {
		gameContext.currentDate = dayMonthYear;
		LOG_DEBUG(
			"Current Date: %s %d, %d",
			dayMonthYear.month,
			dayMonthYear.day,
			dayMonthYear.year
		);

		ui_updateInGameDate();
	}

	// Update the game version if it's different from the last one we saw
	char versionBuffer[GAME_STATUS_STRING_BUFFER_SIZE] = {0};
	getGameVersion(&processContext, versionBuffer, GAME_STATUS_STRING_BUFFER_SIZE);
	if (strncmp(versionBuffer, gameContext.gameVersion, GAME_STATUS_STRING_BUFFER_SIZE) != 0) {
		strncpy(gameContext.gameVersion, versionBuffer, GAME_STATUS_STRING_BUFFER_SIZE);
		LOG_INFO("Game Version: %s", versionBuffer);

		ui_updateGameStatus();
	}
}

static inline void updateWhileDisconnected(void) {
	clearCaches();
	platform_openProcess(&processContext);

	if (processContext.handle != NULL) {
		handleConnect();
	}
}


static void handleDisconnect(void) {
	processContext.handle = NULL;

	gameContext.gameVersion[0] = '\0';
	gameContext.currentDate = (DayMonthYear){0};

	ui_update();
}

static void handleConnect(void) {
	LOG_INFO(
		"Process opened with PID: %u (base module address of %p)",
		processContext.pid,
		(void*)processContext.moduleBaseAddress
	);
	ui_update();
	update(NULL);

	runMultiThreadedCache();
}
