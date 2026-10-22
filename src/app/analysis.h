#pragma once
#include "app/types.h"

#define DEPTH_LIMIT 20
typedef struct {
	uint16_t minCondition, maxCondition;
	uint8_t minAge, maxAge;
	bool excludeInjured;
} BestElevenFilters;

bool analysis_bestEleven(const Player *players, uint32_t count, const PositionCode positions[11],
												 BestElevenFilters filters, BestElevenRow rows[11]);
size_t analysis_depth(const Player *players, uint32_t count, PositionCode position, BestElevenRow rows[DEPTH_LIMIT]);
