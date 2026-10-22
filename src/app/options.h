// SPDX-FileCopyrightText: © 2026 Phil Armstead <philarmstead@mailbox.org>
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once
#include <stdbool.h>

bool options_init(void);
void options_defaults(void);
void options_update(void);
const char *options_getPath(void);
const char *options_error(void);
void options_shutdown(void);
