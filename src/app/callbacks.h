// SPDX-FileCopyrightText: © 2026 Phil Armstead <philarmstead@mailbox.org>
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <gtk/gtk.h>


void callbacks_init(void);
G_MODULE_EXPORT void callbacks_onShowQualityAccordionToggle(void);
G_MODULE_EXPORT void callbacks_onShowPositionAccordionToggle(void);
G_MODULE_EXPORT void callbacks_onShowCurrentPlayer(void);
gboolean callbacks_onWindowKeypress(
	GtkEventControllerKey *controller,
	guint keyval,
	guint keycode,
	GdkModifierType state,
	gpointer window
);
