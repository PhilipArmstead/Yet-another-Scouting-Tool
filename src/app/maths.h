// SPDX-FileCopyrightText: © 2026 Phil Armstead <philarmstead@mailbox.org>
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include "types.h"

#include <math.h>


// Assumes little-endian byte order
static inline uint64_t hexBytesToInt(const uint8_t *bytes, const uint8_t length) {
	uint64_t value = 0;
	for (uint8_t i = 0; i < length; ++i) {
		value |= (uint64_t)bytes[i] << (8 * i);
	}
	return value;
}

// Converts a number 0-100 to 1-20
static inline uint8_t convertTo20Scale(const uint8_t value) {
	return (uint8_t)((value + 4) / 5);
}

typedef struct {
	float r;
	float g;
	float b;
} RGB;

static inline RGB hueToRgb(float hue) {
	RGB rgb = {0};
	hue = fmodf(hue, 360.f);
	if (hue < 0.f) {
		hue += 360.f;
	}

	const float x = 1.f - fabsf(fmodf(hue / 60.f, 2.f) - 1.f);

	if (hue < 60.0) {
		rgb.r = 1.f;
		rgb.g = x;
	} else if (hue < 120.0) {
		rgb.r = x;
		rgb.g = 1.f;
	} else if (hue < 180.0) {
		rgb.g = 1.f;
		rgb.b = x;
	} else if (hue < 240.0) {
		rgb.g = x;
		rgb.b = 1.f;
	} else if (hue < 300.0) {
		rgb.r = x;
		rgb.b = 1.f;
	} else {
		rgb.r = 1.f;
		rgb.b = x;
	}
	return rgb;
}

static inline void hueToHex(const float hue, char hex[8]) {
	const RGB rgb = hueToRgb(hue);
	snprintf(
		hex,
		8,
		"#%02x%02x%02x",
		(uint8_t)roundf(rgb.r * 255.f),
		(uint8_t)roundf(rgb.g * 255.f),
		(uint8_t)roundf(rgb.b * 255.f)
	);
}
