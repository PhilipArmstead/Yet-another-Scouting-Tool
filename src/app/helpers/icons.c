// SPDX-FileCopyrightText: © 2026 Phil Armstead <philarmstead@mailbox.org>
// SPDX-License-Identifier: GPL-3.0-or-later

#include "icons.h"
#include "formatter.h"

#include <stdbool.h>


#define HEART_SIZE 16
#define FACE_SIZE 32
#define SHOE_WIDTH 18
#define SHOE_HEIGHT 32

typedef enum {
	GLYPH_HEART,
	GLYPH_FACE,
	GLYPH_SHOE,
	GLYPH_COUNT
} Glyph;

static const char *const glyphPaths[GLYPH_COUNT] = {
	[GLYPH_HEART] =
	"M7 3c-1.535 0-3.078.5-4.25 1.7-2.343 2.4-2.279 6.1 0 8.5L12 23l9.25-9.8c2.279-2.4 2.343-6.1 0-8.5-2.343-2.3-6.157-2.3-8.5 0l-.75.8-.75-.8C10.078 3.5 8.536 3 7 3",
	[GLYPH_FACE] =
	"M28 51.906c13.055 0 23.906-10.828 23.906-23.906 0-13.055-10.875-23.906-23.93-23.906C14.899 4.094 4.095 14.945 4.095 28c0 13.078 10.828 23.906 23.906 23.906m-7.336-26.133c-1.406 0-2.578-1.242-2.578-3.023 0-1.758 1.172-3 2.578-3 1.43 0 2.625 1.242 2.625 3 0 1.781-1.195 3.023-2.625 3.023m14.11 0c-1.43 0-2.602-1.242-2.602-3.023 0-1.758 1.172-3 2.601-3 1.407 0 2.602 1.242 2.602 3 0 1.781-1.195 3.023-2.602 3.023m-7.102 13.805c-5.86 0-9.492-4.148-9.492-5.695 0-.328.21-.446.468-.258 1.688 1.36 4.758 2.906 9.024 2.906s7.242-1.476 9-2.93c.258-.187.492-.046.492.282 0 1.547-3.656 5.695-9.492 5.695",
	[GLYPH_SHOE] =
	"M47.016 37.062c-2.01 0-3.972.188-5.906.452-.718 3.144-1.971 7.934.263 11.32 2.406 3.647 8.395 3.647 11.106.445 1.832-2.165 1.902-7.582 1.766-11.565-2.354-.396-4.761-.652-7.229-.652M45.46.002c-6.205-.22-9.052 14.307-7.592 18.321.717 1.969 2.455 5.842 3.283 9.563.687 3.076.545 6.098.319 7.77 1.82-.226 3.661-.381 5.545-.381 2.44 0 4.815.249 7.151.628-.048-1.009-.094-1.832-.094-2.324 0-3.212 1.588-8.719 2.192-11.388C57.942 14.748 51.663.221 45.46.002",
};

typedef struct {
	uint8_t glyph;
	// The shoe outline is authored as the right foot, so the left is drawn flipped
	bool mirrored;
	// The face's eyes and mouth are inner subpaths, so they have to punch through rather than fill
	cairo_fill_rule_t fillRule;
	int16_t width;
	int16_t height;
} Icon;

static const Icon icons[ICON_COUNT] = {
	[ICON_HEART] = {GLYPH_HEART, false, CAIRO_FILL_RULE_WINDING, HEART_SIZE, HEART_SIZE},
	[ICON_FACE] = {GLYPH_FACE, false, CAIRO_FILL_RULE_EVEN_ODD, FACE_SIZE, FACE_SIZE},
	[ICON_SHOE_LEFT] = {GLYPH_SHOE, true, CAIRO_FILL_RULE_WINDING, SHOE_WIDTH, SHOE_HEIGHT},
	[ICON_SHOE_RIGHT] = {GLYPH_SHOE, false, CAIRO_FILL_RULE_WINDING, SHOE_WIDTH, SHOE_HEIGHT},
};

static GskPath *glyphs[GLYPH_COUNT];
static graphene_rect_t glyphBounds[GLYPH_COUNT];

/**
 * Parses the icon outlines once up front so neither the draw callbacks nor the render paths that
 * build rows have to branch on them.
 */
void icons_init(void) {
	for (uint8_t i = 0; i < GLYPH_COUNT; ++i) {
		glyphs[i] = gsk_path_parse(glyphPaths[i]);
		// The authored SVG viewBoxes crop the outlines, so fit to the geometry itself instead
		(void)gsk_path_get_bounds(glyphs[i], &glyphBounds[i]);
	}
}

static void draw(GtkDrawingArea *area, cairo_t *cr, const int width, const int height, gpointer data) {
	(void)area;

	const guint packed = GPOINTER_TO_UINT(data);
	const Icon *icon = &icons[packed >> 8];
	const graphene_rect_t *bounds = &glyphBounds[icon->glyph];

	const float scaleX = (float)width / bounds->size.width;
	const float scaleY = (float)height / bounds->size.height;
	const float scale = scaleX < scaleY ? scaleX : scaleY;
	const float drawnWidth = bounds->size.width * scale;

	cairo_translate(cr, ((float)width - drawnWidth) * .5f, ((float)height - bounds->size.height * scale) * .5f);
	if (icon->mirrored) {
		cairo_translate(cr, drawnWidth, 0);
		cairo_scale(cr, -scale, scale);
	} else {
		cairo_scale(cr, scale, scale);
	}
	cairo_translate(cr, -bounds->origin.x, -bounds->origin.y);

	const RGB rgb = formatter_qualityColour((uint8_t)(packed & 0xFFu));
	cairo_set_source_rgb(cr, rgb.r, rgb.g, rgb.b);

	cairo_set_fill_rule(cr, icon->fillRule);
	gsk_path_to_cairo(glyphs[icon->glyph], cr);
	cairo_fill(cr);
}

/**
 * @param quality - 0-120, as returned by the icons_*Quantise helpers
 */
GtkWidget *icons_new(const IconId id, const uint8_t quality) {
	const Icon *icon = &icons[id];
	GtkWidget *area = gtk_drawing_area_new();
	gtk_widget_set_size_request(area, icon->width, icon->height);
	gtk_drawing_area_set_draw_func(GTK_DRAWING_AREA(area), draw, GUINT_TO_POINTER(quality | (guint)id << 8), NULL);

	return area;
}
