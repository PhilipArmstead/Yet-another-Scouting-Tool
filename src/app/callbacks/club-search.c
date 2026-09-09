// SPDX-FileCopyrightText: © 2026 Phil Armstead <philarmstead@mailbox.org>
// SPDX-License-Identifier: GPL-3.0-or-later

#include "club-search.h"
#include "app/search-handler.h"
#include "app/ui.h"
#include "app/callbacks/filters.h"
#include "core/logger.h"
#include "platform/platform.h"


extern GameContext gameContext;

static void runThreadedSearch(SearchContext *context);

void callbacks_OnClubNameChange(GtkEditable *editable, SearchDatalist *dataList) {
	gameContext.filterOptions.filterMask &= ~(uint32_t)FILTER_HAS_CLUB;

	const char *searchValue = gtk_editable_get_text(editable);
	if (strlen(searchValue) < 3) {
		gtk_popover_popdown(dataList->popover);
		return;
	}

	SearchContext *context = malloc(sizeof(SearchContext));
	context->dataList = dataList;
	context->count = 0;
	strncpy(context->searchValue, searchValue, CLUB_SEARCH_NAME_LENGTH - 1);
	context->searchValue[CLUB_SEARCH_NAME_LENGTH - 1] = '\0';

	runThreadedSearch(context);
}

void callbacks_onClubNameSelected(
	const GtkListBox *box,
	GtkListBoxRow *row,
	const SearchDatalist *dataList
) {
	(void)box;

	if (row == NULL) {
		return;
	}

	GtkWidget *child = gtk_list_box_row_get_child(row);
	const char *text = gtk_label_get_text(GTK_LABEL(child));

	// Block the change handler while we update the text
	g_signal_handlers_block_by_func(
		dataList->entry,
		(gpointer)(uintptr_t)callbacks_OnClubNameChange,
		(gpointer)dataList
	);

	gtk_editable_set_text(GTK_EDITABLE(dataList->entry), text);

	// Unblock the handler
	g_signal_handlers_unblock_by_func(
		dataList->entry,
		(gpointer)(uintptr_t)callbacks_OnClubNameChange,
		(gpointer)dataList
	);

	gtk_popover_popdown(dataList->popover);

	gameContext.filterOptions.filterMask |= FILTER_HAS_CLUB;
	gameContext.filterOptions.clubIndex = GPOINTER_TO_INT(g_object_get_data(G_OBJECT(child), "index"));
	char buffer[48] = {0};
	snprintf(buffer, 48, "Club: %s", gameContext.clubs[gameContext.filterOptions.clubIndex].shortName);
	ui_createClubFilterTag(buffer, GTK_EDITABLE(dataList->entry));

	// NOTE: this is the same as the on-enter handler
	searchHandler_doSearch(true);
	callbacks_updateFilterTags();
}

// This runs on the main thread; it's safe to modify the UI
static gboolean updateUIWithResults(gpointer userData) {
	SearchContext *context = userData;
	const SearchDatalist *dataList = context->dataList;
	gtk_popover_popdown(dataList->popover);

	// Clear the list
	GtkWidget *child;
	while ((child = gtk_widget_get_first_child(GTK_WIDGET(dataList->listBox))) != NULL) {
		gtk_list_box_remove(dataList->listBox, GTK_WIDGET(child));
	}

	// Add matching options using results from worker thread
	for (uint64_t i = 0; i < context->count; i++) {
		const uint64_t clubIndex = context->clubIndices[i];
		GtkWidget *label = gtk_label_new(gameContext.clubs[clubIndex].shortName);
		g_object_set_data(G_OBJECT(label), "index", GINT_TO_POINTER(clubIndex));
		gtk_widget_set_halign(label, GTK_ALIGN_START);
		gtk_list_box_append(dataList->listBox, label);
	}

	// Show/hide popover based on matches
	if (gtk_widget_get_first_child(GTK_WIDGET(dataList->listBox)) != NULL) {
		gtk_popover_popup(dataList->popover);
	} else {
		gtk_popover_popdown(dataList->popover);
	}

	free(context);
	return G_SOURCE_REMOVE; // Don't repeat this callback
}

// This runs on the worker thread; CPU-intensive work only
static void runSearch(SearchContext *context) {
	const int64_t timeStart = platform_getMicroseconds();
	const char *searchValue = context->searchValue;

	// Add matching options
	context->count = 0;
	for (uint64_t i = 0; i < gameContext.clubCount && context->count < CLUB_SEARCH_LIMIT; i++) {
		if (
			g_str_match_string(searchValue, gameContext.clubs[i].name, TRUE)
			|| g_str_match_string(searchValue, gameContext.clubs[i].shortName, TRUE)
		) {
			context->clubIndices[context->count] = i;
			++context->count;
		}
	}

	for (uint32_t i = 1; i < context->count; ++i) {
		const uint64_t clubIndex = context->clubIndices[i];
		char *key = gameContext.clubs[clubIndex].shortName;
		int64_t j = i - 1;

		while (j >= 0 && strncmp(gameContext.clubs[context->clubIndices[j]].shortName, key, CLUB_SHORT_NAME_LENGTH) > 0) {
			context->clubIndices[j + 1] = context->clubIndices[j];
			j--;
		}
		context->clubIndices[j + 1] = clubIndex;
	}

	const int64_t timeEnd = platform_getMicroseconds();
	LOG_DEBUG("Searched %zu clubs in %zu microseconds", context->count, timeEnd - timeStart);

	// Schedule UI update on the main thread
	g_idle_add(updateUIWithResults, context);
}

#ifdef ARCH_WIN
static DWORD WINAPI threadFunction(LPVOID arg) {
	runSearch(arg);
	return 0;
}
#else
static void *threadFunction(void *arg) {
	runSearch(arg);
	return NULL;
}
#endif

static void runThreadedSearch(SearchContext *context) {
#ifdef ARCH_WIN
	if (CreateThread(
		NULL,
		0,
		threadFunction,
		context,
		0,
		NULL
	) == NULL) {
		LOG_ERROR("Failed to create thread.");
		free(context);
	}
#else
	pthread_t thread;
	if (pthread_create(&thread, NULL, threadFunction, context) != 0) {
		LOG_ERROR("Failed to create club-search thread");
		free(context);
	} else {
		pthread_detach(thread);
	}
#endif
}
