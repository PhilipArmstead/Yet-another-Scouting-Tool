// SPDX-FileCopyrightText: © 2026 Phil Armstead <philarmstead@mailbox.org>
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <stdint.h>


#define INJURY_NAME_NONE 0

/**
 * Interns an injury name, returning the index players store. Safe to call from the cache workers.
 * Returns INJURY_NAME_NONE if the table is full, which renders as no name rather than failing.
 */
uint16_t injuryNames_intern(const char *name, uint8_t length);

/** Never NULL, and valid for the life of the process, so snapshots can hold an index indefinitely. */
const char *injuryNames_get(uint16_t index);
void injuryNames_destroy(void);
