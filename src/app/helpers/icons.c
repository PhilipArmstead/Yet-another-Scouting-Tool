// SPDX-FileCopyrightText: © 2026 Phil Armstead <philarmstead@mailbox.org>
// SPDX-License-Identifier: GPL-3.0-or-later

#include "icons.h"
#include "formatter.h"


#define HEART_PATH_D "M7 3c-1.535 0-3.078.5-4.25 1.7-2.343 2.4-2.279 6.1 0 8.5L12 23l9.25-9.8c2.279-2.4 2.343-6.1 0-8.5-2.343-2.3-6.157-2.3-8.5 0l-.75.8-.75-.8C10.078 3.5 8.536 3 7 3"
// The co-ordinates the path above is authored against
#define HEART_PATH_EXTENT 24.0

static GskPath *heartPath;

/**
 * Parses the heart outline once up front so neither the draw callback nor the render paths that
 * build rows have to branch on it.
 */
void icons_init(void) {
	if (heartPath == NULL) {
		heartPath = gsk_path_parse(HEART_PATH_D);
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
