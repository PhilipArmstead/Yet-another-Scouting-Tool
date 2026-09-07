// SPDX-FileCopyrightText: © 2026 Phil Armstead <philarmstead@mailbox.org>
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include "app/types.h"


const char *date_getOrdinal(uint16_t day);
DayMonthYearTime date_prettify(DateTime dateTime);
