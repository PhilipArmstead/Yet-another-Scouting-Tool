// SPDX-FileCopyrightText: © 2026 Phil Armstead <philarmstead@mailbox.org>
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include "app/types.h"


Player getPlayer(void *handle, bool skipIsValidCheck, uint64_t personAddress, uint64_t playerAddress);
// Workers acquire raw data; finish once on the main thread before publishing it.
Player player_read(void *handle, bool skipIsValidCheck, uint64_t personAddress, uint64_t playerAddress, DateTime date);
void player_finishRead(Player *player);
uint8_t player_ageAtDate(Date dateOfBirth, DateTime date);
uint64_t getPersonAddressFromPlayerAddress(void *handle, uint64_t playerAddress);
uint64_t getCurrentPersonUniqueId(const ProcessContext *processContext);
Player getPlayerById(const ProcessContext *processContext, uint64_t uniqueId);
PositionWeights *getWeightsForPosition(PositionGrouped position);
void getSortedPositionRatings(Player *player);
uint8_t player_getAge(Date dateOfBirth);
