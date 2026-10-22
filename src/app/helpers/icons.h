// SPDX-FileCopyrightText: © 2026 Phil Armstead <philarmstead@mailbox.org>
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include "raylib.h"
#include <stdint.h>


#define CONDITION_MAX 10000.f

typedef enum {
	YAST_ICON_HEART,
	YAST_ICON_FACE,
	YAST_ICON_SHOE_LEFT,
	YAST_ICON_SHOE_RIGHT,
	YAST_ICON_COUNT
} IconId;

bool icons_init(void);
void icons_draw(IconId id, Rectangle bounds, uint8_t quality);
void icons_shutdown(void);

// Condition, sharpness and fatigue are all stored values 1-10000
static inline uint8_t icons_conditionQuantise(const float condition) {
	const float clamped = condition < 0.f ? 0.f : (condition < CONDITION_MAX ? condition : CONDITION_MAX);
	return (uint8_t)(clamped * (120.f / CONDITION_MAX));
}

// Morale is a 1-20 rating, widened to the 1-120 index the colour gradient works in
static inline uint8_t icons_moraleQuantise(const uint8_t morale) {
	return morale * 6;
}

// Footedness is a 1-100 rating, widened to the same 1-120 index
static inline uint8_t icons_footQuantise(const uint8_t footedness) {
	return (uint8_t)((float)footedness * 1.2f);
}
