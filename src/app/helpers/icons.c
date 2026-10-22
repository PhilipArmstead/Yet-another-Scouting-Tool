// SPDX-License-Identifier: GPL-3.0-or-later
#include "icons.h"
#include "app/resources.h"
#include "core/logger.h"
#include "formatter.h"

static Texture2D textures[3];

bool icons_init(void) {
	const char *names[] = {"assets/icons/heart.png", "assets/icons/face.png", "assets/icons/shoe.png"};
	for (unsigned i = 0; i < 3; ++i) {
		const Resource *resource = resources_get(names[i]);
		if (!resource) {
			LOG_ERROR("Missing icon %s", names[i]);
			return false;
		}
		Image image = LoadImageFromMemory(".png", resource->data, (int)resource->size);
		textures[i] = LoadTextureFromImage(image);
		UnloadImage(image);
		if (!IsTextureValid(textures[i])) {
			LOG_ERROR("Cannot load icon %s", names[i]);
			return false;
		}
		SetTextureFilter(textures[i], TEXTURE_FILTER_BILINEAR);
	}
	return true;
}

void icons_draw(IconId id, Rectangle bounds, uint8_t quality) {
	Texture2D texture = textures[id < YAST_ICON_SHOE_LEFT ? id : 2];
	Rectangle source = {0, 0, (float)texture.width, (float)texture.height};
	if (id == YAST_ICON_SHOE_LEFT)
		source.width = -source.width;
	const RGB rgb = formatter_qualityColour(quality);
	Color colour = {(unsigned char)(rgb.r * 255), (unsigned char)(rgb.g * 255), (unsigned char)(rgb.b * 255), 255};
	DrawTexturePro(texture, source, bounds, (Vector2){0}, 0, colour);
}

void icons_shutdown(void) {
	for (unsigned i = 0; i < 3; ++i)
		if (textures[i].id)
			UnloadTexture(textures[i]);
}
