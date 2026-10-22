// SPDX-License-Identifier: GPL-3.0-or-later
#include "app/helpers/formatter.h"
#include "app/helpers/icons.h"
#include "app/injury-names.h"
#include "app/search-handler.h"
#include "internal.h"
#include <math.h>

static void search(void) {
	char error[128];
	if (!searchHandler_doSearch(true, error))
		ui_setCurrentStatus(error);
	else {
		ui.tableScroll = 0;
		ui.selectedRow = -1;
		ui_setCurrentStatus("Search complete");
	}
}

static void clear(void) {
	searchHandler_clearFilters();
	autocomplete_query(&ui.clubs, "");
	autocomplete_query(&ui.nations, "");
	ui.popup = 0;
	ui.selectedRow = -1;
	ui.tableScroll = 0;
}

static void autocompleteField(unsigned id, Rectangle bounds, bool nations) {
	FilterBuffer *buffer = &gameContext.filterBuffer;
	char *text = nations ? buffer->nationality : buffer->club;
	if (ui_edit(id, bounds, text, 128, nations ? "Filter by nation" : "Filter by club", false)) {
		const uint32_t mask = nations ? FILTER_HAS_NATIONALITY : FILTER_HAS_CLUB;
		const bool selected = (gameContext.filterOptions.filterMask & mask) != 0;
		gameContext.filterOptions.filterMask &= ~mask;
		autocomplete_query(nations ? &ui.nations : &ui.clubs, text);
		ui.popup = text[0] ? (nations ? 2 : 1) : 0;
		ui.popupRow = ui.popupFirst = 0;
		if (selected || !text[0])
			search();
	}
	if (ui.popup == (nations ? 2 : 1)) {
		ui.popupBounds = (Rectangle){bounds.x, bounds.y + bounds.height + 2, fmaxf(260, bounds.width), 0};
	}
}

static void sidebar(Rectangle area) {
	DrawRectangleRec(area, ui.colour.sidebar);
	Rectangle previous = ui_clip(area);
	ui_scroll(10, area, 570, &ui.sidebarScroll, false);
	const float x = area.x + 10, width = area.width - 24;
	float y = area.y + 8 - ui.sidebarScroll;
	ui_text((Rectangle){x, y, width, 22}, "FILTERS", 11, ui.colour.title, true);
	y += 28;
	autocompleteField(20, (Rectangle){x, y, width, 28}, false);
	y += 34;
	autocompleteField(21, (Rectangle){x, y, width, 28}, true);
	y += 40;
	char *fields[] = {gameContext.filterBuffer.minAge, gameContext.filterBuffer.maxAge,
										gameContext.filterBuffer.minCA, gameContext.filterBuffer.maxCA, gameContext.filterBuffer.minPA,
										gameContext.filterBuffer.maxPA, gameContext.filterBuffer.minRating, gameContext.filterBuffer.maxRating};
	const char *labels[] = {"Age", "CA", "PA", "Rating"};
	float entry = (width - 52) * .5f;
	for (unsigned i = 0; i < 4; ++i) {
		ui_text((Rectangle){x, y, 42, 28}, labels[i], 12, ui.colour.muted, false);
		ui_edit(22 + i * 2, (Rectangle){x + 44, y, entry, 28}, fields[i * 2], 16, "min", false);
		ui_edit(23 + i * 2, (Rectangle){x + 48 + entry, y, entry, 28}, fields[i * 2 + 1], 16, "max", false);
		y += 34;
	}
	y += 10;
	ui_text((Rectangle){x, y, width, 20}, "POSITIONS", 11, ui.colour.title, true);
	y += 24;
	const Rectangle pitch = {x, y, width, 240};
	DrawTexturePro(ui.pitch, (Rectangle){0, 0, (float)ui.pitch.width, (float)ui.pitch.height}, pitch, (Vector2){0}, 0, WHITE);
	static const Vector2 coordinates[14] = {{.5f, .87f}, {.16f, .72f}, {.5f, .72f}, {.84f, .72f}, {.16f, .56f}, {.5f, .56f}, {.84f, .56f}, {.16f, .4f}, {.5f, .4f}, {.84f, .4f}, {.16f, .24f}, {.5f, .24f}, {.84f, .24f}, {.5f, .08f}};
	for (unsigned i = 0; i < 14; ++i) {
		Rectangle button = {x + coordinates[i].x * width - 22, y + coordinates[i].y * 240 - 11, 44, 22};
		bool on = (gameContext.filterBuffer.positions & (1u << i)) != 0;
		if (ui_button(40 + i, button, ui_positionNames[i], on, NULL)) {
			gameContext.filterBuffer.positions ^= (uint16_t)(1u << i);
			search();
		}
	}
	y += 254;
	if (ui_button(30, (Rectangle){x, y, width - 60, 28}, "Search", true, "Search (Enter)"))
		search();
	if (ui_button(31, (Rectangle){x + width - 54, y, 54, 28}, "Clear", false, "Clear all filters"))
		clear();
	ui_unclip(previous);
}

static bool chip(unsigned id, Rectangle area, float *x, float *y, const char *label) {
	const float width = fminf(area.width - 12, ui_measureText(label, 11, false) + 26);
	if (*x + width > area.x + area.width - 6) {
		*x = area.x + 6;
		*y += 26;
	}
	bool result = ui_button(id, (Rectangle){*x, *y, width, 22}, label, false, "Remove this filter");
	*x += width + 4;
	return result;
}

static float chips(Rectangle area) {
	DrawRectangleRec((Rectangle){area.x, area.y, area.width, 34}, ui.colour.sidebar);
	float x = area.x + 6, y = area.y + 6;
	char label[160];
	char *fields[] = {gameContext.filterBuffer.minAge, gameContext.filterBuffer.maxAge,
										gameContext.filterBuffer.minCA, gameContext.filterBuffer.maxCA, gameContext.filterBuffer.minPA,
										gameContext.filterBuffer.maxPA, gameContext.filterBuffer.minRating, gameContext.filterBuffer.maxRating};
	const char *names[] = {"Age >=", "Age <=", "CA >=", "CA <=", "PA >=", "PA <=", "Rating >=", "Rating <="};
	FilterOptions *options = &gameContext.filterOptions;
	const uint32_t masks[] = {FILTER_HAS_MIN_AGE, FILTER_HAS_MAX_AGE, FILTER_HAS_MIN_CA, FILTER_HAS_MAX_CA,
														FILTER_HAS_MIN_PA, FILTER_HAS_MAX_PA, FILTER_HAS_MIN_RATING, FILTER_HAS_MAX_RATING};
	const double values[] = {options->minAge, options->maxAge, options->minCA, options->maxCA,
													 options->minPA, options->maxPA, options->minRating, options->maxRating};
	bool changed = false;
	for (unsigned i = 0; i < 8; ++i) {
		if (!(options->filterMask & masks[i]))
			continue;
		snprintf(label, sizeof(label), "%s %g  x", names[i], values[i]);
		if (chip(100 + i, area, &x, &y, label)) {
			fields[i][0] = '\0';
			options->filterMask &= ~masks[i];
			changed = true;
		}
	}
	if (gameContext.filterOptions.filterMask & FILTER_HAS_CLUB) {
		snprintf(label, sizeof(label), "%s  x", gameContext.filterBuffer.club);
		if (chip(108, area, &x, &y, label)) {
			gameContext.filterBuffer.club[0] = '\0';
			gameContext.filterOptions.filterMask &= ~(uint32_t)FILTER_HAS_CLUB;
			changed = true;
		}
	}
	if (gameContext.filterOptions.filterMask & FILTER_HAS_NATIONALITY) {
		snprintf(label, sizeof(label), "%s  x", gameContext.filterBuffer.nationality);
		if (chip(109, area, &x, &y, label)) {
			gameContext.filterBuffer.nationality[0] = '\0';
			gameContext.filterOptions.filterMask &= ~(uint32_t)FILTER_HAS_NATIONALITY;
			changed = true;
		}
	}
	for (unsigned i = 0; i < 14; ++i)
		if (options->positions & (1u << i)) {
			snprintf(label, sizeof(label), "%s  x", ui_positionNames[i]);
			if (chip(110 + i, area, &x, &y, label)) {
				options->positions &= (uint16_t)~(1u << i);
				gameContext.filterBuffer.positions &= (uint16_t)~(1u << i);
				if (!options->positions)
					options->filterMask &= ~(uint32_t)FILTER_HAS_POSITION;
				changed = true;
			}
		}
	if (changed) {
		char error[128];
		searchHandler_doSearch(false, error);
		ui.tableScroll = 0;
		ui.selectedRow = -1;
	}
	const bool all = !gameContext.filterOptions.filterMask;
	snprintf(label, sizeof(label), "%zu results%s", playerTable_count(), gameContext.searchActive && all ? " (showing all)" : "");
	if (x + 155 > area.x + area.width) {
		x = area.x + 6;
		y += 26;
	}
	ui_text((Rectangle){x, y, area.width - (x - area.x), 22}, label, 12, ui.colour.muted, false);
	return y - area.y + 30;
}

static void cell(Column column, const Player *player, Rectangle bounds, bool selected) {
	char value[160] = "";
	Color colour = selected ? WHITE : ui.colour.text;
	bool bold = false;
	switch (column) {
		case COLUMN_NATIONALITIES:
			for (unsigned i = 0; i < 4 && player->nationality[i] != 255; ++i)
				ui_flag(player->nationality[i], (Rectangle){bounds.x + (float)i * 22, bounds.y + 8, 20, 12});
			return;
		case COLUMN_NAME:
			player_formatName(player, value, sizeof(value));
			bold = true;
			break;
		case COLUMN_AGE: snprintf(value, sizeof(value), "%u", player->age); break;
		case COLUMN_POSITIONS: player_formatPositions(player, value, sizeof(value)); break;
		case COLUMN_CA:
			snprintf(value, sizeof(value), "%u", player->ca);
			if (!selected)
				colour = ui.colour.ca;
			break;
		case COLUMN_PA:
			snprintf(value, sizeof(value), "%u", player->pa);
			if (!selected)
				colour = ui.colour.pa;
			break;
		case COLUMN_CA_PA_DELTA: snprintf(value, sizeof(value), "%d", (int)player->pa - player->ca); break;
		case COLUMN_STATUS: {
			float x = bounds.x;
			if (player->injury.duration) {
				icons_draw(YAST_ICON_HEART, (Rectangle){x, bounds.y + 7, 16, 16}, 0);
				snprintf(value, sizeof(value), "%s (%u days)", injuryNames_get(player->injury.nameIndex), player->injury.duration);
				ui_tip(400000u + player->uid, (Rectangle){x, bounds.y, 18, bounds.height}, value);
				x += 20;
			}
			if (player->canDevelopQuickly) {
				ui_text((Rectangle){x, bounds.y, 18, bounds.height}, "L", 12, selected ? WHITE : ui.colour.ca, true);
				ui_tip(500000u + player->uid, (Rectangle){x, bounds.y, 18, bounds.height}, "Fast learner");
				x += 20;
			}
			if (player->isHotProspect) {
				ui_text((Rectangle){x, bounds.y, 18, bounds.height}, "P", 12, selected ? WHITE : ui.colour.orange, true);
				ui_tip(600000u + player->uid, (Rectangle){x, bounds.y, 18, bounds.height}, "Hot prospect");
			}
			return;
		}
		case COLUMN_RATING:
			formatter_formatRating(player->ratings[0].value, value);
			if (!selected)
				colour = ui_quality(player->ratings[0].value);
			break;
		case COLUMN_VALUE:
			snprintf(value, sizeof(value), "%u", player->guideValue);
			formatter_printCurrency(value);
			break;
		case COLUMN_CLUB: {
			const Club *club = entities_getClub(player->clubIndex);
			snprintf(value, sizeof(value), "%s", player->clubIndex == -1 ? "Free agent" : club ? club->shortName
																																												 : "");
			break;
		}
		case COLUMN_REPUTATION_HOME: snprintf(value, sizeof(value), "%u", player->homeReputation); break;
		case COLUMN_REPUTATION_CURRENT: snprintf(value, sizeof(value), "%u", player->currentReputation); break;
		case COLUMN_REPUTATION_WORLD: snprintf(value, sizeof(value), "%u", player->worldReputation); break;
		default: break;
	}
	ui_text(bounds, value, 12, colour, bold);
	if (column == COLUMN_NAME || column == COLUMN_CLUB || column == COLUMN_POSITIONS)
		ui_tip(700000u + (unsigned)column, bounds, value);
}

static void table(Rectangle area) {
	static const float widths[COLUMN_COUNT] = {96, 185, 44, 128, 48, 48, 54, 78, 65, 100, 180, 76, 76, 76};
	static const char *labels[COLUMN_COUNT] = {"Nation", "Name", "Age", "Positions", "CA", "PA", "Diff", "Status", "Rating", "Value", "Club", "Home rep.", "Current rep.", "World rep."};
	float contentWidth = 0;
	for (unsigned i = 0; i < COLUMN_COUNT; ++i)
		contentWidth += widths[i];
	const size_t count = playerTable_count();
	Rectangle body = {area.x, area.y + 32, area.width - 10, fmaxf(0, area.height - 42)};
	ui_scroll(301, body, (float)count * 30, &ui.tableScroll, false);
	ui_scroll(302, area, contentWidth, &ui.tableHorizontal, true);
	Rectangle previous = ui_clip(area);
	float x = area.x - ui.tableHorizontal;
	for (unsigned i = 0; i < COLUMN_COUNT; ++i) {
		Rectangle header = {x, area.y, widths[i], 30};
		if (CheckCollisionRecs(header, ui.clip)) {
			char label[48];
			int direction = playerTable_direction((Column)i);
			snprintf(label, sizeof(label), "%s%s", labels[i], direction > 0 ? " ^" : direction < 0 ? " v"
																																														 : "");
			if (playerTable_sortable((Column)i)) {
				if (ui_button(310 + i, header, label, false, "Sort; Shift-click to add a secondary sort")) {
					uint32_t uid = ui.selectedRow >= 0 && playerTable_player((size_t)ui.selectedRow) ? playerTable_player((size_t)ui.selectedRow)->uid : 0;
					playerTable_sort((Column)i, IsKeyDown(KEY_LEFT_SHIFT) || IsKeyDown(KEY_RIGHT_SHIFT));
					for (size_t row = 0; uid && row < count; ++row)
						if (playerTable_player(row)->uid == uid) {
							ui.selectedRow = (int)row;
							break;
						}
				}
			} else
				ui_text((Rectangle){x + 6, area.y, widths[i] - 12, 30}, label, 11, ui.colour.muted, true);
		}
		x += widths[i];
	}
	ui_clip((Rectangle){body.x, body.y, body.width - 10, body.height});
	if (ui_focusable(300, body)) {
		int row = (int)((ui.mouse.y - body.y + ui.tableScroll) / 30);
		if (row >= 0 && (size_t)row < count) {
			if (ui.selectedRow == row && ui.lastClickId == 300 && GetTime() - ui.lastClick < .35)
				ui_openPlayer(playerTable_player((size_t)row));
			ui.selectedRow = row;
			ui.lastClickId = 300;
			ui.lastClick = GetTime();
		}
	}
	if (ui.focus == 300 && !ui.keyConsumed && count) {
		int before = ui.selectedRow;
		if (IsKeyPressed(KEY_DOWN) || IsKeyPressedRepeat(KEY_DOWN))
			++ui.selectedRow;
		if (IsKeyPressed(KEY_UP) || IsKeyPressedRepeat(KEY_UP))
			--ui.selectedRow;
		if (IsKeyPressed(KEY_PAGE_DOWN))
			ui.selectedRow += (int)(body.height / 30);
		if (IsKeyPressed(KEY_PAGE_UP))
			ui.selectedRow -= (int)(body.height / 30);
		if (IsKeyPressed(KEY_HOME))
			ui.selectedRow = 0;
		if (IsKeyPressed(KEY_END))
			ui.selectedRow = (int)count - 1;
		if (ui.selectedRow < 0)
			ui.selectedRow = 0;
		if ((size_t)ui.selectedRow >= count)
			ui.selectedRow = (int)count - 1;
		if (before != ui.selectedRow) {
			if ((float)ui.selectedRow * 30 < ui.tableScroll)
				ui.tableScroll = (float)ui.selectedRow * 30;
			if ((float)(ui.selectedRow + 1) * 30 > ui.tableScroll + body.height)
				ui.tableScroll = (float)(ui.selectedRow + 1) * 30 - body.height;
		}
		if (IsKeyPressed(KEY_LEFT))
			ui.tableHorizontal = fmaxf(0, ui.tableHorizontal - 90);
		if (IsKeyPressed(KEY_RIGHT))
			ui.tableHorizontal = fminf(fmaxf(0, contentWidth - area.width), ui.tableHorizontal + 90);
		if (IsKeyPressed(KEY_ENTER)) {
			ui_openPlayer(playerTable_player((size_t)ui.selectedRow));
			ui.keyConsumed = true;
		}
	}
	size_t first = (size_t)(ui.tableScroll / 30);
	size_t end = first + (size_t)(body.height / 30) + 2;
	if (end > count)
		end = count;
	for (size_t row = first; row < end; ++row) {
		const Player *player = playerTable_player(row);
		if (!player)
			continue;
		Rectangle line = {body.x, body.y + (float)row * 30 - ui.tableScroll, body.width - 10, 30};
		bool selected = ui.selectedRow >= 0 && (size_t)ui.selectedRow == row;
		DrawRectangleRec(line, selected ? ui.colour.accent : ui_hit(line) ? ui.colour.hover
																											 : row & 1			? ui.colour.alternate
																																			: ui.colour.background);
		x = area.x - ui.tableHorizontal;
		for (unsigned column = 0; column < COLUMN_COUNT; ++column) {
			Rectangle bounds = {x + 6, line.y, widths[column] - 12, 30};
			if (CheckCollisionRecs(bounds, ui.clip))
				cell((Column)column, player, bounds, selected);
			x += widths[column];
		}
	}
	if (!count)
		ui_text((Rectangle){body.x + 16, body.y + 20, body.width - 32, 48}, gameContext.searchActive ? "No players match these filters." : "Choose filters and press Search to show players.", 12, ui.colour.muted, false);
	ui_unclip(previous);
}

void ui_search(Rectangle bounds) {
	sidebar((Rectangle){bounds.x, bounds.y, 210, bounds.height});
	Rectangle area = {bounds.x + 210, bounds.y, bounds.width - 210, bounds.height};
	float top = chips(area);
	table((Rectangle){area.x, area.y + top, area.width, area.height - top});
	if (!ui.keyConsumed && !ui.popup && IsKeyPressed(KEY_ENTER) && ui.focus < 100) {
		search();
		ui.keyConsumed = true;
	}
}

void ui_searchPopup(void) {
	if (ui.popup != 1 && ui.popup != 2)
		return;
	Autocomplete *source = ui.popup == 1 ? &ui.clubs : &ui.nations;
	int visible = (int)fminf(10, (float)source->count);
	if (!visible)
		visible = 1;
	Rectangle box = ui.popupBounds;
	box.height = (float)visible * 26 + 4;
	if (box.y + box.height > (float)GetScreenHeight() - 32)
		box.height = fmaxf(30, (float)GetScreenHeight() - box.y - 32);
	visible = (int)((box.height - 4) / 26);
	if (visible < 1)
		visible = 1;
	ui.popupBounds = box;
	Rectangle entry = {box.x, box.y - 32, 186, 30};
	if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT) && !CheckCollisionPointRec(ui.mouse, box) && !CheckCollisionPointRec(ui.mouse, entry)) {
		ui.popup = 0;
		return;
	}
	if (ui.focus != (ui.popup == 1 ? 20u : 21u)) {
		ui.popup = 0;
		return;
	}
	if (IsKeyPressed(KEY_DOWN) || IsKeyPressedRepeat(KEY_DOWN))
		++ui.popupRow;
	if (IsKeyPressed(KEY_UP) || IsKeyPressedRepeat(KEY_UP))
		--ui.popupRow;
	if (CheckCollisionPointRec(ui.mouse, box))
		ui.popupRow -= (int)GetMouseWheelMove();
	if (ui.popupRow < 0)
		ui.popupRow = 0;
	if (source->count && (size_t)ui.popupRow >= source->count)
		ui.popupRow = (int)source->count - 1;
	if (ui.popupRow < ui.popupFirst)
		ui.popupFirst = ui.popupRow;
	if (ui.popupRow >= ui.popupFirst + visible)
		ui.popupFirst = ui.popupRow - visible + 1;
	DrawRectangleRec(box, ui.colour.card);
	DrawRectangleLinesEx(box, 1, ui.colour.accent);
	bool choose = IsKeyPressed(KEY_ENTER) && !ui.keyConsumed && source->count;
	Rectangle previous = ui_clip(box);
	for (int i = ui.popupFirst; i < ui.popupFirst + visible && (size_t)i < source->count; ++i) {
		Rectangle row = {box.x + 2, box.y + 2 + (float)(i - ui.popupFirst) * 26, box.width - 4, 26};
		bool hover = CheckCollisionPointRec(ui.mouse, row);
		if (hover && IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
			ui.popupRow = i;
			choose = true;
		}
		if (i == ui.popupRow || hover)
			DrawRectangleRec(row, ui.colour.hover);
		ui_text((Rectangle){row.x + 6, row.y, row.width - 12, row.height}, source->matches[i].label, 12, ui.colour.text, false);
	}
	if (!source->count)
		ui_text((Rectangle){box.x + 8, box.y, box.width - 16, 26}, source->job || source->pending ? "Searching..." : "No matches", 12, ui.colour.muted, false);
	ui_unclip(previous);
	EndScissorMode();
	if (choose) {
		const SearchMatch *match = &source->matches[ui.popupRow];
		FilterBuffer *buffer = &gameContext.filterBuffer;
		if (ui.popup == 1) {
			gameContext.filterOptions.clubIndex = (int64_t)match->index;
			gameContext.filterOptions.filterMask |= FILTER_HAS_CLUB;
			snprintf(buffer->club, sizeof(buffer->club), "%s", match->label);
		} else {
			gameContext.filterOptions.nationalityIndex = (uint8_t)match->index;
			gameContext.filterOptions.filterMask |= FILTER_HAS_NATIONALITY;
			snprintf(buffer->nationality, sizeof(buffer->nationality), "%s", match->label);
		}
		ui.popup = 0;
		ui.keyConsumed = true;
		ui.editor = ui.focus = 0;
		search();
	}
}
