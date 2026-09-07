// SPDX-FileCopyrightText: © 2026 Phil Armstead <philarmstead@mailbox.org>
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <gtk/gtk.h>


G_MODULE_EXPORT void callbacks_onFiltersClear(void);
G_MODULE_EXPORT void callbacks_onFilterRun(void);
G_MODULE_EXPORT gboolean callbacks_onFiltersKeypress(
	GtkEventControllerKey *controller,
	guint keyval,
	guint keycode,
	GdkModifierType state,
	gpointer data
);
void callbacks_updateFilterTags(void);
