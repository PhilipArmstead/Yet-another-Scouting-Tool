// SPDX-FileCopyrightText: © 2026 Phil Armstead <philarmstead@mailbox.org>
// SPDX-License-Identifier: GPL-3.0-or-later

#include "app/ui.h"
#include "app/helpers/formatter.h"
#include "app/helpers/vector-shared-pointer.h"
#include "app/helpers/vector.h"
#include "core/logger.h"
#include "platform/platform.h"


typedef struct {
	uint8_t minAge;
	uint8_t maxAge;
	uint16_t minCondition;
	uint16_t maxCondition;
} BestElevenFilters;

extern GameContext gameContext;

#define INFEASIBLE_COST 1e9f

static float getPlayerPositionalRating(const Player *player, PositionCode position);
static void hungarian(float **matrix, uint32_t n, uint32_t *outAssignment);
static void assignFormation(
	const uint32_t *playerIds,
	const PositionCode positions[FORMATION_POSITION_COUNT],
	BestElevenFilters filters,
	BestElevenRow outSlots[FORMATION_POSITION_COUNT]
);
static void renderBestElevenTable(WindowContext context);
static void onFormationSelected(GObject *object, GParamSpec *pspec, gpointer userData);
static void onFilterChange(GObject *object, gpointer userData);

WindowContext ui_createBestElevenWindow(void) {
	const WindowContext context = openWindow("best-xi", "window:best-xi");
	gtk_window_set_default_size(GTK_WINDOW(context.window), 420, 900);

	SharedPointer *snapshot = gameContext.searchResults;
	sharedPointer_ref(snapshot);
	g_object_set_data_full(
		G_OBJECT(context.window),
		"best-xi:search-results",
		snapshot,
		(GDestroyNotify)sharedPointer_unref
	);

	GtkDropDown *formationDropDown = GTK_DROP_DOWN(gtk_builder_get_object(context.builder, "dropdown:formation"));

	// Heap allocate WindowContext so I can get it from inside this callback
	WindowContext *cbContext = g_new(WindowContext, 1);
	*cbContext = context; // copy the two pointers
	g_signal_connect_data(
		formationDropDown,
		"notify::selected",
		G_CALLBACK(onFormationSelected),
		cbContext,
		(GClosureNotify)g_free,
		0
	);

	GtkSpinButton *spinMinAge = GTK_SPIN_BUTTON(gtk_builder_get_object(context.builder, "spin:min-age"));
	GtkSpinButton *spinMaxAge = GTK_SPIN_BUTTON(gtk_builder_get_object(context.builder, "spin:max-age"));
	GtkSpinButton *spinMinCondition = GTK_SPIN_BUTTON(gtk_builder_get_object(context.builder, "spin:min-condition"));
	GtkSpinButton *spinMaxCondition = GTK_SPIN_BUTTON(gtk_builder_get_object(context.builder, "spin:max-condition"));
	g_signal_connect_data(spinMinAge, "value-changed",G_CALLBACK(onFilterChange), cbContext, (GClosureNotify)g_free, 0);
	g_signal_connect_data(spinMaxAge, "value-changed",G_CALLBACK(onFilterChange), cbContext, (GClosureNotify)g_free, 0);
	g_signal_connect_data(
		spinMinCondition,
		"value-changed",
		G_CALLBACK(onFilterChange),
		cbContext,
		(GClosureNotify)g_free,
		0
	);
	g_signal_connect_data(
		spinMaxCondition,
		"value-changed",
		G_CALLBACK(onFilterChange),
		cbContext,
		(GClosureNotify)g_free,
		0
	);

	return context;
}

void ui_renderBestElevenWindow(const WindowContext context) {
	GtkStringList *formationList = GTK_STRING_LIST(gtk_builder_get_object(context.builder, "string-list:formation"));
	while (gtk_string_list_get_string(formationList, 0) != NULL)
		gtk_string_list_remove(formationList, 0);
	for (uint8_t i = 0; i < vector_length(gameContext.options.formations); ++i)
		gtk_string_list_append(formationList, gameContext.options.formations[i].name);
}

static void renderBestElevenTable(const WindowContext context) {
	GtkDropDown *formationDropDown = GTK_DROP_DOWN(gtk_builder_get_object(context.builder, "dropdown:formation"));
	uint32_t selectedFormationIndex = gtk_drop_down_get_selected(formationDropDown);
	if (selectedFormationIndex == GTK_INVALID_LIST_POSITION) {
		selectedFormationIndex = 0;
	}
	const Formation formation = gameContext.options.formations[selectedFormationIndex];

	const SharedPointer *snapshot = g_object_get_data(G_OBJECT(context.window), "best-xi:search-results");

	GtkListBox *listBox = GTK_LIST_BOX(gtk_builder_get_object(context.builder, "list-box:best-xi"));
	gtk_list_box_remove_all(listBox);

	const uint32_t *playerIds = snapshot ? snapshot->data : NULL;
	const size_t playerCount = playerIds ? vector_length(playerIds) : 0;

	GtkSpinButton *spinMinAge = GTK_SPIN_BUTTON(gtk_builder_get_object(context.builder, "spin:min-age"));
	GtkSpinButton *spinMaxAge = GTK_SPIN_BUTTON(gtk_builder_get_object(context.builder, "spin:max-age"));
	GtkSpinButton *spinMinCondition = GTK_SPIN_BUTTON(gtk_builder_get_object(context.builder, "spin:min-condition"));
	GtkSpinButton *spinMaxCondition = GTK_SPIN_BUTTON(gtk_builder_get_object(context.builder, "spin:max-condition"));

	const BestElevenFilters filters = {
		.maxAge = (uint8_t)gtk_spin_button_get_value_as_int(spinMaxAge),
		.minAge = (uint8_t)gtk_spin_button_get_value_as_int(spinMinAge),
		// Convert percentages to 0-10000 scale
		.minCondition = (uint16_t)gtk_spin_button_get_value_as_int(spinMinCondition) * 100,
		.maxCondition = (uint16_t)gtk_spin_button_get_value_as_int(spinMaxCondition) * 100,
	};

	BestElevenRow rows[FORMATION_POSITION_COUNT] = {0};
	const int64_t timeStart = platform_getMicroseconds();
	assignFormation(playerIds, formation.positions, filters, rows);
	const int64_t timeEnd = platform_getMicroseconds();
	LOG_DEBUG("Found best XI for %d players in %zu microseconds", playerCount, timeEnd - timeStart);

	for (uint8_t i = 0; i < FORMATION_POSITION_COUNT; ++i) {
		GtkWidget *widgetRow = gtk_list_box_row_new();
		GtkListBoxRow *row = GTK_LIST_BOX_ROW(widgetRow);
		gtk_list_box_row_set_activatable(row, false);
		gtk_list_box_row_set_selectable(row, false);
		gtk_list_box_append(listBox, widgetRow);

		GtkWidget *widgetGrid = gtk_grid_new();
		GtkGrid *grid = GTK_GRID(widgetGrid);
		gtk_grid_set_column_spacing(grid, 12);


		GtkWidget *widgetLabelPosition = gtk_label_new(positionCodeNames[formation.positions[i]]);
		gtk_label_set_yalign(GTK_LABEL(widgetLabelPosition), GTK_ALIGN_CENTER);
		gtk_widget_add_css_class(widgetLabelPosition, "position");

		GtkWidget *widgetLabelPlayer = gtk_label_new("");
		GtkWidget *widgetLabelRating = gtk_label_new("");

		const Player *player = rows[i].player;
		if (player != NULL) {
			gtk_widget_set_hexpand(widgetLabelPlayer, true);
			gtk_label_set_xalign(GTK_LABEL(widgetLabelPlayer), 0);
			gtk_label_set_yalign(GTK_LABEL(widgetLabelPlayer), GTK_ALIGN_CENTER);
			gtk_widget_add_css_class(widgetLabelPlayer, "name");

			if (player->commonName[0] == '\0') {
				char buffer[PERSON_FORENAME_LENGTH + PERSON_SURNAME_LENGTH + 2];
				snprintf(buffer, sizeof(buffer), "%s %s", player->forename, player->surname);
				gtk_label_set_text(GTK_LABEL(widgetLabelPlayer), buffer);
			} else {
				gtk_label_set_text(GTK_LABEL(widgetLabelPlayer), player->commonName);
			}

			char ratingBuffer[44];
			formatter_formatRating(player->ratings[0].value, ratingBuffer);
			gtk_label_set_markup(GTK_LABEL(widgetLabelRating), ratingBuffer);
			gtk_label_set_xalign(GTK_LABEL(widgetLabelRating), 1.f);
		}

		gtk_grid_attach(grid, widgetLabelPosition, 1, i, 1, 1);
		gtk_grid_attach(grid, widgetLabelPlayer, 2, i, 1, 1);
		gtk_grid_attach(grid, widgetLabelRating, 3, i, 1, 1);
		gtk_list_box_row_set_child(row, widgetGrid);

		// TODO: show age/condition, if we're going to filter on them
	}
}

static void onFormationSelected(GObject *object, GParamSpec *pspec, gpointer userData) {
	(void)object;
	(void)pspec;
	renderBestElevenTable(*(const WindowContext*)userData);
}

static void onFilterChange(GObject *object, gpointer userData) {
	(void)object;
	renderBestElevenTable(*(const WindowContext*)userData);
}

/**
 * O(n³) Hungarian algorithm (shortest-augmenting-path / Jonker–Volgenant variant).
 * Solves the square minimisation assignment problem.
 * @param matrix n×n cost matrix (1-indexed internally)
 * @param n size of matrix
 * @param outAssignment array of indices
 * @returns outAssignment[row] = col (both 0-indexed)
 */
static void hungarian(float **matrix, const uint32_t n, uint32_t *outAssignment) {
	// u[i] = potential for row i (1-indexed), v[j] = potential for col j (1-indexed)
	float *u = calloc(n + 1, sizeof(float));
	float *v = calloc(n + 1, sizeof(float));
	// p[j] = row assigned to column j (1-indexed); p[0] is a sentinel
	uint32_t *p = calloc(n + 1, sizeof(uint32_t));
	int64_t *way = calloc(n + 1, sizeof(int64_t));


	for (uint32_t i = 0; i < n; i++) {
		p[0] = i + 1;
		int64_t j0 = 0;
		float *minDist = malloc((n + 1) * sizeof(float));
		bool *used = calloc(n + 1, sizeof(bool));
		for (uint32_t j = 0; j < n; ++j) {
			minDist[j] = INFINITY;
		}

		do {
			used[j0] = true;
			const uint32_t i0 = p[j0];
			float delta = INFINITY;
			int64_t j1 = -1;

			for (uint32_t j = 1; j <= n; j++) {
				if (!used[j]) {
					const float cur = matrix[i0 - 1][j - 1] - u[i0] - v[j];
					if (cur < minDist[j]) {
						minDist[j] = cur;
						way[j] = j0;
					}
					if (minDist[j] < delta) {
						delta = minDist[j];
						j1 = (int64_t)j;
					}
				}
			}

			for (uint32_t j = 0; j <= n; j++) {
				if (used[j]) {
					u[p[j]] += delta;
					v[j] -= delta;
				} else {
					minDist[j] -= delta;
				}
			}

			j0 = j1;
		} while (p[j0] != 0);

		do {
			const int64_t j1 = way[j0];
			p[j0] = p[j1];
			j0 = j1;
		} while (j0);

		free(used);
		free(minDist);
	}

	// Invert p: assignment[row] = col (0-indexed)
	for (uint32_t j = 1; j <= n; j++) {
		if (p[j] != 0) {
			outAssignment[p[j] - 1] = j - 1;
		}
	}

	free(way);
	free(p);
	free(v);
	free(u);
}

/**
 * Returns a player's rating for the grouped role that owns `position`, or
 * INFEASIBLE_COST if the player is not sufficiently proficient in `position`
 * or has no rating for the corresponding role. Higher is better.
 */
static float getPlayerPositionalRating(const Player *player, const PositionCode position) {
	if (player->positions[position] < MINIMUM_POSITIONAL_PROFICIENCY) {
		return INFEASIBLE_COST;
	}

	PositionGrouped groupedPosition;
	switch (position) {
		case POSITION_CODE_GK:
			groupedPosition = POSITION_GROUPED_GK;
			break;
		case POSITION_CODE_DL:
		case POSITION_CODE_DR:
			groupedPosition = POSITION_GROUPED_FB;
			break;
		case POSITION_CODE_WBL:
		case POSITION_CODE_WBR:
			groupedPosition = POSITION_GROUPED_WB;
			break;
		case POSITION_CODE_DC:
			groupedPosition = POSITION_GROUPED_CB;
			break;
		case POSITION_CODE_DM:
			groupedPosition = POSITION_GROUPED_DM;
			break;
		case POSITION_CODE_ML:
		case POSITION_CODE_AML:
		case POSITION_CODE_MR:
		case POSITION_CODE_AMR:
			groupedPosition = POSITION_GROUPED_W;
			break;
		case POSITION_CODE_AMC:
			groupedPosition = POSITION_GROUPED_AM;
			break;
		default:
			groupedPosition = POSITION_GROUPED_ST;
	}

	for (PositionGrouped role = 0; role < POSITION_GROUPED_COUNT; ++role) {
		if (player->ratings[role].position == groupedPosition) {
			return player->ratings[role].value;
		}
	}

	return INFEASIBLE_COST;
}

/**
 * Inserts (searchIndex, rating) into a rating-descending top-K list held in
 * topIndices/topValues, keeping at most `capacity` best entries. `*count` is the
 * current size. `searchIndex` is an index into the caller's playerIds array.
 */
static void insertTopK(
	uint32_t *topIndices,
	float *topValues,
	uint8_t *count,
	const uint8_t capacity,
	const uint32_t searchIndex,
	const float rating
) {
	if (*count < capacity) {
		uint8_t pos = (*count)++;
		while (pos > 0 && topValues[pos - 1] < rating) {
			topValues[pos] = topValues[pos - 1];
			topIndices[pos] = topIndices[pos - 1];
			--pos;
		}
		topValues[pos] = rating;
		topIndices[pos] = searchIndex;
	} else if (capacity > 0 && rating > topValues[capacity - 1]) {
		uint8_t pos = capacity - 1;
		while (pos > 0 && topValues[pos - 1] < rating) {
			topValues[pos] = topValues[pos - 1];
			topIndices[pos] = topIndices[pos - 1];
			--pos;
		}
		topValues[pos] = rating;
		topIndices[pos] = searchIndex;
	}
}

/**
 * Assigns players from `playerIds` to each slot in `positions` so as to maximise
 * the cumulative positional rating. A player is eligible for a slot only if they
 * meet MINIMUM_POSITIONAL_PROFICIENCY for the slot's position and have a rating
 * for the grouped role that owns it. Eligible cost = −rating (minimisation ≡
 * maximisation); ineligible/empty cost is chosen so the solver never prefers it
 * over a real, higher-rated candidate.
 *
 * Optimality-preserving pruning: with only FORMATION_POSITION_COUNT slots, an
 * optimal assignment can only ever draw from the top FORMATION_POSITION_COUNT
 * players per distinct slot-position. We therefore reduce the n players (which
 * may number in the tens of thousands) to a candidate set of at most
 * distinctPositions × FORMATION_POSITION_COUNT (≤ 121) before solving. Cost is
 * O(n · distinctPositions) for the scan plus O(dim³) for the solver, where
 * dim = max(FORMATION_POSITION_COUNT, candidateCount) is bounded by ~121.
 */
static void assignFormation(
	const uint32_t *playerIds,
	const PositionCode positions[FORMATION_POSITION_COUNT],
	const BestElevenFilters filters,
	BestElevenRow outSlots[FORMATION_POSITION_COUNT]
) {
	// Default every slot to empty; filled in as the solver assigns players.
	for (uint8_t i = 0; i < FORMATION_POSITION_COUNT; ++i) {
		outSlots[i] = (BestElevenRow){.player = NULL, .rating = 0};
	}

	const uint32_t playerCount = playerIds ? (uint32_t)vector_length(playerIds) : 0;
	if (playerCount == 0) {
		LOG_WARN("No players available to fill formation");
		return;
	}
	if (playerCount < FORMATION_POSITION_COUNT) {
		LOG_WARN("Not enough players (%u) to fill formation; some slots will be empty", playerCount);
	}

	// Distinct positions requested by the formation.
	PositionCode distinctPositions[FORMATION_POSITION_COUNT];
	uint8_t distinctCount = 0;
	for (uint8_t i = 0; i < FORMATION_POSITION_COUNT; ++i) {
		bool seen = false;
		for (uint8_t d = 0; d < distinctCount; ++d) {
			if (distinctPositions[d] == positions[i]) {
				seen = true;
				break;
			}
		}
		if (!seen) {
			distinctPositions[distinctCount++] = positions[i];
		}
	}

	// Collect the union of the top-N eligible players for each distinct position.
	// `candidates` holds indices into playerIds; `chosen` dedupes across positions.
	bool *chosen = calloc(playerCount, sizeof(bool));
	uint32_t *candidates = malloc((size_t)distinctCount * FORMATION_POSITION_COUNT * sizeof(uint32_t));
	uint32_t candidateCount = 0;

	uint32_t topIndices[FORMATION_POSITION_COUNT];
	float topValues[FORMATION_POSITION_COUNT];

	for (uint8_t d = 0; d < distinctCount; ++d) {
		const PositionCode position = distinctPositions[d];
		uint8_t topCount = 0;
		for (uint32_t j = 0; j < playerCount; ++j) {
			const Player *player = &gameContext.players[playerIds[j]];

			if (
				player->age < filters.minAge ||
				player->age > filters.maxAge ||
				player->condition < filters.minCondition ||
				player->condition > filters.maxCondition
			) {
				continue;
			}

			const float rating = getPlayerPositionalRating(player, position);
			if (rating >= INFEASIBLE_COST) {
				continue;
			}
			insertTopK(topIndices, topValues, &topCount, FORMATION_POSITION_COUNT, j, rating);
		}
		for (uint8_t k = 0; k < topCount; ++k) {
			const uint32_t searchIndex = topIndices[k];
			if (!chosen[searchIndex]) {
				chosen[searchIndex] = true;
				candidates[candidateCount++] = searchIndex;
			}
		}
	}

	// Square cost matrix over slots (rows) and candidates (cols), padded so the
	// solver always finds a perfect matching. Real eligible pairs get −rating so
	// minimisation maximises total rating; ineligible pairs get INFEASIBLE_COST;
	// padding (dummy rows / empty cols) costs 0 so it is only ever used when no
	// feasible candidate remains.
	const uint32_t dim = candidateCount > FORMATION_POSITION_COUNT ? candidateCount : FORMATION_POSITION_COUNT;

	float **matrix = malloc(dim * sizeof(float*));
	for (uint32_t i = 0; i < dim; ++i) {
		matrix[i] = malloc(dim * sizeof(float));
		for (uint32_t j = 0; j < dim; ++j) {
			if (i < FORMATION_POSITION_COUNT && j < candidateCount) {
				const uint32_t pid = playerIds[candidates[j]];
				const float rating = getPlayerPositionalRating(&gameContext.players[pid], positions[i]);
				matrix[i][j] = rating >= INFEASIBLE_COST ? INFEASIBLE_COST : -rating;
			} else {
				matrix[i][j] = 0.f;
			}
		}
	}

	uint32_t *assignment = malloc(dim * sizeof(uint32_t));
	hungarian(matrix, dim, assignment);

	for (uint8_t i = 0; i < FORMATION_POSITION_COUNT; ++i) {
		const uint32_t col = assignment[i];
		if (col >= candidateCount) {
			continue; // slot matched to padding: no eligible player
		}

		const uint32_t pid = playerIds[candidates[col]];
		const Player *player = &gameContext.players[pid];
		const float rating = getPlayerPositionalRating(player, positions[i]);
		if (rating >= INFEASIBLE_COST) {
			LOG_DEBUG("No player is eligible for position \"%s\"", positionCodeNames[positions[i]]);
			continue;
		}

		outSlots[i] = (BestElevenRow){.player = player, .rating = rating};
	}

	free(assignment);
	for (uint32_t i = 0; i < dim; ++i) {
		free(matrix[i]);
	}
	free(matrix);
	free(candidates);
	free(chosen);
}
