// SPDX-FileCopyrightText: © 2026 Phil Armstead <philarmstead@mailbox.org>
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include "types/entities.h"
#include "types/position.h"


int positionGroupToIndices(PositionGrouped p, int outIndices[5]);
PositionGrouped positionCodeToGroup(PositionCode position);
float getRatingForPosition(const Player *player, PositionCode position);
