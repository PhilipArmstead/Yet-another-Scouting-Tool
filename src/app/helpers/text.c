#include "text.h"
#include "core/logger.h"
#include <stdlib.h>
#include <string.h>
#include <utf8proc.h>

char *text_fold(const char *text, const bool alternate) {
	utf8proc_uint8_t *normalized = NULL;
	const utf8proc_option_t flags = UTF8PROC_NULLTERM | UTF8PROC_STABLE | UTF8PROC_CASEFOLD |
																	(alternate ? UTF8PROC_DECOMPOSE | UTF8PROC_STRIPMARK : UTF8PROC_COMPOSE);
	const utf8proc_ssize_t length = utf8proc_map((const utf8proc_uint8_t *)text, 0, &normalized, flags);
	if (length < 0) {
		LOG_ERROR("Cannot normalize UTF-8: %s", utf8proc_errmsg(length));
		return NULL;
	}
	char *out = malloc((size_t)length * 2 + 1);
	if (!out) {
		free(normalized);
		LOG_ERROR("Could not allocate normalized search key");
		return NULL;
	}
	size_t written = 0;
	for (utf8proc_ssize_t offset = 0; offset < length;) {
		utf8proc_int32_t code;
		const utf8proc_ssize_t size = utf8proc_iterate(normalized + offset, length - offset, &code);
		if (size <= 0)
			break;
		const utf8proc_category_t category = utf8proc_category(code);
		if (category >= UTF8PROC_CATEGORY_LU && category <= UTF8PROC_CATEGORY_NO) {
			const char *replacement = NULL;
			if (alternate) {
				switch (code) {
					case 0xf8: replacement = "o"; break;
					case 0x142: replacement = "l"; break;
					case 0x111:
					case 0xf0: replacement = "d"; break;
					case 0xfe: replacement = "th"; break;
					case 0xe6: replacement = "ae"; break;
					case 0x153: replacement = "oe"; break;
					default: break;
				}
			}
			if (replacement) {
				const size_t count = strlen(replacement);
				memcpy(out + written, replacement, count);
				written += count;
			} else {
				memcpy(out + written, normalized + offset, (size_t)size);
				written += (size_t)size;
			}
		} else if (written && out[written - 1] != ' ')
			out[written++] = ' ';
		offset += size;
	}
	if (written && out[written - 1] == ' ')
		--written;
	out[written] = '\0';
	free(normalized);
	return out;
}

static bool tokenMatches(const char *query, const size_t length, const char *candidate) {
	if (!candidate)
		return false;
	for (const char *token = candidate; *token;) {
		if (strncmp(token, query, length) == 0)
			return true;
		const char *space = strchr(token, ' ');
		if (!space)
			break;
		token = space + 1;
	}
	return false;
}

bool text_matches(const char *query, const char *folded, const char *alternate) {
	if (!query || !folded)
		return false;
	while (*query) {
		const char *space = strchr(query, ' ');
		const size_t length = space ? (size_t)(space - query) : strlen(query);
		if (!tokenMatches(query, length, folded) && !tokenMatches(query, length, alternate))
			return false;
		if (!space)
			break;
		query = space + 1;
	}
	return true;
}

size_t text_previous(const char *text, size_t position) {
	if (position)
		--position;
	while (position && ((unsigned char)text[position] & 0xc0) == 0x80)
		--position;
	return position;
}

size_t text_next(const char *text, size_t position) {
	if (text[position])
		++position;
	while (((unsigned char)text[position] & 0xc0) == 0x80)
		++position;
	return position;
}
