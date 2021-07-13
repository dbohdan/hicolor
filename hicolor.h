/* HiColor image file format encoder/decoder library.
 *
 * Copyright (c) 2021 D. Bohdan and contributors listed in AUTHORS.
 * License: MIT.
 *
 * This header file contains both the interface and the implementation for
 * HiColor.  To instantiate the implementation put the line
 *     #define HICOLOR_IMPLEMENTATION
 * in a single source code file of your project above where you include this
 * file.
 */

#ifndef HICOLOR_H
#define HICOLOR_H

#include <inttypes.h>
#include <stdbool.h>
#include <stdio.h>

#define HICOLOR_BAYER_SIZE 8
#define HICOLOR_LIBRARY_VERSION 201

/* Types. */

static const uint8_t hicolor_magic[7] = "HiColor";

static const double hicolor_bayer[HICOLOR_BAYER_SIZE * HICOLOR_BAYER_SIZE] = {
     0.0/64, 48.0/64, 12.0/64, 60.0/64,  3.0/64, 51.0/64, 15.0/64, 63.0/64,
    32.0/64, 16.0/64, 44.0/64, 28.0/64, 35.0/64, 19.0/64, 47.0/64, 31.0/64,
     8.0/64, 56.0/64,  4.0/64, 52.0/64, 11.0/64, 59.0/64,  7.0/64, 55.0/64,
    40.0/64, 24.0/64, 36.0/64, 20.0/64, 43.0/64, 27.0/64, 39.0/64, 23.0/64,
     2.0/64, 50.0/64, 14.0/64, 62.0/64,  1.0/64, 49.0/64, 13.0/64, 61.0/64,
    34.0/64, 18.0/64, 46.0/64, 30.0/64, 33.0/64, 17.0/64, 45.0/64, 29.0/64,
    10.0/64, 58.0/64,  6.0/64, 54.0/64,  9.0/64, 57.0/64,  5.0/64, 53.0/64,
    42.0/64, 26.0/64, 38.0/64, 22.0/64, 41.0/64, 25.0/64, 37.0/64, 21.0/64
};


typedef enum hicolor_version {
    HICOLOR_VERSION_5,
    HICOLOR_VERSION_6
} hicolor_version;

typedef struct hicolor_metadata {
    hicolor_version version;
    uint16_t width;
    uint16_t height;
} hicolor_metadata;

typedef enum hicolor_result {
    HICOLOR_OK,
    HICOLOR_IO_ERROR,
    HICOLOR_UNKNOWN_VERSION,
    HICOLOR_INVALID_VALUE,
    HICOLOR_INSUFFICIENT_DATA,
    HICOLOR_BAD_MAGIC
} hicolor_result;

typedef struct hicolor_rgb {
    uint8_t r;
    uint8_t g;
    uint8_t b;
} hicolor_rgb;

typedef uint16_t hicolor_value;

/* Functions. */

const char* hicolor_error_message(hicolor_result res);

hicolor_result hicolor_char_to_version(
    const uint8_t ch,
    hicolor_version* version
);
hicolor_result hicolor_version_to_char(
    const hicolor_version version,
    uint8_t* ch
);

hicolor_result hicolor_value_to_rgb(
    const hicolor_version version,
    const hicolor_value value,
    hicolor_rgb* rgb
);
hicolor_result hicolor_rgb_to_value(
    const hicolor_version version,
    const hicolor_rgb rgb,
    hicolor_value* value
);

hicolor_result hicolor_read_header(
    FILE* stream,
    hicolor_metadata* meta
);
hicolor_result hicolor_write_header(
    FILE* stream,
    const hicolor_metadata meta
);

/* Quantize using ordered (Bayer) dithering. */
hicolor_result hicolor_quantize_rgb_image(
    const hicolor_metadata meta,
    bool dither,
    hicolor_rgb* image
);

hicolor_result hicolor_read_rgb_image(
    FILE* stream,
    const hicolor_metadata meta,
    hicolor_rgb* image
);
hicolor_result hicolor_write_rgb_image(
    FILE* stream,
    const hicolor_metadata meta,
    const hicolor_rgb* image
);

#endif /* HICOLOR_H */

/* -------------------------------------------------------------------------- */

#ifdef HICOLOR_IMPLEMENTATION

const char* hicolor_error_message(hicolor_result res)
{
    switch (res) {
    case HICOLOR_OK:
        return "OK";
    case HICOLOR_IO_ERROR:
        return "I/O error";
    case HICOLOR_UNKNOWN_VERSION:
        return "unknown version";
    case HICOLOR_INVALID_VALUE:
        return "invalid value";
    case HICOLOR_INSUFFICIENT_DATA:
        return "insufficient data";
    case HICOLOR_BAD_MAGIC:
        return "bad magic value";
    default:
        return "";
    }
}

hicolor_result hicolor_char_to_version(
    const uint8_t ch,
    hicolor_version* version
)
{
    switch (ch) {
    case '5':
        *version = HICOLOR_VERSION_5;
        return HICOLOR_OK;
    case '6':
        *version = HICOLOR_VERSION_6;
        return HICOLOR_OK;
    default:
        return HICOLOR_UNKNOWN_VERSION;
    };
}


hicolor_result hicolor_version_to_char(
    const hicolor_version version,
    uint8_t* ch
)
{
    switch (version) {
    case HICOLOR_VERSION_5:
        *ch = '5';
        return HICOLOR_OK;
    case HICOLOR_VERSION_6:
        *ch = '6';
        return HICOLOR_OK;
    default:
        return HICOLOR_UNKNOWN_VERSION;
    };
}

hicolor_result hicolor_value_to_rgb(
    const hicolor_version version,
    const hicolor_value value,
    hicolor_rgb* rgb
)
{
    switch (version) {
    case HICOLOR_VERSION_5:
        if (value & 0x8000) return HICOLOR_INVALID_VALUE;
        rgb->r = (value & 0x1f) << 3;
        rgb->g = (value & 0x3ff) >> (5 - 3);
        rgb->b = (value & 0x7fff) >> (10 - 3);
        return HICOLOR_OK;
    case HICOLOR_VERSION_6:
        rgb->r = (value & 0x1f) << 3;
        rgb->g = (value & 0x7ff) >> (5 - 2);
        rgb->b = value >> (11 - 3);
        return HICOLOR_OK;
    default:
        return HICOLOR_UNKNOWN_VERSION;
    };
}

hicolor_result hicolor_rgb_to_value(
    const hicolor_version version,
    const hicolor_rgb rgb,
    hicolor_value* value
)
{
    switch (version) {
    case HICOLOR_VERSION_5:
        *value = (rgb.r >> 3) | (rgb.g >> 3 << 5) | (rgb.b >> 3 << 10);
        return HICOLOR_OK;
    case HICOLOR_VERSION_6:
        *value = (rgb.r >> 3) | (rgb.g >> 2 << 5) | (rgb.b >> 3 << 11);
        return HICOLOR_OK;
    default:
        return HICOLOR_UNKNOWN_VERSION;
    };
}

hicolor_result hicolor_read_header(
    FILE* stream,
    hicolor_metadata* meta
)
{
    size_t total = 0;
    hicolor_result res;

    uint8_t magic[7];
    total += fread(magic, 1, sizeof(magic), stream);
    if (memcmp(magic, hicolor_magic, sizeof(magic)) != 0) {
        return HICOLOR_BAD_MAGIC;
    }

    uint8_t vch;
    total += fread(&vch, 1, sizeof(vch), stream);
    res = hicolor_char_to_version(vch, &meta->version);
    if (res != HICOLOR_OK) {
        return res;
    }

    uint8_t b[2];
    total += fread(&b, 1, sizeof(b), stream);
    meta->width = b[0] + (b[1] << 8);
    total += fread(&b, 1, sizeof(b), stream);
    meta->height = b[0] + (b[1] << 8);

    if (total == 12) return HICOLOR_OK;

    return HICOLOR_INSUFFICIENT_DATA;
}

hicolor_result hicolor_write_header(
    FILE* stream,
    const hicolor_metadata meta
)
{
    size_t total = 0;

    total += fwrite(hicolor_magic, 1, sizeof(hicolor_magic), stream);

    uint8_t vch;
    hicolor_result res = hicolor_version_to_char(meta.version, &vch);
    if (res != HICOLOR_OK) return res;
    total += fwrite(&vch, 1, sizeof(vch), stream);

    uint8_t wb1 = meta.width & 0xff;
    uint8_t wb2 = (meta.width >> 8) & 0xff;
    total += fwrite(&wb1, 1, sizeof(wb1), stream);
    total += fwrite(&wb2, 1, sizeof(wb2), stream);

    uint8_t hb1 = meta.height & 0xff;
    uint8_t hb2 = (meta.height >> 8) & 0xff;
    total += fwrite(&hb1, 1, sizeof(hb1), stream);
    total += fwrite(&hb2, 1, sizeof(hb2), stream);

    if (total == 12) return HICOLOR_OK;

    return HICOLOR_IO_ERROR;
}

uint8_t hicolor_bayerize_channel(
    uint8_t value,
    double factor,
    double threshold
)
{
    double bv = (double) value + factor * threshold;
    if (bv < 0) bv = 0;
    if (bv > 255) bv = 255;

    return (uint8_t) bv;
}

void hicolor_bayerize_rgb(
    hicolor_version version,
    uint16_t x,
    uint16_t y,
    const hicolor_rgb rgb,
    hicolor_rgb* output
)
{
    uint8_t bayer_coord =
        (y % HICOLOR_BAYER_SIZE) * HICOLOR_BAYER_SIZE +
        x % HICOLOR_BAYER_SIZE;
    double factor = hicolor_bayer[bayer_coord];

    double threshold = 8.0;
    double threshold_g = version == HICOLOR_VERSION_5 ? threshold : 4.0;

    output->r = hicolor_bayerize_channel(rgb.r, factor, threshold);
    output->g = hicolor_bayerize_channel(rgb.g, factor, threshold_g);
    output->b = hicolor_bayerize_channel(rgb.b, factor, threshold);
}

hicolor_result hicolor_quantize_rgb_image(
    const hicolor_metadata meta,
    bool dither,
    hicolor_rgb* image
)
{
    hicolor_rgb rgb;
    hicolor_value value;

    for (uint16_t y = 0; y < meta.height; y++) {
        for (uint16_t x = 0; x < meta.width; x++) {
            rgb = image[y * meta.width + x];

            hicolor_rgb quant_rgb = rgb;
            if (dither) {
                hicolor_bayerize_rgb(meta.version, x, y, rgb, &quant_rgb);
            }

            hicolor_result res = hicolor_rgb_to_value(
                meta.version,
                quant_rgb,
                &value
            );
            if (res != HICOLOR_OK) {
                return res;
            }

            res = hicolor_value_to_rgb(
                meta.version,
                value,
                &image[y * meta.width + x]
            );
            if (res != HICOLOR_OK) {
                return res;
            }
        }
    }

    return HICOLOR_OK;
};

hicolor_result hicolor_read_rgb_image(
    FILE* stream,
    const hicolor_metadata meta,
    hicolor_rgb* image
)
{
    size_t total = 0;

    for (int i = 0; i < meta.width * meta.height; i++) {
        hicolor_value value;
        total += fread(&value, 1, sizeof(value), stream);

        hicolor_result res =
            hicolor_value_to_rgb(meta.version, value, &image[i]);
        if (res != HICOLOR_OK) return res;
    }

    if (total == 2 * meta.width * meta.height) return HICOLOR_OK;

    return HICOLOR_INSUFFICIENT_DATA;
}

hicolor_result hicolor_write_rgb_image(
    FILE* stream,
    const hicolor_metadata meta,
    const hicolor_rgb* image
)
{
    size_t total = 0;

    for (int i = 0; i < meta.width * meta.height; i++) {
        hicolor_value value;
        hicolor_result res =
            hicolor_rgb_to_value(meta.version, image[i], &value);
        if (res != HICOLOR_OK) return res;

        total += fwrite(&value, 1, sizeof(value), stream);
    }

    if (total == 2 * meta.width * meta.height) return HICOLOR_OK;

    return HICOLOR_IO_ERROR;
}

#endif /* HICOLOR_IMPLEMENTATION */
