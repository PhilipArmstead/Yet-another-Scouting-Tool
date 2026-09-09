// SPDX-FileCopyrightText: © 2026 Phil Armstead <philarmstead@mailbox.org>
// SPDX-License-Identifier: GPL-3.0-or-later

#include "callbacks.h"

#include "app/player-table.h"
#include "app/callbacks/club-search.h"
#include "app/callbacks/filters.h"
#include "core/logger.h"


extern GameContext gameContext;

static void accordionToggle(const char *name);

void callbacks_init(void) {
	// Capture keypresses within sidebar to trigger search
	GtkEventControllerKey *controllerKey = GTK_EVENT_CONTROLLER_KEY(gtk_event_controller_key_new());
	g_signal_connect(controllerKey, "key-released", G_CALLBACK(callbacks_onFiltersKeypress), NULL);
	gtk_event_controller_set_propagation_phase(GTK_EVENT_CONTROLLER(controllerKey), GTK_PHASE_BUBBLE);

	GtkWidget *sidebar = GTK_WIDGET(gtk_builder_get_object(gameContext.builder, "box:sidebar"));
	gtk_widget_add_controller(sidebar, GTK_EVENT_CONTROLLER(controllerKey));

	// Attach datalist callbacks
	SearchDatalist *dataList = gameContext.dataList;
	g_signal_connect(dataList->entry, "changed", G_CALLBACK(callbacks_OnClubNameChange), dataList);
	g_signal_connect(dataList->listBox, "row-activated", G_CALLBACK(callbacks_onClubNameSelected), dataList);
}

gboolean callbacks_onWindowKeypress(
	GtkEventControllerKey *controller,
	guint keyval,
	guint keycode,
	GdkModifierType state,
	gpointer window
) {
	(void)controller;
	(void)keycode;

	const gboolean ctrl = (state & GDK_CONTROL_MASK) != 0;
	const gboolean meta = (state & GDK_META_MASK) != 0;
	const gboolean alt = (state & GDK_ALT_MASK) != 0;

#ifdef ARCH_MACOS
	if (meta) {
		if (keyval == GDK_KEY_q || keyval == GDK_KEY_Q) {
			g_application_quit(G_APPLICATION(gameContext.app));
			return TRUE;
		}
		if (keyval == GDK_KEY_w || keyval == GDK_KEY_W) {
			gtk_window_close(GTK_WINDOW(window));
			return TRUE;
		}
	}
#else
	if (alt && keyval == GDK_KEY_F4) {
		g_application_quit(G_APPLICATION(gameContext.app));
		return TRUE;
	}
	if (ctrl && (keyval == GDK_KEY_w || keyval == GDK_KEY_W || keyval == GDK_KEY_F4)) {
		gtk_window_close(GTK_WINDOW(window));
		return TRUE;
	}
#endif

	return FALSE;
}
