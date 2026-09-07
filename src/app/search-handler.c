// SPDX-FileCopyrightText: © 2026 Phil Armstead <philarmstead@mailbox.org>
// SPDX-License-Identifier: GPL-3.0-or-later

#include "search-handler.h"
#include "app/player-table.h"
#include "app/search.h"

#include <gtk/gtk.h>


extern GameContext gameContext;

static inline bool valueFitsOneByte(int64_t value) {
	return value >= 0 && value <= 0xFF;
}

void searchHandler_cacheFilters(void) {
	FilterOptions *options = &gameContext.filterOptions;
	const uint32_t hasClub = options->filterMask & FILTER_HAS_CLUB;
	options->filterMask = hasClub;

	const gchar *minAge = gtk_entry_buffer_get_text(gameContext.filterBuffer.minAge);
	const gchar *maxAge = gtk_entry_buffer_get_text(gameContext.filterBuffer.maxAge);
	const gchar *minCA = gtk_entry_buffer_get_text(gameContext.filterBuffer.minCA);
	const gchar *maxCA = gtk_entry_buffer_get_text(gameContext.filterBuffer.maxCA);
	const gchar *minPA = gtk_entry_buffer_get_text(gameContext.filterBuffer.minPA);
	const gchar *maxPA = gtk_entry_buffer_get_text(gameContext.filterBuffer.maxPA);
	const gchar *minRating = gtk_entry_buffer_get_text(gameContext.filterBuffer.minRating);
	const gchar *maxRating = gtk_entry_buffer_get_text(gameContext.filterBuffer.maxRating);

	const CheckBox *checks = &gameContext.checkboxes;
	const bool isPositionGK = gtk_check_button_get_active(checks->positionGK);
	const bool isPositionDL = gtk_check_button_get_active(checks->positionDL);
	const bool isPositionDC = gtk_check_button_get_active(checks->positionDC);
	const bool isPositionDR = gtk_check_button_get_active(checks->positionDR);
	const bool isPositionWBL = gtk_check_button_get_active(checks->positionWBL);
	const bool isPositionDM = gtk_check_button_get_active(checks->positionDM);
	const bool isPositionWBR = gtk_check_button_get_active(checks->positionWBR);
	const bool isPositionML = gtk_check_button_get_active(checks->positionML);
	const bool isPositionMC = gtk_check_button_get_active(checks->positionMC);
	const bool isPositionMR = gtk_check_button_get_active(checks->positionMR);
	const bool isPositionAML = gtk_check_button_get_active(checks->positionAML);
	const bool isPositionAMC = gtk_check_button_get_active(checks->positionAMC);
	const bool isPositionAMR = gtk_check_button_get_active(checks->positionAMR);
	const bool isPositionST = gtk_check_button_get_active(checks->positionST);

	options->positions = (isPositionGK ? POSITION_MASK_GK : 0)
		| (isPositionDL ? POSITION_MASK_DL : 0)
		| (isPositionDC ? POSITION_MASK_DC : 0)
		| (isPositionDR ? POSITION_MASK_DR : 0)
		| (isPositionWBL ? POSITION_MASK_WBL : 0)
		| (isPositionDM ? POSITION_MASK_DM : 0)
		| (isPositionWBR ? POSITION_MASK_WBR : 0)
		| (isPositionML ? POSITION_MASK_ML : 0)
		| (isPositionMC ? POSITION_MASK_MC : 0)
		| (isPositionMR ? POSITION_MASK_MR : 0)
		| (isPositionAML ? POSITION_MASK_AML : 0)
		| (isPositionAMC ? POSITION_MASK_AMC : 0)
		| (isPositionAMR ? POSITION_MASK_AMR : 0)
		| (isPositionST ? POSITION_MASK_ST : 0);

	int64_t value;

	if (minAge != NULL && *minAge != '\0') {
		value = g_ascii_strtoll(minAge, NULL, 10);
		if (valueFitsOneByte(value)) {
			options->minAge = (uint8_t)value;
			options->filterMask |= FILTER_HAS_MIN_AGE;
		}
	}
	if (maxAge != NULL && *maxAge != '\0') {
		value = g_ascii_strtoll(maxAge, NULL, 10);
		if (valueFitsOneByte(value)) {
			options->maxAge = (uint8_t)value;
			options->filterMask |= FILTER_HAS_MAX_AGE;
		}
	}
	if (minCA != NULL && *minCA != '\0') {
		value = g_ascii_strtoll(minCA, NULL, 10);
		if (valueFitsOneByte(value)) {
			options->minCA = (uint8_t)value;
			options->filterMask |= FILTER_HAS_MIN_CA;
		}
	}
	if (maxCA != NULL && *maxCA != '\0') {
		value = g_ascii_strtoll(maxCA, NULL, 10);
		if (valueFitsOneByte(value)) {
			options->maxCA = (uint8_t)value;
			options->filterMask |= FILTER_HAS_MAX_CA;
		}
	}
	if (minPA != NULL && *minPA != '\0') {
		value = g_ascii_strtoll(minPA, NULL, 10);
		if (valueFitsOneByte(value)) {
			options->minPA = (uint8_t)value;
			options->filterMask |= FILTER_HAS_MIN_PA;
		}
	}
	if (maxPA != NULL && *maxPA != '\0') {
		value = g_ascii_strtoll(maxPA, NULL, 10);
		if (valueFitsOneByte(value)) {
			options->maxPA = (uint8_t)value;
			options->filterMask |= FILTER_HAS_MAX_PA;
		}
	}
	if (minRating != NULL && *minRating != '\0') {
		options->minRating = (float)g_ascii_strtod(minRating, NULL);
		options->filterMask |= FILTER_HAS_MIN_RATING;
	}
	if (maxRating != NULL && *maxRating != '\0') {
		options->maxRating = (float)g_ascii_strtod(maxRating, NULL);
		options->filterMask |= FILTER_HAS_MAX_RATING;
	}
}

void searchHandler_doSearch(const bool refreshFilterCache) {
	if (refreshFilterCache) {
		searchHandler_cacheFilters();
	}

	search_findPlayers();
	playerTable_populate();
}

void searchHandler_clearFilters(void) {
	gtk_entry_buffer_set_text(gameContext.filterBuffer.minAge, "", 1);
	gtk_entry_buffer_set_text(gameContext.filterBuffer.maxAge, "", 1);
	gtk_entry_buffer_set_text(gameContext.filterBuffer.minCA, "", 1);
	gtk_entry_buffer_set_text(gameContext.filterBuffer.maxCA, "", 1);
	gtk_entry_buffer_set_text(gameContext.filterBuffer.minPA, "", 1);
	gtk_entry_buffer_set_text(gameContext.filterBuffer.maxPA, "", 1);
	gtk_entry_buffer_set_text(gameContext.filterBuffer.minRating, "", 1);
	gtk_entry_buffer_set_text(gameContext.filterBuffer.maxRating, "", 1);
	gtk_editable_set_text(GTK_EDITABLE(gameContext.dataList->entry), "");
}
