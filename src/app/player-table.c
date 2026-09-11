// SPDX-FileCopyrightText: © 2026 Phil Armstead <philarmstead@mailbox.org>
// SPDX-License-Identifier: GPL-3.0-or-later

#include "player-table.h"
#include "app/helpers/formatter.h"
#include "app/helpers/vector-shared-pointer.h"
#include "app/helpers/vector.h"
#include "app/ui.h"
#include "core/logger.h"
#include "platform/platform.h"

#include <glib.h>

#include "maths.h"


typedef struct {
	GtkColumnView *table;
	GtkSingleSelection *selection;
	double savedScrollValue;
	gulong scrollGuardId;
} PlayerTableContext;

static PlayerTableContext context = {0};

extern GameContext gameContext;

G_DEFINE_TYPE(SearchPlayerRow, search_player_row, G_TYPE_OBJECT)

static void search_player_row_finalize(GObject *object) {
	G_OBJECT_CLASS(search_player_row_parent_class)->finalize(object);
}

enum {
	PROP_PLAYER = 1,
	PROP_SURNAME,
	PROP_AGE,
	PROP_CA,
	PROP_PA,
	PROP_DIFF,
	PROP_RATING,
	N_PROPS
};

typedef enum {
	COLUMN_NATIONALITIES,
	COLUMN_NAME,
	COLUMN_AGE,
	COLUMN_POSITIONS,
	COLUMN_CA,
	COLUMN_PA,
	COLUMN_CA_PA_DELTA,
	COLUMN_STATUS,
	COLUMN_RATING,
	COLUMN_VALUE,
	COLUMN_CLUB,
	COLUMN_REPUTATION_HOME,
	COLUMN_REPUTATION_CURRENT,
	COLUMN_REPUTATION_WORLD,
} Columns;

static GParamSpec *properties[N_PROPS];

static void search_player_row_set_property(
	GObject *object,
	const guint propertyId,
	const GValue *value,
	GParamSpec *pspec
) {
	SearchPlayerRow *self = SEARCH_PLAYER_ROW(object);
	if (propertyId == PROP_PLAYER) {
		self->player = (Player*)g_value_get_pointer(value);
	} else {
		G_OBJECT_WARN_INVALID_PROPERTY_ID(object, propertyId, pspec);
	}
}

static void search_player_row_get_property(GObject *object, const guint propertyId, GValue *value, GParamSpec *pspec) {
	const SearchPlayerRow *self = SEARCH_PLAYER_ROW(object);
	if (propertyId == PROP_PLAYER) {
		g_value_set_pointer(value, self->player);
	} else if (propertyId == PROP_SURNAME) {
		g_value_set_string(value, self->player->surname);
	} else if (propertyId == PROP_AGE) {
		g_value_set_uint(value, self->player->age);
	} else if (propertyId == PROP_CA) {
		g_value_set_uint(value, self->player->ca);
	} else if (propertyId == PROP_PA) {
		g_value_set_uint(value, self->player->pa);
	} else if (propertyId == PROP_DIFF) {
		g_value_set_uint(value, self->player->pa - self->player->ca);
	} else if (propertyId == PROP_RATING) {
		g_value_set_float(value, self->player->ratings[0].value);
	} else {
		G_OBJECT_WARN_INVALID_PROPERTY_ID(object, propertyId, pspec);
	}
}

static void search_player_row_init(SearchPlayerRow *self) {
	// Don't do anything here — let properties handle initialization
	(void)self;
}

static void search_player_row_class_init(SearchPlayerRowClass *klass) {
	GObjectClass *object_class = G_OBJECT_CLASS(klass);

	object_class->set_property = search_player_row_set_property;
	object_class->get_property = search_player_row_get_property;
	object_class->finalize = search_player_row_finalize;

	properties[PROP_PLAYER] = g_param_spec_pointer(
		"player",
		"Player",
		"Player data",
		G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY
	);
	properties[PROP_SURNAME] = g_param_spec_string(
		"surname",
		"Surname",
		"Player surname",
		NULL,
		G_PARAM_READABLE
	);
	properties[PROP_AGE] = g_param_spec_uint(
		"age",
		"Age",
		"Player age",
		0,
		100,
		0,
		G_PARAM_READABLE
	);
	properties[PROP_CA] = g_param_spec_uint(
		"ca",
		"CA",
		"Player CA",
		0,
		200,
		0,
		G_PARAM_READABLE
	);
	properties[PROP_PA] = g_param_spec_uint(
		"pa",
		"PA",
		"Player PA",
		0,
		200,
		0,
		G_PARAM_READABLE
	);
	properties[PROP_DIFF] = g_param_spec_uint(
		"diff",
		"Diff",
		"Player PA - CA",
		0,
		200,
		0,
		G_PARAM_READABLE
	);
	properties[PROP_RATING] = g_param_spec_float(
		"rating",
		"Rating",
		"Player's best rating",
		0,
		200,
		0,
		G_PARAM_READABLE
	);

	g_object_class_install_properties(object_class, N_PROPS, properties);
}

static void onRowClicked(
	const GtkGestureClick *gesture,
	const uint8_t clickCount,
	const double x,
	const double y,
	gpointer data
) {
	(void)gesture;
	(void)x;
	(void)y;

	GtkListItem *listItem = GTK_LIST_ITEM(data);
	const SearchPlayerRow *row = gtk_list_item_get_item(listItem);

	if (row != NULL && row->player != NULL && clickCount == 2) {
		ui_createPlayerInfoWindow(row->player);
	}
}

static void bindClickHandler(GtkWidget *widget, GtkListItem *item) {
	GtkGestureClick *gesture = GTK_GESTURE_CLICK(gtk_gesture_click_new());
	gtk_widget_add_controller(widget, GTK_EVENT_CONTROLLER(gesture));
	g_signal_connect(gesture, "pressed", G_CALLBACK(onRowClicked), item);
}

static void setupTextLabel(const GtkSignalListItemFactory *factory, GtkListItem *item, gpointer data) {
	(void)factory;
	(void)data;

	GtkWidget *label = gtk_label_new("");
	gtk_list_item_set_child(item, label);

	bindClickHandler(label, item);
}

static void setupBox(const GtkSignalListItemFactory *factory, GtkListItem *item, gpointer data) {
	(void)factory;
	(void)data;

	GtkWidget *box = gtk_box_new(0, 2);
	gtk_list_item_set_child(item, box);

	bindClickHandler(box, item);
}

static void printNumeric(GtkWidget *label, const int64_t value) {
	gchar buffer[32];
	g_snprintf(buffer, sizeof(buffer), "%zu", value);
	if (value > 999) {
		formatter_printNumber(buffer);
	}
	gtk_label_set_text(GTK_LABEL(label), buffer);
}

static void printCurrency(GtkWidget *label, uint64_t value) {
	gchar buffer[32];
	g_snprintf(buffer, sizeof(buffer), "%zu", value);
	formatter_printCurrency(buffer);
	gtk_label_set_xalign(GTK_LABEL(label), 0);
	gtk_label_set_text(GTK_LABEL(label), buffer);
}

static GString *formatPlayerPositions(const uint8_t *positions) {
	GString *str = g_string_new(NULL);
	bool hasPrevious = false;
	bool sameRow = false;

	if (positions[POSITION_CODE_GK] >= MINIMUM_POSITIONAL_PROFICIENCY) {
		g_string_append(str, positionCodeNames[POSITION_CODE_GK]);
		hasPrevious = true;
	}

	if (positions[POSITION_CODE_DL] >= MINIMUM_POSITIONAL_PROFICIENCY) {
		if (hasPrevious) {
			g_string_append(str, ", ");
		}
		hasPrevious = true;
		sameRow = true;
		g_string_append(str, positionCodeNames[POSITION_CODE_DL]);
	}

	if (positions[POSITION_CODE_DR] >= MINIMUM_POSITIONAL_PROFICIENCY) {
		if (sameRow) {
			sameRow = false;
			g_string_append(str, "/R");
		} else if (hasPrevious) {
			g_string_append(str, ", ");
		} else {
			g_string_append(str, positionCodeNames[POSITION_CODE_DR]);
		}
		hasPrevious = true;
	}

	if (positions[POSITION_CODE_DC] >= MINIMUM_POSITIONAL_PROFICIENCY) {
		if (hasPrevious) {
			g_string_append(str, ", ");
		}
		g_string_append(str, positionCodeNames[POSITION_CODE_DC]);
		hasPrevious = true;
	}


	if (positions[POSITION_CODE_WBL] >= MINIMUM_POSITIONAL_PROFICIENCY) {
		if (hasPrevious) {
			g_string_append(str, ", ");
		}
		hasPrevious = true;
		sameRow = true;
		g_string_append(str, positionCodeNames[POSITION_CODE_WBL]);
	}

	if (positions[POSITION_CODE_WBR] >= MINIMUM_POSITIONAL_PROFICIENCY) {
		if (sameRow) {
			sameRow = false;
			g_string_append(str, "/R");
		} else {
			if (hasPrevious) {
				g_string_append(str, ", ");
			}

			g_string_append(str, positionCodeNames[POSITION_CODE_WBR]);
		}
		hasPrevious = true;
	}

	if (positions[POSITION_CODE_DM] >= MINIMUM_POSITIONAL_PROFICIENCY) {
		if (hasPrevious) {
			g_string_append(str, ", ");
		}
		hasPrevious = true;
		g_string_append(str, positionCodeNames[POSITION_CODE_DM]);
	}

	sameRow = false;

	if (positions[POSITION_CODE_ML] >= MINIMUM_POSITIONAL_PROFICIENCY) {
		if (hasPrevious) {
			g_string_append(str, ", ");
		}
		hasPrevious = true;
		sameRow = true;
		g_string_append(str, positionCodeNames[POSITION_CODE_ML]);
	}

	if (positions[POSITION_CODE_MR] >= MINIMUM_POSITIONAL_PROFICIENCY) {
		if (sameRow) {
			g_string_append(str, "/R");
		} else {
			if (hasPrevious) {
				g_string_append(str, ", ");
			}

			g_string_append(str, positionCodeNames[POSITION_CODE_MR]);
		}
		hasPrevious = true;
	}

	sameRow = false;

	if (positions[POSITION_CODE_MC] >= MINIMUM_POSITIONAL_PROFICIENCY) {
		if (hasPrevious) {
			g_string_append(str, ", ");
		}
		hasPrevious = true;
		g_string_append(str, positionCodeNames[POSITION_CODE_MC]);
	}

	if (positions[POSITION_CODE_AML] >= MINIMUM_POSITIONAL_PROFICIENCY) {
		if (hasPrevious) {
			g_string_append(str, ", ");
		}
		hasPrevious = true;
		sameRow = true;
		g_string_append(str, positionCodeNames[POSITION_CODE_AML]);
	}

	if (positions[POSITION_CODE_AMR] >= MINIMUM_POSITIONAL_PROFICIENCY) {
		if (sameRow) {
			g_string_append(str, "/R");
		} else {
			if (hasPrevious) {
				g_string_append(str, ", ");
			}

			g_string_append(str, positionCodeNames[POSITION_CODE_AMR]);
		}
		hasPrevious = true;
	}

	sameRow = false;

	if (positions[POSITION_CODE_AMC] >= MINIMUM_POSITIONAL_PROFICIENCY) {
		if (hasPrevious) {
			g_string_append(str, ", ");
		}
		hasPrevious = true;
		g_string_append(str, positionCodeNames[POSITION_CODE_AMC]);
	}

	if (positions[POSITION_CODE_ST] >= MINIMUM_POSITIONAL_PROFICIENCY) {
		if (hasPrevious) {
			g_string_append(str, ", ");
		}
		hasPrevious = true;
		g_string_append(str, positionCodeNames[POSITION_CODE_ST]);
	}

	return str;
}

static void bindCellValue(const GtkSignalListItemFactory *factory, GtkListItem *item, const uint8_t columnId) {
	(void)factory;

	GtkWidget *label = gtk_list_item_get_child(GTK_LIST_ITEM(item));
	gtk_label_set_justify(GTK_LABEL(label), GTK_JUSTIFY_LEFT);
	const SearchPlayerRow *row = gtk_list_item_get_item(GTK_LIST_ITEM(item));

	if (row == NULL || row->player == NULL) {
		return;
	}

	const Player *player = row->player;

	switch (columnId) {
		case COLUMN_NAME:
			if (player->commonName[0] == '\0') {
				char buffer[PERSON_FORENAME_LENGTH + PERSON_SURNAME_LENGTH + 2];
				snprintf(buffer, sizeof(buffer), "%s %s", player->forename, player->surname);
				gtk_label_set_text(GTK_LABEL(label), buffer);
			} else {
				gtk_label_set_text(GTK_LABEL(label), player->commonName);
			}
			gtk_label_set_xalign(GTK_LABEL(label), 0);
			break;
		case COLUMN_AGE:
			printNumeric(label, player->age);
			break;
		case COLUMN_POSITIONS: {
			GString *str = formatPlayerPositions(row->player->positions);
			gtk_label_set_text(GTK_LABEL(label), str->str);
			gtk_label_set_xalign(GTK_LABEL(label), 0);
			g_string_free(str, TRUE);
			break;
		}
		case COLUMN_CA:
			printNumeric(label, player->ca);
			break;
		case COLUMN_PA:
			printNumeric(label, player->pa);
			break;
		case COLUMN_CA_PA_DELTA:
			printNumeric(label, player->pa - player->ca);
			break;
		case COLUMN_RATING: {
			char buffer[44];
			formatter_formatRating(player->ratings[0].value, buffer);
			gtk_label_set_markup(GTK_LABEL(label), buffer);
			break;
		}
		case COLUMN_VALUE:
			printCurrency(label, player->guideValue);
			break;
		case COLUMN_CLUB:
			if (row->player->clubIndex >= 0) {
				gtk_label_set_text(GTK_LABEL(label), gameContext.clubs[row->player->clubIndex].shortName);
				gtk_label_set_xalign(GTK_LABEL(label), 0);
			}
			break;
		case COLUMN_REPUTATION_HOME:
			printNumeric(label, player->homeReputation);
			break;
		case COLUMN_REPUTATION_CURRENT:
			printNumeric(label, player->currentReputation);
			break;
		case COLUMN_REPUTATION_WORLD:
			printNumeric(label, player->worldReputation);
			break;
		default:
			break;
	}
}

static void bindStatusValue(const GtkSignalListItemFactory *factory, GtkListItem *item, gpointer data) {
	(void)factory;
	(void)data;

	GtkWidget *box = gtk_list_item_get_child(GTK_LIST_ITEM(item));
	const SearchPlayerRow *row = gtk_list_item_get_item(GTK_LIST_ITEM(item));

	GtkWidget *child;
	while ((child = gtk_widget_get_first_child(GTK_WIDGET(box))) != NULL) {
		gtk_box_remove(GTK_BOX(box), child);
	}

	if (row == NULL || row->player == NULL) {
		return;
	}

	if (row->player->injury.name != NULL) {
		GtkWidget *label = gtk_label_new("🚑");
		gtk_box_append(GTK_BOX(box), label);
		char buffer[128] = {0};
		snprintf(buffer, 128, "Injured: %s", row->player->injury.name);
		gtk_widget_set_tooltip_text(label, buffer);
	}

	if (row->player->canDevelopQuickly) {
		GtkWidget *label = gtk_label_new("🧠");
		gtk_box_append(GTK_BOX(box), label);
		gtk_widget_set_tooltip_text(label, "Can develop quickly");
	}
	if (row->player->isHotProspect) {
		GtkWidget *label = gtk_label_new("🔥");
		gtk_box_append(GTK_BOX(box), label);
		gtk_widget_set_tooltip_text(label, "A hot prospect");
	}
}

static void bindNationalitiesValue(const GtkSignalListItemFactory *factory, GtkListItem *item, gpointer data) {
	(void)factory;
	(void)data;

	GtkWidget *box = gtk_list_item_get_child(GTK_LIST_ITEM(item));
	const SearchPlayerRow *row = gtk_list_item_get_item(GTK_LIST_ITEM(item));

	GtkWidget *child;
	while ((child = gtk_widget_get_first_child(GTK_WIDGET(box))) != NULL) {
		gtk_box_remove(GTK_BOX(box), child);
	}

	if (row == NULL || row->player == NULL) {
		return;
	}

	uint8_t nationalityIndex = 0;
	while (nationalityIndex < 4 && row->player->nationality[nationalityIndex] != 0xFF) {
		char pathToFlag[256] = {0};
		const Nation nation = gameContext.nations[row->player->nationality[nationalityIndex]];
		snprintf(
			pathToFlag,
			sizeof(pathToFlag),
			RESOURCE_BASE "/assets/flags/%s.png",
			nation.code
		);
		GtkWidget *flagImage = gtk_image_new_from_resource(pathToFlag);
		gtk_box_append(GTK_BOX(box), flagImage);
		gtk_widget_set_tooltip_text(flagImage, nation.name);
		nationalityIndex++;
	}
}

static gboolean restoreScroll(gpointer data) {
	GtkAdjustment *adj = data;
	const double max = gtk_adjustment_get_upper(adj) - gtk_adjustment_get_page_size(adj);
	gtk_adjustment_set_value(adj, CLAMP(context.savedScrollValue, 0, max));
	return G_SOURCE_REMOVE;
}

// One-shot correction: the sort makes GTK scroll the focused row into view, and
// that happens on an indeterminate later frame — so instead of guessing when,
// we react. The first time the scroll value changes after a sort, we snap it
// back to the saved position and immediately disconnect, so normal user
// scrolling afterwards is untouched.
static void onScrollGuard(GtkAdjustment *adjustment, gpointer data) {
	(void)data;

	if (context.scrollGuardId != 0) {
		g_signal_handler_disconnect(adjustment, context.scrollGuardId);
		context.scrollGuardId = 0;
	}

	g_idle_add_full(GDK_PRIORITY_REDRAW - 1, restoreScroll, adjustment, NULL);
}

// Runs when the user clicks a column header to change the sort. This fires
// before the GtkSortListModel reorders (we connect first), so we snapshot the
// current scroll position here and arm a one-shot guard that undoes GTK's
// follow-the-focused-row scroll whenever it lands.
static void onSorterChanged(GtkSorter *sorter, const GtkSorterChange change, gpointer data) {
	(void)sorter;
	(void)change;
	(void)data;

	if (context.table == NULL) {
		return;
	}

	GtkAdjustment *adjustment = gtk_scrollable_get_vadjustment(GTK_SCROLLABLE(context.table));
	if (adjustment == NULL) {
		return;
	}

	context.savedScrollValue = gtk_adjustment_get_value(adjustment);

	if (context.scrollGuardId == 0) {
		context.scrollGuardId = g_signal_connect(
			adjustment,
			"value-changed",
			G_CALLBACK(onScrollGuard),
			NULL
		);
	}
}

static GtkColumnViewColumn *createColumn(const char *title, const uint8_t columnType, GtkSorter *sorter) {
	GtkListItemFactory *factory = gtk_signal_list_item_factory_new();
	if (columnType == COLUMN_NATIONALITIES) {
		g_signal_connect(factory, "setup", G_CALLBACK(setupBox), NULL);
		g_signal_connect(factory, "bind", G_CALLBACK(bindNationalitiesValue), NULL);
	} else if (columnType == COLUMN_STATUS) {
		g_signal_connect(factory, "setup", G_CALLBACK(setupBox), NULL);
		g_signal_connect(factory, "bind", G_CALLBACK(bindStatusValue), NULL);
	} else {
		g_signal_connect(factory, "setup", G_CALLBACK(setupTextLabel), NULL);
		g_signal_connect(factory, "bind", G_CALLBACK(bindCellValue), (void *)(uint64_t)columnType);
	}

	GtkColumnViewColumn *column = gtk_column_view_column_new(title, factory);

	// Attach sorter if provided
	if (sorter != NULL) {
		gtk_column_view_column_set_sorter(column, sorter);
	}

	return column;
}

void playerTable_init(void) {
	GtkSorter *nameSorter = GTK_SORTER(
		gtk_string_sorter_new(
			gtk_property_expression_new(SEARCH_TYPE_PLAYER_ROW, NULL, "surname")
		)
	);
	GtkSorter *ageSorter = GTK_SORTER(
		gtk_numeric_sorter_new(
			gtk_property_expression_new(SEARCH_TYPE_PLAYER_ROW, NULL, "age")
		)
	);
	GtkSorter *caSorter = GTK_SORTER(
		gtk_numeric_sorter_new(
			gtk_property_expression_new(SEARCH_TYPE_PLAYER_ROW, NULL, "ca")
		)
	);
	GtkSorter *paSorter = GTK_SORTER(
		gtk_numeric_sorter_new(
			gtk_property_expression_new(SEARCH_TYPE_PLAYER_ROW, NULL, "pa")
		)
	);
	GtkSorter *diffSorter = GTK_SORTER(
		gtk_numeric_sorter_new(
			gtk_property_expression_new(SEARCH_TYPE_PLAYER_ROW, NULL, "diff")
		)
	);
	GtkSorter *ratingSorter = GTK_SORTER(
		gtk_numeric_sorter_new(
			gtk_property_expression_new(SEARCH_TYPE_PLAYER_ROW, NULL, "rating")
		)
	);

	GtkColumnView *table = GTK_COLUMN_VIEW(GTK_WIDGET(gtk_builder_get_object(gameContext.builder, "table:player-list")));
	context.table = table;

	GtkColumnViewColumn *ratingColumn = createColumn("Rating", COLUMN_RATING, ratingSorter);

	gtk_column_view_append_column(table, createColumn("", COLUMN_NATIONALITIES, NULL));
	gtk_column_view_append_column(table, createColumn("Name", COLUMN_NAME, nameSorter));
	gtk_column_view_append_column(table, createColumn("Age", COLUMN_AGE, ageSorter));
	gtk_column_view_append_column(table, createColumn("Positions", COLUMN_POSITIONS, NULL));
	gtk_column_view_append_column(table, createColumn("CA", COLUMN_CA, caSorter));
	gtk_column_view_append_column(table, createColumn("PA", COLUMN_PA, paSorter));
	gtk_column_view_append_column(table, createColumn("Diff", COLUMN_CA_PA_DELTA, diffSorter));
	gtk_column_view_append_column(table, createColumn("Status", COLUMN_STATUS, NULL));
	gtk_column_view_append_column(table, ratingColumn);
	gtk_column_view_append_column(table, createColumn("Value", COLUMN_VALUE, NULL));
	gtk_column_view_append_column(table, createColumn("Club", COLUMN_CLUB, NULL));
	gtk_column_view_append_column(table, createColumn("Home rep", COLUMN_REPUTATION_HOME, NULL));
	gtk_column_view_append_column(table, createColumn("Current rep", COLUMN_REPUTATION_CURRENT, NULL));
	gtk_column_view_append_column(table, createColumn("World rep", COLUMN_REPUTATION_WORLD, NULL));

	context.selection = NULL;

	GtkSorter *columnViewSorter = gtk_column_view_get_sorter(context.table);
	if (columnViewSorter != NULL) {
		g_signal_connect(columnViewSorter, "changed", G_CALLBACK(onSorterChanged), NULL);
	}

	gameContext.searchResults = sharedPointer_new(NULL);
	playerTable_clear();

	gtk_column_view_sort_by_column(context.table, ratingColumn, GTK_SORT_DESCENDING);
}

static SearchPlayerRow *search_player_row_new(Player *player) {
	return g_object_new(SEARCH_TYPE_PLAYER_ROW, "player", player, NULL);
}

void playerTable_clear(void) {
	if (gameContext.searchResults != NULL && vector_length(gameContext.searchResults->data) > 0) {
		sharedPointer_unref(gameContext.searchResults);
		gameContext.searchResults = sharedPointer_new(NULL);
	}
	playerTable_populate();
}

void playerTable_populate(void) {
	const int64_t timeStart = platform_getMicroseconds();

	GtkAdjustment *adjustment = gtk_scrollable_get_vadjustment(GTK_SCROLLABLE(context.table));
	gtk_adjustment_set_value(adjustment, 0);

	const uint32_t *playerIds = gameContext.searchResults->data;
	const size_t playerCount = vector_length(playerIds);

	GtkWidget *clearAll = GTK_WIDGET(gtk_builder_get_object(gameContext.builder, "button:clear-all"));
	gtk_widget_set_visible(clearAll, playerCount > 0);

	GListStore *store = g_list_store_new(SEARCH_TYPE_PLAYER_ROW);
	for (size_t i = 0; i < playerCount; ++i) {
		const Player *player = &gameContext.players[playerIds[i]];
		SearchPlayerRow *row = search_player_row_new((Player*)player);
		g_list_store_append(store, row);
		g_object_unref(row);
	}

	GtkSorter *sorter = g_object_ref(gtk_column_view_get_sorter(context.table));
	GtkSortListModel *sortModel = gtk_sort_list_model_new(G_LIST_MODEL(store), sorter);
	GtkSingleSelection *newSelection = gtk_single_selection_new(G_LIST_MODEL(sortModel));
	gtk_single_selection_set_autoselect(newSelection, false);

	GtkSingleSelection *oldSelection = context.selection;

	gtk_column_view_set_model(context.table, GTK_SELECTION_MODEL(newSelection));

	if (oldSelection != NULL)
		g_object_unref(oldSelection);

	context.selection = newSelection;

	GtkLabel *resultsCountLabel = GTK_LABEL(
		GTK_WIDGET(gtk_builder_get_object(gameContext.builder, "label:results-count"))
	);
	char resultCountString[32] = {0};
	char formattedResults[12] = {0};
	snprintf(formattedResults, 12, "%zu", playerCount);
	formatter_printNumber(formattedResults);
	snprintf(
		resultCountString,
		sizeof(resultCountString),
		playerCount == 1 ? "1 result" : "%s results",
		formattedResults
	);
	gtk_label_set_text(resultsCountLabel, resultCountString);

	const int64_t timeEnd = platform_getMicroseconds();

	LOG_DEBUG("Rendered %zu players in %lld microseconds", playerCount, (long long)(timeEnd - timeStart));
}
