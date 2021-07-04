#ifndef HICOLOR_H
#define HICOLOR_H

#include <inttypes.h>
#include <stdbool.h>
#include <stdio.h>

#define HICOLOR_LIBRARY_VERSION 1

/* Types. */

const uint8_t hicolor_magic[7] = "HiColor";

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

hicolor_result hicolor_quantize_rgb(
    const hicolor_metadata meta,
    const hicolor_rgb rgb,
    hicolor_rgb* quant_rgb,
    int8_t* quant_error_r,
    int8_t* quant_error_g,
    int8_t* quant_error_b
);
/* Quantize with Floyd-Steinberg dithering. */
hicolor_result hicolor_quantize_rgb_image(
    const hicolor_metadata meta,
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
){
    switch (version) {
    case HICOLOR_VERSION_5:
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

hicolor_result hicolor_quantize_rgb(
    const hicolor_metadata meta,
    const hicolor_rgb rgb,
    hicolor_rgb* quant_rgb,
    int8_t* quant_error_r,
    int8_t* quant_error_g,
    int8_t* quant_error_b
) {
    hicolor_result res;
    hicolor_value value;

    res = hicolor_rgb_to_value(meta.version, rgb, &value);
    if (res != HICOLOR_OK) {
        return res;
    }

    res = hicolor_value_to_rgb(meta.version, value, quant_rgb);
    if (res != HICOLOR_OK) {
        return res;
    }

    /* abs(quant_error_?) < 8 */
    *quant_error_r = (int8_t)((int16_t) rgb.r - (int16_t) quant_rgb->r);
    *quant_error_g = (int8_t)((int16_t) rgb.g - (int16_t) quant_rgb->g);
    *quant_error_b = (int8_t)((int16_t) rgb.b - (int16_t) quant_rgb->b);

    fprintf(stderr, "err %i %i %i\n", *quant_error_r,*quant_error_g,*quant_error_b);

    return HICOLOR_OK;
}

#define HICOLOR_2D(X, Y) ((Y) * meta.width + (X))
hicolor_result hicolor_quantize_rgb_image(
    const hicolor_metadata meta,
    hicolor_rgb* image
)
{
    hicolor_rgb quant_rgb;
    int8_t quant_error_r, quant_error_g, quant_error_b;

    for (uint16_t y = 0; y < meta.height; y++) {
        for (uint16_t x = 0; x < meta.width; x++) {
            hicolor_result res = hicolor_quantize_rgb(
                meta,
                image[HICOLOR_2D(x, y)],
                &quant_rgb,
                &quant_error_r,
                &quant_error_g,
                &quant_error_b
            );
            if (res != HICOLOR_OK) {
                return res;
            }

            image[HICOLOR_2D(x, y)] = quant_rgb;

            if (x < meta.width - 1) {
                image[HICOLOR_2D(x + 1, y)].r += quant_error_r * 7 / 16;
                image[HICOLOR_2D(x + 1, y)].g += quant_error_g * 7 / 16;
                image[HICOLOR_2D(x + 1, y)].b += quant_error_r * 7 / 16;
            }
            if (x > 0 && y < meta.height - 1) {
                image[HICOLOR_2D(x - 1, y + 1)].r += quant_error_r * 3 / 16;
                image[HICOLOR_2D(x - 1, y + 1)].g += quant_error_g * 3 / 16;
                image[HICOLOR_2D(x - 1, y + 1)].b += quant_error_b * 3 / 16;
            }
            if (y < meta.height - 1) {
                image[HICOLOR_2D(x, y + 1)].r += quant_error_r * 5 / 16;
                image[HICOLOR_2D(x, y + 1)].g += quant_error_g * 5 / 16;
                image[HICOLOR_2D(x, y + 1)].b += quant_error_b * 5 / 16;
            }
            if (x < meta.width - 1 && y < meta.height - 1) {
                image[HICOLOR_2D(x + 1, y + 1)].r += quant_error_r * 1 / 16;
                image[HICOLOR_2D(x + 1, y + 1)].g += quant_error_g * 1 / 16;
                image[HICOLOR_2D(x + 1, y + 1)].b += quant_error_b * 1 / 16;
            }
        }
    }

    return HICOLOR_OK;
};
#undef HICOLOR_2D

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
