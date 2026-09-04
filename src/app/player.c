// SPDX-FileCopyrightText: © 2026 Phil Armstead <philarmstead@mailbox.org>
// SPDX-License-Identifier: GPL-3.0-or-later

#include "player.h"
#include "app/constants.h"
#include "app/game-status.h"
#include "app/maths.h"
#include "app/mocks.h"
#include "core/logger.h"
#include "platform/platform.h"


extern GameContext gameContext;

static bool isPlayerAlsoStaff(void *handle, uint32_t playerAddress);
static bool isPersonAlsoStaff(void *handle, uint32_t personAddress);
static bool isPlayerValid(void *handle, uint32_t personAddress);
static bool isPersonValid(void *handle, uint32_t personAddress);
static uint32_t getPlayerAddressFromPersonAddress(void *handle, uint32_t personAddress);
static inline void getPersonName(void *handle, uint8_t pointer[4], char str[PERSON_COMMON_NAME_LENGTH]);
static inline void getPersonForename(void *handle, uint32_t attributeBase, char str[PERSON_COMMON_NAME_LENGTH]);
static inline void getPersonSurname(void *handle, uint32_t attributeBase, char str[PERSON_COMMON_NAME_LENGTH]);
static inline void getPersonCommonName(void *handle, uint32_t attributeBase, char str[PERSON_COMMON_NAME_LENGTH]);
static uint8_t getAge(void *handle, uint32_t address);
static int32_t getClubIndexFromPerson(void *handle, uint32_t personAddress);
static uint32_t getPersonAddressFromUid(const ProcessContext *processContext, uint32_t uid);
static void getSortedPositionRatings(Player *player);
static float getRatingPerPosition(const Player *player, PositionGrouped position);

// Assumes valid ProcessContext
uint32_t getCurrentPersonUniqueId(const ProcessContext *processContext) {
	#ifndef PLAYER_BY_ID
	uint8_t bytes[4];
	void *handle = processContext->handle;
	readFromMemory(handle, processContext->moduleBaseAddress + CURRENT_SCREEN_PLAYER_ID_PTR_BASE, 4, bytes);
	readFromMemory(handle, hexBytesToInt(bytes, 4) + CURRENT_SCREEN_PLAYER_ID_PTR_BASE_OFFSET_1, 4, bytes);
	readFromMemory(handle, hexBytesToInt(bytes, 4) + CURRENT_SCREEN_PLAYER_ID_PTR_BASE_OFFSET_2, 4, bytes);
	readFromMemory(handle, hexBytesToInt(bytes, 4) + CURRENT_SCREEN_PLAYER_ID_PTR_BASE_OFFSET_3, 4, bytes);
	readFromMemory(handle, hexBytesToInt(bytes, 4) + CURRENT_SCREEN_PLAYER_ID_PTR_BASE_OFFSET_4, 4, bytes);
	readFromMemory(handle, hexBytesToInt(bytes, 4) + CURRENT_SCREEN_PLAYER_ID_PTR_BASE_OFFSET_5, 4, bytes);
	readFromMemory(handle, hexBytesToInt(bytes, 4) + CURRENT_SCREEN_PLAYER_ID_PTR_BASE_OFFSET_6, 4, bytes);
	readFromMemory(handle, hexBytesToInt(bytes, 4) + CURRENT_SCREEN_PLAYER_ID_PTR_BASE_OFFSET_7, 4, bytes);
	readFromMemory(handle, hexBytesToInt(bytes, 4) + CURRENT_SCREEN_PLAYER_ID_PTR_BASE_OFFSET_8, 4, bytes);
	return (uint32_t)hexBytesToInt(bytes, 4);
	#else
	const Player player = PLAYER_BY_ID;
	return player.uid;
	#endif
}

// Assumes valid ProcessContext
Player getPlayerById(const ProcessContext *processContext, const uint32_t uniqueId) {
	const uint32_t personAddress = getPersonAddressFromUid(processContext, uniqueId);
	if (personAddress && isPlayerValid(processContext->handle, personAddress)) {
		return getPlayer(processContext->handle, false, personAddress, 0);
	}

	LOG_ERROR("Could not find player by ID %d", uniqueId);

	return (Player){0};
}

static uint32_t getPlayerAddressFromPersonAddress(void *handle, const uint32_t personAddress) {
	const int32_t offset = isPersonAlsoStaff(handle, personAddress)
		? STAFF_OFFSET_FROM_PERSON
		: PLAYER_OFFSET_FROM_PERSON;

	return personAddress + (uint32_t)offset;
}

uint32_t getPersonAddressFromPlayerAddress(void *handle, const uint32_t playerAddress) {
	const int32_t offset = isPlayerAlsoStaff(handle, playerAddress)
		? STAFF_OFFSET_FROM_PERSON
		: PLAYER_OFFSET_FROM_PERSON;

	return playerAddress - (uint32_t)offset;
}

Player getPlayer(void *handle, const bool skipIsValidCheck, const uint32_t personAddress, uint32_t playerAddress) {
	#ifndef PLAYER_BY_ID
	if (!skipIsValidCheck && !isPlayerValid(handle, personAddress)) {
		return (Player){0};
	}

	if (playerAddress == 0) {
		playerAddress = getPlayerAddressFromPersonAddress(handle, personAddress);
	}

	uint8_t bytes[6];
	readFromMemory(handle, personAddress + PERSON_OFFSET_UNIQUE_ID, 4, bytes);
	const uint32_t uid = (uint32_t)hexBytesToInt(bytes, 4);
	if (!uid) {
		return (Player){0};
	}

	uint8_t ability[3];
	readFromMemory(handle, playerAddress + PLAYER_OFFSET_ABILITY, 3, ability);

	readFromMemory(handle, personAddress + PERSON_OFFSET_ROW_ID, 4, bytes);
	const uint32_t rowId = (uint32_t)hexBytesToInt(bytes, 4);
	readFromMemory(handle, personAddress + PERSON_OFFSET_RANDOM_ID, 4, bytes);
	const uint32_t rid = (uint32_t)hexBytesToInt(bytes, 4);

	Player player = {
		.personAddress = personAddress,
		.playerAddress = playerAddress,
		.age = getAge(handle, personAddress),
		.ca = ability[ABILITY_CA],
		.pa = ability[ABILITY_PA],
		.rid = rid,
		.uid = uid,
		.rowId = rowId,
		.nationality = {0xFF, 0xFF, 0xFF, 0xFF}
	};
	getPersonForename(handle, personAddress, player.forename);
	getPersonSurname(handle, personAddress, player.surname);
	getPersonCommonName(handle, personAddress, player.commonName);

	readFromMemory(
		handle,
		personAddress + PERSON_OFFSET_PERSONALITY,
		PERSONALITY_COUNT,
		player.attributes + TRUE_ATTRIBUTE_COUNT
	);
	readFromMemory(handle, playerAddress + PLAYER_OFFSET_ATTRIBUTES, TRUE_ATTRIBUTE_COUNT, player.attributes);
	readFromMemory(handle, playerAddress + PLAYER_OFFSET_POSITIONS, 15, player.positions);
	player.clubIndex = getClubIndexFromPerson(handle, personAddress);
	getSortedPositionRatings(&player);

	// Normalise personality
	for (uint8_t i = TRUE_ATTRIBUTE_COUNT; i < ATTRIBUTE_COUNT; ++i) {
		player.attributes[i] *= 5;
	}

	// Sharpness, fatigue, condition
	readFromMemory(handle, playerAddress + PLAYER_OFFSET_SHARPNESS, 6, bytes);
	player.sharpness = (uint16_t)hexBytesToInt(bytes, 2);
	player.fatigue = (int16_t)hexBytesToInt(bytes + 2, 2);
	player.condition = (uint16_t)hexBytesToInt(bytes + 4, 2);

	// Nationality
	readFromMemory(handle, personAddress + PERSON_OFFSET_NATIONALITY, 4, bytes);
	const uint32_t country = (uint32_t)hexBytesToInt(bytes, 4);
	readFromMemory(handle, country + NATION_OFFSET_ROW_ID, 4, bytes);
	player.nationality[0] = (uint8_t)hexBytesToInt(bytes, 4);

	uint8_t nationalityIndex = 1;
	readFromMemory(handle, personAddress + PERSON_OFFSET_RELATIONSHIPS, 4, bytes);
	const uint32_t relationships = (uint32_t)hexBytesToInt(bytes, 4);
	readFromMemory(handle, relationships, 4, bytes);
	uint32_t relationshipStart = (uint32_t)hexBytesToInt(bytes, 4);
	readFromMemory(handle, relationships + 0x08, 4, bytes);
	const uint32_t relationshipEnd = (uint32_t)hexBytesToInt(bytes, 4);
	while (relationshipStart < relationshipEnd && nationalityIndex < 4) {
		readFromMemory(handle, relationshipStart + RELATIONSHIP_OFFSET_TYPE, 2, bytes);
		const uint16_t type = (uint16_t)hexBytesToInt(bytes, 2);
		if (type == 0x0908) {
			readFromMemory(handle, relationshipStart + RELATIONSHIP_OFFSET_TARGET_ADDRESS, 4, bytes);
			const uint32_t nationality = (uint32_t)hexBytesToInt(bytes, 4);
			readFromMemory(handle, nationality + NATION_OFFSET_ROW_ID, 4, bytes);
			player.nationality[nationalityIndex] = (uint8_t)hexBytesToInt(bytes, 4);

			nationalityIndex++;
		}
		relationshipStart += 0x10;
	}

	// Reputation
	readFromMemory(handle, playerAddress + PLAYER_OFFSET_HOME_REPUTATION, 6, bytes);
	player.homeReputation = (uint16_t)hexBytesToInt(bytes, 2);
	player.currentReputation = (uint16_t)hexBytesToInt(bytes + 2, 2);
	player.worldReputation = (uint16_t)hexBytesToInt(bytes + 4, 2);

	// Value
	readFromMemory(handle, playerAddress + PLAYER_OFFSET_GUIDE_VALUE, 4, bytes);
	player.guideValue = (uint32_t)hexBytesToInt(bytes, 4);

	// Extra
	player.canDevelopQuickly = player.age <= 23 &&
		player.attributes[ATTR_INJ] < 70 &&
		player.attributes[ATTR_AMB] > 10 &&
		player.attributes[ATTR_PRO] > 10 &&
		player.attributes[ATTR_DET] > 50;
	if (player.age >= 15 && player.age <= 19) {
		player.isHotProspect = player.ca >= 80 + 5 * (player.age - 15);
	} else if (player.age <= 23) {
		player.isHotProspect = player.ca >= 140;
	} else {
		player.isHotProspect = false;
	}
	#else
	const Player player = PLAYER_BY_ID;
	#endif

	return player;
}

static bool isPlayerAlsoStaff(void *handle, const uint32_t playerAddress) {
	uint8_t bytes[8];
	readFromMemory(handle, playerAddress - (uint32_t)PLAYER_OFFSET_FROM_PERSON + 0x0C, 8, bytes);
	const uint64_t ids = hexBytesToInt(bytes, 8);
	return ids >> 32 != (ids & 0xFFFFFFFF);
}

static bool isPersonAlsoStaff(void *handle, const uint32_t personAddress) {
	uint8_t bytes[4];
	readFromMemory(handle, personAddress + (uint32_t)PLAYER_OFFSET_FROM_PERSON + 0x08, 4, bytes);
	return hexBytesToInt(bytes, 4) == 0;
}

static bool isPlayerValid(void *handle, const uint32_t personAddress) {
	#ifndef MOCKS_MODE
	const uint32_t playerAddress = personAddress + (uint32_t)PLAYER_OFFSET_FROM_PERSON;
	for (uint8_t i = 0; i < 5; ++i) {
		const uint8_t attribute = readByte(handle, playerAddress + PLAYER_OFFSET_HIDDEN_ATTRIBUTES + i);
		if (!attribute || attribute > 100) {
			return false;
		}
	}

	return isPersonValid(handle, personAddress);
	#else
	return true;
	#endif
}

static bool isPersonValid(void *handle, const uint32_t personAddress) {
	#ifndef MOCKS_MODE
	for (uint8_t i = 0; i < 8; ++i) {
		const uint8_t attribute = readByte(handle, personAddress + PERSON_OFFSET_PERSONALITY + i);
		if (!attribute || attribute > 20) {
			return false;
		}
	}
	#endif

	return true;
}

static inline void getPersonName(void *handle, uint8_t pointer[4], char str[PERSON_COMMON_NAME_LENGTH]) {
	uint32_t a = (uint32_t)hexBytesToInt(pointer, 4);
	if (!a) {
		str[0] = '\0';
		return;
	}
	readFromMemory(handle, a, 4, pointer);
	a = (uint32_t)hexBytesToInt(pointer, 4);
	readFromMemory(handle, a + 4, PERSON_COMMON_NAME_LENGTH, (uint8_t*)str);
}

static inline void getPersonForename(void *handle, const uint32_t attributeBase, char str[PERSON_COMMON_NAME_LENGTH]) {
	uint8_t pointer[4];
	readFromMemory(handle, attributeBase + PERSON_OFFSET_FORENAME, 4, pointer);
	getPersonName(handle, pointer, str);
}

static inline void getPersonSurname(void *handle, const uint32_t attributeBase, char str[PERSON_COMMON_NAME_LENGTH]) {
	uint8_t pointer[4];
	readFromMemory(handle, attributeBase + PERSON_OFFSET_SURNAME, 4, pointer);
	getPersonName(handle, pointer, str);
}

static inline void getPersonCommonName(
	void *handle,
	const uint32_t attributeBase,
	char str[PERSON_COMMON_NAME_LENGTH]
) {
	uint8_t pointer[4];
	readFromMemory(handle, attributeBase + PERSON_OFFSET_COMMON_NAME, 4, pointer);
	getPersonName(handle, pointer, str);
}

static uint8_t getAge(void *handle, const uint32_t address) {
	uint8_t bytes[4];
	readFromMemory(handle, address + PERSON_OFFSET_DOB, 4, bytes);
	const uint8_t yearBytes[2] = {bytes[2], bytes[3]};
	const uint16_t yearOfBirth = (uint16_t)hexBytesToInt(yearBytes, 2);
	const uint16_t dayOfBirth = (uint16_t)hexBytesToInt(bytes, 2);

	uint8_t age = (uint8_t)(gameContext.currentDate.year - yearOfBirth);
	if (gameContext.currentDate.day < dayOfBirth) {
		--age;
	}

	return age;
}

static int32_t getClubIndexFromPerson(void *handle, const uint32_t personAddress) {
	uint8_t pointer[4];
	readFromMemory(handle, personAddress + PERSON_OFFSET_CONTRACTS, 4, pointer);
	const uint32_t contractAddress = (uint32_t)hexBytesToInt(pointer, 4);
	if (!contractAddress) {
		return -1;
	}

	readFromMemory(handle, contractAddress + CONTRACTS_OFFSET_TEAM, 4, pointer);
	const uint32_t teamAddress = (uint32_t)hexBytesToInt(pointer, 4);
	readFromMemory(handle, teamAddress + TEAM_OFFSET_CLUB, 4, pointer);
	const uint32_t clubAddress = (uint32_t)hexBytesToInt(pointer, 4);
	readFromMemory(handle, clubAddress + CLUB_OFFSET_ROW_ID, 4, pointer);
	return (int32_t)hexBytesToInt(pointer, 4);
}

/* Helper structure to represent sparse attribute weights */
typedef struct {
	int idx;
	float weight;
} AttrPair;

/* Personality weights are fixed length 8 */

static void fillWeightsForPosition(
	const PositionGrouped p,
	float outAttr[ATTRIBUTE_COUNT],
	float *outScale
) {
	PositionGrouped generalIndex = 0;
	PositionGrouped matchingIndex = 0xFF;
	for (PositionGrouped i = 0; i < POSITION_GROUPED_COUNT; ++i) {
		if (gameContext.options.weights[i].scale == 0) {
			break;
		}
		if (gameContext.options.weights[i].position == p) {
			matchingIndex = i;
			break;
		}
		if (gameContext.options.weights[i].position == POSITION_GROUPED_COUNT) {
			generalIndex = i;
		}
	}

	if (matchingIndex == 0xFF) {
		matchingIndex = generalIndex;
	}

	memcpy(outAttr, gameContext.options.weights[matchingIndex].weights, ATTRIBUTE_COUNT * sizeof outAttr[0]);
	*outScale = gameContext.options.weights[matchingIndex].scale;
}

static inline int positionGroupToIndices(const PositionGrouped p, int outIndices[5]) {
	/* returns number of indices filled in outIndices (max 5) */
	int n = 0;
	switch (p) {
		case POSITION_GROUPED_GK: outIndices[n++] = POSITION_CODE_GK;
			break;
		case POSITION_GROUPED_FB: outIndices[n++] = POSITION_CODE_DL;
			outIndices[n++] = POSITION_CODE_DR;
			break;
		case POSITION_GROUPED_CB: outIndices[n++] = POSITION_CODE_DC;
			break;
		case POSITION_GROUPED_WB: outIndices[n++] = POSITION_CODE_WBL;
			outIndices[n++] = POSITION_CODE_WBR;
			break;
		case POSITION_GROUPED_DM: outIndices[n++] = POSITION_CODE_DM;
			break;
		case POSITION_GROUPED_MC: outIndices[n++] = POSITION_CODE_MC;
			break;
		case POSITION_GROUPED_W: outIndices[n++] = POSITION_CODE_ML;
			outIndices[n++] = POSITION_CODE_MR;
			outIndices[n++] = POSITION_CODE_AML;
			outIndices[n++] = POSITION_CODE_AMR;
			break;
		case POSITION_GROUPED_AM: outIndices[n++] = POSITION_CODE_AMC;
			break;
		default: outIndices[n++] = POSITION_CODE_ST;
			break;
	}
	return n;
}

void getWeightsForPosition(const PositionGrouped position, float outAttributes[ATTRIBUTE_COUNT], float *outScale) {
	fillWeightsForPosition(position, outAttributes, outScale);
}

static float getRatingPerPosition(const Player *player, const PositionGrouped position) {
	/* Check positional proficiency */
	int indices[5];
	const int count = positionGroupToIndices(position, indices);
	bool canPlay = false;
	for (int i = 0; i < count; ++i) {
		const int idx = indices[i];
		if (player->positions[idx] >= MINIMUM_POSITIONAL_PROFICIENCY) {
			canPlay = true;
			break;
		}
	}
	if (!canPlay) {
		return 0.0f;
	}
	// LOG_INFO("%s can play	%d", player->forename, position);

	float attributeWeights[ATTRIBUTE_COUNT] = {0};
	float totalScale = 1.0f;
	fillWeightsForPosition(position, attributeWeights, &totalScale);
	float rating = 0.0f;
	for (int i = 0; i < ATTRIBUTE_COUNT; ++i) {
		if (attributeWeights[i] > 0.1f) {
			const float value = (float)player->attributes[i] / 100.f;
			rating += attributeWeights[i] * value;
			// LOG_INFO("Attr %d: %f * %f = %f (new rating: %f)", i, value, attrWeights[i], attrWeights[i] * value, rating);
		}
	}

	if (totalScale != 0.f) {
		rating /= totalScale;
	}
	// LOG_INFO("Final rating after dividing by %f: %f", totalWeight, rating);

	return rating;
}

static void getSortedPositionRatings(Player *player) {
	PositionGrouped i = 0;
	PositionGrouped j = 0;
	while (i < POSITION_GROUPED_COUNT) {
		const float value = getRatingPerPosition(player, i);
		if (value > 0.f) {
			player->ratings[j].value = value;
			player->ratings[j].position = i;
			++j;
		}
		++i;
	}

	/* simple selection sort descending */
	for (i = 0; i < POSITION_GROUPED_COUNT; ++i) {
		PositionGrouped best = i;
		for (j = i + 1; j < POSITION_GROUPED_COUNT; ++j) {
			if (player->ratings[j].value > player->ratings[best].value) {
				best = j;
			}
		}
		if (best != i) {
			const Rating tmp = player->ratings[i];
			player->ratings[i] = player->ratings[best];
			player->ratings[best] = tmp;
		}
	}
}

static uint32_t getPersonAddressFromUid(const ProcessContext *processContext, uint32_t uid) {
	#ifndef PLAYER_BY_ID
	// Iterate over all players to find one with the matching UID
	uint8_t bytes[4];
	readFromMemory(processContext->handle, processContext->moduleBaseAddress + PLAYER_COUNT_PTR_BASE, 4, bytes);
	const uint32_t playerCount = (uint32_t)hexBytesToInt(bytes, 4);
	if (playerCount < 1 || playerCount > 500000) {
		LOG_ERROR("Unexpected number of players in database: %d", playerCount);
		return 0;
	}

	readFromMemory(
		processContext->handle,
		processContext->moduleBaseAddress + PLAYER_LIST_PTR_BASE,
		4,
		bytes
	);
	const uint32_t allPlayers = (uint32_t)hexBytesToInt(bytes, 4);

	for (uint32_t i = 0; i < playerCount; ++i) {
		readFromMemory(processContext->handle, allPlayers + PLAYER_LIST_STRIDE * i, 4, bytes);
		const uint32_t playerAddress = (uint32_t)hexBytesToInt(bytes, 4);
		const uint32_t personAddress = playerAddress - (uint32_t)PLAYER_OFFSET_FROM_PERSON;

		readFromMemory(processContext->handle, personAddress + (uint32_t)PERSON_OFFSET_UNIQUE_ID, 4, bytes);
		const uint32_t foundPlayerId = (uint32_t)hexBytesToInt(bytes, 4);
		if (foundPlayerId == uid) {
			return personAddress;
		}
	}

	return 0;
	#else
	return 0x5E18008;
	#endif
}
