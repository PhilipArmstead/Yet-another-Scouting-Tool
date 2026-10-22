#include "player-table.h"
#include "app/helpers/vector-shared-pointer.h"
#include "app/helpers/vector.h"
#include "types/position-code-names.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

extern GameContext gameContext;
static struct {
	Column column;
	int direction;
} sortKeys[6] = {{COLUMN_RATING, -1}};
static size_t sortCount = 1;

static int compare(const void *a, const void *b) {
	const Player *left = &gameContext.players[*(const uint32_t *)a];
	const Player *right = &gameContext.players[*(const uint32_t *)b];
	for (size_t i = 0; i < sortCount; ++i) {
		int result = 0;
		switch (sortKeys[i].column) {
			case COLUMN_NAME: result = strcmp(left->surname, right->surname); break;
			case COLUMN_AGE: result = (int)left->age - right->age; break;
			case COLUMN_CA: result = (int)left->ca - right->ca; break;
			case COLUMN_PA: result = (int)left->pa - right->pa; break;
			case COLUMN_CA_PA_DELTA: result = ((int)left->pa - left->ca) - ((int)right->pa - right->ca); break;
			case COLUMN_RATING: result = (left->ratings[0].value > right->ratings[0].value) - (left->ratings[0].value < right->ratings[0].value); break;
			default: break;
		}
		if (result)
			return (result > 0 ? 1 : -1) * sortKeys[i].direction;
	}
	return (left->uid > right->uid) - (left->uid < right->uid);
}

void playerTable_init(void) {
	sortKeys[0].column = COLUMN_RATING;
	sortKeys[0].direction = -1;
	sortCount = 1;
	if (!gameContext.searchResults)
		gameContext.searchResults = sharedPointer_new(NULL);
}

void playerTable_clear(void) {
	if (gameContext.searchResults)
		sharedPointer_unref(gameContext.searchResults);
	gameContext.searchResults = sharedPointer_new(NULL);
}

void playerTable_populate(void) {
	const size_t count = playerTable_count();
	if (count > 1)
		qsort(gameContext.searchResults->data, count, sizeof(uint32_t), compare);
}

bool playerTable_sortable(const Column column) {
	return column == COLUMN_NAME || column == COLUMN_AGE || column == COLUMN_CA || column == COLUMN_PA ||
				 column == COLUMN_CA_PA_DELTA || column == COLUMN_RATING;
}

int playerTable_direction(const Column column) {
	for (size_t i = 0; i < sortCount; ++i)
		if (sortKeys[i].column == column)
			return sortKeys[i].direction;
	return 0;
}

void playerTable_sort(const Column column, const bool append) {
	if (!playerTable_sortable(column))
		return;
	int direction = playerTable_direction(column);
	direction = direction ? -direction : 1;
	size_t index = 0;
	if (append) {
		while (index < sortCount && sortKeys[index].column != column)
			++index;
		if (index == sortCount && sortCount < 6)
			++sortCount;
		if (index >= 6)
			index = 5;
	} else
		sortCount = 1;
	sortKeys[index].column = column;
	sortKeys[index].direction = direction;
	playerTable_populate();
}

size_t playerTable_count(void) {
	return gameContext.searchResults ? vector_length(gameContext.searchResults->data) : 0;
}

const Player *playerTable_player(const size_t row) {
	if (row >= playerTable_count())
		return NULL;
	const uint32_t index = ((const uint32_t *)gameContext.searchResults->data)[row];
	return index < gameContext.playerCount ? &gameContext.players[index] : NULL;
}

void playerTable_shutdown(void) {
	if (gameContext.searchResults)
		sharedPointer_unref(gameContext.searchResults);
	gameContext.searchResults = NULL;
}

void player_formatName(const Player *player, char *buffer, const size_t size) {
	if (player->commonName[0])
		snprintf(buffer, size, "%s", player->commonName);
	else
		snprintf(buffer, size, "%s %s", player->forename, player->surname);
}

void player_formatPositions(const Player *player, char *buffer, const size_t size) {
	static const PositionCode order[] = {POSITION_CODE_GK, POSITION_CODE_DL, POSITION_CODE_DR, POSITION_CODE_DC,
																			 POSITION_CODE_WBL, POSITION_CODE_WBR, POSITION_CODE_DM, POSITION_CODE_ML, POSITION_CODE_MR, POSITION_CODE_MC,
																			 POSITION_CODE_AML, POSITION_CODE_AMR, POSITION_CODE_AMC, POSITION_CODE_ST};
	size_t used = 0;
	buffer[0] = '\0';
	for (size_t i = 0; i < sizeof(order) / sizeof(order[0]); ++i) {
		if (player->positions[order[i]] < MINIMUM_POSITIONAL_PROFICIENCY)
			continue;
		const bool pair = i && ((order[i] == POSITION_CODE_DR && player->positions[POSITION_CODE_DL] >= MINIMUM_POSITIONAL_PROFICIENCY) ||
														(order[i] == POSITION_CODE_WBR && player->positions[POSITION_CODE_WBL] >= MINIMUM_POSITIONAL_PROFICIENCY) ||
														(order[i] == POSITION_CODE_MR && player->positions[POSITION_CODE_ML] >= MINIMUM_POSITIONAL_PROFICIENCY) ||
														(order[i] == POSITION_CODE_AMR && player->positions[POSITION_CODE_AML] >= MINIMUM_POSITIONAL_PROFICIENCY));
		const int written = snprintf(buffer + used, size - used, "%s%s", pair ? "" : (used ? ", " : ""), pair ? "/R" : positionCodeNames[order[i]]);
		if (written < 0 || (size_t)written >= size - used)
			return;
		used += (size_t)written;
	}
}
