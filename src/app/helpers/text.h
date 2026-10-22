#pragma once
#include <stdbool.h>
#include <stddef.h>

char *text_fold(const char *text, bool alternate);
bool text_matches(const char *query, const char *folded, const char *alternate);
size_t text_previous(const char *text, size_t position);
size_t text_next(const char *text, size_t position);
