// SPDX-FileCopyrightText: © 2026 Phil Armstead <philarmstead@mailbox.org>
// SPDX-License-Identifier: GPL-3.0-or-later

#include "app/entities.h"
#include "app/player.h"
#include "app/ui.h"
#include "app/helpers/formatter.h"
#include "app/helpers/ratings.h"
#include "app/helpers/vector.h"
#include "types/position-code-names.h"

#include <stdlib.h>


/** A player eligible for one position grid, paired with their rating for that exact position. */
typedef struct {
	const Player *player;
	float rating;
} DepthRow;

/** Rating-descending. Ties break on uid so equal ratings keep a stable order between renders. */
static int compareDepthRows(const void *a, const void *b) {
	const DepthRow *rowA = a;
	const DepthRow *rowB = b;
	if (rowA->rating != rowB->rating) {
		return rowA->rating < rowB->rating ? 1 : -1;
	}

	return rowA->player->uid < rowB->player->uid ? -1 : rowA->player->uid > rowB->player->uid;
}

/** The positions the window offers, in display order. Drives the toggles and the depth grids. */
static const uint8_t positionFilters[] = {
	POSITION_CODE_GK,
	POSITION_CODE_DL,
	POSITION_CODE_DC,
	POSITION_CODE_DR,
	POSITION_CODE_WBL,
	POSITION_CODE_DM,
	POSITION_CODE_WBR,
	POSITION_CODE_ML,
	POSITION_CODE_MC,
	POSITION_CODE_MR,
	POSITION_CODE_AML,
	POSITION_CODE_AMC,
	POSITION_CODE_AMR,
	POSITION_CODE_ST,
};

static void onFilterChange(GObject *object, gpointer userData);
static void onPlayerRowClicked(
	const GtkGestureClick *gesture,
	int clickCount,
	double x,
	double y,
	const Player *player
);
static void renderSquadDepthGrids(WindowContext context);

void ui_createSquadDepthWindow(void) {
	// The window works from its own copy of the results: the indices point into a buffer the cache
	// is free to replace, and the players themselves are re-matched by uid after every publish.
	const SharedPointer *results = gameContext.searchResults;
	const uint32_t *playerIds = results != NULL ? results->data : NULL;
	PlayerSnapshot *snapshot = playerSnapshot_create(playerIds, playerIds != NULL ? vector_length(playerIds) : 0);
	if (snapshot == NULL) {
		return;
	}

	const WindowContext context = openWindow("squad-depth", "window:squad-depth", WINDOW_BEST_XI, snapshot);
	g_object_set_data_full(G_OBJECT(context.window), "squad-depth:players", snapshot, playerSnapshot_free);

	WindowContext *cbContext = g_new(WindowContext, 1);
	*cbContext = context; // copy the two pointers
	g_object_set_data_full(G_OBJECT(context.window), "squad-depth:context", cbContext, g_free);

	GtkBuilder *b = context.builder;
	GtkBox *filterBox = GTK_BOX(gtk_builder_get_object(b, "box:filter-list"));
	for (uint32_t i = 0; i < G_N_ELEMENTS(positionFilters); ++i) {
		GtkWidget *label = gtk_label_new(positionCodeNames[positionFilters[i]]);
		gtk_widget_set_hexpand(label, true);

		GtkCheckButton *checkbox = GTK_CHECK_BUTTON(gtk_check_button_new());
		gtk_check_button_set_child(checkbox, label);
		gtk_widget_add_css_class(GTK_WIDGET(checkbox), "position-toggle");
		// Activate before connecting, so the initial state does not trigger a redundant render.
		gtk_check_button_set_active(checkbox, true);
		g_signal_connect(checkbox, "toggled", G_CALLBACK(onFilterChange), cbContext);

		gtk_box_append(filterBox, GTK_WIDGET(checkbox));
	}

	ui_renderSquadDepthWindow(context);
	ui_presentWindow(context);
}

void ui_renderSquadDepthWindow(const WindowContext context) {
	const PlayerSnapshot *snapshot = context.data;
	Player *players = snapshot != NULL ? snapshot->players : NULL;
	const uint32_t playerCount = snapshot != NULL ? snapshot->count : 0;
	for (uint32_t i = 0; i < playerCount; ++i) {
		getSortedPositionRatings(&players[i]);
		players[i].age = player_getAge(players[i].dateOfBirth);
	}

	renderSquadDepthGrids(context);
}

/**
 * The snapshot's players are re-matched to the new cache by uid, so the window keeps showing the
 * players with their latest data. Re-rendering also rebuilds the rows, which is what keeps the
 * click gestures pointing at live entries.
 */
void ui_rebindSquadDepthWindow(const WindowContext context, const PlayerLookup *lookup) {
	playerSnapshot_refresh(context.data, lookup);
	renderSquadDepthGrids(context);
}

static void onFilterChange(GObject *object, gpointer userData) {
	(void)object;
	renderSquadDepthGrids(*(const WindowContext*)userData);
}

/**
 * The snapshot owns the player, and re-rendering rebuilds every row, so the pointer handed to the
 * gesture is always the live entry. Player Info takes its own copy, so it outlives this window.
 */
static void onPlayerRowClicked(
	const GtkGestureClick *gesture,
	const int clickCount,
	const double x,
	const double y,
	const Player *player
) {
	(void)gesture;
	(void)x;
	(void)y;

	if (player != NULL && clickCount == 2) {
		ui_createPlayerInfoWindow(player);
	}
}

static void renderSquadDepthGrids(const WindowContext context) {
	GtkBuilder *b = context.builder;
	GtkGrid *grid = GTK_GRID(gtk_builder_get_object(b, "grid:squad-positions"));

	{
		GtkWidget *gridWidget = GTK_WIDGET(grid);
		GtkWidget *child;
		while ((child = gtk_widget_get_first_child(gridWidget)) != NULL) {
			gtk_widget_unparent(child);
		}
	}

	// The toggles were appended in positionFilters[] order, so a sibling walk stays in lockstep
	// with the table and avoids a per-render builder lookup.
	GtkWidget *checkbox = gtk_widget_get_first_child(GTK_WIDGET(gtk_builder_get_object(b, "box:filter-list")));

	const PlayerSnapshot *snapshot = context.data;
	const uint32_t playerCount = snapshot != NULL ? snapshot->count : 0;
	// One scratch buffer shared by every grid: no position can hold more than the whole snapshot.
	DepthRow *depthRows = playerCount > 0 ? g_new(DepthRow, playerCount) : NULL;

	int32_t row = 0;
	int32_t column = 0;
	for (uint32_t i = 0; i < G_N_ELEMENTS(positionFilters) && checkbox != NULL; ++i) {
		const bool isActive = gtk_check_button_get_active(GTK_CHECK_BUTTON(checkbox));
		checkbox = gtk_widget_get_next_sibling(checkbox);
		if (!isActive) {
			continue;
		}

		GtkWidget *depthBoxWidget = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
		GtkBox *depthBox = GTK_BOX(depthBoxWidget);
		gtk_widget_set_halign(depthBoxWidget, GTK_ALIGN_CENTER);
		gtk_widget_add_css_class(depthBoxWidget, "squad-depth-box");

		GtkWidget *positionLabel = gtk_label_new(positionCodeNames[positionFilters[i]]);
		gtk_widget_add_css_class(positionLabel, "squad-depth-heading");
		gtk_widget_set_halign(positionLabel, GTK_ALIGN_START);
		gtk_widget_set_margin_start(positionLabel, 12);
		gtk_widget_set_margin_end(positionLabel, 12);
		gtk_widget_set_margin_top(positionLabel, 10);
		gtk_widget_set_margin_bottom(positionLabel, 10);

		GtkWidget *separator1 = gtk_separator_new(GTK_ORIENTATION_HORIZONTAL);
		gtk_widget_add_css_class(separator1, "card-separator");

		GtkWidget *tableHeaderBoxWidget = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
		GtkBox *tableHeaderBox = GTK_BOX(tableHeaderBoxWidget);
		gtk_widget_add_css_class(tableHeaderBoxWidget, "table-header");
		gtk_widget_set_size_request(tableHeaderBoxWidget, -1, 38);

		GtkWidget *headerNameLabel = gtk_label_new("NAME");
		gtk_widget_set_hexpand(headerNameLabel, true);
		gtk_label_set_xalign(GTK_LABEL(headerNameLabel), 0.f);
		gtk_widget_set_margin_start(headerNameLabel, 12);
		gtk_widget_set_size_request(headerNameLabel, 150, -1);
		gtk_widget_add_css_class(headerNameLabel, "squad-depth-column-heading");
		GtkWidget *ageNameLabel = gtk_label_new("AGE");
		gtk_label_set_xalign(GTK_LABEL(ageNameLabel), 0.5f);
		gtk_widget_add_css_class(ageNameLabel, "squad-depth-column-heading");
		gtk_widget_set_size_request(ageNameLabel, 55, -1);
		GtkWidget *ratingNameLabel = gtk_label_new("RTG.");
		gtk_label_set_xalign(GTK_LABEL(ratingNameLabel), 1.f);
		gtk_widget_set_margin_end(ratingNameLabel, 12);
		gtk_widget_set_size_request(ratingNameLabel, 70, -1);
		gtk_widget_add_css_class(ratingNameLabel, "squad-depth-column-heading");

		gtk_box_append(depthBox, positionLabel);
		gtk_box_append(depthBox, separator1);
		gtk_box_append(depthBox, tableHeaderBoxWidget);
		gtk_box_append(tableHeaderBox, headerNameLabel);
		gtk_box_append(tableHeaderBox, ageNameLabel);
		gtk_box_append(tableHeaderBox, ratingNameLabel);

		uint32_t depthCount = 0;
		for (uint32_t j = 0; j < playerCount; ++j) {
			const Player *player = &snapshot->players[j];
			if (player->positions[positionFilters[i]] < MINIMUM_POSITIONAL_PROFICIENCY) {
				continue;
			}

			depthRows[depthCount].player = player;
			depthRows[depthCount].rating = getRatingForPosition(player, positionFilters[i]);
			++depthCount;
		}
		// Guards the 0/1 cases, where depthRows may legitimately be NULL.
		if (depthCount > 1) {
			qsort(depthRows, depthCount, sizeof(*depthRows), compareDepthRows);
		}

		#define SQUAD_DEPTH_MAX_ROWS 20
		const uint8_t maxRows = depthCount > SQUAD_DEPTH_MAX_ROWS ? SQUAD_DEPTH_MAX_ROWS : (uint8_t)depthCount;
		for (uint32_t j = 0; j < maxRows; ++j) {
			const Player *player = depthRows[j].player;

			GtkWidget *playerRowBoxWidget = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
			GtkBox *playerRowBox = GTK_BOX(playerRowBoxWidget);
			gtk_widget_set_size_request(playerRowBoxWidget, -1, 43);
			gtk_widget_add_css_class(playerRowBoxWidget, "player-row");

			GtkGesture *gesture = gtk_gesture_click_new();
			gtk_widget_add_controller(playerRowBoxWidget, GTK_EVENT_CONTROLLER(gesture));
			g_signal_connect(gesture, "pressed", G_CALLBACK(onPlayerRowClicked), (gpointer)player);

			GtkWidget *nameLabel = gtk_label_new("");
			if (player->commonName[0] == '\0') {
				char buffer[PERSON_FORENAME_LENGTH + PERSON_SURNAME_LENGTH + 2];
				snprintf(buffer, sizeof(buffer), "%s %s", player->forename, player->surname);
				gtk_label_set_text(GTK_LABEL(nameLabel), buffer);
			} else {
				gtk_label_set_text(GTK_LABEL(nameLabel), player->commonName);
			}
			gtk_label_set_xalign(GTK_LABEL(nameLabel), 0.f);
			gtk_widget_set_hexpand(nameLabel, true);
			gtk_widget_set_margin_start(nameLabel, 12);
			gtk_widget_add_css_class(nameLabel, "name-label");
			gtk_widget_set_size_request(nameLabel, 150, -1);

			char ageText[8];
			snprintf(ageText, sizeof(ageText), "%u", player->age);
			GtkWidget *ageLabel = gtk_label_new(ageText);
			gtk_label_set_xalign(GTK_LABEL(ageLabel), 0.5f);
			gtk_widget_add_css_class(ageLabel, "age-label");
			gtk_widget_set_size_request(ageLabel, 55, -1);

			char ratingText[FORMATTER_RATING_SIZE];
			formatter_formatRating(depthRows[j].rating, ratingText);
			GtkWidget *ratingLabel = gtk_label_new("");
			gtk_label_set_markup(GTK_LABEL(ratingLabel), ratingText);
			gtk_label_set_xalign(GTK_LABEL(ratingLabel), 1.f);
			gtk_widget_set_margin_end(ratingLabel, 12);
			gtk_widget_add_css_class(ratingLabel, "rating-label");
			gtk_widget_set_size_request(ratingLabel, 70, -1);

			gtk_box_append(playerRowBox, nameLabel);
			gtk_box_append(playerRowBox, ageLabel);
			gtk_box_append(playerRowBox, ratingLabel);
			gtk_box_append(depthBox, playerRowBoxWidget);
		}

		gtk_grid_attach(grid, depthBoxWidget, column, row, 1, 1);

		if (column == 2) {
			column = 0;
			row++;
		} else {
			++column;
		}
	}

	g_free(depthRows);
}
