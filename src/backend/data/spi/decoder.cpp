/*
 *    Copyright (C) 2026
 *    Marco Greco <marcogrecopriolo@gmail.com>
 *
 *    This file is part of the guglielmo FM DAB tuner software package.
 *
 *    guglielmo is free software; you can redistribute it and/or modify
 *    it under the terms of the GNU General Public License as published by
 *    the Free Software Foundation, version 2 of the License.
 *
 *    guglielmo is distributed in the hope that it will be useful,
 *    but WITHOUT ANY WARRANTY; without even the implied warranty of
 *    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *    GNU General Public License for more details.
 *
 *    You should have received a copy of the GNU General Public License
 *    along with guglielmo; if not, write to the Free Software
 *    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 *    decoder.c - DAB SPI binary decoder
 */

#include "backend/data/spi/decoder.h"

#include <errno.h>
#include <limits.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <zlib.h>
#include "logging.h"

static int fail(int error, const char *function, int line) {
    log(LOG_SPI, LOG_MIN, "SPI decode failed at %s:%i: %s", function, line, strerror(error));
    errno = error;
    return -1;
}

static uint32_t unsigned_value(const unsigned char *p, size_t bytes) {
    uint32_t value = 0;
    while (bytes--)
	value = (value << 8) | *p++;
    return value;
}

static int next_tlv(const unsigned char **cursor, const unsigned char *end, tlv *item) {
    const unsigned char *p = *cursor;
    size_t bytes;

    if ((size_t)(end - p) < 2)
	return SPI_FAIL(EBADMSG);
    item->tag = *p++;
    item->length = *p++;
    if (item->length == SPI_LENGTH16 || item->length == SPI_LENGTH24) {
	bytes = item->length == SPI_LENGTH16 ? 2 : 3;
	if ((size_t)(end - p) < bytes)
	    return SPI_FAIL(EBADMSG);
	item->length = unsigned_value(p, bytes);
	p += bytes;
    }
    if (item->length > (size_t)(end - p))
	return SPI_FAIL(EBADMSG);
    item->data = p;
    *cursor = p + item->length;
    return 0;
}

static const element_definition *find_element(unsigned int tag) {
    size_t i;
    for (i = 0; i < COUNT(elements); ++i)
	if (elements[i].tag == tag)
	    return &elements[i];
    return NULL;
}

static const attribute_definition *find_attribute(unsigned int parent, unsigned int tag) {
    size_t i;
    for (i = 0; i < COUNT(attributes); ++i)
	if (attributes[i].parent == parent && attributes[i].tag == tag)
	    return &attributes[i];
    return NULL;
}

/* Validate UTF-8 and the character repertoire allowed by clause 5.3.1. */
static int valid_string(const unsigned char *p, size_t length) {
    const unsigned char *end = p + length;
    uint32_t value, minimum;
    unsigned int extra;

    while (p < end) {
	value = *p++;
	if (value < 0x80) {
	    if (value < 0x20 && value != '\t' && value != '\n' && value != '\r')
		return SPI_FAIL(EBADMSG);
	    continue;
	}
	if (value >= 0xc2 && value <= 0xdf) {
	    value &= 0x1f;
	    extra = 1;
	    minimum = 0x80;
	} else if (value >= 0xe0 && value <= 0xef) {
	    value &= 0x0f;
	    extra = 2;
	    minimum = 0x800;
	} else if (value >= 0xf0 && value <= 0xf4) {
	    value &= 0x07;
	    extra = 3;
	    minimum = 0x10000;
	} else {
	    return SPI_FAIL(EBADMSG);
	}
	if ((size_t)(end - p) < extra)
	    return SPI_FAIL(EBADMSG);
	while (extra--) {
	    if ((*p & 0xc0) != 0x80)
		return SPI_FAIL(EBADMSG);
	    value = (value << 6) | (*p++ & 0x3f);
	}
	if (value < minimum || value > 0x10ffff || (value >= 0xd800 && value <= 0xdfff) ||
	    (value >= 0xe000 && value <= 0xf8ff) || value == 0xfffe || value == 0xffff)
	    return SPI_FAIL(EBADMSG);
    }
    return 0;
}

static int is_token(unsigned int byte) {
    return (byte >= 0x01 && byte <= 0x08) || byte == 0x0b || byte == 0x0c ||
	(byte >= 0x0e && byte <= 0x13);
}

static char *decode_string(decoder *state, const tlv *item, int expand) {
    size_t i, length = 0, n;
    char *text, *out;
    const string_view *token;

    for (i = 0; i < item->length; ++i) {
	token = expand && is_token(item->data[i]) ? &state->tokens[item->data[i]] : NULL;
	if (token && !token->data) {
	    SPI_FAIL(EBADMSG);
	    return NULL;
	}
	n = token ? token->length : 1;
	if (n > (size_t)INT_MAX - length) {
	    SPI_FAIL(EOVERFLOW);
	    return NULL;
	}
	length += n;
    }
    text = (char *)malloc(length + 1);
    if (!text) {
	SPI_FAIL(ENOMEM);
	return NULL;
    }
    out = text;
    for (i = 0; i < item->length; ++i) {
	if (expand && is_token(item->data[i])) {
	    token = &state->tokens[item->data[i]];
	    memcpy(out, token->data, token->length);
	    out += token->length;
	} else {
	    *out++ = (char)item->data[i];
	}
    }
    *out = '\0';
    if (valid_string((const unsigned char *)text, length)) {
	free(text);
	return NULL;
    }
    return text;
}

/* The table applies even to attributes preceding it in the binary object. */
static int read_metadata(decoder *state, const tlv *root) {
    const unsigned char *p = root->data, *end = p + root->length, *q, *limit;
    int table_seen = 0, language_seen = 0;
    unsigned int tag, length;
    tlv item;

    while (p < end) {
	if (next_tlv(&p, end, &item))
	    return -1;
	if (item.tag == SPI_TOKENS) {
	    if (table_seen++)
		return SPI_FAIL(EBADMSG);
	    q = item.data;
	    limit = q + item.length;
	    while (q < limit) {
		if ((size_t)(limit - q) < 2)
		    return SPI_FAIL(EBADMSG);
		tag = *q++;
		length = *q++;
		if (!is_token(tag) || state->tokens[tag].data || length > (size_t)(limit - q) ||
		    valid_string(q, length))
		    return SPI_FAIL(EBADMSG);
		state->tokens[tag].data = q;
		state->tokens[tag].length = length;
		q += length;
	    }
	} else if (item.tag == SPI_LANGUAGE) {
	    if (language_seen++)
		return SPI_FAIL(EBADMSG);
	    state->language = decode_string(state, &item, 0);
	    if (!state->language)
		return -1;
	    if (!state->language[0])
		return SPI_FAIL(EBADMSG);
	}
    }
    return 0;
}

static int coordinate(QVariantMap &node, const unsigned char *p) {
    int32_t latitude = (int32_t)unsigned_value(p, 3);
    int32_t longitude = (int32_t)unsigned_value(p + 3, 3);

    if (latitude & 0x800000)
	latitude -= 0x1000000;
    if (longitude & 0x800000)
	longitude -= 0x1000000;
    if (latitude < -90 * 92000 || latitude > 90 * 92000 || longitude < -180 * 46000 ||
	longitude > 180 * 46000)
	return SPI_FAIL(EBADMSG);
    node.insert("latitude", latitude / 92000.0);
    node.insert("longitude", longitude / 46000.0);
    return 0;
}

static int decode_coordinates(QVariantMap &node, const tlv *item) {
    QVariantList points;
    size_t i, count;

    if (item->tag == SPI_POINT)
	return item->length == 6 ? coordinate(node, item->data) : SPI_FAIL(EBADMSG);
    if (!item->length || item->length % 6)
	return SPI_FAIL(EBADMSG);
    count = item->length / 6;
    for (i = 0; i < count; ++i) {
	QVariantMap point;

	if (coordinate(point, item->data + 6 * i))
	    return -1;
	points.append(point);
    }
    node.insert("point", points);
    return 0;
}

/* MJD epoch is 1858-11-17. Do not depend on time_t width or host timezone. */
static void calendar_date(int mjd, int *year, int *month, int *day) {

    int days = mjd + 320, y = 1858, m = 0, leap, length;

    for (;;) {
	leap = y % 4 == 0 && (y % 100 != 0 || y % 400 == 0);
	length = 365 + leap;
	if (days < length)
	    break;
	days -= length;
	++y;
    }
    while (days >= (length = months[m] + (m == 1 && leap))) {
	days -= length;
	++m;
    }
    *year = y;
    *month = m + 1;
    *day = days + 1;
}

static int timepoint(const tlv *item, char *text, size_t capacity) {
    uint32_t word;
    int mjd, hour, minute, second = 0, offset = 0, local;
    int year, month, day, long_form, has_offset, absolute;
    size_t bytes;

    if (item->length < 4)
	return SPI_FAIL(EBADMSG);
    word = unsigned_value(item->data, 4);
    mjd = (int)((word >> 14) & 0x1ffff);
    has_offset = (word >> 12) & 1;
    long_form = (word >> 11) & 1;
    hour = (word >> 6) & 31;
    minute = word & 63;
    bytes = long_form ? 6 : 4;
    if (item->length != bytes + (size_t)has_offset || mjd > 99999 || hour > 23 || minute > 59)
	return SPI_FAIL(EBADMSG);
    if (long_form)
	second = item->data[4] >> 2;
    if (second > 60)
	return SPI_FAIL(EBADMSG);
    if (has_offset) {
	offset = item->data[bytes] & 31;
	if (offset > 28)
	    return SPI_FAIL(EBADMSG);
	offset *= item->data[bytes] & 0x20 ? -30 : 30;
    }
    local = hour * 60 + minute + offset;
    if (local < 0) {
	local += 1440;
	--mjd;
    } else if (local >= 1440) {
	local -= 1440;
	++mjd;
    }
    calendar_date(mjd, &year, &month, &day);
    if (!offset) {
	snprintf(text, capacity, "%04d-%02d-%02dT%02d:%02d:%02dZ", year, month, day, local / 60,
	    local % 60, second);
    } else {
	absolute = offset < 0 ? -offset : offset;
	snprintf(text, capacity, "%04d-%02d-%02dT%02d:%02d:%02d%c%02d:%02d", year, month, day,
	    local / 60, local % 60, second, offset < 0 ? '-' : '+', absolute / 60, absolute % 60);
    }
    return 0;
}

static int bearer_id(const tlv *item, char *text, size_t capacity) {
    unsigned int sid, ecc, ensemble, scids, gcc, width;

    if (item->length != 6 && item->length != 8)
	return SPI_FAIL(EBADMSG);

    /* Ens and X-PAD flag encodings are prescribed for DAB by clause 5.4.5.1. */
    width = item->data[0] & 0x10 ? 8 : 4;
    if ((item->data[0] & 0x60) != 0x40 || item->length != (width == 8 ? 8u : 6u))
	return SPI_FAIL(EBADMSG);
    scids = item->data[0] & 15;
    ecc = item->data[1];
    ensemble = unsigned_value(item->data + 2, 2);
    sid = unsigned_value(item->data + 4, item->length - 4);
    gcc = ((sid >> (width == 8 ? 28 : 12)) << 8) | ecc;
    snprintf(text, capacity, "dab:%03x.%04x.%0*x.%x", gcc, ensemble, (int)width, sid, scids);
    return 0;
}

static const char *enumeration(value_format format, unsigned int value) {

    const char *const *values = NULL;
    size_t count = 0;

    switch (format) {
    case VALUE_GROUP_TYPE:
	values = groups;
	count = COUNT(groups);
	break;
    case VALUE_BROADCAST:
	values = broadcast;
	count = COUNT(broadcast);
	break;
    case VALUE_RECOMMENDATION:
	values = recommendation;
	count = COUNT(recommendation);
	break;
    case VALUE_LOGO_TYPE:
	values = logos;
	count = COUNT(logos);
	break;
    case VALUE_GENRE_TYPE:
	values = genres;
	count = COUNT(genres);
	break;
    case VALUE_BOOLEAN:
	values = booleans;
	count = COUNT(booleans);
	break;
    default:
	break;
    }
    return value < count ? values[value] : NULL;
}

static int decode_attribute(
    decoder *state, QVariantMap &node, const attribute_definition *definition, const tlv *item) {
    char value[80], *allocated;
    const char *label;
    unsigned int number, scheme;
    size_t i, used;

    switch (definition->format) {
    case VALUE_STRING:
    case VALUE_LITERAL_STRING:
	allocated = decode_string(state, item, definition->format == VALUE_STRING);
	if (!allocated)
	    return -1;
	node.insert(QString::fromUtf8(definition->name), QString::fromUtf8(allocated));
	free(allocated);
	return 0;
    case VALUE_UINT16:
    case VALUE_DURATION:
    case VALUE_UINT24:
	if (item->length != (definition->format == VALUE_UINT24 ? 3u : 2u))
	    return SPI_FAIL(EBADMSG);
	node.insert(QString::fromUtf8(definition->name), (int)unsigned_value(item->data, item->length));
	return 0;
    case VALUE_TIMEPOINT:
	if (timepoint(item, value, sizeof(value)))
	    return -1;
	break;
    case VALUE_BEARER_ID:
	if (bearer_id(item, value, sizeof(value)))
	    return -1;
	break;
    case VALUE_ENSEMBLE_ID:
	if (item->length != 3)
	    return SPI_FAIL(EBADMSG);
	snprintf(value, sizeof(value), "%02x.%04x", item->data[0],
	    (unsigned int)unsigned_value(item->data + 1, 2));
	break;
    case VALUE_GENRE_HREF:
	if (item->length < 1 || item->length > 4)
	    return SPI_FAIL(EBADMSG);
	scheme = item->data[0] & 15;

	/* Undefined schemes must be ignored, clause 5.4.5.4. */
	if (!scheme || scheme > 8)
	    return 0;
	used = (size_t)snprintf(value, sizeof(value), "%u", scheme);
	for (i = 1; i < item->length; ++i)
	    used += (size_t)snprintf(value + used, sizeof(value) - used, ".%u", item->data[i]);
	break;
    default:
	if (item->length != 1)
	    return SPI_FAIL(EBADMSG);
	number = item->data[0];
	label = enumeration(definition->format, number);
	node.insert(QString::fromUtf8(definition->name),
	    label ? QVariant(QString::fromUtf8(label)) : QVariant((int)number));
	return 0;
    }
    node.insert(QString::fromUtf8(definition->name), QString::fromUtf8(value));
    return 0;
}

static void defaults(decoder *state, QVariantMap &node, unsigned int tag, const unsigned char *seen) {
    size_t i;
    int has_language = tag == SPI_EPG || tag == SPI_SI;
    int language_seen = 0;
    const attribute_definition *a;

    for (i = 0; i < COUNT(attributes); ++i) {
	a = &attributes[i];
	if (a->parent != tag)
	    continue;
	if (!strcmp(a->name, "xml:lang")) {
	    has_language = 1;
	    language_seen = seen[a->tag];
	}
	if (a->initial && !seen[a->tag]) {
	    node.insert(QString::fromUtf8(a->name), a->format == VALUE_UINT16
		? QVariant(1) : QVariant(QString::fromUtf8(a->initial)));
	}
    }
    if (has_language && !language_seen)
	node.insert("xml:lang", QString::fromUtf8(state->language ? state->language : "en"));
}

/* Clause 5.4.5.4 requires ignoring genres with undefined CS identifiers. */
static int supported_element(const tlv *element) {
    const unsigned char *p = element->data, *end = p + element->length;
    tlv item;
    unsigned int scheme;

    if (element->tag != 0x14)
	return 1;
    while (p < end) {
	if (next_tlv(&p, end, &item))
	    return -1;
	if (item.tag == 0x80) {
	    if (!item.length || item.length > 4)
		return SPI_FAIL(EBADMSG);
	    scheme = item.data[0] & 15;
	    return scheme >= 1 && scheme <= 8;
	}
    }
    return 1;
}

static int decode_element(decoder *state, QVariantMap &node, const tlv *element, unsigned int depth) {
    const unsigned char *p = element->data, *end = p + element->length;
    unsigned int counts[256] = {0};
    unsigned char seen[256] = {0};
    QVariantList collections[256];
    const element_definition *definition;
    const attribute_definition *attribute;
    char *text;
    int supported;
    tlv item;

    if (depth > SPI_MAX_DEPTH)
	return SPI_FAIL(EOVERFLOW);
    if (element->tag == SPI_POINT || element->tag == SPI_POLYGON)
	return decode_coordinates(node, element);

    /* First pass checks framing, parents and non-repeatable siblings. */
    while (p < end) {
	if (next_tlv(&p, end, &item))
	    return -1;
	definition = find_element(item.tag);
	if (definition) {
	    if (!(definition->parents & P(element->tag)))
		return SPI_FAIL(EBADMSG);
	    supported = supported_element(&item);
	    if (supported < 0)
		return -1;
	    if (!supported)
		continue;
	    if (++counts[item.tag] > 1 && !definition->repeated)
		return SPI_FAIL(EBADMSG);
	} else if ((item.tag == SPI_TOKENS || item.tag == SPI_LANGUAGE) && depth) {
	    return SPI_FAIL(EBADMSG);
	}
    }
    p = element->data;
    while (p < end) {
	if (next_tlv(&p, end, &item))
	    return -1;
	if (item.tag == SPI_TOKENS || item.tag == SPI_LANGUAGE)
	    continue;
	if (item.tag == SPI_TEXT) {
	    if (seen[SPI_TEXT]++)
		return SPI_FAIL(EBADMSG);
	    text = decode_string(state, &item, 1);
	    if (!text)
		return -1;
	    node.insert("text", QString::fromUtf8(text));
	    free(text);
	} else if ((attribute = find_attribute(element->tag, item.tag))) {
	    if (seen[item.tag]++)
		return SPI_FAIL(EBADMSG);
	    if (decode_attribute(state, node, attribute, &item))
		return -1;
	} else if ((definition = find_element(item.tag))) {
	    supported = supported_element(&item);
	    if (supported < 0)
		return -1;
	    if (!supported)
		continue;
	    QVariantMap child;

	    if (decode_element(state, child, &item, depth + 1))
		return -1;
	    if (definition->repeated)
		collections[item.tag].append(child);
	    else
		node.insert(QString::fromUtf8(definition->name), child);
	}

	/* Unknown tags have already been skipped using their declared length. */
    }
    for (size_t i = 0; i < COUNT(elements); ++i) {
	definition = &elements[i];
	if (definition->repeated && counts[definition->tag])
	    node.insert(QString::fromUtf8(definition->name), collections[definition->tag]);
    }
    defaults(state, node, element->tag, seen);
    return 0;
}

static int gunzip_body(
    const unsigned char *data, size_t length, unsigned char **result, size_t *result_length) {
    z_stream stream = {};
    unsigned char *buffer = NULL, *grown;
    size_t capacity = 0, next, used = 0, available;
    Bytef *next_input;
    uInt remaining;
    int status, error = EBADMSG;

    if (length > UINT_MAX)
	return SPI_FAIL(EOVERFLOW);
    status = inflateInit2(&stream, 15 + 16);
    if (status != Z_OK)
	return SPI_FAIL(status == Z_MEM_ERROR ? ENOMEM : EBADMSG);
    stream.next_in = (Bytef *)data;
    stream.avail_in = (uInt)length;
    for (;;) {
	if (used == capacity) {
	    if (capacity == SPI_MAX_OBJECT + 1) {
		error = EOVERFLOW;
		goto bad;
	    }
	    next = capacity ? capacity * 2 : 32768;
	    if (next > SPI_MAX_OBJECT + 1)
		next = SPI_MAX_OBJECT + 1;
	    grown = (unsigned char *)realloc(buffer, next);
	    if (!grown) {
		error = ENOMEM;
		goto bad;
	    }
	    buffer = grown;
	    capacity = next;
	}
	stream.next_out = buffer + used;
	available = capacity - used;
	stream.avail_out = (uInt)available;
	status = inflate(&stream, Z_NO_FLUSH);
	used += available - stream.avail_out;
	if (used > SPI_MAX_OBJECT) {
	    error = EOVERFLOW;
	    goto bad;
	}
	if (status == Z_MEM_ERROR) {
	    error = ENOMEM;
	    goto bad;
	}
	if (status != Z_OK && status != Z_STREAM_END)
	    goto bad;
	if (status == Z_OK && !stream.avail_in && stream.avail_out)
	    goto bad;
	if (status == Z_STREAM_END) {
	    if (!stream.avail_in)
		break;

	    /* RFC 1952 permits concatenated members forming one payload. */
	    next_input = stream.next_in;
	    remaining = stream.avail_in;
	    if (inflateReset(&stream) != Z_OK)
		goto bad;
	    stream.next_in = next_input;
	    stream.avail_in = remaining;
	}
    }
    *result = buffer;
    *result_length = used;
    inflateEnd(&stream);
    return 0;
bad:
    inflateEnd(&stream);
    free(buffer);
    return SPI_FAIL(error);
}

int spi_decode(QVariantMap *output, const void *data, size_t length) {
    decoder state = {};
    QVariantMap result, root;
    const unsigned char *bytes = static_cast<const unsigned char *>(data), *cursor, *end;
    unsigned char *inflated = NULL;
    const element_definition *definition;
    tlv item;
    int error, hash;

    if (!output || !data || !length)
	return SPI_FAIL(EINVAL);
    if (length >= 2 && bytes[0] == 0x1f && bytes[1] == 0x8b) {
	if (gunzip_body(bytes, length, &inflated, &length))
	    return -1;
	bytes = inflated;
    }
    if (length > SPI_MAX_OBJECT) {
	error = EOVERFLOW;
	goto bad;
    }
    cursor = bytes;
    end = bytes + length;
    if (next_tlv(&cursor, end, &item))
	goto failure;
    if (cursor != end) {
	error = EBADMSG;
	goto bad;
    }
    if (item.tag != SPI_EPG && item.tag != SPI_SI) {
	error = ENOTSUP;
	goto bad;
    }
    definition = find_element(item.tag);
    if (read_metadata(&state, &item))
	goto failure;
    if (decode_element(&state, root, &item, 0))
	goto failure;
    result.insert(QString::fromUtf8(definition->name), root);
    output->swap(result);
    hash = (int)(crc32(0L, bytes, (uInt)length) & 0x7fffffffUL);
    free(state.language);
    free(inflated);
    errno = 0;
    return hash;
failure:
    error = errno ? errno : EBADMSG;
bad:
    free(state.language);
    free(inflated);
    return SPI_FAIL(error);
}
