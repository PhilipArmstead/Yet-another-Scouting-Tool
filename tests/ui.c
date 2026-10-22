// SPDX-License-Identifier: GPL-3.0-or-later
#include "app/injury-names.h"
#include "app/options.h"
#include "app/player.h"
#include "app/search-handler.h"
#include "app/ui/internal.h"
#include "platform/platform.h"
#include <assert.h>
#include <time.h>

GameContext gameContext;
ProcessContext processContext;

// Raylib 6.2 automation event IDs (rcore.c); replay does not send OS-wide input.
enum { EVENT_KEY_UP = 1,
			 EVENT_KEY_DOWN = 2,
			 MOUSE_UP = 5,
			 MOUSE_DOWN = 6,
			 MOUSE_POSITION = 7 };

static void frame(void) {
	ui_update();
	ui_draw();
}
static void event(unsigned type, int a, int b) {
	PlayAutomationEvent((AutomationEvent){.type = type, .params = {a, b}});
}
static void key(int code) {
	event(EVENT_KEY_DOWN, code, 0);
	frame();
	event(EVENT_KEY_UP, code, 0);
	frame();
}
static void click(int x, int y) {
	event(MOUSE_POSITION, x, y);
	event(MOUSE_DOWN, MOUSE_BUTTON_LEFT, 0);
	frame();
	event(MOUSE_UP, MOUSE_BUTTON_LEFT, 0);
	frame();
}

int main(void) {
	assert(injuryNames_init());
	options_defaults();
	playerTable_init();
	assert(ui_init());
	gameContext.playerCount = 100000;
	gameContext.players = calloc((size_t)gameContext.playerCount, sizeof(Player));
	assert(gameContext.players);
	for (uint32_t i = 0; i < gameContext.playerCount; ++i) {
		Player *player = &gameContext.players[i];
		player->uid = i + 1;
		player->age = (uint8_t)(16 + i % 30);
		player->condition = 9000;
		player->ca = 150;
		player->pa = 200;
		memset(player->nationality, 255, sizeof(player->nationality));
		memset(player->attributes, 80, sizeof(player->attributes));
		player->positions[ui_positionCodes[i % 14]] = 20;
		snprintf(player->commonName, sizeof(player->commonName), "Player %u", i);
		getSortedPositionRatings(player);
	}
	++gameContext.revision;
	frame();
	ui.focus = 20;
	snprintf(gameContext.filterBuffer.club, sizeof(gameContext.filterBuffer.club), "M\xc3\xbcnchen");
	frame();
	key(KEY_BACKSPACE);
	assert(!strcmp(gameContext.filterBuffer.club, "M\xc3\xbcnche"));
	event(EVENT_KEY_DOWN, KEY_LEFT_CONTROL, 0);
	key(KEY_A);
	event(EVENT_KEY_UP, KEY_LEFT_CONTROL, 0);
	frame();
	key(KEY_BACKSPACE);
	assert(!gameContext.filterBuffer.club[0]);
	ui.popup = 0;
	ui.focus = 20;
	key(KEY_TAB);
	assert(ui.focus == 21);
	event(EVENT_KEY_DOWN, KEY_LEFT_SHIFT, 0);
	key(KEY_TAB);
	event(EVENT_KEY_UP, KEY_LEFT_SHIFT, 0);
	frame();
	assert(ui.focus == 20);
	snprintf(gameContext.filterBuffer.club, sizeof(gameContext.filterBuffer.club), "\xc3\xa9");
	ui.editor = 0;
	frame();
	key(KEY_BACKSPACE);
	assert(!gameContext.filterBuffer.club[0] && ui.caret == 0);
	ui.focus = 22;
	snprintf(gameContext.filterBuffer.minAge, sizeof(gameContext.filterBuffer.minAge), "20");
	key(KEY_ENTER);
	assert(gameContext.searchActive && playerTable_count() > 0);
	assert(playerTable_count() < gameContext.playerCount);
	ui.focus = 48;
	key(KEY_SPACE);
	assert(gameContext.filterBuffer.positions & POSITION_MASK_MC);
	assert(ui.extraCodepoints == NULL);
	ui.focus = 300;
	key(KEY_DOWN);
	assert(ui.selectedRow >= 0);
	key(KEY_ENTER);
	assert(vector_length(ui.views) == 1 && ui.active == ui.views[0]->id);
	const View *view = ui.views[0];
	assert(view->snapshot->players != gameContext.players && view->snapshot->count == 1);
	click(GetScreenWidth() - 50, 100);
	assert(view->pinned);
	event(EVENT_KEY_DOWN, KEY_LEFT_CONTROL, 0);
	key(KEY_W);
	event(EVENT_KEY_UP, KEY_LEFT_CONTROL, 0);
	frame();
	assert(!vector_length(ui.views) && !ui.active);
	searchHandler_clearFilters();
	char error[128];
	assert(searchHandler_doSearch(true, error) && playerTable_count() == 100000);
	ui.focus = ui.editor = 0;
	const Column columns[] = {COLUMN_NAME, COLUMN_AGE, COLUMN_CA, COLUMN_PA, COLUMN_CA_PA_DELTA, COLUMN_RATING};
	const int64_t sortStart = platform_getMicroseconds();
	for (unsigned i = 0; i < 6; ++i) {
		playerTable_sort(columns[i], false);
		playerTable_sort(columns[i], false);
	}
	printf("100k table, twelve sorts: %.2f ms\n", (double)(platform_getMicroseconds() - sortStart) / 1000);
	const clock_t cpu = clock();
	const int64_t start = platform_getMicroseconds();
	for (unsigned i = 0; i < 20; ++i)
		frame();
	printf("100k virtualized table: %.2f ms/frame (including frame cap)\n", (double)(platform_getMicroseconds() - start) / 20000);
	printf("100k visible frame CPU: %.2f ms/frame\n", (double)(clock() - cpu) * 1000 / CLOCKS_PER_SEC / 20);
	ui_prepareText("\xe4\xb8\xad");
	frame();
	assert(ui.fallback.texture.id && ui.fallback.glyphs[GetGlyphIndex(ui.fallback, 0x4e2d)].value == 0x4e2d);
	ui.focus = 2;
	key(KEY_ENTER);
	View *eleven = ui.views[0];
	assert(eleven->type == VIEW_BEST_XI && eleven->snapshot->count == 100000);
	for (unsigned i = 0; i < 11; ++i)
		assert(eleven->eleven[i].player);
	ui.focus = ui_id(eleven, 22);
	key(KEY_BACKSPACE);
	key(KEY_BACKSPACE);
	assert(eleven->minAge == 0 && eleven->dirty);
	key(KEY_ENTER);
	assert(eleven->minAge == 14 && !eleven->dirty);
	key(KEY_TAB);
	assert(ui.focus == ui_id(eleven, 21));
	key(KEY_ENTER);
	assert(eleven->minAge == 15);
	ui.focus = ui_id(eleven, 11);
	key(KEY_ENTER);
	assert(eleven->formation == 1);
	ui.focus = ui_id(eleven, 36);
	key(KEY_SPACE);
	assert(!eleven->excludeInjured);
	ui.focus = 3;
	key(KEY_ENTER);
	View *depth = ui.views[1];
	assert(depth->type == VIEW_DEPTH);
	for (unsigned i = 0; i < 14; ++i)
		assert(depth->depthCount[i] == DEPTH_LIMIT);
	ui.focus = ui_id(depth, 20);
	key(KEY_SPACE);
	assert(!(depth->positions & POSITION_MASK_GK));
	vector_free(gameContext.options.formations);
	++gameContext.optionsRevision;
	ui.focus = 2;
	key(KEY_ENTER);
	assert(ui.views[2]->type == VIEW_BEST_XI && !vector_length(gameContext.options.formations));
	ui.focus = 4;
	key(KEY_ENTER);
	SetWindowSize(800, 560);
	frame();
	frame();
	ui.focus = 10;
	key(KEY_END);
	assert(ui.sidebarScroll > 0);
	const size_t controls = vector_length(ui.focusOrder);
	for (size_t i = 0; i < controls && ui.focus != 30; ++i)
		key(KEY_TAB);
	assert(ui.focus == 30);
	key(KEY_ENTER);
	assert(gameContext.searchActive && playerTable_count() == 100000);
	ui.focus = 10;
	key(KEY_HOME);
	assert(ui.sidebarScroll == 0);
	ui.focus = 302;
	key(KEY_END);
	assert(ui.tableHorizontal > 0);
	key(KEY_HOME);
	assert(ui.tableHorizontal == 0);
	ui_shutdown();
	playerTable_shutdown();
	free(gameContext.players);
	options_shutdown();
	injuryNames_destroy();
	puts("UI checks passed");
	return 0;
}
