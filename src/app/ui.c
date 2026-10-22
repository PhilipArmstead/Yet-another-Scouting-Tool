// SPDX-License-Identifier: GPL-3.0-or-later
#include "app/cache.h"
#include "app/configuration.h"
#include "app/helpers/date.h"
#include "app/helpers/formatter.h"
#include "app/helpers/icons.h"
#include "app/options.h"
#include "app/player.h"
#include "app/resources.h"
#include "app/search-handler.h"
#include "core/logger.h"
#include "ui/internal.h"
#include <math.h>

Ui ui;
const char *const ui_positionNames[14] = {"GK", "DL", "DC", "DR", "WBL", "DM", "WBR", "ML", "MC", "MR", "AML", "AMC", "AMR", "ST"};
const PositionCode ui_positionCodes[14] = {POSITION_CODE_GK, POSITION_CODE_DL, POSITION_CODE_DC, POSITION_CODE_DR,
																					 POSITION_CODE_WBL, POSITION_CODE_DM, POSITION_CODE_WBR, POSITION_CODE_ML, POSITION_CODE_MC, POSITION_CODE_MR,
																					 POSITION_CODE_AML, POSITION_CODE_AMC, POSITION_CODE_AMR, POSITION_CODE_ST};

static void theme(void) {
	ui.dark = gameContext.options.darkMode;
	ui.colour = ui.dark ? (Palette){
													.background = {15, 18, 22, 255}, .sidebar = {16, 20, 26, 255}, .topbar = {20, 24, 29, 255}, .card = {27, 36, 48, 255}, .input = {26, 32, 40, 255}, .border = {42, 53, 66, 255}, .text = {194, 201, 208, 255}, .muted = {154, 164, 173, 255}, .title = {242, 242, 242, 255}, .accent = {47, 111, 237, 255}, .alternate = {18, 23, 29, 255}, .hover = {40, 54, 72, 255}, .high = {51, 64, 9, 255}, .mid = {68, 57, 17, 255}, .orange = {224, 138, 44, 255}, .ca = {102, 181, 229, 255}, .pa = {102, 203, 147, 255}}
											: (Palette){.background = WHITE, .sidebar = {246, 248, 250, 255}, .topbar = {246, 248, 250, 255}, .card = WHITE, .input = {238, 241, 245, 255}, .border = {194, 203, 212, 255}, .text = {36, 41, 47, 255}, .muted = {75, 83, 91, 255}, .title = {27, 31, 36, 255}, .accent = {26, 90, 217, 255}, .alternate = {246, 248, 250, 255}, .hover = {221, 227, 234, 255}, .high = {214, 235, 168, 255}, .mid = {248, 233, 175, 255}, .orange = {168, 92, 20, 255}, .ca = {26, 111, 168, 255}, .pa = {26, 127, 75, 255}};
	GuiSetStyle(DEFAULT, TEXT_SIZE, 12);
	GuiSetStyle(DEFAULT, BORDER_COLOR_NORMAL, (int)ColorToInt(ui.colour.border));
	GuiSetStyle(DEFAULT, BASE_COLOR_NORMAL, (int)ColorToInt(ui.colour.input));
	GuiSetStyle(DEFAULT, TEXT_COLOR_NORMAL, (int)ColorToInt(ui.colour.text));
	GuiSetStyle(DEFAULT, BORDER_COLOR_FOCUSED, (int)ColorToInt(ui.colour.accent));
	GuiSetStyle(DEFAULT, BASE_COLOR_FOCUSED, (int)ColorToInt(ui.colour.hover));
	GuiSetStyle(DEFAULT, TEXT_COLOR_FOCUSED, (int)ColorToInt(ui.colour.title));
	GuiSetStyle(DEFAULT, BORDER_COLOR_PRESSED, (int)ColorToInt(ui.colour.accent));
	GuiSetStyle(DEFAULT, BASE_COLOR_PRESSED, (int)ColorToInt(ui.colour.accent));
	GuiSetStyle(DEFAULT, TEXT_COLOR_PRESSED, (int)ColorToInt(WHITE));
	GuiSetStyle(DEFAULT, BACKGROUND_COLOR, (int)ColorToInt(ui.colour.background));
	GuiSetStyle(DEFAULT, TEXT_SPACING, 0);
	formatter_init();
}

static bool loadTexture(const char *name, Texture2D *texture) {
	const Resource *resource = resources_get(name);
	if (!resource) {
		LOG_ERROR("Missing embedded resource %s", name);
		return false;
	}
	Image image = LoadImageFromMemory(".png", resource->data, (int)resource->size);
	if (!IsImageValid(image)) {
		LOG_ERROR("Cannot decode %s", name);
		return false;
	}
	*texture = LoadTextureFromImage(image);
	UnloadImage(image);
	if (!IsTextureValid(*texture)) {
		LOG_ERROR("Cannot upload %s", name);
		return false;
	}
	SetTextureFilter(*texture, TEXTURE_FILTER_BILINEAR);
	return true;
}

static bool loadFonts(void) {
	int codepoints[1800], count = 0;
	for (int cp = 32; cp <= 0x52f; ++cp)
		codepoints[count++] = cp;
	for (int cp = 0x1e00; cp <= 0x1eff; ++cp)
		codepoints[count++] = cp;
	codepoints[count++] = 0x2013;
	codepoints[count++] = 0x2014;
	codepoints[count++] = 0x2019;
	codepoints[count++] = 0x2022;
	codepoints[count++] = 0x2026;
	const Resource *regular = resources_get("assets/fonts/NotoSans-Regular.ttf");
	const Resource *bold = resources_get("assets/fonts/NotoSans-Bold.ttf");
	if (!regular || !bold) {
		LOG_ERROR("Missing embedded fonts");
		return false;
	}
	ui.fontDpi = fmaxf(1, GetWindowScaleDPI().x);
	const int size = (int)(16 * ui.fontDpi);
	Font normalFont = LoadFontFromMemory(".ttf", regular->data, (int)regular->size, size, codepoints, count);
	Font boldFont = LoadFontFromMemory(".ttf", bold->data, (int)bold->size, size, codepoints, count);
	if (!IsFontValid(normalFont) || !IsFontValid(boldFont) ||
			normalFont.texture.id == GetFontDefault().texture.id || boldFont.texture.id == GetFontDefault().texture.id) {
		if (normalFont.texture.id && normalFont.texture.id != GetFontDefault().texture.id)
			UnloadFont(normalFont);
		if (boldFont.texture.id && boldFont.texture.id != GetFontDefault().texture.id)
			UnloadFont(boldFont);
		LOG_ERROR("Cannot load embedded fonts");
		return false;
	}
	if (ui.regular.texture.id)
		UnloadFont(ui.regular);
	if (ui.bold.texture.id)
		UnloadFont(ui.bold);
	ui.regular = normalFont;
	ui.bold = boldFont;
	SetTextureFilter(ui.regular.texture, TEXTURE_FILTER_BILINEAR);
	SetTextureFilter(ui.bold.texture, TEXTURE_FILTER_BILINEAR);
	GuiSetFont(ui.regular);
	GuiSetStyle(DEFAULT, TEXT_SIZE, 12);
	ui.fontsDirty = true;
	return true;
}

static void updateFallback(void) {
	if (!ui.fontsDirty)
		return;
	ui.fontsDirty = false;
	if (!vector_length(ui.extraCodepoints))
		return;
	const Resource *resource = resources_get("assets/fonts/Unifont.otf");
	if (!resource) {
		LOG_ERROR("Missing international fallback font");
		ui_setCurrentStatus("Missing international font.");
		return;
	}
	Font font = LoadFontFromMemory(".otf", resource->data, (int)resource->size, (int)(16 * ui.fontDpi), ui.extraCodepoints, (int)vector_length(ui.extraCodepoints));
	if (!IsFontValid(font) || font.texture.id == GetFontDefault().texture.id) {
		LOG_ERROR("Cannot load international fallback font");
		ui_setCurrentStatus("Cannot load international font.");
		return;
	}
	if (ui.fallback.texture.id)
		UnloadFont(ui.fallback);
	ui.fallback = font;
	SetTextureFilter(font.texture, TEXTURE_FILTER_BILINEAR);
	if ((size_t)font.glyphCount != vector_length(ui.extraCodepoints)) {
		LOG_WARN("Some characters are not present in the bundled fonts");
		ui_setCurrentStatus("Some characters are missing from the bundled fonts. See the log.");
	}
}

bool ui_init(void) {
	ui.nextId = 100;
	ui.selectedRow = -1;
	SetConfigFlags(FLAG_WINDOW_RESIZABLE | FLAG_WINDOW_HIGHDPI | FLAG_VSYNC_HINT);
	InitWindow(1255, 800, "Yet another Scouting Tool");
	if (!IsWindowReady()) {
		LOG_ERROR("Could not create the application window");
		return false;
	}
	SetWindowMinSize(800, 560);
	SetExitKey(KEY_NULL);
	SetTargetFPS(60);
	if (!loadFonts() || !icons_init() || !loadTexture("assets/icons/pitch.png", &ui.pitch))
		return false;
#ifndef ARCH_MACOS
	const Resource *icon = resources_get("assets/branding/png/128.png");
	if (icon) {
		Image image = LoadImageFromMemory(".png", icon->data, (int)icon->size);
		SetWindowIcon(image);
		UnloadImage(image);
	}
#endif
	theme();
	return true;
}

void ui_setCurrentStatus(const char *status) {
	snprintf(ui.status, sizeof(ui.status), "%s", status);
}

static View *newView(ViewType type, PlayerSnapshot *snapshot) {
	if (!snapshot) {
		ui_setCurrentStatus("Cannot allocate player snapshot. See the log.");
		return NULL;
	}
	View *view = calloc(1, sizeof(*view));
	if (!view) {
		playerSnapshot_free(snapshot);
		LOG_ERROR("Cannot allocate view");
		ui_setCurrentStatus("Cannot allocate view.");
		return NULL;
	}
	view->id = ui.nextId++;
	view->type = type;
	view->snapshot = snapshot;
	view->dirty = true;
	view->minAge = 14;
	view->maxAge = 50;
	view->minCondition = 80;
	view->maxCondition = 100;
	view->excludeInjured = true;
	view->positions = 0x3fff;
	vector_push(ui.views, view);
	ui.active = view->id;
	ui.popup = 0;
	ui.focus = ui.editor = 0;
	return view;
}

void ui_openPlayer(const Player *player) {
	if (!player || !player->uid) {
		ui_setCurrentStatus("No player is available.");
		return;
	}
	View *view = newView(VIEW_PLAYER, playerSnapshot_createOne(player));
	if (!view)
		return;
	player_formatName(player, view->name, sizeof(view->name));
	ui_prepareText(view->name);
	snprintf(view->title, sizeof(view->title), "%s", view->name);
}

static void openAnalysis(ViewType type) {
	const uint32_t *ids = gameContext.searchResults ? gameContext.searchResults->data : NULL;
	View *view = newView(type, playerSnapshot_create(ids, vector_length(ids)));
	if (view)
		snprintf(view->title, sizeof(view->title), "%s", type == VIEW_BEST_XI ? "Best XI" : "Squad depth");
}

static void closeView(size_t index) {
	View *view = ui.views[index];
	if (ui.active == view->id)
		ui.active = 0;
	playerSnapshot_free(view->snapshot);
	free(view);
	vector_splice(ui.views, index, &view);
	ui.focus = ui.editor = 0;
}

static void updateFlags(void) {
	for (unsigned i = 0; i < 255; ++i) {
		const Nation *nation = entities_getNation((uint8_t)i);
		const char *code = nation ? nation->code : "";
		if (!strcmp(ui.flags[i].code, code))
			continue;
		if (ui.flags[i].texture.id)
			UnloadTexture(ui.flags[i].texture);
		ui.flags[i].texture = (Texture2D){0};
		snprintf(ui.flags[i].code, sizeof(ui.flags[i].code), "%s", code);
		if (!code[0])
			continue;
		char path[64];
		snprintf(path, sizeof(path), "assets/flags/%s.png", code);
		if (resources_get(path))
			loadTexture(path, &ui.flags[i].texture);
		else
			LOG_WARN("No flag artwork for %s; showing its country code", code);
	}
}

void ui_flag(uint8_t nationIndex, Rectangle bounds) {
	const Nation *nation = entities_getNation(nationIndex);
	if (!nation)
		return;
	Texture2D texture = ui.flags[nationIndex].texture;
	if (texture.id)
		DrawTexturePro(texture, (Rectangle){0, 0, (float)texture.width, (float)texture.height}, bounds, (Vector2){0}, 0, WHITE);
	else
		ui_text(bounds, nation->code, 10, ui.colour.text, true);
	ui_tip(800000u + nationIndex, bounds, nation->name);
}

void ui_update(void) {
	if (fabsf(ui.fontDpi - fmaxf(1, GetWindowScaleDPI().x)) > .01f && !loadFonts())
		ui_setCurrentStatus("Cannot update fonts for this display.");
	if (ui.indexedClubs != gameContext.clubs) {
		if (!autocomplete_rebuild(&ui.clubs, false))
			ui_setCurrentStatus("Club autocomplete could not be rebuilt.");
		ui.indexedClubs = gameContext.clubs;
	}
	if (ui.indexedNations != gameContext.nations) {
		if (!autocomplete_rebuild(&ui.nations, true))
			ui_setCurrentStatus("Nation autocomplete could not be rebuilt.");
		ui.indexedNations = gameContext.nations;
		updateFlags();
	}
	if (autocomplete_poll(&ui.clubs) && ui.clubs.error[0])
		ui_setCurrentStatus(ui.clubs.error);
	if (autocomplete_poll(&ui.nations) && ui.nations.error[0])
		ui_setCurrentStatus(ui.nations.error);
	const bool optionsChanged = ui.optionsRevision != gameContext.optionsRevision;
	if (optionsChanged) {
		theme();
		char error[128];
		if (gameContext.searchActive && !searchHandler_doSearch(false, error))
			ui_setCurrentStatus(error);
	}
	if (ui.revision != gameContext.revision || optionsChanged) {
		PlayerLookup lookup = {0};
		if (vector_length(ui.views) && gameContext.playerCount && !playerLookup_build(&lookup))
			ui_setCurrentStatus("Cannot refresh player snapshots.");
		for (size_t i = 0; i < vector_length(ui.views); ++i) {
			View *view = ui.views[i];
			playerSnapshot_refresh(view->snapshot, &lookup);
			if (optionsChanged) {
				for (uint32_t p = 0; p < view->snapshot->count; ++p)
					getSortedPositionRatings(&view->snapshot->players[p]);
			}
			view->dirty = true;
		}
		playerLookup_destroy(&lookup);
		ui.revision = gameContext.revision;
		ui.optionsRevision = gameContext.optionsRevision;
		if (ui.selectedRow >= (int)playerTable_count())
			ui.selectedRow = -1;
		for (uint64_t i = 0; i < gameContext.playerCount; ++i) {
			ui_prepareText(gameContext.players[i].forename);
			ui_prepareText(gameContext.players[i].surname);
			ui_prepareText(gameContext.players[i].commonName);
		}
		for (uint64_t i = 0; i < gameContext.clubCount; ++i) {
			ui_prepareText(gameContext.clubs[i].name);
			ui_prepareText(gameContext.clubs[i].shortName);
		}
		for (uint64_t i = 0; i < gameContext.nationCount; ++i)
			ui_prepareText(gameContext.nations[i].name);
		for (size_t i = 0; i < vector_length(gameContext.options.formations); ++i)
			ui_prepareText(gameContext.options.formations[i].name);
	}
	updateFallback();
	const bool command = IsKeyDown(KEY_LEFT_CONTROL) || IsKeyDown(KEY_RIGHT_CONTROL) ||
											 IsKeyDown(KEY_LEFT_SUPER) || IsKeyDown(KEY_RIGHT_SUPER);
	if (command && IsKeyPressed(KEY_W)) {
		for (size_t i = 0; i < vector_length(ui.views); ++i)
			if (ui.views[i]->id == ui.active) {
				closeView(i);
				break;
			}
	}
	if (command && IsKeyPressed(KEY_Q))
		ui.quit = true;
}

static void toolbar(float width) {
	DrawRectangleRec((Rectangle){0, 0, width, 40}, ui.colour.topbar);
	ui_text((Rectangle){12, 4, 40, 30}, "Show:", 12, ui.colour.text, true);
	if (ui_button(1, (Rectangle){52, 6, 110, 28}, "Current player", false, "Show the player currently selected in FM24")) {
#ifdef MOCKS_MODE
		if (gameContext.playerCount)
			ui_openPlayer(&gameContext.players[0]);
		else
			ui_setCurrentStatus("The mock cache is not ready.");
#else
		if (processContext.handle) {
			Player player = getPlayerById(&processContext, getCurrentPersonUniqueId(&processContext));
			if (player.uid)
				ui_openPlayer(&player);
			else
				ui_setCurrentStatus("Could not read the current player. See the log.");
		} else
			ui_setCurrentStatus("Connect to a supported FM24 process first.");
#endif
	}
	if (ui_button(2, (Rectangle){168, 6, 72, 28}, "Best XI", false, "Create a Best XI from a snapshot of the current results"))
		openAnalysis(VIEW_BEST_XI);
	if (ui_button(3, (Rectangle){246, 6, 100, 28}, "Squad depth", false, "Create squad depth from the current results"))
		openAnalysis(VIEW_DEPTH);
	DayMonthYearTime date = date_prettify(gameContext.currentDate);
	char status[128];
	if (gameContext.currentDate.year)
		snprintf(status, sizeof(status), "%u %s %u  %s  %s", date.day, date.month, date.year, date.timeString, gameContext.gameVersion);
	else
		snprintf(status, sizeof(status), "Not connected");
	ui_text((Rectangle){fmaxf(360, width - 380), 4, fminf(370, width - 360), 32}, status, 12, ui.colour.muted, false);
}

static void tabs(float width) {
	Rectangle area = {0, 40, width, 36};
	DrawRectangleRec(area, ui.colour.sidebar);
	Rectangle previous = ui_clip(area);
	float x = 6 - ui.tabScroll;
	if (ui_button(4, (Rectangle){x, 44, 112, 26}, "Player search", ui.active == 0, NULL)) {
		ui.active = 0;
		ui.popup = 0;
	}
	x += 120;
	for (size_t i = 0; i < vector_length(ui.views); ++i) {
		View *view = ui.views[i];
		if (ui_button(ui_id(view, 1), (Rectangle){x, 44, 142, 26}, view->title, ui.active == view->id, view->title)) {
			ui.active = view->id;
			ui.popup = 0;
		}
		if (ui_button(ui_id(view, 2), (Rectangle){x + 144, 44, 22, 26}, "x", false, "Close view (Ctrl/Cmd+W)")) {
			closeView(i);
			--i;
		}
		x += 174;
	}
	ui_unclip(previous);
	ui_scroll(5, area, x + ui.tabScroll, &ui.tabScroll, true);
}

static void views(Rectangle area) {
	View *active = NULL;
	size_t panels = 1;
	for (size_t i = 0; i < vector_length(ui.views); ++i) {
		if (ui.views[i]->id == ui.active)
			active = ui.views[i];
		if (ui.views[i]->pinned && ui.views[i]->id != ui.active)
			++panels;
	}
	const float minMain = active ? 390 : 650;
	const float total = fmaxf(area.width, minMain + 390 * (float)(panels - 1));
	const float otherWidth = panels > 1 ? fmaxf(390, (total - minMain) / (float)panels) : 0;
	const float mainWidth = total - otherWidth * (float)(panels - 1);
	static float horizontal;
	ui_scroll(6, area, total, &horizontal, true);
	Rectangle previous = ui_clip((Rectangle){area.x, area.y, area.width, area.height - (total > area.width ? 10 : 0)});
	Rectangle panel = {area.x - horizontal, area.y, mainWidth, ui.clip.height};
	if (!active)
		ui_search(panel);
	else if (active->type == VIEW_PLAYER)
		ui_playerInfo(active, panel);
	else if (active->type == VIEW_BEST_XI)
		ui_bestEleven(active, panel);
	else
		ui_squadDepth(active, panel);
	panel.x += mainWidth;
	panel.width = otherWidth;
	for (size_t i = 0; i < vector_length(ui.views); ++i) {
		View *view = ui.views[i];
		if (!view->pinned || view == active)
			continue;
		if (CheckCollisionRecs(panel, ui.clip))
			ui_playerInfo(view, panel);
		DrawLineEx((Vector2){panel.x, panel.y}, (Vector2){panel.x, panel.y + panel.height}, 1, ui.colour.border);
		panel.x += otherWidth;
	}
	ui_unclip(previous);
}

void ui_draw(void) {
	BeginDrawing();
	ClearBackground(ui.colour.background);
	ui_beginControls();
	float width = (float)GetScreenWidth(), height = (float)GetScreenHeight();
	toolbar(width);
	tabs(width);
	views((Rectangle){0, 76, width, height - 104});
	EndScissorMode();
	ui.clip = (Rectangle){0, 0, width, height};
	DrawRectangleRec((Rectangle){0, height - 28, width, 28}, ui.colour.sidebar);
	const char *error = options_error();
	ui_text((Rectangle){10, height - 28, width - 180, 28}, error[0] ? error : ui.status, 12, ui.colour.muted, false);
	ui_text((Rectangle){width - 170, height - 28, 160, 28}, APP_NAME " " APP_VERSION, 12, ui.colour.muted, false);
	ui_searchPopup();
	if (ui.tooltip[0] && GetTime() - ui.hoverSince >= .6 && !ui.popup) {
		const float w = fminf(width - 16, ui_measureText(ui.tooltip, 12, false) + 20);
		Rectangle box = {fminf(ui.mouse.x + 12, width - w - 8), fminf(ui.mouse.y + 18, height - 36), w, 28};
		DrawRectangleRec(box, ui.colour.card);
		DrawRectangleLinesEx(box, 1, ui.colour.border);
		ui_text((Rectangle){box.x + 10, box.y, box.width - 20, box.height}, ui.tooltip, 12, ui.colour.text, false);
	}
	EndScissorMode();
	EndDrawing();
}

bool ui_shouldClose(void) {
	return ui.quit || WindowShouldClose();
}

#ifdef MOCKS_MODE
int ui_smokeTest(const char *directory) {
	static unsigned stage;
	static double next;
	if (!gameContext.playerCount || cache_isRunning() || GetTime() < next)
		return 0;
	static const char *const names[] = {"search", "player", "comparison", "best-xi", "depth", "depth-dark", "search-dark", "player-small"};
	if (stage && directory) {
		char path[4096];
		snprintf(path, sizeof(path), "%s/%s.png", directory, names[stage - 1]);
		Image image = LoadImageFromScreen();
		const bool saved = ExportImage(image, path);
		UnloadImage(image);
		if (!saved)
			return -1;
	}
	switch (stage++) {
		case 0: {
			char error[128];
			if (!searchHandler_doSearch(true, error) || playerTable_count() != gameContext.playerCount)
				return -1;
			break;
		}
		case 1: ui_openPlayer(&gameContext.players[1]); break;
		case 2:
			ui.views[0]->pinned = true;
			ui_openPlayer(&gameContext.players[3]);
			break;
		case 3:
			ui.views[0]->pinned = false;
			openAnalysis(VIEW_BEST_XI);
			break;
		case 4: openAnalysis(VIEW_DEPTH); break;
		case 5:
			gameContext.options.darkMode = true;
			++gameContext.optionsRevision;
			break;
		case 6: ui.active = 0; break;
		case 7:
			SetWindowSize(800, 560);
			ui.active = ui.views[0]->id;
			break;
		default: return 1;
	}
	next = GetTime() + .15;
	return 0;
}
#endif

void ui_shutdown(void) {
	autocomplete_destroy(&ui.clubs);
	autocomplete_destroy(&ui.nations);
	while (vector_length(ui.views))
		closeView(vector_length(ui.views) - 1);
	vector_free(ui.views);
	vector_free(ui.focusOrder);
	vector_free(ui.extraCodepoints);
	for (unsigned i = 0; i < 255; ++i)
		if (ui.flags[i].texture.id)
			UnloadTexture(ui.flags[i].texture);
	if (ui.pitch.id)
		UnloadTexture(ui.pitch);
	icons_shutdown();
	if (ui.regular.texture.id)
		UnloadFont(ui.regular);
	if (ui.bold.texture.id)
		UnloadFont(ui.bold);
	if (ui.fallback.texture.id)
		UnloadFont(ui.fallback);
	if (IsWindowReady())
		CloseWindow();
}
