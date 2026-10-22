#include "autocomplete.h"
#include "app/helpers/text.h"
#include "app/types.h"
#include "core/logger.h"
#include <stdatomic.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

extern GameContext gameContext;

typedef struct {
	SearchMatch match;
	char names[2][128];
	char *folded[2], *alternate[2];
} Entry;

typedef struct {
	atomic_uint references;
	Entry *entries;
	size_t count;
	bool nations;
} Index;

typedef struct {
	Index *index;
	char query[128];
	SearchMatch matches[AUTOCOMPLETE_LIMIT];
	size_t count;
	bool failed;
	atomic_bool done, cancel;
} SearchJob;

static void release(Index *index) {
	if (!index || atomic_fetch_sub(&index->references, 1) != 1)
		return;
	for (size_t i = 0; i < index->count; ++i) {
		for (unsigned j = 0; j < 2; ++j) {
			free(index->entries[i].folded[j]);
			free(index->entries[i].alternate[j]);
		}
	}
	free(index->entries);
	free(index);
}

bool autocomplete_rebuild(Autocomplete *search, const bool nations) {
	Index *index = calloc(1, sizeof(*index));
	if (!index) {
		LOG_ERROR("Could not allocate autocomplete index");
		return false;
	}
	atomic_init(&index->references, 1);
	index->nations = nations;
	const size_t count = (size_t)(nations ? gameContext.nationCount : gameContext.clubCount);
	index->entries = count ? calloc(count, sizeof(Entry)) : NULL;
	if (count && !index->entries) {
		release(index);
		LOG_ERROR("Could not allocate autocomplete entries");
		return false;
	}
	for (size_t i = 0; i < count; ++i) {
		const char *name = nations ? gameContext.nations[i].name : gameContext.clubs[i].name;
		if (!name[0] || (nations && i >= 255))
			continue;
		Entry *entry = &index->entries[index->count++];
		entry->match.index = i;
		if (nations) {
			snprintf(entry->match.label, sizeof(entry->match.label), "%s (%s)", name, gameContext.nations[i].code);
			snprintf(entry->names[0], sizeof(entry->names[0]), "%s", name);
			snprintf(entry->names[1], sizeof(entry->names[1]), "%s", gameContext.nations[i].code);
		} else {
			const char *shortName = gameContext.clubs[i].shortName;
			snprintf(entry->match.label, sizeof(entry->match.label), "%s", shortName[0] ? shortName : name);
			snprintf(entry->names[0], sizeof(entry->names[0]), "%s", name);
			snprintf(entry->names[1], sizeof(entry->names[1]), "%s", shortName);
		}
	}
	release(search->index);
	search->index = index;
	search->count = 0;
	search->pending = search->query[0] != '\0';
	if (search->job)
		atomic_store(&((SearchJob *)search->job)->cancel, true);
	return true;
}

void autocomplete_query(Autocomplete *search, const char *query) {
	if (!strcmp(search->query, query))
		return;
	snprintf(search->query, sizeof(search->query), "%s", query);
	search->count = 0;
	search->pending = true;
	if (search->job)
		atomic_store(&((SearchJob *)search->job)->cancel, true);
}

static int compareMatches(const void *a, const void *b) {
	return strcmp(((const SearchMatch *)a)->label, ((const SearchMatch *)b)->label);
}

static void executeSearch(void *argument) {
	SearchJob *job = argument;
	char *query = text_fold(job->query, false);
	if (!query) {
		job->failed = true;
		goto done;
	}
	for (size_t i = 0; i < job->index->count && job->count < AUTOCOMPLETE_LIMIT; ++i) {
		if (atomic_load(&job->cancel))
			break;
		Entry *entry = &job->index->entries[i];
		bool matched = false;
		for (unsigned j = 0; j < 2; ++j) {
			if (!entry->folded[j])
				entry->folded[j] = text_fold(entry->names[j], false);
			if (!entry->alternate[j])
				entry->alternate[j] = text_fold(entry->names[j], true);
			if (!entry->folded[j] || !entry->alternate[j]) {
				job->failed = true;
				break;
			}
			matched |= entry->names[j][0] && text_matches(query, entry->folded[j], entry->alternate[j]);
		}
		if (job->failed)
			break;
		if (matched)
			job->matches[job->count++] = entry->match;
	}
	qsort(job->matches, job->count, sizeof(SearchMatch), compareMatches);
	free(query);
done:
	atomic_store(&job->done, true);
}

bool autocomplete_poll(Autocomplete *search) {
	bool changed = false;
	if (search->job) {
		SearchJob *job = search->job;
		if (!atomic_load(&job->done))
			return false;
		thread_join(&search->thread);
		if (!atomic_load(&job->cancel) && job->index == search->index && !strcmp(job->query, search->query)) {
			if (job->failed) {
				snprintf(search->error, sizeof(search->error), "Could not normalize autocomplete names. See the log.");
				search->count = 0;
			} else {
				search->error[0] = '\0';
				memcpy(search->matches, job->matches, job->count * sizeof(SearchMatch));
				search->count = job->count;
			}
			changed = true;
		}
		release(job->index);
		free(job);
		search->job = NULL;
	}
	if (search->pending && search->index) {
		search->pending = false;
		Index *index = search->index;
		if (strlen(search->query) < (index->nations ? 1u : 3u))
			return changed;
		SearchJob *job = calloc(1, sizeof(*job));
		if (!job) {
			snprintf(search->error, sizeof(search->error), "Could not allocate autocomplete request");
			LOG_ERROR("%s", search->error);
			return true;
		}
		job->index = index;
		atomic_fetch_add(&index->references, 1);
		atomic_init(&job->done, false);
		atomic_init(&job->cancel, false);
		snprintf(job->query, sizeof(job->query), "%s", search->query);
		if (!thread_start(&search->thread, executeSearch, job)) {
			release(index);
			free(job);
			snprintf(search->error, sizeof(search->error), "Could not start autocomplete worker. See the log.");
			return true;
		}
		search->job = job;
	}
	return changed;
}

void autocomplete_destroy(Autocomplete *search) {
	SearchJob *job = search->job;
	if (job) {
		atomic_store(&job->cancel, true);
		thread_join(&search->thread);
		release(job->index);
		free(job);
	}
	release(search->index);
	*search = (Autocomplete){0};
}
