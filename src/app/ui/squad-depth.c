// SPDX-License-Identifier: GPL-3.0-or-later
#include "internal.h"
#include <math.h>

void ui_squadDepth(View *view, Rectangle bounds) {
	Rectangle previous = ui_clip(bounds);
	float x = bounds.x + 12, y = bounds.y + 8, width = bounds.width - 30;
	char label[96];
	snprintf(label, sizeof(label), "SQUAD DEPTH  /  %u players in snapshot", view->snapshot->count);
	ui_text((Rectangle){x, y, width, 28}, label, 14, ui.colour.title, true);
	y += 34;
	float buttonX = x;
	for (unsigned i = 0; i < 14; ++i) {
		if (buttonX + 48 > x + width) {
			buttonX = x;
			y += 30;
		}
		if (ui_button(ui_id(view, 20 + i), (Rectangle){buttonX, y, 44, 25}, ui_positionNames[i], (view->positions & (1u << i)) != 0, NULL))
			view->positions ^= (uint16_t)(1u << i);
		buttonX += 48;
	}
	y += 36;
	if (view->dirty) {
		for (unsigned i = 0; i < 14; ++i)
			view->depthCount[i] = analysis_depth(view->snapshot->players, view->snapshot->count, ui_positionCodes[i], view->depth[i]);
		view->dirty = false;
	}
	int columns = (int)(width / 360);
	if (columns < 1)
		columns = 1;
	float columnWidth = width / (float)columns;
	unsigned selected = 0;
	for (unsigned i = 0; i < 14; ++i)
		if (view->positions & (1u << i))
			++selected;
	Rectangle content = {bounds.x, y, bounds.width, bounds.height - (y - bounds.y)};
	float cardHeight = 70 + DEPTH_LIMIT * 32;
	ui_scroll(ui_id(view, 40), content, ceilf((float)selected / (float)columns) * (cardHeight + 14), &view->scroll, false);
	ui_clip((Rectangle){content.x, content.y, content.width - 12, content.height});
	unsigned card = 0;
	for (unsigned i = 0; i < 14; ++i) {
		if (!(view->positions & (1u << i)))
			continue;
		Rectangle box = {x + (float)(card % (unsigned)columns) * columnWidth, y + (float)(card / (unsigned)columns) * (cardHeight + 14) - view->scroll, columnWidth - 12, cardHeight};
		++card;
		if (!CheckCollisionRecs(box, ui.clip))
			continue;
		DrawRectangleRounded(box, .025f, 4, ui.colour.card);
		ui_text((Rectangle){box.x + 12, box.y + 8, box.width - 24, 28}, ui_positionNames[i], 14, ui.colour.title, true);
		ui_text((Rectangle){box.x + 8, box.y + 38, box.width - 111, 26}, "NAME", 10, ui.colour.muted, true);
		ui_text((Rectangle){box.x + box.width - 93, box.y + 38, 30, 26}, "AGE", 10, ui.colour.muted, true);
		ui_text((Rectangle){box.x + box.width - 58, box.y + 38, 48, 26}, "RTG.", 10, ui.colour.muted, true);
		for (unsigned row = 0; row < view->depthCount[i]; ++row) {
			const BestElevenRow *entry = &view->depth[i][row];
			Rectangle line = {box.x + 8, box.y + 70 + (float)row * 32, box.width - 16, 30};
			if (!CheckCollisionRecs(line, ui.clip))
				continue;
			char name[80], value[24];
			player_formatName(entry->player, name, sizeof(name));
			if (ui_button(ui_id(view, 100 + i * DEPTH_LIMIT + row), (Rectangle){line.x, line.y, line.width - 95, line.height}, name, false, "Open player details"))
				ui_openPlayer(entry->player);
			snprintf(value, sizeof(value), "%u", entry->player->age);
			ui_text((Rectangle){line.x + line.width - 85, line.y, 30, line.height}, value, 12, ui.colour.muted, false);
			snprintf(value, sizeof(value), "%.1f", (double)entry->rating);
			ui_text((Rectangle){line.x + line.width - 50, line.y, 48, line.height}, value, 12, ui_quality(entry->rating), true);
		}
		if (!view->depthCount[i])
			ui_text((Rectangle){box.x + 12, box.y + 70, box.width - 24, 28}, "No eligible players", 12, ui.colour.muted, false);
	}
	if (!selected)
		ui_text((Rectangle){x, y, width, 32}, "Select a position to show its depth.", 12, ui.colour.muted, false);
	ui_unclip(previous);
}
