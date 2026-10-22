#pragma once
#include <stdbool.h>

bool searchHandler_cacheFilters(char error[128]);
bool searchHandler_doSearch(bool refreshFilterCache, char error[128]);
void searchHandler_clearFilters(void);
