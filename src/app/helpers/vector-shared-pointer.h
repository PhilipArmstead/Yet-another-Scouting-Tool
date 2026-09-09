// SPDX-FileCopyrightText: © 2026 Phil Armstead <philarmstead@mailbox.org>
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include "app/types.h"


SharedPointer *sharedPointer_new(void *data);
void sharedPointer_ref(SharedPointer *self);
void sharedPointer_unref(SharedPointer *self);
