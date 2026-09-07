// SPDX-FileCopyrightText: © 2026 Phil Armstead <philarmstead@mailbox.org>
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include "app/types.h"


DateTime game_getDateTime(const ProcessContext *context);
void game_getVersion(const ProcessContext *context, char *versionBuffer, uint8_t bufferSize);

typedef enum {
	GAME_KEY_NOT_FOUND,
	GAME_KEY_NULL,
	GAME_KEY_FOUND,
} GameKeyStatus;

uint64_t game_getKey(const ProcessContext *context, GameKeyStatus *outStatus);
