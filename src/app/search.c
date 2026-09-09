// SPDX-FileCopyrightText: © 2026 Phil Armstead <philarmstead@mailbox.org>
// SPDX-License-Identifier: GPL-3.0-or-later

#include "search.h"
#include "app/mocks.h"
#include "core/logger.h"
#include "helpers/vector-shared-pointer.h"
#include "helpers/vector.h"
#include "platform/platform.h"


extern GameContext gameContext;

// TODO: multithread this
uint32_t *search_findPlayers(void) {
	const int64_t timeStart = platform_getMicroseconds();

	uint32_t *playerIds = NULL;
	const FilterOptions options = gameContext.filterOptions;

	for (uint32_t i = 0; i < gameContext.playerCount; ++i) {
#ifndef PLAYER_BY_ID
		const Player *player = &gameContext.players[i];

		if (
			(options.filterMask & FILTER_HAS_MIN_CA && player->ca < options.minCA) ||
			(options.filterMask & FILTER_HAS_MAX_CA && player->ca > options.maxCA) ||
			(options.filterMask & FILTER_HAS_MIN_PA && player->pa < options.minPA) ||
			(options.filterMask & FILTER_HAS_MAX_PA && player->pa > options.maxPA)
		) {
			continue;
		}

		if (
			(options.filterMask & FILTER_HAS_MIN_AGE && player->age < options.minAge) ||
			(options.filterMask & FILTER_HAS_MAX_AGE && player->age > options.maxAge)
		) {
			continue;
		}

		if (
			(options.filterMask & FILTER_HAS_MIN_VALUE && player->guideValue < options.minValue) ||
			(options.filterMask & FILTER_HAS_MAX_VALUE && player->guideValue > options.maxValue)
		) {
			continue;
		}

		if (options.filterMask & FILTER_HAS_CLUB && player->clubIndex != options.clubIndex) {
			continue;
		}

		if (options.positions > 0) {
			bool hasPosition = false;
			if (
				(options.positions & POSITION_MASK_GK && player->positions[POSITION_CODE_GK] >=
					MINIMUM_POSITIONAL_PROFICIENCY) ||
				(options.positions & POSITION_MASK_DL && player->positions[POSITION_CODE_DL] >=
					MINIMUM_POSITIONAL_PROFICIENCY) ||
				(options.positions & POSITION_MASK_DC && player->positions[POSITION_CODE_DC] >=
					MINIMUM_POSITIONAL_PROFICIENCY) ||
				(options.positions & POSITION_MASK_DR && player->positions[POSITION_CODE_DR] >=
					MINIMUM_POSITIONAL_PROFICIENCY) ||
				(options.positions & POSITION_MASK_WBL && player->positions[POSITION_CODE_WBL] >=
					MINIMUM_POSITIONAL_PROFICIENCY) ||
				(options.positions & POSITION_MASK_DM && player->positions[POSITION_CODE_DM] >=
					MINIMUM_POSITIONAL_PROFICIENCY) ||
				(options.positions & POSITION_MASK_WBR && player->positions[POSITION_CODE_WBR] >=
					MINIMUM_POSITIONAL_PROFICIENCY) ||
				(options.positions & POSITION_MASK_ML && player->positions[POSITION_CODE_ML] >=
					MINIMUM_POSITIONAL_PROFICIENCY) ||
				(options.positions & POSITION_MASK_MC && player->positions[POSITION_CODE_MC] >=
					MINIMUM_POSITIONAL_PROFICIENCY) ||
				(options.positions & POSITION_MASK_MR && player->positions[POSITION_CODE_MR] >=
					MINIMUM_POSITIONAL_PROFICIENCY) ||
				(options.positions & POSITION_MASK_AML && player->positions[POSITION_CODE_AML] >=
					MINIMUM_POSITIONAL_PROFICIENCY) ||
				(options.positions & POSITION_MASK_AMC && player->positions[POSITION_CODE_AMC] >=
					MINIMUM_POSITIONAL_PROFICIENCY) ||
				(options.positions & POSITION_MASK_AMR && player->positions[POSITION_CODE_AMR] >=
					MINIMUM_POSITIONAL_PROFICIENCY) ||
				(options.positions & POSITION_MASK_ST && player->positions[POSITION_CODE_ST] >=
					MINIMUM_POSITIONAL_PROFICIENCY)
			) {
				hasPosition = true;
			}

			if (!hasPosition) {
				continue;
			}
		}

		if (
			(options.filterMask & FILTER_HAS_MIN_RATING && player->ratings[0].value < options.minRating) ||
			(options.filterMask & FILTER_HAS_MAX_RATING && player->ratings[0].value > options.maxRating)
		) {
			continue;
		}
#endif

		vector_push(playerIds, i);
	}

	sharedPointer_unref(gameContext.searchResults);
	gameContext.searchResults = sharedPointer_new(playerIds);

	const int64_t timeEnd = platform_getMicroseconds();
	LOG_DEBUG("Found %d players in %zu microseconds", vector_length(playerIds), timeEnd - timeStart);
}
