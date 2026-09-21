// SPDX-FileCopyrightText: © 2026 Phil Armstead <philarmstead@mailbox.org>
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#define NATIONALITY_SEARCH_LIMIT 30
#define CLUB_SEARCH_LIMIT 30
#define CLUB_SEARCH_NAME_LENGTH 40

typedef struct {
	GtkSearchEntry *entry;
	GtkPopover *popover;
	GtkListBox *listBox;
} SearchDatalist;

/**
 * The name is copied out of the clubs buffer while the worker still holds the lock, so the popover
 * can be rendered from this alone. Re-reading gameContext.clubs on the main thread would race the
 * cache, which frees and republishes that buffer whenever the save advances.
 */
typedef struct {
	char shortName[CLUB_SHORT_NAME_LENGTH];
	uint64_t index;
} ClubMatch;

typedef struct {
	char searchValue[CLUB_SEARCH_NAME_LENGTH];
	SearchDatalist *dataList;
	// Store results from the worker thread
	ClubMatch matches[CLUB_SEARCH_LIMIT];
	uint64_t count;
	// Value of the club-search generation counter when this search started. Anything that no
	// longer matches has been superseded and is discarded rather than rendered.
	gint generation;
} SearchContext;
