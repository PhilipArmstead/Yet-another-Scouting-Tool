// SPDX-FileCopyrightText: © 2026 Phil Armstead <philarmstead@mailbox.org>
// SPDX-License-Identifier: GPL-3.0-or-later

#include "game.h"
#include "app/config.h"
#include "app/constants.h"
#include "app/maths.h"
#include "app/types.h"
#include "platform/platform.h"

#include <string.h>


DateTime game_parseDateTime(uint8_t dateTimeBytes[4]) {
	const uint8_t yearBytes[2] = {dateTimeBytes[2], dateTimeBytes[3]};
	const uint16_t year = (uint16_t)hexBytesToInt(yearBytes, 2);

	uint16_t days = (uint16_t)hexBytesToInt(dateTimeBytes, 1);
	if (dateTimeBytes[1] & 1) {
		days += 256;
	}

	const uint8_t time = dateTimeBytes[1] >> 1;

	// Adjust for leap year - if leap year and day > Feb 28, subtract 1
	if (days > 59) {
		const bool isLeapYear = !(year % 4) && (year % 100 || !(year % 400));
		days -= isLeapYear;
	}

	return (DateTime){.days = days, .year = year, .time = time};
}

// Assumes valid ProcessContext
DateTime game_getDateTime(const ProcessContext *context) {
#ifndef MOCKS_MODE
	uint8_t bytes[4];
	readFromMemory(context->handle, context->moduleBaseAddress + CURRENT_DATETIME_PTR_BASE, 4, bytes);
	return game_parseDateTime(bytes);
#else
	return (DateTime){.days = 210, .year = 2026, .time = 28};
#endif
}

// Assumes valid ProcessContext
void game_getVersion(const ProcessContext *context, char *versionBuffer, const uint8_t bufferSize) {
#ifndef MOCKS_MODE
	uint8_t bytes[4];
	void *handle = context->handle;
	readFromMemory(handle, context->moduleBaseAddress + GAME_VERSION_PTR_BASE, 4, bytes);
	readFromMemory(handle, hexBytesToInt(bytes, 4) + GAME_VERSION_PTR_OFFSET_1, 4, bytes);
	readFromMemory(
		handle,
		hexBytesToInt(bytes, 4) + GAME_VERSION_PTR_OFFSET_2,
		bufferSize - 1,
		(uint8_t*)versionBuffer
	);
	versionBuffer[bufferSize - 1] = '\0';
#else
	strncpy(versionBuffer, "24.4.2+2081827 (m.e v24.2.0.0)", bufferSize - 1);
#endif
}

// This serves two purposes:
//  1. If we can see the key, we have a save loaded (if not, we don't, or the game isn't running)
//  2. The key will change between loads, invalidating our cache
// I have arbitrarily decided that the memory address of the string value of the name of the first Nation is the key.
// Assumes valid ProcessContext
uint64_t game_getKey(const ProcessContext *context, GameKeyStatus *outStatus) {
#ifndef MOCKS_MODE
	uint8_t bytes[8];
	readFromMemory(context->handle, context->moduleBaseAddress + NATION_LIST_PTR_BASE, 8, bytes);
	const uint64_t nationPointerBase = hexBytesToInt(bytes, 8);
	if (nationPointerBase == 0) {
		*outStatus = GAME_KEY_NOT_FOUND;
		return 0;
	}
	readFromMemory(context->handle, hexBytesToInt(bytes, 8) + NATION_LIST_PTR_BASE_OFFSET, 8, bytes);

	readFromMemory(context->handle, hexBytesToInt(bytes, 8) + NATION_LIST_START, 8, bytes);
	const uint64_t nationStart = hexBytesToInt(bytes, 8);
	if (nationStart == 0) {
		*outStatus = GAME_KEY_NOT_FOUND;
		return 0;
	}
	readFromMemory(context->handle, nationStart, 8, bytes);
	readFromMemory(context->handle, hexBytesToInt(bytes, 8) + NATION_OFFSET_NAME, 8, bytes);
	const uint64_t nameAddress = hexBytesToInt(bytes, 8);
	if (nameAddress) {
		*outStatus = GAME_KEY_FOUND;
	} else {
		*outStatus = GAME_KEY_NULL;
	}
	return nameAddress;
#else
	*outStatus = GAME_KEY_NOT_FOUND;
	return 0x0123456789ABCDEF;
#endif
}
