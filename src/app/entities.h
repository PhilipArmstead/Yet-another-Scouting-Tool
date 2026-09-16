// SPDX-FileCopyrightText: © 2026 Phil Armstead <philarmstead@mailbox.org>
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include "app/types.h"


extern GameContext gameContext;

/**
 * Clubs and nations live in buffers the cache frees and republishes whenever the save changes, and
 * the indices players carry are row IDs read straight out of the game's memory. Neither is worth
 * trusting at the point of use, so every lookup goes through these: NULL means "no entity", which
 * callers render as blank rather than dereferencing whatever the index happened to land on.
 *
 * The counts are zeroed alongside the frees, so the range check covers a NULL buffer too.
 */
static inline const Club *entities_getClub(const int64_t index) {
	return index >= 0 && (uint64_t)index < gameContext.clubCount ? &gameContext.clubs[index] : NULL;
}

// Unused nationality slots hold 0xFF, which the range check rejects along with any stale row ID.
static inline const Nation *entities_getNation(const uint8_t index) {
	return index < gameContext.nationCount ? &gameContext.nations[index] : NULL;
}
