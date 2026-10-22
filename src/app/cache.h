#pragma once
#include <stdbool.h>

void cache_clear(void);
void cache_reset(void);
void cache_run(void);
bool cache_poll(void);
bool cache_isRunning(void);
const char *cache_error(void);
void cache_shutdown(void);
