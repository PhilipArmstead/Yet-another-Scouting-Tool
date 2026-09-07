// SPDX-FileCopyrightText: © 2026 Phil Armstead <philarmstead@mailbox.org>
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#define NON_PLAYERS_THREAD_COUNT 2
#define PLAYERS_THREAD_COUNT 8
#define THREAD_COUNT (NON_PLAYERS_THREAD_COUNT + PLAYERS_THREAD_COUNT)

void cache_clear(void);
void cache_run(void);
