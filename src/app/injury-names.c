// SPDX-FileCopyrightText: © 2026 Phil Armstead <philarmstead@mailbox.org>
// SPDX-License-Identifier: GPL-3.0-or-later

#include "injury-names.h"
#include "core/logger.h"

#include <glib.h>
#include <stdlib.h>
#include <string.h>


// The game ships a couple of hundred injury names; overflowing the table costs a name, not a crash.
#define INJURY_NAME_CAPACITY 1024
#define INJURY_NAME_SLOT_COUNT 2048 // Power of two, ≥ 2 × capacity so the table stays half empty.

/**
 * Interned once and kept for the life of the process: the strings are static game data, and holding
 * them means an index stored by a player — or by a window's snapshot — never goes stale, even after
 * the cache that produced it has been thrown away.
 *
 * The cache workers intern concurrently under `lock`, while the UI reads without it: `count` is only
 * published once the string it covers has been stored, so any index a reader can see is readable.
 */
static const char *names[INJURY_NAME_CAPACITY] = {""};
static uint16_t slots[INJURY_NAME_SLOT_COUNT];
static gint count = 1;
static GMutex lock;

static uint32_t hashName(const char *name, const uint8_t length) {
	uint32_t hash = 2166136261u;
	for (uint8_t i = 0; i < length; ++i) {
		hash = (hash ^ (uint8_t)name[i]) * 16777619u;
	}

	return hash;
}

uint16_t injuryNames_intern(const char *name, const uint8_t length) {
	if (length == 0) {
		return INJURY_NAME_NONE;
	}

	g_mutex_lock(&lock);

	uint32_t slot = hashName(name, length) & (INJURY_NAME_SLOT_COUNT - 1);
	while (slots[slot] != INJURY_NAME_NONE) {
		const char *candidate = names[slots[slot]];
		if (candidate[length] == '\0' && memcmp(candidate, name, length) == 0) {
			const uint16_t index = slots[slot];
			g_mutex_unlock(&lock);
			return index;
		}
		slot = slot + 1 & INJURY_NAME_SLOT_COUNT - 1;
	}

	if (count == INJURY_NAME_CAPACITY) {
		g_mutex_unlock(&lock);
		LOG_WARN("Injury name table is full; \"%.*s\" will show without a name", length, name);
		return INJURY_NAME_NONE;
	}

	char *copy = malloc((size_t)length + 1);
	if (copy == NULL) {
		g_mutex_unlock(&lock);
		LOG_ERROR("Failed to allocate memory for an injury name");
		return INJURY_NAME_NONE;
	}

	memcpy(copy, name, length);
	copy[length] = '\0';

	const uint16_t index = (uint16_t)count;
	names[index] = copy;
	// Published last: a reader that sees this count is guaranteed to see the string behind it.
	g_atomic_int_set(&count, count + 1);
	slots[slot] = index;

	g_mutex_unlock(&lock);

	return index;
}

const char *injuryNames_get(const uint16_t index) {
	return index < (uint16_t)g_atomic_int_get(&count) ? names[index] : "";
}

void injuryNames_destroy(void) {
	g_mutex_lock(&lock);

	for (gint i = 1; i < count; ++i) {
		free((void*)names[i]);
		names[i] = NULL;
	}
	memset(slots, 0, sizeof(slots));
	g_atomic_int_set(&count, 1);

	g_mutex_unlock(&lock);
}
