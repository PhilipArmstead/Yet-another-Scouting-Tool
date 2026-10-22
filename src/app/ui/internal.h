// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include "app/analysis.h"
#include "app/autocomplete.h"
#include "app/entities.h"
#include "app/helpers/vector.h"
#include "app/player-snapshot.h"
#include "app/player-table.h"
#include "app/ui.h"
#include "raygui.h"
#include "raylib.h"

typedef enum { VIEW_PLAYER,
							 VIEW_BEST_XI,
							 VIEW_DEPTH } ViewType;
typedef struct {
	unsigned id;
	ViewType type;
	PlayerSnapshot *snapshot;
	char title[80], name[80], formationName[FORMATION_NAME_LENGTH];
	float scroll, horizontal;
	bool pinned, dirty;
	int formation, minAge, maxAge, minCondition, maxCondition;
	bool excludeInjured;
	uint16_t positions;
	uint8_t highlights[ATTRIBUTE_COUNT];
	BestElevenRow eleven[11], depth[14][DEPTH_LIMIT];
	size_t depthCount[14];
} View;

typedef struct {
	Color background, sidebar, topbar, card, input, border, text, muted, title;
	Color accent, alternate, hover, high, mid, orange, ca, pa;
} Palette;

typedef struct {
	Font regular, bold, fallback;
	int *extraCodepoints;
	unsigned char extraSeen[0x110000 / 8];
	bool fontsDirty;
	float fontDpi;
	Palette colour;
	View **views;
	unsigned nextId, active, focus, editor, drag;
	unsigned *focusOrder;
	size_t caret, anchor;
	float editScroll, dragOffset, tableScroll, tableHorizontal, sidebarScroll, tabScroll;
	int selectedRow, popup, popupRow, popupFirst;
	double lastClick, hoverSince;
	unsigned lastClickId, hoverId, previousHoverId;
	Vector2 mouse;
	Rectangle clip, popupBounds;
	bool mouseConsumed, keyConsumed, quit, dark;
	char status[256], tooltip[256];
	Autocomplete clubs, nations;
	const Club *indexedClubs;
	const Nation *indexedNations;
	uint64_t revision, optionsRevision;
	Texture2D pitch;
	struct {
		Texture2D texture;
		char code[4];
		bool attempted;
	} flags[256];
} Ui;

extern Ui ui;
extern GameContext gameContext;
extern ProcessContext processContext;
extern const char *const ui_positionNames[14];
extern const PositionCode ui_positionCodes[14];

unsigned ui_id(const View *view, unsigned local);
void ui_beginControls(void);
bool ui_hit(Rectangle bounds);
bool ui_focusable(unsigned id, Rectangle bounds);
bool ui_button(unsigned id, Rectangle bounds, const char *text, bool primary, const char *tooltip);
bool ui_check(unsigned id, Rectangle bounds, const char *text, bool *checked);
bool ui_number(unsigned id, Rectangle bounds, const char *label, int *value, int min, int max);
bool ui_edit(unsigned id, Rectangle bounds, char *text, size_t capacity, const char *placeholder, bool readOnly);
void ui_text(Rectangle bounds, const char *text, float size, Color colour, bool bold);
void ui_prepareText(const char *text);
float ui_measureText(const char *text, float size, bool bold);
void ui_tip(unsigned id, Rectangle bounds, const char *text);
void ui_scroll(unsigned id, Rectangle bounds, float content, float *offset, bool horizontal);
Rectangle ui_clip(Rectangle bounds);
void ui_unclip(Rectangle previous);
Color ui_quality(float rating);
void ui_flag(uint8_t nationIndex, Rectangle bounds);
void ui_openPlayer(const Player *player);
void ui_analysisRow(View *view, Rectangle bounds, const BestElevenRow *row, const char *position, unsigned index, bool positional);
void ui_playerInfo(View *view, Rectangle bounds);
void ui_bestEleven(View *view, Rectangle bounds);
void ui_squadDepth(View *view, Rectangle bounds);
void ui_search(Rectangle bounds);
void ui_searchPopup(void);
