// SPDX-FileCopyrightText: © 2026 Phil Armstead <philarmstead@mailbox.org>
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <stdint.h>
#include <gtk/gtk.h>

#include "types/common.h"
#include "types/entities.h"
#include "types/logger.h"
#include "types/position.h"
#include "types/options.h"
#include "types/process.h"
#include "types/search.h"
#include "types/shared-pointer.h"
#include "types/ui.h"

#define FILTER_HAS_MIN_RATING (1 << 0)
#define FILTER_HAS_MAX_RATING (1 << 1)
#define FILTER_HAS_MIN_VALUE (1 << 2)
#define FILTER_HAS_MAX_VALUE (1 << 3)
#define FILTER_HAS_CLUB (1 << 4)
#define FILTER_HAS_MIN_CA (1 << 5)
#define FILTER_HAS_MAX_CA (1 << 6)
#define FILTER_HAS_MIN_PA (1 << 7)
#define FILTER_HAS_MAX_PA (1 << 8)
#define FILTER_HAS_MIN_HOME_REPUTATION (1 << 9)
#define FILTER_HAS_MAX_HOME_REPUTATION (1 << 10)
#define FILTER_HAS_MIN_CURRENT_REPUTATION (1 << 11)
#define FILTER_HAS_MAX_CURRENT_REPUTATION (1 << 12)
#define FILTER_HAS_MIN_WORLD_REPUTATION (1 << 13)
#define FILTER_HAS_MAX_WORLD_REPUTATION (1 << 14)
#define FILTER_HAS_MIN_AGE (1 << 15)
#define FILTER_HAS_MAX_AGE (1 << 16)

#define POSITION_MASK_GK (1 << 0)
#define POSITION_MASK_FB (1 << 1)
#define POSITION_MASK_CB (1 << 2)
#define POSITION_MASK_WB (1 << 3)
#define POSITION_MASK_DM (1 << 4)
#define POSITION_MASK_MC (1 << 5)
#define POSITION_MASK_W (1 << 6)
#define POSITION_MASK_AM (1 << 7)
#define POSITION_MASK_ST (1 << 8)

typedef struct {
	int64_t clubIndex;
	uint32_t filterMask; // 1 bit to represent the presence of each filter
	float minRating;
	float maxRating;
	uint32_t minValue;
	uint32_t maxValue;
	uint16_t positions; // Masked, one bit per position
	uint8_t minAge;
	uint8_t maxAge;
	uint8_t minCA;
	uint8_t maxCA;
	uint8_t minPA;
	uint8_t maxPA;
} FilterOptions;

typedef struct {
	FilterBuffer filterBuffer;
	FilterOptions filterOptions;
	Options options;
	char gameVersion[GAME_STATUS_STRING_BUFFER_SIZE];
	SearchDatalist *dataList;
	SharedPointer *searchResults;
	DayMonthYear currentDate;
	GtkBuilder *builder;
	GtkApplication *app;
	Club *clubs;
	Nation *nations;
	Player *players;
	uint64_t gameKey;
	uint64_t clubCount;
	uint64_t nationCount;
	uint64_t playerCount;
} GameContext;
