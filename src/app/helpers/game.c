// SPDX-FileCopyrightText: © 2026 Phil Armstead <philarmstead@mailbox.org>
// SPDX-License-Identifier: GPL-3.0-or-later

#include "game.h"
#include "app/config.h"
#include "app/constants.h"
#include "app/maths.h"
#include "app/types.h"
#include "platform/platform.h"

#include <string.h>


#define DAYS_IN_JANUARY 31
#define DAYS_IN_FEBRUARY 28
#define DAYS_IN_MARCH 31
#define DAYS_IN_APRIL 30
#define DAYS_IN_MAY 31
#define DAYS_IN_JUNE 30
#define DAYS_IN_JULY 31
#define DAYS_IN_AUGUST 31
#define DAYS_IN_SEPTEMBER 30
#define DAYS_IN_OCTOBER 31
#define DAYS_IN_NOVEMBER 30
#define DAYS_IN_DECEMBER 31

#define DAYS_BEFORE_FEBRUARY DAYS_IN_JANUARY
#define DAYS_BEFORE_MARCH (DAYS_IN_FEBRUARY + DAYS_BEFORE_FEBRUARY)
#define DAYS_BEFORE_APRIL (DAYS_IN_MARCH + DAYS_BEFORE_MARCH)
#define DAYS_BEFORE_MAY (DAYS_IN_APRIL + DAYS_BEFORE_APRIL)
#define DAYS_BEFORE_JUNE (DAYS_IN_MAY + DAYS_BEFORE_MAY)
#define DAYS_BEFORE_JULY (DAYS_IN_JUNE + DAYS_BEFORE_JUNE)
#define DAYS_BEFORE_AUGUST (DAYS_IN_JULY + DAYS_BEFORE_JULY)
#define DAYS_BEFORE_SEPTEMBER (DAYS_IN_AUGUST + DAYS_BEFORE_AUGUST)
#define DAYS_BEFORE_OCTOBER (DAYS_IN_SEPTEMBER + DAYS_BEFORE_SEPTEMBER)
#define DAYS_BEFORE_NOVEMBER (DAYS_IN_OCTOBER + DAYS_BEFORE_OCTOBER)
#define DAYS_BEFORE_DECEMBER (DAYS_IN_NOVEMBER + DAYS_BEFORE_NOVEMBER)

static Date getDate(const ProcessContext *context);

// Assumes valid ProcessContext
DayMonthYear game_getDayMonthYear(const ProcessContext *context) {
	#ifndef MOCKS_MODE
	const Date date = getDate(context);

	static const struct {
		uint16_t threshold;
		uint16_t offset;
		const char *name;
	} months[12] = {
		{DAYS_BEFORE_FEBRUARY, 0, "January"},
		{DAYS_BEFORE_MARCH, DAYS_BEFORE_FEBRUARY, "February"},
		{DAYS_BEFORE_APRIL, DAYS_BEFORE_MARCH, "March"},
		{DAYS_BEFORE_MAY, DAYS_BEFORE_APRIL, "April"},
		{DAYS_BEFORE_JUNE, DAYS_BEFORE_MAY, "May"},
		{DAYS_BEFORE_JULY, DAYS_BEFORE_JUNE, "June"},
		{DAYS_BEFORE_AUGUST, DAYS_BEFORE_JULY, "July"},
		{DAYS_BEFORE_SEPTEMBER, DAYS_BEFORE_AUGUST, "August"},
		{DAYS_BEFORE_OCTOBER, DAYS_BEFORE_SEPTEMBER, "September"},
		{DAYS_BEFORE_NOVEMBER, DAYS_BEFORE_OCTOBER, "October"},
		{DAYS_BEFORE_DECEMBER, DAYS_BEFORE_NOVEMBER, "November"},
		{UINT16_MAX, DAYS_BEFORE_DECEMBER, "December"},
	};

	DayMonthYear dayMonthYear = {.year = date.year};
	for (uint8_t i = 0; i < 12; ++i) {
		if (date.days <= months[i].threshold) {
			dayMonthYear.day = date.days - months[i].offset;
			strncpy(dayMonthYear.month, months[i].name, MONTH_NAME_LENGTH - 1);
			dayMonthYear.month[MONTH_NAME_LENGTH - 1] = '\0';
			break;
		}
	}
	#else
	const DayMonthYear dayMonthYear = {.day = 11, .month = "August", .year = 2026};
	#endif

	return dayMonthYear;
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
}

// Assumes valid ProcessContext
static Date getDate(const ProcessContext *context) {
	uint8_t bytes[4];
	readFromMemory(context->handle, context->moduleBaseAddress + CURRENT_DATETIME_PTR_BASE, 4, bytes);

	const uint8_t yearBytes[2] = {bytes[2], bytes[3]};
	const uint16_t year = (uint16_t)hexBytesToInt(yearBytes, 2);

	uint16_t days = (uint16_t)hexBytesToInt(bytes, 1);
	if (bytes[1] & 1) {
		days += 256;
	}

	// Adjust for leap year - if leap year and day > Feb 28, subtract 1
	if (days > 59) {
		const bool isLeapYear = !(year % 4) && (year % 100 || !(year % 400));
		days -= isLeapYear;
	}

	return (Date){days, year};
}
