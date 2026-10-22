// SPDX-License-Identifier: GPL-3.0-or-later
#include "app/helpers/formatter.h"
#include "app/helpers/icons.h"
#include "app/injury-names.h"
#include "internal.h"
#include "types/position-code-names.h"
#include <math.h>

void ui_analysisRow(View *view, Rectangle bounds, const BestElevenRow *row, const char *position, unsigned index, bool positional) {
	if (!CheckCollisionRecs(bounds, ui.clip))
		return;
	DrawRectangleRec(bounds, index & 1 ? ui.colour.alternate : ui.colour.card);
	ui_text((Rectangle){bounds.x + 8, bounds.y, 38, bounds.height}, position, 12, ui.colour.muted, true);
	const Player *player = row->player;
	if (!player) {
		ui_text((Rectangle){bounds.x + 90, bounds.y, bounds.width - 100, bounds.height}, "No eligible player", 12, ui.colour.muted, false);
		return;
	}
	ui_flag(player->nationality[0], (Rectangle){bounds.x + 48, bounds.y + 11, 22, 14});
	char name[80], value[80];
	player_formatName(player, name, sizeof(name));
	if (ui_button(ui_id(view, 300 + index), (Rectangle){bounds.x + 80, bounds.y + 3, bounds.width - 246, bounds.height - 6}, name, false, "Open player details"))
		ui_openPlayer(player);
	snprintf(value, sizeof(value), "%u yrs", player->age);
	ui_text((Rectangle){bounds.x + bounds.width - 155, bounds.y, 47, bounds.height}, value, 12, ui.colour.muted, false);
	Rectangle heart = {bounds.x + bounds.width - 100, bounds.y + 9, 18, 18};
	icons_draw(YAST_ICON_HEART, heart, player->injury.duration ? 0 : icons_conditionQuantise(player->condition));
	if (player->injury.duration)
		snprintf(value, sizeof(value), "Injured: %s", injuryNames_get(player->injury.nameIndex));
	else
		snprintf(value, sizeof(value), "Condition: %.1f%%", (double)player->condition / 100);
	ui_tip(ui_id(view, 600 + index), heart, value);
	const float rating = positional ? row->rating : player->ratings[0].value;
	formatter_formatRating(rating, value);
	ui_text((Rectangle){bounds.x + bounds.width - 70, bounds.y, 62, bounds.height}, value, 12, ui_quality(rating), true);
}

void ui_bestEleven(View *view, Rectangle bounds) {
	Rectangle previous = ui_clip(bounds);
	const float width = bounds.width - 24, x = bounds.x + 12;
	float y = bounds.y + 10;
	char label[96];
	snprintf(label, sizeof(label), "BEST XI  /  %u players in snapshot", view->snapshot->count);
	ui_text((Rectangle){x, y, width, 28}, label, 14, ui.colour.title, true);
	y += 36;
	const size_t formations = vector_length(gameContext.options.formations);
	if (!formations) {
		ui_text((Rectangle){x, y, width, 50}, "No valid formations in options.yaml.", 12, ui.colour.orange, false);
		ui_unclip(previous);
		return;
	}
	if (view->formation < 0 || (size_t)view->formation >= formations) {
		view->formation = 0;
		view->dirty = true;
	}
	const Formation *formation = &gameContext.options.formations[view->formation];
	if (strcmp(view->formationName, formation->name)) {
		for (size_t i = 0; i < formations; ++i)
			if (!strcmp(view->formationName, gameContext.options.formations[i].name)) {
				view->formation = (int)i;
				break;
			}
		formation = &gameContext.options.formations[view->formation];
		snprintf(view->formationName, sizeof(view->formationName), "%s", formation->name);
	}
	if (ui_button(ui_id(view, 10), (Rectangle){x, y, 26, 28}, "<", false, "Previous formation")) {
		view->formation = (view->formation + (int)formations - 1) % (int)formations;
		view->formationName[0] = '\0';
		view->dirty = true;
	}
	ui_text((Rectangle){x + 34, y, width - 68, 28}, gameContext.options.formations[view->formation].name, 14, ui.colour.text, true);
	if (ui_button(ui_id(view, 11), (Rectangle){x + width - 26, y, 26, 28}, ">", false, "Next formation")) {
		view->formation = (view->formation + 1) % (int)formations;
		view->formationName[0] = '\0';
		view->dirty = true;
	}
	y += 38;
	float half = fminf(240, (width - 10) * .5f);
	bool changed = ui_number(ui_id(view, 20), (Rectangle){x, y, half, 26}, "Age", &view->minAge, 14, 50);
	changed |= ui_number(ui_id(view, 24), (Rectangle){x + half + 8, y, half, 26}, "to", &view->maxAge, 14, 50);
	y += 34;
	changed |= ui_number(ui_id(view, 28), (Rectangle){x, y, half, 26}, "Cond.", &view->minCondition, 0, 100);
	changed |= ui_number(ui_id(view, 32), (Rectangle){x + half + 8, y, half, 26}, "to", &view->maxCondition, 0, 100);
	y += 32;
	changed |= ui_check(ui_id(view, 36), (Rectangle){x, y, 180, 26}, "Exclude injured", &view->excludeInjured);
	y += 38;
	if (changed)
		view->dirty = true;
	const bool inRange = view->minAge >= 14 && view->maxAge <= 50 && view->minAge <= 50 &&
											 view->maxAge >= 14 && view->minCondition >= 0 && view->maxCondition <= 100 &&
											 view->minCondition <= 100 && view->maxCondition >= 0;
	bool valid = inRange && view->minAge <= view->maxAge && view->minCondition <= view->maxCondition;
	if (!valid) {
		ui_text((Rectangle){x, y, width, 30}, inRange ? "Minimum must not exceed maximum." : "Use ages 14-50 and condition 0-100%.", 12, ui.colour.orange, false);
		ui_unclip(previous);
		return;
	}
	if (view->dirty) {
		const BestElevenFilters filters = {.minAge = (uint8_t)view->minAge, .maxAge = (uint8_t)view->maxAge, .minCondition = (uint16_t)(view->minCondition * 100), .maxCondition = (uint16_t)(view->maxCondition * 100), .excludeInjured = view->excludeInjured};
		if (!analysis_bestEleven(view->snapshot->players, view->snapshot->count, gameContext.options.formations[view->formation].positions, filters, view->eleven))
			ui_setCurrentStatus("Cannot calculate Best XI. See the log.");
		view->dirty = false;
	}
	Rectangle table = {bounds.x, y, bounds.width, bounds.height - (y - bounds.y)};
	ui_scroll(ui_id(view, 40), table, 12 * 38 + 30, &view->scroll, false);
	ui_scroll(ui_id(view, 41), table, 540, &view->horizontal, true);
	ui_clip((Rectangle){table.x, table.y, table.width - 10, table.height - 10});
	float rating = 0;
	unsigned count = 0;
	for (unsigned i = 0; i < 11; ++i) {
		ui_analysisRow(view, (Rectangle){x - view->horizontal, y + (float)i * 38 - view->scroll, fmaxf(516, width - 10), 36}, &view->eleven[i], positionCodeNames[gameContext.options.formations[view->formation].positions[i]], i, false);
		if (view->eleven[i].player) {
			rating += view->eleven[i].player->ratings[0].value;
			++count;
		}
	}
	snprintf(label, sizeof(label), "Average: %.1f  /  %u of 11 positions filled", count ? (double)rating / count : 0, count);
	ui_text((Rectangle){x, y + 11 * 38 - view->scroll, width, 30}, label, 12, ui.colour.orange, true);
	ui_unclip(previous);
}
