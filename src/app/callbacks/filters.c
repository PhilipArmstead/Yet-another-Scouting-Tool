// SPDX-FileCopyrightText: © 2026 Phil Armstead <philarmstead@mailbox.org>
// SPDX-License-Identifier: GPL-3.0-or-later

#include "filters.h"
#include "app/entities.h"
#include "app/player-table.h"
#include "app/search-handler.h"
#include "app/ui.h"
#include "app/helpers/vector.h"
#include "core/logger.h"


extern GameContext gameContext;

// Set while we mutate filter widgets ourselves, so their change signals do not re-run a search
static bool filtersAreResetting = false;

G_MODULE_EXPORT void callbacks_onFiltersClear(void) {
	filtersAreResetting = true;
	searchHandler_clearFilters();
	filtersAreResetting = false;

	ui_clearFilterTags();
	playerTable_clear();
}

G_MODULE_EXPORT void callbacks_onPositionToggled(GtkCheckButton *button, gpointer data) {
	(void)button;
	(void)data;

	if (filtersAreResetting) {
		return;
	}

	searchHandler_cacheFilters();
	if (gameContext.filterOptions.filterMask) {
		searchHandler_doSearch(false);
	} else {
		playerTable_clear();
	}

	callbacks_updateFilterTags();
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
		snprintf(buffer, 32, "Rating ≥ %.2f%%", options.minRating);
		ui_createFilterTag(buffer, fb.minRating);
	} else {
		gtk_entry_buffer_set_text(fb.minRating, "", 1);
	}
	if (options.filterMask & FILTER_HAS_MAX_RATING) {
		snprintf(buffer, 32, "Rating ≤ %.2f%%", options.maxRating);
		ui_createFilterTag(buffer, fb.maxRating);
	} else {
		gtk_entry_buffer_set_text(fb.maxRating, "", 1);
	}
	if (options.filterMask & FILTER_HAS_CLUB) {
		const Club *club = entities_getClub(options.clubIndex);
		if (club != NULL) {
			ui_createClubFilterTag(club->shortName, GTK_EDITABLE(gameContext.clubDatalist->entry));
		}
	}
	if (options.filterMask & FILTER_HAS_NATIONALITY) {
		const Nation *nation = entities_getNation(options.nationalityIndex);
		if (nation != NULL) {
			ui_createNationalityFilterTag(nation->name, GTK_EDITABLE(gameContext.nationalityDatalist->entry));
		}
	}

	if (options.filterMask & FILTER_HAS_POSITION) {
		// Indices line up with the POSITION_MASK_* bit positions
		static const char *const names[] = {
			"GK",
			"DL",
			"DC",
			"DR",
			"WBL",
			"DM",
			"WBR",
			"ML",
			"MC",
			"MR",
			"AML",
			"AMC",
			"AMR",
			"ST",
		};
		const CheckBox *checks = &gameContext.checkboxes;
		GtkCheckButton *const buttons[] = {
			checks->positionGK,
			checks->positionDL,
			checks->positionDC,
			checks->positionDR,
			checks->positionWBL,
			checks->positionDM,
			checks->positionWBR,
			checks->positionML,
			checks->positionMC,
			checks->positionMR,
			checks->positionAML,
			checks->positionAMC,
			checks->positionAMR,
			checks->positionST,
		};

		for (uint8_t i = 0; i < G_N_ELEMENTS(names); ++i) {
			if (options.positions & (1u << i)) {
				ui_createPositionFilterTag(names[i], buttons[i]);
			}
		}
	}

	if (!options.filterMask && gameContext.searchResults != NULL && vector_length(gameContext.searchResults->data) > 0) {
		ui_createFilterTag("Showing: all", NULL);
	}
}
