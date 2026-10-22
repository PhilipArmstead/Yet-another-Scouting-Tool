#pragma once

typedef struct {
	char minAge[16], maxAge[16];
	char minCA[16], maxCA[16];
	char minPA[16], maxPA[16];
	char minRating[16], maxRating[16];
	char club[128], nationality[128];
	uint16_t positions;
} FilterBuffer;
