#pragma once
#include "app/types.h"

typedef enum {
	COLUMN_NATIONALITIES,
	COLUMN_NAME,
	COLUMN_AGE,
	COLUMN_POSITIONS,
	COLUMN_CA,
	COLUMN_PA,
	COLUMN_CA_PA_DELTA,
	COLUMN_STATUS,
	COLUMN_RATING,
	COLUMN_VALUE,
	COLUMN_CLUB,
	COLUMN_REPUTATION_HOME,
	COLUMN_REPUTATION_CURRENT,
	COLUMN_REPUTATION_WORLD,
	COLUMN_COUNT
} Column;

void playerTable_init(void);
void playerTable_clear(void);
void playerTable_populate(void);
void playerTable_sort(Column column, bool append);
bool playerTable_sortable(Column column);
int playerTable_direction(Column column);
const Player *playerTable_player(size_t row);
size_t playerTable_count(void);
void playerTable_shutdown(void);
void player_formatName(const Player *player, char *buffer, size_t size);
void player_formatPositions(const Player *player, char *buffer, size_t size);
