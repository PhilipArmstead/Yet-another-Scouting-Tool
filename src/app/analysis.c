#include "analysis.h"
#include "app/helpers/ratings.h"
#include "core/logger.h"
#include <math.h>
#include <stdlib.h>
#include <string.h>

#define INFEASIBLE_COST 1e9f

static float positionalRating(const Player *player, const PositionCode position) {
	if (player->positions[position] < MINIMUM_POSITIONAL_PROFICIENCY)
		return INFEASIBLE_COST;
	const float rating = getRatingForPosition(player, position);
	return isfinite(rating) && rating > 0 ? rating : INFEASIBLE_COST;
}

static bool hungarian(const float *matrix, const uint32_t n, uint32_t *assignment) {
	float *u = calloc(n + 1, sizeof(float)), *v = calloc(n + 1, sizeof(float));
	uint32_t *p = calloc(n + 1, sizeof(uint32_t)), *way = calloc(n + 1, sizeof(uint32_t));
	float *minDist = malloc((n + 1) * sizeof(float));
	bool *used = malloc((n + 1) * sizeof(bool));
	bool success = u && v && p && way && minDist && used;
	if (!success)
		goto cleanup;
	for (uint32_t i = 0; i < n; ++i) {
		assignment[i] = n;
		p[0] = i + 1;
		uint32_t j0 = 0;
		for (uint32_t j = 0; j <= n; ++j)
			minDist[j] = INFINITY;
		memset(used, 0, (n + 1) * sizeof(bool));
		do {
			used[j0] = true;
			const uint32_t i0 = p[j0];
			float delta = INFINITY;
			uint32_t j1 = 0;
			for (uint32_t j = 1; j <= n; ++j) {
				if (used[j])
					continue;
				const float cost = matrix[(i0 - 1) * n + j - 1] - u[i0] - v[j];
				if (cost < minDist[j]) {
					minDist[j] = cost;
					way[j] = j0;
				}
				if (minDist[j] < delta) {
					delta = minDist[j];
					j1 = j;
				}
			}
			if (!j1) {
				success = false;
				goto cleanup;
			}
			for (uint32_t j = 0; j <= n; ++j) {
				if (used[j]) {
					u[p[j]] += delta;
					v[j] -= delta;
				} else
					minDist[j] -= delta;
			}
			j0 = j1;
		} while (p[j0]);
		do {
			const uint32_t j1 = way[j0];
			p[j0] = p[j1];
			j0 = j1;
		} while (j0);
	}
	for (uint32_t j = 1; j <= n; ++j)
		if (p[j])
			assignment[p[j] - 1] = j - 1;
cleanup:
	free(u);
	free(v);
	free(p);
	free(way);
	free(minDist);
	free(used);
	if (!success)
		LOG_ERROR("Best XI assignment failed");
	return success;
}

bool analysis_bestEleven(const Player *players, const uint32_t count, const PositionCode positions[11],
												 const BestElevenFilters filters, BestElevenRow rows[11]) {
	memset(rows, 0, 11 * sizeof(*rows));
	if (!count)
		return true;
	PositionCode distinct[11];
	uint32_t distinctCount = 0;
	for (unsigned i = 0; i < 11; ++i) {
		if (positions[i] < 0 || positions[i] >= POSITION_CODE_COUNT) {
			LOG_ERROR("Invalid formation position");
			return false;
		}
		unsigned j = 0;
		while (j < distinctCount && distinct[j] != positions[i])
			++j;
		if (j == distinctCount)
			distinct[distinctCount++] = positions[i];
	}
	bool *chosen = calloc(count, sizeof(bool));
	if (!chosen) {
		LOG_ERROR("Could not allocate Best XI candidates");
		return false;
	}
	uint32_t candidates[121], candidateCount = 0;
	for (unsigned d = 0; d < distinctCount; ++d) {
		uint32_t topIndices[11], topCount = 0;
		float topValues[11];
		for (uint32_t i = 0; i < count; ++i) {
			const Player *player = &players[i];
			if (player->age < filters.minAge || player->age > filters.maxAge ||
					player->condition < filters.minCondition || player->condition > filters.maxCondition ||
					(filters.excludeInjured && player->injury.duration))
				continue;
			const float rating = positionalRating(player, distinct[d]);
			if (rating >= INFEASIBLE_COST || (topCount == 11 && rating <= topValues[10]))
				continue;
			unsigned at = topCount < 11 ? topCount++ : 10;
			while (at && topValues[at - 1] < rating) {
				topValues[at] = topValues[at - 1];
				topIndices[at] = topIndices[at - 1];
				--at;
			}
			topValues[at] = rating;
			topIndices[at] = i;
		}
		for (unsigned i = 0; i < topCount; ++i)
			if (!chosen[topIndices[i]]) {
				chosen[topIndices[i]] = true;
				candidates[candidateCount++] = topIndices[i];
			}
	}
	free(chosen);
	const uint32_t n = candidateCount > 11 ? candidateCount : 11;
	float *matrix = malloc((size_t)n * n * sizeof(float));
	if (!matrix) {
		LOG_ERROR("Could not allocate Best XI matrix");
		return false;
	}
	for (uint32_t i = 0; i < n; ++i)
		for (uint32_t j = 0; j < n; ++j) {
			float cost = 0;
			if (i < 11 && j < candidateCount) {
				const float rating = positionalRating(&players[candidates[j]], positions[i]);
				cost = rating >= INFEASIBLE_COST ? INFEASIBLE_COST : -rating;
			}
			matrix[i * n + j] = cost;
		}
	uint32_t assignment[121];
	const bool success = hungarian(matrix, n, assignment);
	free(matrix);
	if (!success)
		return false;
	for (unsigned i = 0; i < 11; ++i) {
		if (assignment[i] >= candidateCount)
			continue;
		const Player *player = &players[candidates[assignment[i]]];
		const float rating = positionalRating(player, positions[i]);
		if (rating < INFEASIBLE_COST)
			rows[i] = (BestElevenRow){player, rating};
	}
	return true;
}

size_t analysis_depth(const Player *players, const uint32_t count, const PositionCode position, BestElevenRow rows[DEPTH_LIMIT]) {
	size_t length = 0;
	for (uint32_t i = 0; i < count; ++i) {
		const Player *player = &players[i];
		if (player->positions[position] < MINIMUM_POSITIONAL_PROFICIENCY)
			continue;
		const float rating = getRatingForPosition(player, position);
		size_t at = 0;
		while (at < length && (rows[at].rating > rating ||
													 (rows[at].rating == rating && rows[at].player->uid < player->uid)))
			++at;
		if (at >= DEPTH_LIMIT)
			continue;
		if (length < DEPTH_LIMIT)
			++length;
		memmove(rows + at + 1, rows + at, (length - at - 1) * sizeof(*rows));
		rows[at] = (BestElevenRow){player, rating};
	}
	return length;
}
