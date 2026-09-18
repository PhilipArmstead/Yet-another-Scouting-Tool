// SPDX-FileCopyrightText: © 2026 Phil Armstead <philarmstead@mailbox.org>
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <gtk/gtk.h>


/**
 * Marks a signal handler that GtkBuilder resolves by name out of the .ui files.
 *
 * Nothing in the binary references these, so the toolchain is free to conclude they are dead:
 * link-time optimisation internalises them and section GC drops them outright, leaving a build
 * that links cleanly and then silently ignores every click. `used` is what holds them in place.
 * G_MODULE_EXPORT then keeps the symbol visible to the runtime lookup itself.
 */
#if defined(__GNUC__) || defined(__clang__)
#define BUILDER_CALLBACK G_MODULE_EXPORT __attribute__((used))
#else
#define BUILDER_CALLBACK G_MODULE_EXPORT
#endif
