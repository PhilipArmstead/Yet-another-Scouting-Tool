// SPDX-FileCopyrightText: © 2026 Phil Armstead <philarmstead@mailbox.org>
// SPDX-License-Identifier: GPL-3.0-or-later

#include <gtk/gtk.h>

#include "app/callbacks.h"
#include "app/options.h"
#include "app/player-table.h"
#include "app/ui.h"
#include "app/helpers/vector-shared-pointer.h"
#include "core/logger.h"


ProcessContext processContext = {0};
GameContext gameContext = {0};

static void activate(GtkApplication *app) {
	ui_init(app);
	callbacks_init();
	playerTable_init();
}

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
