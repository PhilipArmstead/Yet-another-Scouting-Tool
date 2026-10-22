// SPDX-FileCopyrightText: © 2026 Phil Armstead <philarmstead@mailbox.org>
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include "app/maths.h"

#include <stdbool.h>


// printNumber grows the string in place and printCurrency prefixes it, so callers of either must
// size for the result, not the input: 20 digits of uint64 (or 19 plus a sign), six separators,
// the two bytes of UTF-8 £, and the terminator.
#define FORMATTER_NUMBER_SIZE 32

// Buffers include the terminator; colours are returned separately for immediate-mode drawing.
#define FORMATTER_RATING_SIZE 16
#define FORMATTER_RATING_PLAIN_SIZE 16
#define FORMATTER_PERCENTAGE_SIZE 16

void formatter_init(void);
void formatter_printNumber(char *outString);
void formatter_printCurrency(char *outString);
void formatter_formatRating(float rating, char *outString);
void formatter_formatRatingPlain(float rating, char *outString);
void formatter_formatPercentage(float percentage, bool inverted, char *outString);
RGB formatter_qualityColour(uint8_t value);
