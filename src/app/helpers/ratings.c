// SPDX-FileCopyrightText: © 2026 Phil Armstead <philarmstead@mailbox.org>
// SPDX-License-Identifier: GPL-3.0-or-later

#include "ratings.h"

#include "types/position.h"


int positionGroupToIndices(const PositionGrouped p, int outIndices[5]) {
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

/** Ratings are computed per grouped role, so several position codes share one entry. */
PositionGrouped positionCodeToGroup(const PositionCode position) {
	static const PositionGrouped groups[POSITION_CODE_COUNT] = {
		[POSITION_CODE_GK] = POSITION_GROUPED_GK,
		[POSITION_CODE_SW] = POSITION_GROUPED_CB,
		[POSITION_CODE_DL] = POSITION_GROUPED_FB,
		[POSITION_CODE_DC] = POSITION_GROUPED_CB,
		[POSITION_CODE_DR] = POSITION_GROUPED_FB,
		[POSITION_CODE_DM] = POSITION_GROUPED_DM,
		[POSITION_CODE_ML] = POSITION_GROUPED_W,
		[POSITION_CODE_MC] = POSITION_GROUPED_MC,
		[POSITION_CODE_MR] = POSITION_GROUPED_W,
		[POSITION_CODE_AML] = POSITION_GROUPED_W,
		[POSITION_CODE_AMC] = POSITION_GROUPED_AM,
		[POSITION_CODE_AMR] = POSITION_GROUPED_W,
		[POSITION_CODE_ST] = POSITION_GROUPED_ST,
		[POSITION_CODE_WBL] = POSITION_GROUPED_WB,
		[POSITION_CODE_WBR] = POSITION_GROUPED_WB,
	};

	return groups[position];
}

/**
 * Returns the player's rating for the grouped role that owns `position`, or 0 when they have none.
 * `player->ratings` is sorted value-descending, so the first entry matching the role is the real
 * one and any zeroed trailing entries are never reached first.
 */
float getRatingForPosition(const Player *player, const PositionCode position) {
	const PositionGrouped group = positionCodeToGroup(position);
	for (PositionGrouped i = 0; i < POSITION_GROUPED_COUNT; ++i) {
		if (player->ratings[i].position == group) {
			return player->ratings[i].value;
		}
	}

	return 0.f;
}
