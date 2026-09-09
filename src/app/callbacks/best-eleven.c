// SPDX-FileCopyrightText: © 2026 Phil Armstead <philarmstead@mailbox.org>
// SPDX-License-Identifier: GPL-3.0-or-later

#include "best-eleven.h"
#include "app/ui.h"


G_MODULE_EXPORT void callbacks_onShowBestEleven(void) {
	ui_createBestElevenWindow();
}
