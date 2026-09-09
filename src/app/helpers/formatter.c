// SPDX-FileCopyrightText: © 2026 Phil Armstead <philarmstead@mailbox.org>
// SPDX-License-Identifier: GPL-3.0-or-later

#include "formatter.h"
#include "app/maths.h"
#include "app/types.h"
#include "core/logger.h"

#include <string.h>


/**
 * Add thousands separators to string-representations of integers.
 * Ensure the string is writable and there are enough spaces in
 * the string buffer to add them.
 */
void formatter_printNumber(char *outString) {
	if (outString == NULL) {
		return;
	}

	const uint64_t length = strlen(outString);
	const uint8_t minus = outString[0] == '-';
	if (length - minus < 4) {
		return;
	}

	const uint64_t commaCount = (length - 1 - minus) / 3;

	uint64_t src = length;
	uint64_t dest = length + commaCount;
	outString[dest--] = '\0';
	int64_t i = 0;
	while (src > minus) {
		outString[dest--] = outString[--src];

		if (src > minus && ++i == 3) {
			outString[dest--] = ',';
			i = 0;
		}
	}
}

#define POUND_SIGN_WIDTH 2
/**
 * Prefix a string with the GBP symbol (£)
 * Ensure the string is writable and there are
 * enough space sin the string buffer to add them.
 */
void formatter_printCurrency(char *outString) {
	if (outString == NULL) {
		return;
	}

	formatter_printNumber(outString);

	const uint64_t length = strlen(outString);
	memmove(outString + 2, outString, length + 1);

	// UTF-8 encoding of U+00A3 (£)
	outString[0] = (char)0xC2;
	outString[1] = (char)0xA3;
}

/**
 * Returns a string containing your rating, colourised on a scale of 1-120
 * @param rating
 * @param outString - should be at least 44 bytes
 */
void formatter_formatRating(const float rating, char *outString) {
	const uint8_t value = (uint8_t)((rating < MAX_RATING_VALUE ? rating / MAX_RATING_VALUE : 1.f) * 120);
	char colour[9];
	hueToHex(value, colour);
	snprintf(outString, 44, "<span foreground=\"%s\">%.2f%%</span>", colour, rating);
}
