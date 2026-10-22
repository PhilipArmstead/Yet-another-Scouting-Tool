// SPDX-License-Identifier: GPL-3.0-or-later
#include "app/helpers/formatter.h"
#include "app/helpers/icons.h"
#include "app/injury-names.h"
#include "app/player.h"
#include "internal.h"
#include <math.h>

typedef struct {
	const char *label;
	uint8_t attribute;
} Attribute;
static const Attribute technical[] = {
	{"Corners", ATTR_COR}, {"Crossing", ATTR_CRO}, {"Dribbling", ATTR_DRI}, {"Finishing", ATTR_FIN}, {"First touch", ATTR_FIR}, {"Free kicks", ATTR_FRE}, {"Heading", ATTR_HEA}, {"Long shots", ATTR_LON}, {"Long throws", ATTR_LTH}, {"Marking", ATTR_MAR}, {"Passing", ATTR_PAS}, {"Penalty taking", ATTR_PEN}, {"Tackling", ATTR_TCK}, {"Technique", ATTR_TEC}};
static const Attribute goalkeeper[] = {
	{"Aerial reach", ATTR_AER}, {"Command of area", ATTR_CMD}, {"Communication", ATTR_COM}, {"Eccentricity", ATTR_ECC}, {"First touch", ATTR_FIR}, {"Handling", ATTR_HAN}, {"Kicking", ATTR_KIC}, {"One on ones", ATTR_ONE}, {"Passing", ATTR_PAS}, {"Punching tendency", ATTR_TTP}, {"Reflexes", ATTR_REF}, {"Rushing out tendency", ATTR_TRO}, {"Throwing", ATTR_THR}};
static const Attribute mental[] = {
	{"Aggression", ATTR_AGG}, {"Anticipation", ATTR_ANT}, {"Bravery", ATTR_BRA}, {"Concentration", ATTR_CNT}, {"Composure", ATTR_CMP}, {"Decisions", ATTR_DEC}, {"Determination", ATTR_DET}, {"Flair", ATTR_FLA}, {"Leadership", ATTR_LDR}, {"Off the ball", ATTR_OTB}, {"Positioning", ATTR_POS}, {"Teamwork", ATTR_TEA}, {"Vision", ATTR_VIS}, {"Work rate", ATTR_WOR}};
static const Attribute physical[] = {
	{"Acceleration", ATTR_ACC}, {"Agility", ATTR_AGI}, {"Balance", ATTR_BAL}, {"Jumping reach", ATTR_JUM}, {"Natural fitness", ATTR_NAT}, {"Pace", ATTR_PAC}, {"Stamina", ATTR_STA}, {"Strength", ATTR_STR}};
static const Attribute hidden[] = {
	{"Adaptability", ATTR_ADA}, {"Ambition", ATTR_AMB}, {"Consistency", ATTR_CON}, {"Controversy", ATTR_CNY}, {"Dirtiness", ATTR_DIR}, {"Important matches", ATTR_IMP}, {"Injury proneness", ATTR_INJ}, {"Loyalty", ATTR_LOY}, {"Pressure", ATTR_PRE}, {"Professionalism", ATTR_PRO}, {"Sportsmanship", ATTR_SPO}, {"Temperament", ATTR_TEM}, {"Versatility", ATTR_VER}};
static const char *const roles[POSITION_GROUPED_COUNT] = {
	"Goalkeeper", "Full back", "Centre back", "Wing back", "Defensive midfielder", "Midfielder",
	"Winger", "Attacking midfielder", "Striker"};

static void attributes(View *view, Rectangle area, const char *title, const Attribute *rows, size_t count) {
	const Player *player = view->snapshot->players;
	DrawRectangleRounded(area, .04f, 4, ui.colour.card);
	ui_text((Rectangle){area.x + 10, area.y + 4, area.width - 20, 28}, title, 12, ui.colour.title, true);
	for (size_t i = 0; i < count; ++i) {
		const Attribute row = rows[i];
		Rectangle bounds = {area.x + 5, area.y + 34 + (float)i * 23, area.width - 10, 22};
		uint8_t highlight = view->highlights[row.attribute];
		if (highlight)
			DrawRectangleRec(bounds, highlight == 2 ? ui.colour.high : ui.colour.mid);
		ui_text((Rectangle){bounds.x + 5, bounds.y, bounds.width - 35, bounds.height}, row.label, 12, ui.colour.text, false);
		ui_tip(ui_id(view, 200u + row.attribute), bounds, row.label);
		char value[8];
		snprintf(value, sizeof(value), "%u", convertTo20Scale(player->attributes[row.attribute]));
		ui_text((Rectangle){bounds.x + bounds.width - 26, bounds.y, 24, bounds.height}, value, 12, ui.colour.text, true);
	}
}

static void header(View *view, Rectangle area) {
	const Player *player = view->snapshot->players;
	float x = area.x + 12, width = area.width - 24, y = area.y + 8;
	ui_edit(ui_id(view, 10), (Rectangle){x, y, width - 94, 32}, view->name, sizeof(view->name), "", true);
	if (ui_button(ui_id(view, 11), (Rectangle){x + width - 86, y, 86, 32}, view->pinned ? "Unpin" : "Compare", view->pinned, "Pin this player alongside other views"))
		view->pinned = !view->pinned;
	y += 40;
	const Club *club = entities_getClub(player->clubIndex);
	ui_text((Rectangle){x, y, width, 24}, player->clubIndex == -1 ? "Free agent" : club ? club->name
																																											: "Club unavailable",
					14, ui.colour.title, true);
	y += 28;
	for (unsigned i = 0; i < 4 && player->nationality[i] != 255; ++i)
		ui_flag(player->nationality[i], (Rectangle){x + (float)i * 26, y + 5, 22, 14});
	char value[160], positions[100];
	player_formatPositions(player, positions, sizeof(positions));
	snprintf(value, sizeof(value), "%u yrs  %u cm  %u kg   %s", player->age, player->height, player->weight, positions);
	ui_text((Rectangle){x + 110, y, width - 110, 24}, value, 12, ui.colour.muted, false);
	y += 30;
	snprintf(value, sizeof(value), "CA %u", player->ca);
	ui_text((Rectangle){x, y, 72, 28}, value, 14, ui.colour.ca, true);
	snprintf(value, sizeof(value), "PA %u", player->pa);
	ui_text((Rectangle){x + 74, y, 72, 28}, value, 14, ui.colour.pa, true);
	icons_draw(YAST_ICON_SHOE_LEFT, (Rectangle){x + 155, y, 11, 27}, icons_footQuantise(player->attributes[ATTR_LEF]));
	icons_draw(YAST_ICON_SHOE_RIGHT, (Rectangle){x + 170, y, 11, 27}, icons_footQuantise(player->attributes[ATTR_RIG]));
	snprintf(value, sizeof(value), "Left foot: %u / 20; right foot: %u / 20", convertTo20Scale(player->attributes[ATTR_LEF]), convertTo20Scale(player->attributes[ATTR_RIG]));
	ui_tip(ui_id(view, 12), (Rectangle){x + 151, y, 35, 28}, value);
	icons_draw(YAST_ICON_FACE, (Rectangle){x + 200, y, 27, 27}, icons_moraleQuantise(player->morale));
	snprintf(value, sizeof(value), "Morale: %u / 20", player->morale);
	ui_tip(ui_id(view, 13), (Rectangle){x + 196, y, 35, 28}, value);
	if (player->canDevelopQuickly)
		ui_text((Rectangle){x + 240, y, 110, 28}, "Fast learner", 12, ui.colour.ca, true);
	y += 34;
	const int values[] = {player->condition, player->sharpness, abs(player->fatigue)};
	const char *labels[] = {"Condition", "Sharpness", "Fatigue"};
	for (unsigned i = 0; i < 3; ++i) {
		float column = width / 3;
		snprintf(value, sizeof(value), "%s %.1f%%", labels[i], (double)values[i] / 100);
		ui_text((Rectangle){x + (float)i * column, y, column - 4, 24}, value, 12, ui_quality(i == 2 ? 120 - (float)values[i] * .012f : (float)values[i] * .012f), false);
	}
	y += 28;
	if (player->injury.duration) {
		snprintf(value, sizeof(value), "Injured: %s (%u days)", injuryNames_get(player->injury.nameIndex), player->injury.duration);
		ui_text((Rectangle){x, y, width, 24}, value, 12, ui.colour.orange, true);
		y += 24;
	}
	if (player->isHotProspect)
		ui_text((Rectangle){x, y, width, 24}, "Hot prospect", 12, ui.colour.orange, true);
}

void ui_playerInfo(View *view, Rectangle bounds) {
	if (!view->snapshot->count)
		return;
	const Player *player = view->snapshot->players;
	if (view->dirty) {
		player_formatName(player, view->name, sizeof(view->name));
		const PositionWeights *weights = getWeightsForPosition(player->ratings[0].position);
		float max = 0;
		memset(view->highlights, 0, sizeof(view->highlights));
		for (size_t i = 0; i < vector_length(weights->weights); ++i)
			if (weights->weights[i].weight > max)
				max = weights->weights[i].weight;
		for (size_t i = 0; i < vector_length(weights->weights); ++i) {
			RatingWeight weight = weights->weights[i];
			view->highlights[weight.attribute] = weight.weight > max * .5f ? 2 : weight.weight > max * .15f ? 1
																																																			: 0;
		}
		view->dirty = false;
	}
	int columns = bounds.width >= 900 ? 4 : 2;
	float columnWidth = (bounds.width - 36) / (float)columns;
	const float attributesTop = 228, groupHeight = 366;
	float rolesTop = attributesTop + groupHeight * (columns == 4 ? 1 : 2) + 16;
	float contentHeight = rolesTop + 40 + 40 * POSITION_GROUPED_COUNT;
	Rectangle previous = ui_clip(bounds);
	ui_scroll(ui_id(view, 14), bounds, contentHeight, &view->scroll, false);
	Rectangle content = bounds;
	content.y -= view->scroll;
	header(view, content);
	for (int i = 0; i < 4; ++i) {
		Rectangle group = {content.x + 10 + (float)(i % columns) * columnWidth, content.y + attributesTop + (float)(i / columns) * groupHeight, columnWidth - 8, groupHeight - 8};
		if (!CheckCollisionRecs(group, ui.clip))
			continue;
		if (i == 0)
			attributes(view, group, player->positions[0] >= 12 ? "GOALKEEPING" : "TECHNICAL", player->positions[0] >= 12 ? goalkeeper : technical, player->positions[0] >= 12 ? sizeof(goalkeeper) / sizeof(*goalkeeper) : sizeof(technical) / sizeof(*technical));
		else if (i == 1)
			attributes(view, group, "MENTAL", mental, sizeof(mental) / sizeof(*mental));
		else if (i == 2)
			attributes(view, group, "PHYSICAL", physical, sizeof(physical) / sizeof(*physical));
		else
			attributes(view, group, "HIDDEN / PERSONALITY", hidden, sizeof(hidden) / sizeof(*hidden));
	}
	float x = content.x + 16, y = content.y + rolesTop, width = content.width - 40;
	ui_text((Rectangle){x, y, width, 28}, "ROLE RATINGS", 12, ui.colour.title, true);
	y += 34;
	for (unsigned i = 0; i < POSITION_GROUPED_COUNT && player->ratings[i].value > 0; ++i) {
		Rating rating = player->ratings[i];
		char value[32];
		formatter_formatRating(rating.value, value);
		ui_text((Rectangle){x, y, width - 64, 22}, roles[rating.position], 12, ui.colour.text, true);
		ui_text((Rectangle){x + width - 60, y, 60, 22}, value, 12, ui_quality(rating.value), true);
		DrawRectangleRounded((Rectangle){x, y + 26, width, 5}, 1, 4, ui.colour.input);
		DrawRectangleRounded((Rectangle){x, y + 26, width * fminf(1, rating.value / 100), 5}, 1, 4, ui.colour.orange);
		y += 40;
	}
	ui_unclip(previous);
}
