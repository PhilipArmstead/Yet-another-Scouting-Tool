// SPDX-FileCopyrightText: © 2026 Phil Armstead <philarmstead@mailbox.org>
// SPDX-License-Identifier: GPL-3.0-or-later

#include "vector-shared-pointer.h"
#include "app/helpers/vector.h"


SharedPointer *sharedPointer_new(void *data) {
	SharedPointer *self = malloc(sizeof(SharedPointer));
	if (self == NULL) {
		return NULL;
	}

	self->data = data;
	self->refCount = 1;

	return self;
}

void sharedPointer_ref(SharedPointer *self) {
	if (self != NULL) {
		self->refCount++;
	}
}

void sharedPointer_unref(SharedPointer *self) {
	if (self != NULL) {
		self->refCount--;
		if (self->refCount == 0) {
			vector_free(self->data);
			free(self);
		}
	}
}
