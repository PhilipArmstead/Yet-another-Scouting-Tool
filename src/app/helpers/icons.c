// SPDX-FileCopyrightText: © 2026 Phil Armstead <philarmstead@mailbox.org>
// SPDX-License-Identifier: GPL-3.0-or-later

#include "icons.h"
#include "formatter.h"


#define HEART_PATH_D "M7 3c-1.535 0-3.078.5-4.25 1.7-2.343 2.4-2.279 6.1 0 8.5L12 23l9.25-9.8c2.279-2.4 2.343-6.1 0-8.5-2.343-2.3-6.157-2.3-8.5 0l-.75.8-.75-.8C10.078 3.5 8.536 3 7 3"
// The co-ordinates the path above is authored against
#define HEART_PATH_EXTENT 24.0

#define SHOE_PATH_D "M47.016 37.062c-2.01 0-3.972.188-5.906.452-.718 3.144-1.971 7.934.263 11.32 2.406 3.647 8.395 3.647 11.106.445 1.832-2.165 1.902-7.582 1.766-11.565-2.354-.396-4.761-.652-7.229-.652M45.46.002c-6.205-.22-9.052 14.307-7.592 18.321.717 1.969 2.455 5.842 3.283 9.563.687 3.076.545 6.098.319 7.77 1.82-.226 3.661-.381 5.545-.381 2.44 0 4.815.249 7.151.628-.048-1.009-.094-1.832-.094-2.324 0-3.212 1.588-8.719 2.192-11.388C57.942 14.748 51.663.221 45.46.002"
// Set when the caller wants the shoe mirrored to read as the left foot
#define SHOE_MIRRORED_BIT 0x100u

static GskPath *heartPath;
static GskPath *shoePath;
static graphene_rect_t shoeBounds;

/**
 * Parses the icon outlines once up front so neither the draw callbacks nor the render paths that
 * build rows have to branch on them.
 */
void icons_init(void) {
	if (heartPath == NULL) {
		heartPath = gsk_path_parse(HEART_PATH_D);
	}

	if (shoePath == NULL) {
		shoePath = gsk_path_parse(SHOE_PATH_D);
		// The authored SVG's viewBox crops the outline, so fit to the geometry itself instead
		(void)gsk_path_get_bounds(shoePath, &shoeBounds);
	}
}

/**
 * Maps a raw game value onto the 0-120 index that formatter_qualityColour expects. Done ahead of
 * the draw callback so redraws stay a table lookup.
 */
uint8_t icons_heartQuantise(const float value, const float max) {
	const float clamped = value < 0.f ? 0.f : (value < max ? value : max);
	return (uint8_t)(clamped / max * 120.f);
}

static void drawHeart(GtkDrawingArea *area, cairo_t *cr, const int width, const int height, gpointer data) {
	(void)area;

	const double scale = (width < height ? width : height) / HEART_PATH_EXTENT;
	cairo_scale(cr, scale, scale);

	const RGB rgb = formatter_qualityColour((uint8_t)GPOINTER_TO_UINT(data));
	const GdkRGBA colour = {.red = rgb.r, .green = rgb.g, .blue = rgb.b, .alpha = 1.f};
	gdk_cairo_set_source_rgba(cr, &colour);

	gsk_path_to_cairo(heartPath, cr);
	cairo_fill(cr);
}

/**
 * Points an existing drawing area at the heart, replacing whatever it was drawing before so this is
 * safe to call again when a window is refreshed.
 * @param quality - 0-120, as returned by icons_heartQuantise
 */
void icons_heartAttach(GtkDrawingArea *area, const uint8_t quality) {
	gtk_drawing_area_set_draw_func(area, drawHeart, GUINT_TO_POINTER(quality), NULL);
}

GtkWidget *icons_heartNew(const uint8_t quality) {
	GtkWidget *area = gtk_drawing_area_new();
	gtk_widget_set_size_request(area, HEART_SIZE, HEART_SIZE);
	icons_heartAttach(GTK_DRAWING_AREA(area), quality);

	return area;
}

static void drawShoe(GtkDrawingArea *area, cairo_t *cr, const int width, const int height, gpointer data) {
	(void)area;

	const guint packed = GPOINTER_TO_UINT(data);
	const float scaleX = (float)width / shoeBounds.size.width;
	const float scaleY = (float)height / shoeBounds.size.height;
	const float scale = scaleX < scaleY ? scaleX : scaleY;
	const float drawnWidth = shoeBounds.size.width * scale;

	cairo_translate(cr, ((float)width - drawnWidth) * 0.5f, ((float)height - shoeBounds.size.height * scale) * 0.5f);
	if (packed & SHOE_MIRRORED_BIT) {
		cairo_translate(cr, drawnWidth, 0);
		cairo_scale(cr, -scale, scale);
	} else {
		cairo_scale(cr, scale, scale);
	}
	cairo_translate(cr, -shoeBounds.origin.x, -shoeBounds.origin.y);

	const RGB rgb = formatter_qualityColour((uint8_t)(packed & 0xFFu));
	const GdkRGBA colour = {.red = rgb.r, .green = rgb.g, .blue = rgb.b, .alpha = 1.f};
	gdk_cairo_set_source_rgba(cr, &colour);

	gsk_path_to_cairo(shoePath, cr);
	cairo_fill(cr);
}

/**
 * @param quality - 0-120
 * @param mirrored - true to flip the outline horizontally, forming the left shoe
 */
GtkWidget *icons_shoeNew(const uint8_t quality, const bool mirrored) {
	GtkWidget *area = gtk_drawing_area_new();
	gtk_widget_set_size_request(area, SHOE_WIDTH, SHOE_HEIGHT);
	gtk_drawing_area_set_draw_func(
		GTK_DRAWING_AREA(area),
		drawShoe,
		GUINT_TO_POINTER(quality | (mirrored ? SHOE_MIRRORED_BIT : 0u)),
		NULL
	);

	return area;
}
