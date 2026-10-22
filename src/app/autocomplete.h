#pragma once
#include "platform/thread.h"
#include <stddef.h>
#include <stdint.h>

#define AUTOCOMPLETE_LIMIT 30
typedef struct {
	uint64_t index;
	char label[128];
} SearchMatch;
typedef struct {
	void *index, *job;
	Thread thread;
	char query[128];
	char error[96];
	SearchMatch matches[AUTOCOMPLETE_LIMIT];
	size_t count;
	bool pending;
} Autocomplete;

bool autocomplete_rebuild(Autocomplete *search, bool nations);
void autocomplete_query(Autocomplete *search, const char *query);
bool autocomplete_poll(Autocomplete *search);
void autocomplete_destroy(Autocomplete *search);
