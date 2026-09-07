// SPDX-FileCopyrightText: © 2026 Phil Armstead <philarmstead@mailbox.org>
// SPDX-License-Identifier: GPL-3.0-or-later

#include "ui.h"
#include "app/callbacks.h"
#include "app/config.h"
#include "app/data.h"
#include "app/game-status.h"
#include "app/search-handler.h"
#include "app/helpers/date.h"
#include "core/logger.h"
#include "platform/platform.h"

#include <gtk/gtk.h>

#include "player-table.h"


extern ProcessContext processContext;
extern GameContext gameContext;

static void handleDisconnect(void);
static void handleConnect(void);
static inline void updateWhileConnected(void);
static inline void updateWhileDisconnected(void);
static void onFilterTagClick(GtkWidget *self, GtkEntryBuffer *buffer);
static void onClubFilterTagClick(GtkWidget *self, GtkEditable *buffer);
static void loadStylesheet(const char *fileName);

void ui_init(GtkApplication *app) {
	loadStylesheet("styles.css");

	// Show main window
	const WindowContext context = openWindow("show-players", "window:show-players");
	gameContext.builder = context.builder;
	gtk_window_set_application(GTK_WINDOW(context.window), GTK_APPLICATION(app));

	#ifdef MOCKS_MODE
	// In mock mode, we never have the process-connected callback run
	runMultiThreadedCache();
	#endif

	// Periodic callbacks
	g_timeout_add(1000, update, NULL);
	update(NULL);

	// Create datalist box
	SearchDatalist *dataList = g_new0(SearchDatalist, 1);

	dataList->entry = GTK_SEARCH_ENTRY(gtk_search_entry_new());
	dataList->popover = GTK_POPOVER(gtk_popover_new());
	dataList->listBox = GTK_LIST_BOX(gtk_list_box_new());

	gtk_search_entry_set_placeholder_text(dataList->entry, "Search club");
	gtk_widget_add_css_class(GTK_WIDGET(dataList->entry), "sidebar-search");

	gtk_popover_set_child(dataList->popover, GTK_WIDGET(dataList->listBox));
	gtk_widget_set_parent(GTK_WIDGET(dataList->popover), GTK_WIDGET(dataList->entry));

	gtk_popover_set_pointing_to(dataList->popover, &(GdkRectangle){102, 27, 1, 1});
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

	// Set cursor pointer on filter accordions
	gtk_widget_set_cursor_from_name(
		GTK_WIDGET(gtk_builder_get_object(b, "accordion-button:positions")),
		"pointer"
	);
	gtk_widget_set_cursor_from_name(
		GTK_WIDGET(gtk_builder_get_object(b, "accordion-button:quality-and-age")),
		"pointer"
	);
}

void connectToProcess(void) {
	clearCaches();
	platform_openProcess(&processContext);

	if (processContext.handle != NULL) {
		handleConnect();
	}
}

gboolean update(gpointer userData) {
	(void)userData;

	#ifndef MOCKS_MODE
	if (processContext.handle != NULL) {
		updateWhileConnected();
	} else {
		updateWhileDisconnected();
	}
	#else
	updateWhileConnected();
	#endif
	return G_SOURCE_CONTINUE;
}

void ui_update(void) {
	updateGameStatus();
}

static inline void updateWhileConnected(void) {
	// Get the current time/date
	DayMonthYear dayMonthYear = getDayMonthYear(&processContext);

	// Bail if the date is invalid and assume we're no longer connected
	if (dayMonthYear.day == 0 || dayMonthYear.year == 0) {
		handleDisconnect();
		return;
	}

	// Cache the new date if it's different from the last one we saw
	if (
		dayMonthYear.day != gameContext.currentDate.day ||
		dayMonthYear.year != gameContext.currentDate.year ||
		strncmp(dayMonthYear.month, gameContext.currentDate.month, MONTH_NAME_LENGTH) != 0
	) {
		gameContext.currentDate = dayMonthYear;
		LOG_INFO(
			"Current Date: %s %d, %d",
			dayMonthYear.month,
			dayMonthYear.day,
			dayMonthYear.year
		);

		updateInGameDate();
	}

	// Update the game version if it's different from the last one we saw
	char versionBuffer[GAME_STATUS_STRING_BUFFER_SIZE] = {0};
	getGameVersion(&processContext, versionBuffer, GAME_STATUS_STRING_BUFFER_SIZE);
	if (strncmp(versionBuffer, gameContext.gameVersion, GAME_STATUS_STRING_BUFFER_SIZE) != 0) {
		strncpy(gameContext.gameVersion, versionBuffer, GAME_STATUS_STRING_BUFFER_SIZE);
		LOG_INFO("Game Version: %s", versionBuffer);

		updateGameStatus();
	}
}

static inline void updateWhileDisconnected(void) {
	connectToProcess();
}

void updateInGameDate(void) {
	GtkLabel *dateLabel = GTK_LABEL(GTK_WIDGET(gtk_builder_get_object(gameContext.builder, "label:date")));

	#ifndef MOCKS_MODE
	if (processContext.handle == NULL || gameContext.currentDate.year <= 1970) {
		gtk_label_set_text(dateLabel, "");
		return;
	}
	#endif

	char buffer[64] = {0};
	snprintf(
		buffer,
		64,
		"📅 %s %d%s, %d",
		gameContext.currentDate.month,
		gameContext.currentDate.day,
		getOrdinal(gameContext.currentDate.day),
		gameContext.currentDate.year
	);
	gtk_label_set_text(dateLabel, buffer);
}

void updateGameStatus(void) {
	GtkLabel *versionLabel = GTK_LABEL(GTK_WIDGET(gtk_builder_get_object(gameContext.builder, "label:status")));

	#ifndef MOCKS_MODE
	if (processContext.handle == NULL) {
		gtk_label_set_text(versionLabel, "Not connected");
		return;
	}
	#endif

	if (gameContext.gameVersion[0] == '\0') {
		gtk_label_set_text(versionLabel, "\0");
		return;
	}

	char buffer[64] = {0};
	snprintf(buffer, 64, "📝 %s", gameContext.gameVersion);
	gtk_label_set_text(versionLabel, buffer);
}

WindowContext openWindow(const char *layoutName, const char *windowName) {
	char pathToAppLayout[256] = {0};
	snprintf(pathToAppLayout, sizeof(pathToAppLayout), RESOURCE_BASE "/layouts/%s.ui", layoutName);

	WindowContext context = {0};
	context.builder = gtk_builder_new_from_resource(pathToAppLayout);
	context.window = GTK_WIDGET(gtk_builder_get_object(context.builder, windowName));

	GtkEventControllerKey *closeController = GTK_EVENT_CONTROLLER_KEY(gtk_event_controller_key_new());
	gtk_event_controller_set_propagation_phase(GTK_EVENT_CONTROLLER(closeController), GTK_PHASE_CAPTURE);
	g_signal_connect(closeController, "key-pressed", G_CALLBACK(callbacks_onWindowKeypress), context.window);
	gtk_widget_add_controller(context.window, GTK_EVENT_CONTROLLER(closeController));

	gtk_window_present(GTK_WINDOW(context.window));
	return context;
}

static void handleDisconnect(void) {
	processContext.handle = NULL;

	gameContext.gameVersion[0] = '\0';
	gameContext.currentDate = (DayMonthYear){0};

	ui_update();
}

static void handleConnect(void) {
	LOG_INFO(
		"Process opened with PID: %u (base module address of %p)",
		processContext.pid,
		(void*)processContext.moduleBaseAddress
	);
	ui_update();
	update(NULL);

	runMultiThreadedCache();
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

void onClubFilterTagClick(GtkWidget *self, GtkEditable *buffer) {
	gtk_editable_set_text(buffer, "");
	onTagClick(self);
}

void onFilterTagClick(GtkWidget *self, GtkEntryBuffer *buffer) {
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
