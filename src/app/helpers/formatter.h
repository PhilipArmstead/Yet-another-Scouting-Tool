// SPDX-FileCopyrightText: © 2026 Phil Armstead <philarmstead@mailbox.org>
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include "app/maths.h"

#include <stdbool.h>


void formatter_init(void);
void formatter_printNumber(char *outString);
void formatter_printCurrency(char *outString);
void formatter_formatRating(float rating, char *outString);
void formatter_formatRatingPlain(float rating, char *outString);
void formatter_formatPercentage(float percentage, bool inverted, char *outString);
RGB formatter_qualityColour(uint8_t value);
