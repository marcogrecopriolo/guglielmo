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
 *    decoder.h - DAB SPI binary decoder interface
 */

#ifndef SPI_DECODER_H
#define SPI_DECODER_H

#include <stddef.h>
#include <stdint.h>
#include <QMap>
#include <QVariant>

/*
 * ETSI TS 102 371 V3.3.1 (2023-12), clauses 5.2-5.6 and annexes D-F:
 * https://www.etsi.org/deliver/etsi_ts/102300_102399/102371/03.03.01_60/ts_102371v030301p.pdf
 * Defaults also follow the SPI 3.5 schema (TS 102 818):
 * https://www.worlddab.org/schemas/spi/spi_35.xsd
 *
 * Numeric tags are written once in the lookup tables, as in the standard.
 * Only framing and special cases used by the parser need named constants.
 * Attribute tags are interpreted within their parent element's namespace.
 */
enum {
    SPI_TEXT = 0x01,
    SPI_EPG = 0x02,
    SPI_SI = 0x03,
    SPI_TOKENS = 0x04,
    SPI_LANGUAGE = 0x06,
    SPI_POINT = 0x34,
    SPI_POLYGON = 0x35,
    SPI_LENGTH16 = 0xfe,
    SPI_LENGTH24 = 0xff,
    SPI_MAX_DEPTH = 32,
    SPI_MAX_OBJECT = 0xffffff + 5
};

#define P(tag) (UINT64_C(1) << (tag))
#define NAMED_PARENTS (P(0x23) | P(0x26) | P(0x28) | P(0x1c) | P(0x2e))
#define PROGRAMMES (P(0x1c) | P(0x2e))

typedef struct {
    unsigned char tag;
    const char *name;
    uint64_t parents;
    int repeated;
} element_definition;

/* Annex D. All repeated occurrences become independent subdocuments. */
static const element_definition elements[] = {
    {0x02, "epg", 0, 0},
    {0x03, "serviceInformation", 0, 0},
    {0x10, "shortName", NAMED_PARENTS, 1},
    {0x11, "mediumName", NAMED_PARENTS, 1},
    {0x12, "longName", NAMED_PARENTS, 1},
    {0x13, "mediaDescription", NAMED_PARENTS, 1},
    {0x14, "genre", P(0x23) | P(0x28) | PROGRAMMES, 1},
    {0x16, "keywords", NAMED_PARENTS, 1},
    {0x17, "memberOf", P(0x23) | PROGRAMMES, 1},
    {0x18, "link", NAMED_PARENTS, 1},
    {0x19, "location", PROGRAMMES, 1},
    {0x1a, "shortDescription", P(0x13), 1},
    {0x1b, "longDescription", P(0x13), 1},
    {0x1c, "programme", P(0x21), 1},
    {0x20, "programmeGroups", P(0x02), 1},
    {0x21, "schedule", P(0x02), 1},
    {0x23, "programmeGroup", P(0x20), 1},
    {0x24, "scope", P(0x21), 0},
    {0x25, "serviceScope", P(0x24), 1},
    {0x26, "ensemble", P(0x03), 0},
    {0x28, "service", P(0x26) | P(0x03), 1},
    {0x29, "bearer", P(0x28), 1},
    {0x2a, "presentationLanguage", P(0x28) | P(0x21) | PROGRAMMES, 1},
    {0x2b, "multimedia", P(0x13), 1},
    {0x2c, "time", P(0x19), 1},
    {0x2d, "bearer", P(0x19) | P(0x36), 1},
    {0x2e, "programmeEvent", P(0x1c), 1},
    {0x2f, "relativeTime", P(0x19), 1},
    {0x31, "radiodns", P(0x28), 0},
    {0x32, "geolocation", P(0x28) | P(0x29) | P(0x2d), 1},
    {0x33, "country", P(0x32), 1},
    {0x34, "point", P(0x32), 1},
    {0x35, "polygon", P(0x32), 1},
    {0x36, "onDemand", PROGRAMMES, 1},
    {0x37, "presentationTime", P(0x36), 0},
    {0x38, "acquisitionTime", P(0x36), 1},
    {0x39, "alias", P(0x28) | PROGRAMMES, 1},
    {0x3a, "phoneme", P(0x28) | PROGRAMMES, 1},
};

typedef enum {
    VALUE_STRING,
    VALUE_LITERAL_STRING,
    VALUE_UINT16,
    VALUE_UINT24,
    VALUE_DURATION,
    VALUE_TIMEPOINT,
    VALUE_BEARER_ID,
    VALUE_ENSEMBLE_ID,
    VALUE_GENRE_HREF,
    VALUE_GROUP_TYPE,
    VALUE_BROADCAST,
    VALUE_RECOMMENDATION,
    VALUE_LOGO_TYPE,
    VALUE_GENRE_TYPE,
    VALUE_BOOLEAN
} value_format;

typedef struct {
    unsigned char parent, tag;
    const char *name;
    value_format format;
    const char *initial;
} attribute_definition;

#define A(parent, tag, name, type) {parent, tag, name, type, NULL}
#define D(parent, tag, name, type, value) {parent, tag, name, type, value}
#define LANGUAGE(parent) A(parent, 0x80, "xml:lang", VALUE_STRING)
#define PROGRAMME_ATTRIBUTES(parent) \
    A(parent, 0x80, "id", VALUE_STRING), \
    A(parent, 0x81, "shortId", VALUE_UINT24), \
    D(parent, 0x82, "version", VALUE_UINT16, "1"), \
    D(parent, 0x83, "recommendation", VALUE_RECOMMENDATION, "no"), \
    D(parent, 0x84, "broadcast", VALUE_BROADCAST, "on-air"), \
    A(parent, 0x86, "xml:lang", VALUE_STRING)
#define BEARER_ATTRIBUTES(parent) \
    A(parent, 0x80, "id", VALUE_BEARER_ID), \
    A(parent, 0x82, "url", VALUE_STRING)

/* Annex E. Deprecated attributes are deliberately absent. */
static const attribute_definition attributes[] = {
    D(0x03, 0x80, "version", VALUE_UINT16, "1"),
    A(0x03, 0x81, "creationTime", VALUE_TIMEPOINT),
    A(0x03, 0x82, "originator", VALUE_STRING),
    A(0x03, 0x83, "serviceProvider", VALUE_STRING),
    A(0x03, 0x85, "alphabet", VALUE_STRING),
    LANGUAGE(0x10),
    LANGUAGE(0x11),
    LANGUAGE(0x12),
    A(0x14, 0x80, "href", VALUE_GENRE_HREF),
    D(0x14, 0x81, "type", VALUE_GENRE_TYPE, "main"),
    LANGUAGE(0x16),
    A(0x17, 0x80, "id", VALUE_STRING),
    A(0x17, 0x81, "shortId", VALUE_UINT24),
    A(0x17, 0x82, "index", VALUE_UINT16),
    A(0x18, 0x80, "uri", VALUE_STRING),
    A(0x18, 0x81, "mimeValue", VALUE_STRING),
    A(0x18, 0x82, "language", VALUE_STRING),
    A(0x18, 0x83, "description", VALUE_STRING),
    A(0x18, 0x84, "expiryTime", VALUE_TIMEPOINT),
    A(0x18, 0x85, "xml:lang", VALUE_STRING),
    LANGUAGE(0x1a),
    LANGUAGE(0x1b),
    PROGRAMME_ATTRIBUTES(0x1c),
    D(0x20, 0x80, "version", VALUE_UINT16, "1"),
    A(0x20, 0x81, "creationTime", VALUE_TIMEPOINT),
    A(0x20, 0x82, "originator", VALUE_STRING),
    D(0x21, 0x80, "version", VALUE_UINT16, "1"),
    A(0x21, 0x81, "creationTime", VALUE_TIMEPOINT),
    A(0x21, 0x82, "originator", VALUE_STRING),
    A(0x21, 0x83, "alphabet", VALUE_STRING),
    A(0x23, 0x80, "id", VALUE_STRING),
    A(0x23, 0x81, "shortId", VALUE_UINT24),
    D(0x23, 0x82, "version", VALUE_UINT16, "1"),
    A(0x23, 0x83, "type", VALUE_GROUP_TYPE),
    A(0x23, 0x84, "numOfItems", VALUE_UINT16),
    A(0x24, 0x80, "startTime", VALUE_TIMEPOINT),
    A(0x24, 0x81, "stopTime", VALUE_TIMEPOINT),
    A(0x25, 0x80, "id", VALUE_BEARER_ID),
    A(0x26, 0x80, "id", VALUE_ENSEMBLE_ID),
    D(0x28, 0x80, "version", VALUE_UINT16, "1"),
    BEARER_ATTRIBUTES(0x29),
    A(0x2b, 0x80, "mimeValue", VALUE_STRING),
    A(0x2b, 0x81, "language", VALUE_STRING),
    A(0x2b, 0x82, "url", VALUE_LITERAL_STRING),
    A(0x2b, 0x83, "type", VALUE_LOGO_TYPE),
    A(0x2b, 0x84, "width", VALUE_UINT16),
    A(0x2b, 0x85, "height", VALUE_UINT16),
    A(0x2b, 0x86, "creationTime", VALUE_TIMEPOINT),
    A(0x2c, 0x80, "time", VALUE_TIMEPOINT),
    A(0x2c, 0x81, "duration", VALUE_DURATION),
    A(0x2c, 0x82, "actualTime", VALUE_TIMEPOINT),
    A(0x2c, 0x83, "actualDuration", VALUE_DURATION),
    BEARER_ATTRIBUTES(0x2d),
    PROGRAMME_ATTRIBUTES(0x2e),
    A(0x2f, 0x80, "time", VALUE_DURATION),
    A(0x2f, 0x81, "duration", VALUE_DURATION),
    A(0x2f, 0x82, "actualTime", VALUE_DURATION),
    A(0x2f, 0x83, "actualDuration", VALUE_DURATION),
    A(0x31, 0x80, "fqdn", VALUE_STRING),
    A(0x31, 0x81, "serviceIdentifier", VALUE_STRING),
    A(0x32, 0x80, "xml:id", VALUE_STRING),
    A(0x32, 0x81, "ref", VALUE_STRING),
    A(0x37, 0x80, "start", VALUE_TIMEPOINT),
    A(0x37, 0x81, "end", VALUE_TIMEPOINT),
    A(0x37, 0x82, "duration", VALUE_DURATION),
    A(0x38, 0x80, "start", VALUE_TIMEPOINT),
    A(0x38, 0x81, "end", VALUE_TIMEPOINT),
    LANGUAGE(0x39),
    D(0x39, 0x81, "prefer", VALUE_BOOLEAN, "false"),
    LANGUAGE(0x3a),
    D(0x3a, 0x81, "prefer", VALUE_BOOLEAN, "false"),
    A(0x3a, 0x82, "alphabet", VALUE_STRING),
};

#define COUNT(array) (sizeof(array) / sizeof((array)[0]))
#define SPI_FAIL(error) fail(error, __func__, __LINE__)

typedef struct {
    unsigned char tag;
    const unsigned char *data;
    size_t length;
} tlv;

typedef struct {
    const unsigned char *data;
    size_t length;
} string_view;

typedef struct {
    string_view tokens[0x14];
    char *language;
} decoder;

static const int months[] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};

static const char *const groups[] = {NULL, NULL, "series", "show", "programConcept", "magazine",
    "programCompilation", "otherCollection", "otherChoice", "topic"};

static const char *const broadcast[] = {NULL, "on-air", "off-air"};

static const char *const recommendation[] = {NULL, "no", "yes"};

static const char *const logos[] = {
    NULL, NULL, "logo_unrestricted", NULL, "logo_colour_square", NULL, "logo_colour_rectangle"};

static const char *const genres[] = {NULL, "main", "secondary", "other"};

static const char *const booleans[] = {NULL, "false", "true"};

/*
 * Decode one complete DAB SPI MOT body, optionally GZIP compressed, into
 * a QVariantMap. MOT headers, segmentation and basic/advanced profile merging
 * belong to the caller. This decodes ETSI TS 102 371 V3.3.1 binary objects;
 * it does not generate XML or impose a daily/weekly schedule boundary.
 *
 * The root contains either "epg" or "serviceInformation", a QVariantMap.
 * Element occurrences are QVariantMaps with named attributes and a "text"
 * field for character content. Repeatable children are QVariantLists under
 * their element name, including when there is only one occurrence. Single
 * children (e.g. scope) are maps directly. Lists retain broadcast order;
 * map fields follow QMap key order. There are no numbered keys or counts.
 * The result can be saved and restored with QDataStream.
 *
 * Integers and durations (seconds) use int variants. Timepoints are ISO 8601
 * strings including the local offset; bearer IDs are dab: URIs, ensemble
 * IDs are ecc.eid strings, and genre href is its dotted numeric CS path.
 * Enumerations use their specification names; unknown enum values remain
 * integers. Coordinates are doubles in degrees. Unknown TLVs are skipped.
 * Default values and languages are materialized in the maps.
 *
 * On success output is replaced and the return value is a nonnegative
 * 31-bit CRC32 hash of the uncompressed SPI body. The hash is stable across
 * runs and independent of GZIP wrapping, suitable for cache filenames.
 * On failure output is unchanged and -1 is returned with errno set:
 * EINVAL for arguments, EBADMSG for malformed input, ENOTSUP for an unknown
 * root, EOVERFLOW for size/depth limits, or ENOMEM for string/GZIP buffer
 * allocation failure.
 * GZIP support uses zlib, already used by Guglielmo.
 */
int spi_decode(QVariantMap *output, const void *data, size_t length);

#endif /* SPI_DECODER_H */
