// SPDX-FileCopyrightText: © 2026 Phil Armstead <philarmstead@mailbox.org>
// SPDX-License-Identifier: GPL-3.0-or-later

#include "filters.h"
#include "app/player-table.h"
#include "app/search-handler.h"
#include "app/ui.h"


extern GameContext gameContext;

G_MODULE_EXPORT void callbacks_onFiltersClear(void) {
	searchHandler_clearFilters();

	ui_clearFilterTags();
	playerTable_clear();
}

G_MODULE_EXPORT gboolean callbacks_onFiltersKeypress(
	GtkEventControllerKey *controller,
	guint keyval,
	guint keycode,
	GdkModifierType state,
	gpointer data
) {
	(void)controller;
	(void)keycode;
	(void)state;
	(void)data;

	if (keyval == GDK_KEY_Return) {
		searchHandler_doSearch(true);
		callbacks_updateFilterTags();

		return TRUE;
	}

	return FALSE;
}

G_MODULE_EXPORT void callbacks_onFilterRun(void) {
	searchHandler_doSearch(true);
	callbacks_updateFilterTags();
}

void callbacks_updateFilterTags(void) {
	ui_clearFilterTags();

	const FilterOptions options = gameContext.filterOptions;
	const FilterBuffer fb = gameContext.filterBuffer;

	char buffer[32] = {0};
	if (options.filterMask & FILTER_HAS_MIN_AGE) {
		snprintf(buffer, 32, "Age ≥ %d", options.minAge);
		ui_createFilterTag(buffer, fb.minAge);
	} else {
		gtk_entry_buffer_set_text(fb.minAge, "", 1);
	}
	if (options.filterMask & FILTER_HAS_MAX_AGE) {
		snprintf(buffer, 32, "Age ≤ %d", options.maxAge);
		ui_createFilterTag(buffer, fb.maxAge);
	} else {
		gtk_entry_buffer_set_text(fb.maxAge, "", 1);
	}
	if (options.filterMask & FILTER_HAS_MIN_CA) {
		snprintf(buffer, 32, "CA ≥ %d", options.minCA);
		ui_createFilterTag(buffer, fb.minCA);
	} else {
		gtk_entry_buffer_set_text(fb.minCA, "", 1);
	}
	if (options.filterMask & FILTER_HAS_MAX_CA) {
		snprintf(buffer, 32, "CA ≤ %d", options.maxCA);
		ui_createFilterTag(buffer, fb.maxCA);
	} else {
		gtk_entry_buffer_set_text(fb.maxCA, "", 1);
	}
	if (options.filterMask & FILTER_HAS_MIN_PA) {
		snprintf(buffer, 32, "PA ≥ %d", options.minPA);
		ui_createFilterTag(buffer, fb.minPA);
	} else {
		gtk_entry_buffer_set_text(fb.minPA, "", 1);
	}
	if (options.filterMask & FILTER_HAS_MAX_PA) {
		snprintf(buffer, 32, "PA ≤ %d", options.maxPA);
		ui_createFilterTag(buffer, fb.maxPA);
	} else {
		gtk_entry_buffer_set_text(fb.maxPA, "", 1);
	}
	if (options.filterMask & FILTER_HAS_MIN_RATING) {
		snprintf(buffer, 32, "Rating ≥ %f.2%%", options.minRating);
		ui_createFilterTag(buffer, fb.minRating);
	} else {
		gtk_entry_buffer_set_text(fb.minRating, "", 1);
	}
	if (options.filterMask & FILTER_HAS_MAX_RATING) {
		snprintf(buffer, 32, "Rating ≤ %f.2%%", options.maxRating);
		ui_createFilterTag(buffer, fb.maxRating);
	} else {
		gtk_entry_buffer_set_text(fb.maxRating, "", 1);
	}
	if (options.filterMask & FILTER_HAS_CLUB) {
		ui_createClubFilterTag(gameContext.clubs[options.clubIndex].shortName, GTK_EDITABLE(gameContext.dataList->entry));
	}
}
