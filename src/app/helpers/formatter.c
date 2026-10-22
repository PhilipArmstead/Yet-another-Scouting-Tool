#include "formatter.h"
#include "app/types.h"
#include <stdio.h>
#include <string.h>

extern GameContext gameContext;
static RGB qualityRamp[121];

void formatter_printNumber(char *text) {
	if (!text) return;
	const size_t length = strlen(text), minus = text[0] == '-';
	if (length - minus < 4) return;
	size_t source = length, destination = length + (length - 1 - minus) / 3, digits = 0;
	text[destination--] = '\0';
	while (source > minus) {
		text[destination--] = text[--source];
		if (source > minus && ++digits == 3) { text[destination--] = ','; digits = 0; }
	}
}

void formatter_printCurrency(char *text) {
	if (!text) return;
	formatter_printNumber(text);
	memmove(text + 2, text, strlen(text) + 1);
	text[0] = (char)0xc2; text[1] = (char)0xa3;
}

void formatter_init(void) {
	for (unsigned i = 0; i <= 120; ++i) {
		const RGB hue = hueToRgb((float)i);
		qualityRamp[i] = gameContext.options.darkMode ? hue : capLuminance(hue, 0.1833f);
	}
}

RGB formatter_qualityColour(const uint8_t value) { return qualityRamp[value > 120 ? 120 : value]; }

void formatter_formatRating(const float rating, char *text) {
	snprintf(text, FORMATTER_RATING_SIZE, "%.2f%%", (double)rating);
}

void formatter_formatRatingPlain(const float rating, char *text) {
	snprintf(text, FORMATTER_RATING_PLAIN_SIZE, "%.2f%%", (double)rating);
}

void formatter_formatPercentage(const float percentage, const bool inverted, char *text) {
	(void)inverted;
	snprintf(text, FORMATTER_PERCENTAGE_SIZE, "%.0f%%", (double)fminf(100, fmaxf(0, percentage)));
}
