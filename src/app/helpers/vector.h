// SPDX-FileCopyrightText: © 2026 Phil Armstead <philarmstead@mailbox.org>
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <stddef.h> // for max_align_t
#include <stdint.h> // for SIZE_MAX
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Metadata stored immediately BEFORE the user-visible pointer ("fat pointer").
// Note: a bare `size_t count; size_t capacity;` header is only incidentally
// aligned for common element types. To guarantee correct alignment for ANY
// element type, we over-align the header to max_align_t so the memory that
// follows it (the elements) is suitably aligned.
typedef struct {
	size_t count;
	size_t capacity;
	// Force the whole header (and therefore the byte right after it, where
	// element 0 lives) to be aligned for any standard type.
	_Alignas(max_align_t) char _align;
} Header;

// A larger initial capacity avoids the 1->2->4 realloc churn on the first
// few pushes that VECTOR_INITIAL_CAPACITY 1 caused.
#define VECTOR_INITIAL_CAPACITY 8

#define VECTOR__OOM()																								\
	do {																															\
		fprintf(stderr, "%s:%d: out of memory\n", __FILE__, __LINE__);	\
		abort();																												\
	} while (0)

// Checked size computation: elementSize*capacity + sizeof(Header) with overflow
// detection. Integer overflow here would under-allocate and turn a large push
// count into a heap buffer overflow, so we reject it up front.
static inline size_t vector__allocateSize(size_t elementSize, size_t capacity) {
	// capacity * elementSize must not overflow...
	if (capacity != 0 && elementSize > (SIZE_MAX - sizeof(Header)) / capacity) {
		VECTOR__OOM();
	}
	return elementSize * capacity + sizeof(Header);
}

// IMPORTANT: `vector` is expanded multiple times below, so it MUST be a plain,
// side-effect-free lvalue (e.g. a variable). Do NOT pass something like
// `*p++`. `x` is evaluated exactly once.
#define vector_push(vector, x)																										\
	do {																																						\
		if ((vector) == NULL) {																												\
			Header *header =																														\
				malloc(vector__allocateSize(sizeof(*(vector)), VECTOR_INITIAL_CAPACITY));	\
			if (header == NULL) VECTOR__OOM();																					\
			header->count = 0;																													\
			header->capacity = VECTOR_INITIAL_CAPACITY;																	\
			(vector) = (void*)(header + 1);																							\
		}																																							\
		Header *header = (Header*)(vector) - 1;																				\
		if (header->count >= header->capacity) {																			\
			size_t newCapacity = header->capacity * 2;																	\
			Header *newHeader =																												\
				realloc(header, vector__allocateSize(sizeof(*(vector)), newCapacity));		\
			if (newHeader == NULL) VECTOR__OOM();																			\
			header = newHeader;																												\
			header->capacity = newCapacity;																						\
			(vector) = (void*)(header + 1);																							\
		}																																							\
		(vector)[header->count++] = (x);																							\
	} while (0)

// Safe on a NULL (empty) vector: report length 0 instead of dereferencing
// ((Header*)NULL - 1)
#define vector_length(vector) ((vector) ? ((Header*)(vector) - 1)->count : (size_t)0)

// Last element helper (caller must ensure the vector is non-empty).
#define vector_last(vector) ((vector)[vector_length(vector) - 1])

// Remove and return the last element (caller must ensure non-empty).
#define vector_pop(vector) ((vector)[--((Header*)(vector) - 1)->count])

// Remove the element at `index` and copy following elements down by one.
// Safe on NULL and out-of-range indexes; `result` is left unchanged then.
#define vector_splice(vector, index, result)											\
	do {																														\
		size_t spliceIndex = (index);																	\
		if ((vector) != NULL) {																				\
			Header *header = (Header*)(vector) - 1;											\
			if (spliceIndex < header->count) {													\
				*(result) = (vector)[spliceIndex];												\
				memmove(																									\
					(vector) + spliceIndex,																	\
					(vector) + spliceIndex + 1,															\
					(header->count - spliceIndex - 1) * sizeof(*(vector)));	\
				--header->count;																					\
			}																														\
		}																															\
	} while (0)

// Ensure capacity for at least `n` total elements without changing count.
// Useful to pre-size and avoid repeated reallocs.
#define vector_reserve(vector, n)																										\
	do {																																							\
		size_t need = (n);																															\
		if ((vector) == NULL) {																													\
			size_t cap = need < VECTOR_INITIAL_CAPACITY ? VECTOR_INITIAL_CAPACITY : need;	\
			Header *header = malloc(vector__allocateSize(sizeof(*(vector)), cap));				\
			if (header == NULL) VECTOR__OOM();																						\
			header->count = 0;																														\
			header->capacity = cap;																												\
			(vector) = (void*)(header + 1);																								\
		} else {																																				\
			Header *header = (Header*)(vector) - 1;																				\
			if (header->capacity < need) {																								\
				Header *newHeader =																												\
					realloc(header, vector__allocateSize(sizeof(*(vector)), need));						\
				if (newHeader == NULL) VECTOR__OOM();																			\
				header = newHeader;																												\
				header->capacity = need;																										\
				(vector) = (void*)(header + 1);																							\
			}																																							\
		}																																								\
	} while (0)

// Safe on NULL, and NULLs the pointer afterwards so accidental
// use-after-free / double-free becomes a clean NULL access instead.
#define vector_free(vector)					\
	do {															\
		if ((vector) != NULL) {					\
			free((Header*)(vector) - 1);	\
			(vector) = NULL;							\
		}																\
	} while (0)
