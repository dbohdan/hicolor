/* HiColor image file format encoder/decoder library.
 *
 * Copyright (c) 2021, 2023-2025 D. Bohdan and contributors listed in AUTHORS.
 * License: MIT.
 *
 * This header file contains both the interface and the implementation for
 * HiColor. To instantiate the implementation put the line
 *     #define HICOLOR_IMPLEMENTATION
 * in a single source code file of your project above where you include this
 * file.
 */

#ifndef HICOLOR_H
#define HICOLOR_H

#include <inttypes.h>
#include <math.h>
#include <stdbool.h>
#include <stdio.h>

#define HICOLOR_BAYER_SIZE 8
#define HICOLOR_LIBRARY_VERSION 10001

/* Types. */

static const uint8_t hicolor_magic[7] = "HiColor";

/* These arrays are generated with `scripts/conversion-tables.tcl`. */
static const uint8_t hicolor_256_to_32[] = {
    0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 2, 2, 2, 2, 2, 2, 2, 2,
    3, 3, 3, 3, 3, 3, 3, 3, 4, 4, 4, 4, 4, 4, 4, 4, 5, 5, 5, 5, 5, 5, 5, 5,
    6, 6, 6, 6, 6, 6, 6, 6, 7, 7, 7, 7, 7, 7, 7, 7, 8, 8, 8, 8, 8, 8, 8, 8,
    9, 9, 9, 9, 9, 9, 9, 9, 10, 10, 10, 10, 10, 10, 10, 10, 11, 11, 11, 11,
    11, 11, 11, 11, 12, 12, 12, 12, 12, 12, 12, 12, 13, 13, 13, 13, 13, 13,
    13, 13, 14, 14, 14, 14, 14, 14, 14, 14, 15, 15, 15, 15, 15, 15, 15, 15,
    16, 16, 16, 16, 16, 16, 16, 16, 17, 17, 17, 17, 17, 17, 17, 17, 18, 18,
    18, 18, 18, 18, 18, 18, 19, 19, 19, 19, 19, 19, 19, 19, 20, 20, 20, 20,
    20, 20, 20, 20, 21, 21, 21, 21, 21, 21, 21, 21, 22, 22, 22, 22, 22, 22,
    22, 22, 23, 23, 23, 23, 23, 23, 23, 23, 24, 24, 24, 24, 24, 24, 24, 24,
    25, 25, 25, 25, 25, 25, 25, 25, 26, 26, 26, 26, 26, 26, 26, 26, 27, 27,
    27, 27, 27, 27, 27, 27, 28, 28, 28, 28, 28, 28, 28, 28, 29, 29, 29, 29,
    29, 29, 29, 29, 30, 30, 30, 30, 30, 30, 30, 30, 31, 31, 31, 31, 31, 31,
    31, 31
};

static const uint8_t hicolor_256_to_64[] = {
    0, 0, 0, 0, 1, 1, 1, 1, 2, 2, 2, 2, 3, 3, 3, 3, 4, 4, 4, 4, 5, 5, 5, 5,
    6, 6, 6, 6, 7, 7, 7, 7, 8, 8, 8, 8, 9, 9, 9, 9, 10, 10, 10, 10, 11, 11,
    11, 11, 12, 12, 12, 12, 13, 13, 13, 13, 14, 14, 14, 14, 15, 15, 15, 15,
    16, 16, 16, 16, 17, 17, 17, 17, 18, 18, 18, 18, 19, 19, 19, 19, 20, 20,
    20, 20, 21, 21, 21, 21, 22, 22, 22, 22, 23, 23, 23, 23, 24, 24, 24, 24,
    25, 25, 25, 25, 26, 26, 26, 26, 27, 27, 27, 27, 28, 28, 28, 28, 29, 29,
    29, 29, 30, 30, 30, 30, 31, 31, 31, 31, 32, 32, 32, 32, 33, 33, 33, 33,
    34, 34, 34, 34, 35, 35, 35, 35, 36, 36, 36, 36, 37, 37, 37, 37, 38, 38,
    38, 38, 39, 39, 39, 39, 40, 40, 40, 40, 41, 41, 41, 41, 42, 42, 42, 42,
    43, 43, 43, 43, 44, 44, 44, 44, 45, 45, 45, 45, 46, 46, 46, 46, 47, 47,
    47, 47, 48, 48, 48, 48, 49, 49, 49, 49, 50, 50, 50, 50, 51, 51, 51, 51,
    52, 52, 52, 52, 53, 53, 53, 53, 54, 54, 54, 54, 55, 55, 55, 55, 56, 56,
    56, 56, 57, 57, 57, 57, 58, 58, 58, 58, 59, 59, 59, 59, 60, 60, 60, 60,
    61, 61, 61, 61, 62, 62, 62, 62, 63, 63, 63, 63
};

static const uint8_t hicolor_32_to_256[] = {
    0, 8, 16, 24, 33, 41, 49, 57, 66, 74, 82, 90, 99, 107, 115, 123, 132,
    140, 148, 156, 165, 173, 181, 189, 198, 206, 214, 222, 231, 239, 247,
    255
};

static const uint8_t hicolor_64_to_256[] = {
    0, 4, 8, 12, 16, 20, 24, 28, 32, 36, 40, 44, 48, 52, 56, 60, 65, 69, 73,
    77, 81, 85, 89, 93, 97, 101, 105, 109, 113, 117, 121, 125, 130, 134,
    138, 142, 146, 150, 154, 158, 162, 166, 170, 174, 178, 182, 186, 190,
    195, 199, 203, 207, 211, 215, 219, 223, 227, 231, 235, 239, 243, 247,
    251, 255
};

/* The values in this array are the output of
 * `scripts/bayer-matrix.tcl`.
 */
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

typedef enum hicolor_dither {
    HICOLOR_A_DITHER,
    HICOLOR_BAYER,
    HICOLOR_NO_DITHER
} hicolor_dither;

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

/* Quantize using optional dithering. */
hicolor_result hicolor_quantize_rgb_image(
    const hicolor_metadata meta,
    hicolor_dither dither,
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
        rgb->r = hicolor_32_to_256[value & 0x1f];
        rgb->g = hicolor_32_to_256[(value & 0x3ff) >> 5];
        rgb->b = hicolor_32_to_256[(value & 0x7fff) >> 10];
        return HICOLOR_OK;
    case HICOLOR_VERSION_6:
        rgb->r = hicolor_32_to_256[value & 0x1f];
        rgb->g = hicolor_64_to_256[(value & 0x7ff) >> 5];
        rgb->b = hicolor_32_to_256[value >> 11];
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
        *value = hicolor_256_to_32[rgb.r]
            | hicolor_256_to_32[rgb.g] << 5
            | hicolor_256_to_32[rgb.b] << 10;
        return HICOLOR_OK;
    case HICOLOR_VERSION_6:
        *value = hicolor_256_to_32[rgb.r]
            | hicolor_256_to_64[rgb.g] << 5
            | hicolor_256_to_32[rgb.b] << 11;
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

/* "a dither" is a public-domain dithering algorithm by Øyvind Kolås.
 * This function implements pattern 3.
 * https://pippin.gimp.org/a_dither/
 */
uint8_t hicolor_a_dither_channel(
    uint8_t intensity,
    uint16_t x,
    uint16_t y,
    double levels
)
{
    double mask = (double) ((x + y * 237) * 119 & 255) / 255.0;
    double normalized = (double) intensity / 255.0;
    double dithered_normalized = floor(levels * normalized + mask) / levels;
    if (dithered_normalized > 1) {
        dithered_normalized = 1;
    }

    uint8_t result = dithered_normalized * 255;
    return result;
}

void hicolor_a_dither_rgb(
    hicolor_version version,
    uint16_t x,
    uint16_t y,
    const hicolor_rgb rgb,
    hicolor_rgb* output
)
{
    double levels = 32.0;
    double levels_g = version == HICOLOR_VERSION_5 ? levels : 64.0;

    output->r = hicolor_a_dither_channel(rgb.r, x, y, levels);
    output->g = hicolor_a_dither_channel(rgb.g, x, y, levels_g);
    output->b = hicolor_a_dither_channel(rgb.b, x, y, levels);
}

/* Ordered (Bayer) dithering. */
uint8_t hicolor_bayerize_channel(
    uint8_t intensity,
    double factor,
    double step
)
{
    double dithered = ((double) intensity) / 255 + step / 256 * factor;

    double levels = 128.0 / step;
    return (uint8_t) (round(dithered * levels) / levels * 255);
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

    double step = 8.0;
    double step_g = version == HICOLOR_VERSION_5 ? step : 4.0;

    output->r = hicolor_bayerize_channel(rgb.r, factor, step);
    output->g = hicolor_bayerize_channel(rgb.g, factor, step_g);
    output->b = hicolor_bayerize_channel(rgb.b, factor, step);
}

hicolor_result hicolor_quantize_rgb_image(
    const hicolor_metadata meta,
    hicolor_dither dither,
    hicolor_rgb* image
)
{
    hicolor_rgb rgb;
    hicolor_value value;

    for (uint16_t y = 0; y < meta.height; y++) {
        for (uint16_t x = 0; x < meta.width; x++) {
            rgb = image[y * meta.width + x];

            hicolor_rgb quant_rgb = rgb;
            if (dither == HICOLOR_A_DITHER) {
                hicolor_a_dither_rgb(meta.version, x, y, rgb, &quant_rgb);
            } else if (dither == HICOLOR_BAYER) {
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
