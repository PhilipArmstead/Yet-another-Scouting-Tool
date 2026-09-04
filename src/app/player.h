// SPDX-FileCopyrightText: © 2026 Phil Armstead <philarmstead@mailbox.org>
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include "app/types.h"


Player getPlayer(void *handle, bool skipIsValidCheck, uint64_t personAddress, uint64_t playerAddress);
uint64_t getPersonAddressFromPlayerAddress(void *handle, uint64_t playerAddress);
uint32_t getCurrentPersonUniqueId(const ProcessContext *processContext);
Player getPlayerById(const ProcessContext *processContext, uint32_t uniqueId);
PositionWeights *getWeightsForPosition(PositionGrouped position);
