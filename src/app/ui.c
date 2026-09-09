// SPDX-FileCopyrightText: © 2026 Phil Armstead <philarmstead@mailbox.org>
// SPDX-License-Identifier: GPL-3.0-or-later

#include "ui.h"
#include "app/callbacks.h"
#include "app/cache.h"
#include "app/config.h"
#include "app/search-handler.h"
#include "app/helpers/date.h"
#include "app/helpers/vector.h"
#include "platform/platform.h"

#include <gtk/gtk.h>

#include "player-table.h"


extern ProcessContext processContext;
extern GameContext gameContext;

static gboolean onWindowClose(GtkWidget *widget, gpointer userData);
static void onFilterTagClick(GtkWidget *self, GtkEntryBuffer *buffer);
static void onClubFilterTagClick(GtkWidget *self, GtkEditable *buffer);
static void loadStylesheet(const char *fileName);

void ui_init(GtkApplication *app) {
	loadStylesheet("styles.css");

	// Show main window
	const WindowContext context = openWindow("player-search", "window:player-search", WINDOW_PLAYER_SEARCH);
	gameContext.builder = context.builder;
	gtk_window_set_application(GTK_WINDOW(context.window), GTK_APPLICATION(app));

	// Write application version
	const GtkLabel *appVersionLabel = GTK_LABEL(gtk_builder_get_object(context.builder, "label:application-version"));
	char appVersionBuffer[64];
	snprintf(
		appVersionBuffer,
		sizeof(appVersionBuffer),
		"%s %d.%d.%d",
		APP_NAME,
		APP_VERSION_MAJOR,
		APP_VERSION_MINOR,
		APP_VERSION_PATCH
	);
	gtk_label_set_text(GTK_LABEL(appVersionLabel), appVersionBuffer);

#ifdef MOCKS_MODE
	// In mock mode, we never have the process-connected callback run
	cache_run();
#endif

	// Create datalist box
	SearchDatalist *dataList = g_new0(SearchDatalist, 1);

	dataList->entry = GTK_SEARCH_ENTRY(gtk_search_entry_new());
	dataList->popover = GTK_POPOVER(gtk_popover_new());
	dataList->listBox = GTK_LIST_BOX(gtk_list_box_new());

	gtk_search_entry_set_placeholder_text(dataList->entry, "Search club");
	gtk_widget_add_css_class(GTK_WIDGET(dataList->entry), "sidebar-search");

	gtk_popover_set_child(dataList->popover, GTK_WIDGET(dataList->listBox));
	gtk_widget_set_parent(GTK_WIDGET(dataList->popover), GTK_WIDGET(dataList->entry));

	gtk_popover_set_pointing_to(dataList->popover, &(GdkRectangle){.x = 102, .y = 27, .width = 1, .height = 1});
	gtk_popover_set_position(dataList->popover, GTK_POS_BOTTOM);

	gtk_popover_set_autohide(dataList->popover, FALSE);
	gtk_widget_set_can_focus(GTK_WIDGET(dataList->popover), FALSE);
	gtk_widget_set_can_focus(GTK_WIDGET(dataList->listBox), FALSE);
	gtk_list_box_set_selection_mode(dataList->listBox, GTK_SELECTION_NONE);

	GtkBox *container = GTK_BOX(gtk_builder_get_object(gameContext.builder, "box:club-search-container"));
	gtk_box_append(container, GTK_WIDGET(dataList->entry));
	gameContext.dataList = dataList;

	// Cache field buffers
	GtkBuilder *b = gameContext.builder;
	FilterBuffer *buf = &gameContext.filterBuffer;
	buf->minAge = gtk_entry_get_buffer(GTK_ENTRY(gtk_builder_get_object(b, "entry:age:min")));
	buf->maxAge = gtk_entry_get_buffer(GTK_ENTRY(gtk_builder_get_object(b, "entry:age:max")));
	buf->minCA = gtk_entry_get_buffer(GTK_ENTRY(gtk_builder_get_object(b, "entry:ca:min")));
	buf->maxCA = gtk_entry_get_buffer(GTK_ENTRY(gtk_builder_get_object(b, "entry:ca:max")));
	buf->minPA = gtk_entry_get_buffer(GTK_ENTRY(gtk_builder_get_object(b, "entry:pa:min")));
	buf->maxPA = gtk_entry_get_buffer(GTK_ENTRY(gtk_builder_get_object(b, "entry:pa:max")));
	buf->minRating = gtk_entry_get_buffer(GTK_ENTRY(gtk_builder_get_object(b, "entry:rating:min")));
	buf->maxRating = gtk_entry_get_buffer(GTK_ENTRY(gtk_builder_get_object(b, "entry:rating:max")));

	// Cache check boxes
	CheckBox *check = &gameContext.checkboxes;
	check->positionGK = GTK_CHECK_BUTTON(gtk_builder_get_object(b, "checkbox:position:gk"));
	check->positionDL = GTK_CHECK_BUTTON(gtk_builder_get_object(b, "checkbox:position:dl"));
	check->positionDC = GTK_CHECK_BUTTON(gtk_builder_get_object(b, "checkbox:position:dc"));
	check->positionDR = GTK_CHECK_BUTTON(gtk_builder_get_object(b, "checkbox:position:dr"));
	check->positionWBL = GTK_CHECK_BUTTON(gtk_builder_get_object(b, "checkbox:position:wbl"));
	check->positionDM = GTK_CHECK_BUTTON(gtk_builder_get_object(b, "checkbox:position:dm"));
	check->positionWBR = GTK_CHECK_BUTTON(gtk_builder_get_object(b, "checkbox:position:wbr"));
	check->positionML = GTK_CHECK_BUTTON(gtk_builder_get_object(b, "checkbox:position:ml"));
	check->positionMC = GTK_CHECK_BUTTON(gtk_builder_get_object(b, "checkbox:position:mc"));
	check->positionMR = GTK_CHECK_BUTTON(gtk_builder_get_object(b, "checkbox:position:mr"));
	check->positionAML = GTK_CHECK_BUTTON(gtk_builder_get_object(b, "checkbox:position:aml"));
	check->positionAMC = GTK_CHECK_BUTTON(gtk_builder_get_object(b, "checkbox:position:amc"));
	check->positionAMR = GTK_CHECK_BUTTON(gtk_builder_get_object(b, "checkbox:position:amr"));
	check->positionST = GTK_CHECK_BUTTON(gtk_builder_get_object(b, "checkbox:position:st"));
}

void ui_update(void) {
	ui_updateInGameDate();
	ui_updateGameVersion();
}

void ui_updateInGameDate(void) {
	GtkLabel *dateLabel = GTK_LABEL(GTK_WIDGET(gtk_builder_get_object(gameContext.builder, "label:date")));

#ifndef MOCKS_MODE
	if (processContext.handle == NULL || !gameContext.gameKey || gameContext.currentDate.year == 1900) {
		gtk_label_set_text(dateLabel, "");
		return;
	}
#endif

	DayMonthYearTime d = date_prettify(gameContext.currentDate);
	char buffer[64] = {0};
	snprintf(
		buffer,
		64,
		"📅 %s %d%s, %d %s",
		d.month,
		d.day,
		date_getOrdinal(d.day),
		d.year,
		d.timeString
	);
	gtk_label_set_text(dateLabel, buffer);
}

void ui_updateGameVersion(void) {
	GtkLabel *versionLabel = GTK_LABEL(GTK_WIDGET(gtk_builder_get_object(gameContext.builder, "label:version")));
	if (gameContext.gameVersion[0] == '\0') {
		gtk_label_set_text(versionLabel, "");
		return;
	}

	char buffer[64] = {0};
	snprintf(buffer, 64, "📝 %s", gameContext.gameVersion);
	gtk_label_set_text(versionLabel, buffer);
}

WindowContext openWindow(const char *layoutName, const char *windowName, const WindowType type) {
	char pathToAppLayout[256] = {0};
	snprintf(pathToAppLayout, sizeof(pathToAppLayout), RESOURCE_BASE "/layouts/%s.ui", layoutName);

	WindowContext context = {.type = type};
	context.builder = gtk_builder_new_from_resource(pathToAppLayout);
	context.window = GTK_WIDGET(gtk_builder_get_object(context.builder, windowName));
	g_signal_connect(context.window, "close_request", G_CALLBACK(onWindowClose), NULL);

	GtkEventControllerKey *closeController = GTK_EVENT_CONTROLLER_KEY(gtk_event_controller_key_new());
	gtk_event_controller_set_propagation_phase(GTK_EVENT_CONTROLLER(closeController), GTK_PHASE_CAPTURE);
	g_signal_connect(closeController, "key-pressed", G_CALLBACK(callbacks_onWindowKeypress), context.window);
	gtk_widget_add_controller(context.window, GTK_EVENT_CONTROLLER(closeController));

	gtk_window_present(GTK_WINDOW(context.window));

	vector_push(gameContext.windows, context);

	return context;
}

static gboolean onWindowClose(GtkWidget *widget, gpointer userData) {
	for (uint64_t i = 0; i < vector_length(gameContext.windows); i++) {
		if (gameContext.windows[i].window == widget) {
			WindowContext out;
			vector_splice(gameContext.windows, i, &out);
		}
	}

	(void)userData;
	return G_SOURCE_REMOVE;
}

void ui_refreshAllWindows(void) {
	for (uint64_t i = 0; i < vector_length(gameContext.windows); i++) {
		if (gameContext.windows[i].type == WINDOW_BEST_XI) {
			ui_renderBestElevenWindow(gameContext.windows[i]);
		} else if (gameContext.windows[i].type == WINDOW_PLAYER_INFO) {
			ui_renderPlayerInfoWindow(gameContext.windows[i]);
		}
	}
}

void ui_setCurrentStatus(const char *status) {
	GtkLabel *statusLabel = GTK_LABEL(gtk_builder_get_object(gameContext.builder, "label:application-status"));
	gtk_label_set_text(statusLabel, status);
}

typedef struct {
	GtkWidget *box;
	GtkWidget *label;
	GtkWidget *closeButton;
} FilterTag;

static FilterTag createFilterTag(const char *text) {
	GtkWidget *label = gtk_label_new(text);
	gtk_widget_add_css_class(label, "chip-text");

	GtkWidget *close = gtk_label_new("✕");
	gtk_widget_add_css_class(close, "chip-x");
	GtkWidget *closeButton = gtk_button_new();
	gtk_widget_add_css_class(closeButton, "chip-button");
	gtk_widget_set_parent(close, closeButton);

	GtkWidget *box = gtk_box_new(0, 4);
	gtk_widget_add_css_class(box, "chip");
	gtk_box_append(GTK_BOX(box), label);
	gtk_box_append(GTK_BOX(box), closeButton);

	GtkBox *parent = GTK_BOX(gtk_builder_get_object(gameContext.builder, "box:filter-tags"));
	gtk_box_append(parent, box);

	return (FilterTag){.box = box, .label = label, .closeButton = closeButton};
}

void ui_createFilterTag(const char *text, GtkEntryBuffer *buffer) {
	const FilterTag tag = createFilterTag(text);
	g_signal_connect(tag.closeButton, "clicked", G_CALLBACK(onFilterTagClick), buffer);
}

void ui_createClubFilterTag(const char *text, GtkEditable *buffer) {
	const FilterTag tag = createFilterTag(text);
	g_signal_connect(tag.closeButton, "clicked", G_CALLBACK(onClubFilterTagClick), buffer);
	gtk_widget_set_name(tag.label, "tag:club-name");
}

void ui_clearFilterTags(void) {
	GtkBox *filterTags = GTK_BOX(gtk_builder_get_object(gameContext.builder, "box:filter-tags"));
	GtkWidget *child;
	while ((child = gtk_widget_get_first_child(GTK_WIDGET(filterTags))) != NULL) {
		gtk_box_remove(filterTags, GTK_WIDGET(child));
	}
}

static void onTagClick(GtkWidget *self) {
	GtkWidget *box = gtk_widget_get_parent(self);
	GtkWidget *parent = gtk_widget_get_parent(box);
	GtkWidget *closeLabel = gtk_widget_get_first_child(self);
	if (closeLabel != NULL) {
		gtk_widget_unparent(closeLabel);
	}

	gtk_box_remove(GTK_BOX(parent), box);

	searchHandler_cacheFilters();
	if (gameContext.filterOptions.filterMask) {
		searchHandler_doSearch(false);
	} else {
		playerTable_clear();
	}
}

static void onClubFilterTagClick(GtkWidget *self, GtkEditable *buffer) {
	gtk_editable_set_text(buffer, "");
	onTagClick(self);
}

static void onFilterTagClick(GtkWidget *self, GtkEntryBuffer *buffer) {
	gtk_entry_buffer_set_text(buffer, "", 1);
	onTagClick(self);
}


static void loadStylesheet(const char *fileName) {
	char pathToStylesheet[256] = {0};
	GtkCssProvider *provider = gtk_css_provider_new();

	snprintf(pathToStylesheet, sizeof(pathToStylesheet), RESOURCE_BASE "/layouts/%s", fileName);
	gtk_css_provider_load_from_resource(provider, pathToStylesheet);
	gtk_style_context_add_provider_for_display(
		gdk_display_get_default(),
		GTK_STYLE_PROVIDER(provider),
		GTK_STYLE_PROVIDER_PRIORITY_APPLICATION
	);
	g_object_unref(provider);
}
