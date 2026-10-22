#include "search-handler.h"
#include "app/player-table.h"
#include "app/search.h"
#include "app/types.h"
#include <errno.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>

extern GameContext gameContext;

static bool byteValue(const char *text, uint8_t *out) {
	char *end;
	errno = 0;
	const long value = strtol(text, &end, 10);
	if (errno || end == text || *end || value < 0 || value > 255)
		return false;
	*out = (uint8_t)value;
	return true;
}

bool searchHandler_cacheFilters(char error[128]) {
	const FilterBuffer *buffer = &gameContext.filterBuffer;
	FilterOptions options = gameContext.filterOptions;
	options.filterMask &= FILTER_HAS_CLUB | FILTER_HAS_NATIONALITY;
	const char *strings[] = {buffer->minAge, buffer->maxAge, buffer->minCA, buffer->maxCA, buffer->minPA, buffer->maxPA};
	uint8_t *values[] = {&options.minAge, &options.maxAge, &options.minCA, &options.maxCA, &options.minPA, &options.maxPA};
	const uint32_t masks[] = {FILTER_HAS_MIN_AGE, FILTER_HAS_MAX_AGE, FILTER_HAS_MIN_CA, FILTER_HAS_MAX_CA, FILTER_HAS_MIN_PA, FILTER_HAS_MAX_PA};
	for (size_t i = 0; i < 6; ++i) {
		if (!strings[i][0])
			continue;
		if (!byteValue(strings[i], values[i])) {
			snprintf(error, 128, "Age, CA and PA must be whole numbers from 0 to 255.");
			return false;
		}
		options.filterMask |= masks[i];
	}
	if ((buffer->minAge[0] && buffer->maxAge[0] && options.minAge > options.maxAge) ||
			(buffer->minCA[0] && buffer->maxCA[0] && options.minCA > options.maxCA) ||
			(buffer->minPA[0] && buffer->maxPA[0] && options.minPA > options.maxPA)) {
		snprintf(error, 128, "The minimum must not exceed the maximum.");
		return false;
	}
	const char *ratings[] = {buffer->minRating, buffer->maxRating};
	float *ratingValues[] = {&options.minRating, &options.maxRating};
	for (size_t i = 0; i < 2; ++i) {
		if (!ratings[i][0])
			continue;
		char *end;
		errno = 0;
		const float value = strtof(ratings[i], &end);
		if (errno || end == ratings[i] || *end || !isfinite(value)) {
			snprintf(error, 128, "Rating must be a finite number.");
			return false;
		}
		*ratingValues[i] = value;
		options.filterMask |= i == 0 ? FILTER_HAS_MIN_RATING : FILTER_HAS_MAX_RATING;
	}
	if (ratings[0][0] && ratings[1][0] && options.minRating > options.maxRating) {
		snprintf(error, 128, "The minimum rating must not exceed the maximum.");
		return false;
	}
	options.positions = buffer->positions;
	if (options.positions)
		options.filterMask |= FILTER_HAS_POSITION;
	gameContext.filterOptions = options;
	error[0] = '\0';
	return true;
}

bool searchHandler_doSearch(const bool refreshFilterCache, char error[128]) {
	if (refreshFilterCache && !searchHandler_cacheFilters(error))
		return false;
	gameContext.searchActive = true;
	search_findPlayers();
	playerTable_populate();
	return true;
}

void searchHandler_clearFilters(void) {
	gameContext.filterBuffer = (FilterBuffer){0};
	gameContext.filterOptions = (FilterOptions){0};
	gameContext.searchActive = false;
	playerTable_clear();
}
