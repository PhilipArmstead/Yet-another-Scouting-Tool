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

static inline float srgbToLinear(const float channel) {
	return channel <= 0.04045f ? channel / 12.92f : powf((channel + 0.055f) / 1.055f, 2.4f);
}

static inline float linearToSrgb(const float channel) {
	return channel <= 0.0031308f ? channel * 12.92f : 1.055f * powf(channel, 1.f / 2.4f) - 0.055f;
}

/**
 * Darkens `rgb` just enough for its WCAG relative luminance to reach `maxLuminance`, leaving hue
 * untouched. The scaling is done in linear light, where luminance is linear in each channel, so a
 * single multiply lands exactly on the target rather than needing to iterate.
 */
static inline RGB capLuminance(const RGB rgb, const float maxLuminance) {
	const float r = srgbToLinear(rgb.r);
	const float g = srgbToLinear(rgb.g);
	const float b = srgbToLinear(rgb.b);
	const float luminance = 0.2126f * r + 0.7152f * g + 0.0722f * b;
	if (luminance <= maxLuminance) {
		return rgb;
	}

	const float scale = maxLuminance / luminance;
	return (RGB){
		.r = linearToSrgb(r * scale),
		.g = linearToSrgb(g * scale),
		.b = linearToSrgb(b * scale),
	};
}
