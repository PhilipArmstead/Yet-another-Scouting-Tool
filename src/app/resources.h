#pragma once
#include <stddef.h>

typedef struct {
	const char *name;
	const unsigned char *data;
	size_t size;
} Resource;

const Resource *resources_get(const char *name);
