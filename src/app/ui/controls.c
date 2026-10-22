// SPDX-License-Identifier: GPL-3.0-or-later
#define RAYGUI_IMPLEMENTATION
#define RAYGUI_VALUEBOX_MAX_CHARS 4
#include "app/helpers/formatter.h"
#include "app/helpers/text.h"
#include "core/logger.h"
#include "internal.h"
#include <math.h>

static bool command(void) {
	return IsKeyDown(KEY_LEFT_CONTROL) || IsKeyDown(KEY_RIGHT_CONTROL) ||
				 IsKeyDown(KEY_LEFT_SUPER) || IsKeyDown(KEY_RIGHT_SUPER);
}

static bool shift(void) {
	return IsKeyDown(KEY_LEFT_SHIFT) || IsKeyDown(KEY_RIGHT_SHIFT);
}
static bool pressed(int key) {
	return IsKeyPressed(key) || IsKeyPressedRepeat(key);
}

unsigned ui_id(const View *view, unsigned local) {
	return view ? view->id * 1024u + local : local;
}

void ui_beginControls(void) {
	ui.mouse = GetMousePosition();
	ui.mouseConsumed = ui.popup && CheckCollisionPointRec(ui.mouse, ui.popupBounds);
	ui.keyConsumed = false;
	ui.tooltip[0] = '\0';
	ui.previousHoverId = ui.hoverId;
	ui.hoverId = 0;
	if (IsKeyPressed(KEY_TAB) && vector_length(ui.focusOrder)) {
		const size_t count = vector_length(ui.focusOrder);
		size_t index = 0;
		while (index < count && ui.focusOrder[index] != ui.focus)
			++index;
		index = index == count ? (shift() ? count - 1 : 0) : shift() ? (index + count - 1) % count
																																 : (index + 1) % count;
		ui.focus = ui.focusOrder[index];
		ui.editor = 0;
		ui.keyConsumed = true;
		ui.popup = 0;
	}
	if (ui.focusOrder)
		((Header *)ui.focusOrder - 1)->count = 0;
	if (IsKeyPressed(KEY_ESCAPE)) {
		ui.popup = 0;
		ui.editor = 0;
		ui.focus = 0;
		ui.keyConsumed = true;
	}
	if (ui.popup && GetMouseWheelMove() != 0 && !CheckCollisionPointRec(ui.mouse, ui.popupBounds))
		ui.popup = 0;
	if (!IsMouseButtonDown(MOUSE_BUTTON_LEFT))
		ui.drag = 0;
	ui.clip = (Rectangle){0, 0, (float)GetScreenWidth(), (float)GetScreenHeight()};
}

bool ui_hit(Rectangle bounds) {
	return !ui.mouseConsumed && CheckCollisionPointRec(ui.mouse, bounds) &&
				 CheckCollisionPointRec(ui.mouse, ui.clip);
}

bool ui_focusable(unsigned id, Rectangle bounds) {
	if (!CheckCollisionRecs(bounds, ui.clip))
		return false;
	vector_push(ui.focusOrder, id);
	if (ui_hit(bounds) && IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
		ui.focus = id;
		if (ui.editor != id)
			ui.editor = 0;
		return true;
	}
	return false;
}

static void focusRing(unsigned id, Rectangle bounds) {
	if (ui.focus == id)
		DrawRectangleLinesEx((Rectangle){bounds.x - 2, bounds.y - 2, bounds.width + 4, bounds.height + 4}, 1, ui.colour.accent);
}

void ui_tip(unsigned id, Rectangle bounds, const char *text) {
	if (!text || !text[0] || !ui_hit(bounds))
		return;
	ui.hoverId = id;
	if (ui.previousHoverId != id)
		ui.hoverSince = GetTime();
	snprintf(ui.tooltip, sizeof(ui.tooltip), "%s", text);
}

bool ui_button(unsigned id, Rectangle bounds, const char *text, bool primary, const char *tooltip) {
	ui_focusable(id, bounds);
	const int oldBase = GuiGetStyle(BUTTON, BASE_COLOR_NORMAL);
	const int oldText = GuiGetStyle(BUTTON, TEXT_COLOR_NORMAL);
	if (primary) {
		GuiSetStyle(BUTTON, BASE_COLOR_NORMAL, (int)ColorToInt(ui.colour.accent));
		GuiSetStyle(BUTTON, TEXT_COLOR_NORMAL, (int)ColorToInt(WHITE));
	}
	const bool locked = ui.mouseConsumed || !CheckCollisionPointRec(ui.mouse, ui.clip);
	if (locked)
		GuiLock();
	const bool clicked = GuiButton(bounds, NULL) != 0 && !locked;
	if (locked)
		GuiUnlock();
	GuiSetStyle(BUTTON, BASE_COLOR_NORMAL, oldBase);
	GuiSetStyle(BUTTON, TEXT_COLOR_NORMAL, oldText);
	const float textWidth = ui_measureText(text, 12, false);
	ui_text((Rectangle){bounds.x + fmaxf(4, (bounds.width - textWidth) * .5f), bounds.y, bounds.width - 8, bounds.height},
					text, 12, primary ? WHITE : ui.colour.text, false);
	focusRing(id, bounds);
	ui_tip(id, bounds, tooltip);
	const bool keyboard = !ui.keyConsumed && ui.focus == id && ui.editor != id &&
												(IsKeyPressed(KEY_ENTER) || IsKeyPressed(KEY_SPACE));
	if (keyboard)
		ui.keyConsumed = true;
	if (clicked)
		ui.mouseConsumed = true;
	return clicked || keyboard;
}

bool ui_check(unsigned id, Rectangle bounds, const char *text, bool *checked) {
	ui_focusable(id, bounds);
	const bool before = *checked;
	const bool locked = ui.mouseConsumed || !CheckCollisionPointRec(ui.mouse, ui.clip);
	if (locked)
		GuiLock();
	GuiCheckBox((Rectangle){bounds.x, bounds.y + (bounds.height - 16) * .5f, 16, 16}, text, checked);
	if (locked)
		GuiUnlock();
	if (!ui.keyConsumed && ui.focus == id && (IsKeyPressed(KEY_SPACE) || IsKeyPressed(KEY_ENTER))) {
		*checked = !*checked;
		ui.keyConsumed = true;
	}
	focusRing(id, bounds);
	if (before != *checked)
		ui.mouseConsumed = true;
	return before != *checked;
}

bool ui_number(unsigned id, Rectangle bounds, const char *label, int *value, int min, int max) {
	ui_text((Rectangle){bounds.x, bounds.y, 34, bounds.height}, label, 12, ui.colour.muted, false);
	Rectangle minus = {bounds.x + 36, bounds.y, 22, bounds.height};
	Rectangle box = {minus.x + 24, bounds.y, bounds.width - 86, bounds.height};
	Rectangle plus = {bounds.x + bounds.width - 22, bounds.y, 22, bounds.height};
	int before = *value;
	if (ui_button(id, minus, "-", false, NULL) && *value > min)
		--*value;
	ui_focusable(id + 2, box);
	if (ui_button(id + 1, plus, "+", false, NULL) && *value < max)
		++*value;
	const bool edit = ui.focus == id + 2 && CheckCollisionRecs(box, ui.clip);
	const bool locked = ui.mouseConsumed || (!edit && !CheckCollisionPointRec(ui.mouse, ui.clip));
	if (locked)
		GuiLock();
	const bool finished = GuiValueBox(box, NULL, value, min, max, edit) != 0;
	if (locked)
		GuiUnlock();
	if ((!edit || finished) && *value < min)
		*value = min;
	if ((!edit || finished) && *value > max)
		*value = max;
	focusRing(id + 2, box);
	return before != *value;
}

Rectangle ui_clip(Rectangle bounds) {
	Rectangle previous = ui.clip;
	ui.clip = GetCollisionRec(bounds, previous);
	BeginScissorMode((int)ui.clip.x, (int)ui.clip.y, (int)ui.clip.width, (int)ui.clip.height);
	return previous;
}

void ui_unclip(Rectangle previous) {
	ui.clip = previous;
	BeginScissorMode((int)previous.x, (int)previous.y, (int)previous.width, (int)previous.height);
}

static bool extra(int cp) {
	return cp >= 0 && cp < 0x110000 && (ui.extraSeen[cp / 8] & (1u << (cp % 8))) != 0;
}

void ui_prepareText(const char *text) {
	for (const unsigned char *at = (const unsigned char *)text; at && *at;) {
		if (*at < 128) {
			++at;
			continue;
		}
		int bytes = 0, cp = GetCodepointNext((const char *)at, &bytes);
		if (bytes < 1)
			break;
		at += bytes;
		if (cp < 0 || cp >= 0x110000 || extra(cp))
			continue;
		if (ui.regular.glyphs[GetGlyphIndex(ui.regular, cp)].value == cp)
			continue;
		ui.extraSeen[cp / 8] |= (unsigned char)(1u << (cp % 8));
		vector_push(ui.extraCodepoints, cp);
		ui.fontsDirty = true;
	}
}

static bool hasExtra(const char *text) {
	for (const unsigned char *at = (const unsigned char *)text; *at;) {
		if (*at < 128) {
			++at;
			continue;
		}
		int bytes = 0, cp = GetCodepointNext((const char *)at, &bytes);
		if (extra(cp))
			return true;
		if (bytes < 1)
			break;
		at += bytes;
	}
	return false;
}

static float advance(Font font, int cp, float size) {
	const int index = GetGlyphIndex(font, cp);
	return (font.glyphs[index].advanceX ? (float)font.glyphs[index].advanceX : font.recs[index].width) * size / (float)font.baseSize;
}

float ui_measureText(const char *text, float size, bool bold) {
	if (!text)
		return 0;
	Font font = bold ? ui.bold : ui.regular;
	if (!hasExtra(text))
		return MeasureTextEx(font, text, size, 0).x;
	float width = 0;
	for (const char *at = text; *at;) {
		int bytes = 0, cp = GetCodepointNext(at, &bytes);
		if (bytes < 1)
			break;
		Font glyphFont = extra(cp) && ui.fallback.texture.id ? ui.fallback : font;
		width += advance(glyphFont, cp, size);
		at += bytes;
	}
	return width;
}

void ui_text(Rectangle bounds, const char *text, float size, Color colour, bool bold) {
	if (!text || !text[0] || bounds.width <= 0 || !CheckCollisionRecs(bounds, ui.clip))
		return;
	const bool clipped = ui_measureText(text, size, bold) > bounds.width;
	Rectangle previous = ui.clip;
	if (clipped)
		ui_clip(bounds);
	Vector2 position = {bounds.x, bounds.y + (bounds.height - size) * .5f};
	Font font = bold ? ui.bold : ui.regular;
	if (!hasExtra(text))
		DrawTextEx(font, text, position, size, 0, colour);
	else
		for (const char *at = text; *at;) {
			int bytes = 0, cp = GetCodepointNext(at, &bytes);
			if (bytes < 1)
				break;
			Font glyphFont = extra(cp) && ui.fallback.texture.id ? ui.fallback : font;
			DrawTextCodepoint(glyphFont, cp, position, size, colour);
			position.x += advance(glyphFont, cp, size);
			at += bytes;
		}
	if (clipped)
		ui_unclip(previous);
}

static float textWidth(const char *text, size_t length) {
	char prefix[512];
	if (length >= sizeof(prefix))
		length = sizeof(prefix) - 1;
	memcpy(prefix, text, length);
	prefix[length] = '\0';
	return ui_measureText(prefix, 12, false);
}

static void deleteSelection(char *text) {
	const size_t first = ui.caret < ui.anchor ? ui.caret : ui.anchor;
	const size_t last = ui.caret > ui.anchor ? ui.caret : ui.anchor;
	memmove(text + first, text + last, strlen(text + last) + 1);
	ui.caret = ui.anchor = first;
}

static bool insert(char *text, size_t capacity, const char *bytes, size_t count) {
	size_t selected = ui.caret > ui.anchor ? ui.caret - ui.anchor : ui.anchor - ui.caret;
	if (strlen(text) - selected + count >= capacity)
		return false;
	deleteSelection(text);
	memmove(text + ui.caret + count, text + ui.caret, strlen(text + ui.caret) + 1);
	memcpy(text + ui.caret, bytes, count);
	ui.caret += count;
	ui.anchor = ui.caret;
	return true;
}

bool ui_edit(unsigned id, Rectangle bounds, char *text, size_t capacity, const char *placeholder, bool readOnly) {
	const bool clicked = ui_focusable(id, bounds);
	bool changed = false;
	const bool focused = ui.focus == id;
	if (focused && ui.editor != id) {
		ui.editor = id;
		ui.caret = ui.anchor = strlen(text);
		ui.editScroll = 0;
	}
	if (focused) {
		const size_t length = strlen(text);
		if (ui.caret > length)
			ui.caret = length;
		if (ui.anchor > length)
			ui.anchor = length;
		if (clicked) {
			size_t at = 0;
			float target = ui.mouse.x - bounds.x - 7 + ui.editScroll;
			while (at < length) {
				const size_t next = text_next(text, at);
				if ((textWidth(text, at) + textWidth(text, next)) * .5f >= target)
					break;
				at = next;
			}
			ui.caret = at;
			if (!shift())
				ui.anchor = at;
			if (ui.lastClickId == id && GetTime() - ui.lastClick < .35) {
				ui.anchor = 0;
				ui.caret = length;
			}
			ui.lastClickId = id;
			ui.lastClick = GetTime();
			ui.drag = id;
		} else if (ui.drag == id && IsMouseButtonDown(MOUSE_BUTTON_LEFT)) {
			size_t at = 0;
			while (at < length && textWidth(text, text_next(text, at)) < ui.mouse.x - bounds.x - 7 + ui.editScroll)
				at = text_next(text, at);
			ui.caret = at;
		}
		if (command() && IsKeyPressed(KEY_A)) {
			ui.anchor = 0;
			ui.caret = length;
		}
		if (command() && (IsKeyPressed(KEY_C) || (!readOnly && IsKeyPressed(KEY_X)))) {
			size_t first = ui.caret < ui.anchor ? ui.caret : ui.anchor;
			size_t last = ui.caret > ui.anchor ? ui.caret : ui.anchor;
			if (last > first) {
				char *copy = malloc(last - first + 1);
				if (copy) {
					memcpy(copy, text + first, last - first);
					copy[last - first] = '\0';
					SetClipboardText(copy);
					free(copy);
					if (!readOnly && IsKeyPressed(KEY_X)) {
						deleteSelection(text);
						changed = true;
					}
				} else {
					LOG_ERROR("Could not copy selected text");
					ui_setCurrentStatus("Could not copy selected text. See the log.");
				}
			}
		}
		if (!readOnly && command() && IsKeyPressed(KEY_V)) {
			const char *paste = GetClipboardText();
			if (paste) {
				for (size_t at = 0; paste[at];) {
					int bytes = 0;
					int cp = GetCodepointNext(paste + at, &bytes);
					if (bytes < 1)
						break;
					if (cp >= 32 && cp != 127) {
						int encodedBytes;
						const char *encoded = CodepointToUTF8(cp, &encodedBytes);
						if (!insert(text, capacity, encoded, (size_t)encodedBytes))
							break;
						changed = true;
					}
					at += (size_t)bytes;
				}
			}
		}
		if (pressed(KEY_LEFT))
			ui.caret = command() ? 0 : !shift() && ui.caret != ui.anchor ? (ui.caret < ui.anchor ? ui.caret : ui.anchor)
																																	 : text_previous(text, ui.caret);
		if (pressed(KEY_RIGHT))
			ui.caret = command() ? strlen(text) : !shift() && ui.caret != ui.anchor ? (ui.caret > ui.anchor ? ui.caret : ui.anchor)
																																							: text_next(text, ui.caret);
		if (IsKeyPressed(KEY_HOME))
			ui.caret = 0;
		if (IsKeyPressed(KEY_END))
			ui.caret = strlen(text);
		if (!shift() && (pressed(KEY_LEFT) || pressed(KEY_RIGHT) || IsKeyPressed(KEY_HOME) || IsKeyPressed(KEY_END)))
			ui.anchor = ui.caret;
		if (!readOnly && pressed(KEY_BACKSPACE)) {
			if (ui.caret == ui.anchor)
				ui.anchor = text_previous(text, ui.caret);
			changed |= ui.caret != ui.anchor;
			deleteSelection(text);
		}
		if (!readOnly && pressed(KEY_DELETE)) {
			if (ui.caret == ui.anchor)
				ui.anchor = text_next(text, ui.caret);
			changed |= ui.caret != ui.anchor;
			deleteSelection(text);
		}
		if (!readOnly && !command()) {
			int cp;
			while ((cp = GetCharPressed()) != 0) {
				if (cp < 32 || cp == 127)
					continue;
				int bytes;
				const char *encoded = CodepointToUTF8(cp, &bytes);
				changed |= insert(text, capacity, encoded, (size_t)bytes);
			}
		}
		const float caretX = textWidth(text, ui.caret);
		if (caretX - ui.editScroll > bounds.width - 14)
			ui.editScroll = caretX - bounds.width + 14;
		if (caretX < ui.editScroll)
			ui.editScroll = caretX;
	}
	if (changed)
		ui_prepareText(text);
	DrawRectangleRounded(bounds, .15f, 4, readOnly ? ui.colour.card : ui.colour.input);
	DrawRectangleLinesEx(bounds, 1, focused ? ui.colour.accent : ui.colour.border);
	Rectangle previous = ui_clip((Rectangle){bounds.x + 5, bounds.y + 1, bounds.width - 10, bounds.height - 2});
	const float offset = focused ? ui.editScroll : 0;
	if (focused && ui.caret != ui.anchor) {
		float a = textWidth(text, ui.anchor), b = textWidth(text, ui.caret);
		DrawRectangleRec((Rectangle){bounds.x + 7 + fminf(a, b) - offset, bounds.y + 4, fabsf(b - a), bounds.height - 8}, Fade(ui.colour.accent, .4f));
	}
	ui_text((Rectangle){bounds.x + 7 - offset, bounds.y, bounds.width + offset - 14, bounds.height},
					text[0] ? text : placeholder, 12, text[0] ? ui.colour.text : ui.colour.muted, false);
	if (focused && fmod(GetTime(), 1) < .65) {
		float x = bounds.x + 7 + textWidth(text, ui.caret) - offset;
		DrawLineEx((Vector2){x, bounds.y + 5}, (Vector2){x, bounds.y + bounds.height - 5}, 1, ui.colour.text);
	}
	ui_unclip(previous);
	return changed;
}

void ui_scroll(unsigned id, Rectangle bounds, float content, float *offset, bool horizontal) {
	float extent = horizontal ? bounds.width : bounds.height;
	float max = fmaxf(0, content - extent);
	if (*offset > max)
		*offset = max;
	if (*offset < 0)
		*offset = 0;
	if (max <= 0)
		return;
	if (ui_hit(bounds) && !ui.popup) {
		Vector2 wheel = GetMouseWheelMoveV();
		*offset -= (horizontal ? wheel.x + (shift() ? wheel.y : 0) : (shift() ? 0 : wheel.y)) * 36;
	}
	Rectangle track = horizontal ? (Rectangle){bounds.x, bounds.y + bounds.height - 10, bounds.width, 10}
															 : (Rectangle){bounds.x + bounds.width - 10, bounds.y, 10, bounds.height};
	ui_focusable(id, track);
	if (ui.focus == id && !ui.keyConsumed) {
		float step = 0;
		if (pressed(horizontal ? KEY_LEFT : KEY_UP))
			step = -36;
		else if (pressed(horizontal ? KEY_RIGHT : KEY_DOWN))
			step = 36;
		else if (pressed(KEY_PAGE_UP))
			step = -extent;
		else if (pressed(KEY_PAGE_DOWN))
			step = extent;
		else if (pressed(KEY_HOME))
			step = -max;
		else if (pressed(KEY_END))
			step = max;
		if (step) {
			*offset += step;
			ui.keyConsumed = true;
		}
	}
	*offset = fminf(max, fmaxf(0, *offset));
	const float length = fmaxf(24, extent * extent / content);
	const float travel = extent - length;
	float start = (horizontal ? track.x : track.y);
	float mouse = horizontal ? ui.mouse.x : ui.mouse.y;
	Rectangle thumb = horizontal ? (Rectangle){start + *offset / max * travel, track.y + 2, length, 6}
															 : (Rectangle){track.x + 2, start + *offset / max * travel, 6, length};
	if (ui_hit(track) && IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
		ui.drag = id;
		ui.dragOffset = CheckCollisionPointRec(ui.mouse, thumb) ? mouse - (horizontal ? thumb.x : thumb.y) : length * .5f;
		ui.mouseConsumed = true;
	}
	if (ui.drag == id && travel > 0)
		*offset = (mouse - start - ui.dragOffset) / travel * max;
	*offset = fminf(max, fmaxf(0, *offset));
	if (horizontal)
		thumb.x = start + *offset / max * travel;
	else
		thumb.y = start + *offset / max * travel;
	DrawRectangleRec(track, ui.colour.sidebar);
	DrawRectangleRounded(thumb, 1, 4, ui.colour.muted);
	focusRing(id, track);
}

Color ui_quality(float rating) {
	int index = (int)fminf(120, fmaxf(0, rating));
	RGB rgb = formatter_qualityColour((uint8_t)index);
	return (Color){(unsigned char)(rgb.r * 255), (unsigned char)(rgb.g * 255), (unsigned char)(rgb.b * 255), 255};
}
