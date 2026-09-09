// SPDX-FileCopyrightText: © 2026 Phil Armstead <philarmstead@mailbox.org>
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <gtk/gtk.h>

#include "app/types.h"


void ui_init(GtkApplication *app);
void ui_update(void);
void ui_updateGameVersion(void);
void ui_updateInGameDate(void);
void ui_clearFilterTags(void);
void ui_createFilterTag(const char *text, GtkEntryBuffer *buffer);
void ui_createClubFilterTag(const char *text, GtkEditable *buffer);

WindowContext openWindow(const char *layoutName, const char *windowName, WindowType type);
void ui_createPlayerInfoWindow(const Player *player);
void ui_renderPlayerInfoWindow(WindowContext context);
void ui_createBestElevenWindow(void);
void ui_renderBestElevenWindow(WindowContext context);
void ui_refreshAllWindows(void);

void ui_setCurrentStatus(const char *status);
