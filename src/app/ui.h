// SPDX-FileCopyrightText: © 2026 Phil Armstead <philarmstead@mailbox.org>
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include "app/types.h"

bool ui_init(void);
void ui_update(void);
void ui_draw(void);
void ui_shutdown(void);
bool ui_shouldClose(void);
void ui_setCurrentStatus(const char *status);
#ifdef MOCKS_MODE
int ui_smokeTest(const char *directory);
#endif
