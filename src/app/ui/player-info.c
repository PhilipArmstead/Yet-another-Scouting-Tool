// SPDX-FileCopyrightText: © 2026 Phil Armstead <philarmstead@mailbox.org>
// SPDX-License-Identifier: GPL-3.0-or-later

#include "app/maths.h"
#include "app/player.h"
#include "app/ui.h"


extern GameContext gameContext;

WindowContext ui_createPlayerInfoWindow(void) {
	const WindowContext context = openWindow("player-info", "window:player-info");
	gtk_window_set_default_size(GTK_WINDOW(context.window), 420, 900);

	return context;
}

void ui_renderPlayerInfoWindow(WindowContext context, const Player *player) {
	// Player name
	GtkLabel *commonNameLabel = GTK_LABEL(GTK_WIDGET(gtk_builder_get_object(context.builder, "label:common-name")));
	if (player->commonName[0] == '\0') {
		char buffer[PERSON_FORENAME_LENGTH + PERSON_SURNAME_LENGTH + 2];
		snprintf(buffer, sizeof(buffer), "%s %s", player->forename, player->surname);
		gtk_label_set_text(commonNameLabel, buffer);
	} else {
		gtk_label_set_text(commonNameLabel, player->commonName);
	}

	// Player age
	{
		char buffer[8];
		snprintf(buffer, 8, "%d yrs", player->age);
		GtkLabel *ageLabel = GTK_LABEL(GTK_WIDGET(gtk_builder_get_object(context.builder, "label:age")));
		gtk_label_set_text(ageLabel, buffer);
	}

	// Player status
	GtkWidget *isFastLearner = GTK_WIDGET(gtk_builder_get_object(context.builder, "widget:is-fast-learner"));
	gtk_widget_set_visible(isFastLearner, player->canDevelopQuickly);
	GtkWidget *isHotProspect = GTK_WIDGET(gtk_builder_get_object(context.builder, "widget:is-hot-prospect"));
	gtk_widget_set_visible(isHotProspect, player->isHotProspect);


	// Player nationalities
	uint8_t nationalityIndex = 0;
	while (nationalityIndex < 4 && player->nationality[nationalityIndex] != 0xFF) {
		GtkBox *nationalityBox = GTK_BOX(GTK_WIDGET(gtk_builder_get_object(context.builder, "label:nationality")));
		char pathToFlag[256] = {0};
		const Nation nation = gameContext.nations[player->nationality[nationalityIndex]];
		snprintf(
			pathToFlag,
			sizeof(pathToFlag),
			RESOURCE_BASE "/assets/flags/%s.png",
			nation.code
		);
		GtkWidget *flagImage = gtk_image_new_from_resource(pathToFlag);
		gtk_box_append(nationalityBox, flagImage);
		gtk_widget_set_tooltip_text(flagImage, nation.name);
		nationalityIndex++;
	}

	// Club name
	GtkLabel *clubNameLabel = GTK_LABEL(GTK_WIDGET(gtk_builder_get_object(context.builder, "label:club-name")));
	if (player->clubIndex == -1) {
		gtk_label_set_text(clubNameLabel, "Free agent");
	} else {
		const Club club = gameContext.clubs[player->clubIndex];
		gtk_label_set_text(clubNameLabel, club.name);
	}

	float weights[ATTRIBUTE_COUNT];
	float scale = 1.0f;
	getWeightsForPosition(player->ratings[0].position, weights, &scale);
	float max = 0;
	for (int i = 0; i < ATTRIBUTE_COUNT; ++i) {
		if (weights[i] > max) {
			max = weights[i];
		}
	}

	char buffer[8] = {0};
	char widgetId[64];
	GtkLabel *label;
	GtkWidget *widget;
	#define SET_ROW_TEXT_AND_HIGHLIGHT(id, attributeIndex) {																	\
		snprintf(buffer, 8, "%d", convertTo20Scale(player->attributes[attributeIndex]));		\
		snprintf(widgetId, 64, "label:%s", id);																							\
		label = GTK_LABEL(GTK_WIDGET(gtk_builder_get_object(context.builder, widgetId)));		\
		gtk_label_set_text(label, buffer);																									\
		snprintf(widgetId, 64, "row:%s", id);																								\
		widget = GTK_WIDGET(gtk_builder_get_object(context.builder, widgetId));							\
		if (weights[attributeIndex] > max * 0.5f) {																					\
			gtk_widget_add_css_class(widget, "attribute-row--high");													\
		} else if (weights[attributeIndex] > max * 0.15f) {																	\
			gtk_widget_add_css_class(widget, "attribute-row--mid");														\
		}																																										\
	}

	// Ability scores
	{
		GtkLabel *caLabel = GTK_LABEL(GTK_WIDGET(gtk_builder_get_object(context.builder, "label:ca")));
		GtkLabel *paLabel = GTK_LABEL(GTK_WIDGET(gtk_builder_get_object(context.builder, "label:pa")));
		char ability[4] = {0};
		snprintf(ability, 4, "%d", player->ca);
		gtk_label_set_label(caLabel, ability);
		snprintf(ability, 4, "%d", player->pa);
		gtk_label_set_label(paLabel, ability);
	}

	// Attributes
	GtkWidget *boxGoalkeeper = GTK_WIDGET(gtk_builder_get_object(context.builder, "box:attribute:goalkeeper"));
	GtkWidget *boxTechnical = GTK_WIDGET(gtk_builder_get_object(context.builder, "box:attribute:technical"));

	if (player->positions[0] < 12) {
		gtk_widget_set_visible(boxTechnical, true);
		gtk_widget_set_visible(boxGoalkeeper, false);

		SET_ROW_TEXT_AND_HIGHLIGHT("attribute:technical:corners", ATTR_COR);
		SET_ROW_TEXT_AND_HIGHLIGHT("attribute:technical:crossing", ATTR_CRO);
		SET_ROW_TEXT_AND_HIGHLIGHT("attribute:technical:dribbling", ATTR_DRI);
		SET_ROW_TEXT_AND_HIGHLIGHT("attribute:technical:finishing", ATTR_FIN);
		SET_ROW_TEXT_AND_HIGHLIGHT("attribute:technical:first-touch", ATTR_FIR);
		SET_ROW_TEXT_AND_HIGHLIGHT("attribute:technical:free-kicks", ATTR_FRE);
		SET_ROW_TEXT_AND_HIGHLIGHT("attribute:technical:heading", ATTR_HEA);
		SET_ROW_TEXT_AND_HIGHLIGHT("attribute:technical:long-shots", ATTR_LON);
		SET_ROW_TEXT_AND_HIGHLIGHT("attribute:technical:long-throws", ATTR_LTH);
		SET_ROW_TEXT_AND_HIGHLIGHT("attribute:technical:marking", ATTR_MAR);
		SET_ROW_TEXT_AND_HIGHLIGHT("attribute:technical:passing", ATTR_PAS);
		SET_ROW_TEXT_AND_HIGHLIGHT("attribute:technical:penalty-taking", ATTR_PEN);
		SET_ROW_TEXT_AND_HIGHLIGHT("attribute:technical:tackling", ATTR_TCK);
		SET_ROW_TEXT_AND_HIGHLIGHT("attribute:technical:technique", ATTR_TEC);
	} else {
		gtk_widget_set_visible(boxGoalkeeper, true);
		gtk_widget_set_visible(boxTechnical, false);

		SET_ROW_TEXT_AND_HIGHLIGHT("attribute:goalkeeper:aerial-reach", ATTR_AER);
		SET_ROW_TEXT_AND_HIGHLIGHT("attribute:goalkeeper:command-of-area", ATTR_CMD);
		SET_ROW_TEXT_AND_HIGHLIGHT("attribute:goalkeeper:communication", ATTR_COM);
		SET_ROW_TEXT_AND_HIGHLIGHT("attribute:goalkeeper:eccentricity", ATTR_ECC);
		SET_ROW_TEXT_AND_HIGHLIGHT("attribute:goalkeeper:first-touch", ATTR_FIR);
		SET_ROW_TEXT_AND_HIGHLIGHT("attribute:goalkeeper:handling", ATTR_HAN);
		SET_ROW_TEXT_AND_HIGHLIGHT("attribute:goalkeeper:kicking", ATTR_KIC);
		SET_ROW_TEXT_AND_HIGHLIGHT("attribute:goalkeeper:one-on-ones", ATTR_ONE);
		SET_ROW_TEXT_AND_HIGHLIGHT("attribute:goalkeeper:passing", ATTR_PAS);
		SET_ROW_TEXT_AND_HIGHLIGHT("attribute:goalkeeper:punching-tendency", ATTR_TTP);
		SET_ROW_TEXT_AND_HIGHLIGHT("attribute:goalkeeper:reflexes", ATTR_REF);
		SET_ROW_TEXT_AND_HIGHLIGHT("attribute:goalkeeper:rushing-out-tendency", ATTR_TRO);
		SET_ROW_TEXT_AND_HIGHLIGHT("attribute:goalkeeper:throwing", ATTR_THR);
	}

	SET_ROW_TEXT_AND_HIGHLIGHT("attribute:mental:aggression", ATTR_AGG);
	SET_ROW_TEXT_AND_HIGHLIGHT("attribute:mental:anticipation", ATTR_ANT);
	SET_ROW_TEXT_AND_HIGHLIGHT("attribute:mental:bravery", ATTR_BRA);
	SET_ROW_TEXT_AND_HIGHLIGHT("attribute:mental:concentration", ATTR_CNT);
	SET_ROW_TEXT_AND_HIGHLIGHT("attribute:mental:composure", ATTR_CMP);
	SET_ROW_TEXT_AND_HIGHLIGHT("attribute:mental:decisions", ATTR_DEC);
	SET_ROW_TEXT_AND_HIGHLIGHT("attribute:mental:determination", ATTR_DET);
	SET_ROW_TEXT_AND_HIGHLIGHT("attribute:mental:flair", ATTR_FLA);
	SET_ROW_TEXT_AND_HIGHLIGHT("attribute:mental:leadership", ATTR_LDR);
	SET_ROW_TEXT_AND_HIGHLIGHT("attribute:mental:off-the-ball", ATTR_OTB);
	SET_ROW_TEXT_AND_HIGHLIGHT("attribute:mental:positioning", ATTR_POS);
	SET_ROW_TEXT_AND_HIGHLIGHT("attribute:mental:teamwork", ATTR_TEA);
	SET_ROW_TEXT_AND_HIGHLIGHT("attribute:mental:vision", ATTR_VIS);
	SET_ROW_TEXT_AND_HIGHLIGHT("attribute:mental:work-rate", ATTR_WOR);

	SET_ROW_TEXT_AND_HIGHLIGHT("attribute:physical:acceleration", ATTR_ACC);
	SET_ROW_TEXT_AND_HIGHLIGHT("attribute:physical:agility", ATTR_AGI);
	SET_ROW_TEXT_AND_HIGHLIGHT("attribute:physical:balance", ATTR_BAL);
	SET_ROW_TEXT_AND_HIGHLIGHT("attribute:physical:jumping-reach", ATTR_JUM);
	SET_ROW_TEXT_AND_HIGHLIGHT("attribute:physical:natural-fitness", ATTR_NAT);
	SET_ROW_TEXT_AND_HIGHLIGHT("attribute:physical:pace", ATTR_PAC);
	SET_ROW_TEXT_AND_HIGHLIGHT("attribute:physical:stamina", ATTR_STA);
	SET_ROW_TEXT_AND_HIGHLIGHT("attribute:physical:strength", ATTR_STR);

	SET_ROW_TEXT_AND_HIGHLIGHT("attribute:hidden:adaptability", ATTR_ADA);
	SET_ROW_TEXT_AND_HIGHLIGHT("attribute:hidden:ambition", ATTR_AMB);
	SET_ROW_TEXT_AND_HIGHLIGHT("attribute:hidden:consistency", ATTR_CON);
	SET_ROW_TEXT_AND_HIGHLIGHT("attribute:hidden:controversy", ATTR_CNY);
	SET_ROW_TEXT_AND_HIGHLIGHT("attribute:hidden:dirtiness", ATTR_DIR);
	SET_ROW_TEXT_AND_HIGHLIGHT("attribute:hidden:important-matches", ATTR_IMP);
	SET_ROW_TEXT_AND_HIGHLIGHT("attribute:hidden:injury-proneness", ATTR_INJ);
	SET_ROW_TEXT_AND_HIGHLIGHT("attribute:hidden:loyalty", ATTR_LOY);
	SET_ROW_TEXT_AND_HIGHLIGHT("attribute:hidden:pressure", ATTR_PRE);
	SET_ROW_TEXT_AND_HIGHLIGHT("attribute:hidden:professionalism", ATTR_PRO);
	SET_ROW_TEXT_AND_HIGHLIGHT("attribute:hidden:sportsmanship", ATTR_SPO);
	SET_ROW_TEXT_AND_HIGHLIGHT("attribute:hidden:temperament", ATTR_TEM);
	SET_ROW_TEXT_AND_HIGHLIGHT("attribute:hidden:versatility", ATTR_VER);

	// Ratings
	{
		GtkBox *boxRoles = GTK_BOX(GTK_WIDGET(gtk_builder_get_object(context.builder, "box:top-roles")));
		uint8_t i = 0;
		while (i < POSITION_GROUPED_COUNT && player->ratings[i].value > 0.f) {
			GtkWidget *parent = gtk_box_new(GTK_ORIENTATION_VERTICAL, 2);
			gtk_box_append(boxRoles, parent);

			GtkWidget *labelContainer = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
			GtkWidget *roleLabel = gtk_label_new(positionGroupedNames[player->ratings[i].position]);
			gtk_label_set_xalign(GTK_LABEL(roleLabel), 0);
			gtk_widget_add_css_class(roleLabel, "heading");
			gtk_widget_set_hexpand(roleLabel, true);
			gtk_box_append(GTK_BOX(labelContainer), roleLabel);

			char valueBuffer[8] = {0};
			snprintf(valueBuffer, 8, "%.1f", player->ratings[i].value);
			GtkWidget *roleValue = gtk_label_new(valueBuffer);
			gtk_widget_add_css_class(roleValue, "heading");
			gtk_widget_add_css_class(roleValue, "accent-orange");
			gtk_widget_add_css_class(roleValue, "role-value");
			gtk_box_append(GTK_BOX(labelContainer), roleValue);

			GtkWidget *progressBar = gtk_progress_bar_new();
			gtk_progress_bar_set_fraction(GTK_PROGRESS_BAR(progressBar), player->ratings[i].value / 100.0f);
			gtk_widget_add_css_class(progressBar, "role-bar");

			gtk_box_append(GTK_BOX(parent), labelContainer);
			gtk_box_append(GTK_BOX(parent), progressBar);

			i++;
		}
	}
}
