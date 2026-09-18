// SPDX-FileCopyrightText: © 2026 Phil Armstead <philarmstead@mailbox.org>
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include "app/builder-callback.h"

#include <gtk/gtk.h>


void callbacks_init(void);
BUILDER_CALLBACK void callbacks_onShowQualityAccordionToggle(void);
BUILDER_CALLBACK void callbacks_onShowPositionAccordionToggle(void);
BUILDER_CALLBACK void callbacks_onShowCurrentPlayer(void);
gboolean callbacks_onWindowKeypress(
	GtkEventControllerKey *controller,
	guint keyval,
	guint keycode,
	GdkModifierType state,
	gpointer window
);
