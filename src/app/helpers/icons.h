// SPDX-FileCopyrightText: © 2026 Phil Armstead <philarmstead@mailbox.org>
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <gtk/gtk.h>
#include <stdbool.h>
#include <stdint.h>


#define HEART_SIZE 16
// Condition, sharpness and fatigue are all stored as basis points by the game
#define HEART_MAX_VALUE 10000.f

#define FACE_SIZE 32

#define SHOE_WIDTH 18
#define SHOE_HEIGHT 32

void icons_init(void);
uint8_t icons_heartQuantise(float value, float max);
void icons_heartAttach(GtkDrawingArea *area, uint8_t quality);
GtkWidget *icons_heartNew(uint8_t quality);
GtkWidget *icons_faceNew(uint8_t quality);
GtkWidget *icons_shoeNew(uint8_t quality, bool mirrored);
