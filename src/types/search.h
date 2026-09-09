// SPDX-FileCopyrightText: © 2026 Phil Armstead <philarmstead@mailbox.org>
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#define CLUB_SEARCH_LIMIT 30
#define CLUB_SEARCH_NAME_LENGTH 40

typedef struct {
	GtkSearchEntry *entry;
	GtkPopover *popover;
	GtkListBox *listBox;
} SearchDatalist;

typedef struct {
	char searchValue[CLUB_SEARCH_NAME_LENGTH];
	SearchDatalist *dataList;
	// Store results from the worker thread
	uint64_t clubIndices[CLUB_SEARCH_LIMIT];
	uint64_t count;
} SearchContext;
