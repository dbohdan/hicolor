#ifndef HICOLOR_H
#define HICOLOR_H

#include <inttypes.h>
#include <stdbool.h>
#include <stdio.h>

#define HICOLOR_LIBRARY_VERSION 1

/* Types. */

typedef enum hicolor_version {
    HICOLOR_VERSION_5,
    HICOLOR_VERSION_6
} hicolor_version;

typedef struct hicolor_metadata {
    const hicolor_version version;
    const uint16_t width;
    const uint16_t height;
} hicolor_metadata;

typedef enum hicolor_result {
    HICOLOR_OK,
    HICOLOR_IO_ERROR,
    HICOLOR_UNKNOWN_VERSION,
    HICOLOR_INVALID_VALUE,
    HICOLOR_INSUFFICIENT_DATA
} hicolor_result;

typedef struct hicolor_rgb {
    uint8_t r;
    uint8_t g;
    uint8_t b;
} hicolor_rgb;

typedef uint16_t hicolor_value;

/* Functions. */

hicolor_result hicolor_decode_value(
    const hicolor_version version,
    const hicolor_value value,
    hicolor_rgb* rgb
);
hicolor_value hicolor_encode_value(
    const hicolor_version version,
    const hicolor_rgb rgb,
    hicolor_rgb* quant_error /* nullable */
);

hicolor_result hicolor_read_header(
    FILE* stream,
    hicolor_metadata* meta
);
hicolor_result hicolor_write_header(
    FILE* stream,
    const hicolor_metadata meta
);

hicolor_result hicolor_read_rgb_values(
    FILE* stream,
    const hicolor_metadata meta,
    hicolor_rgb* rgb
);
hicolor_result hicolor_write_rgb_values(
    FILE* stream,
    const hicolor_metadata meta,
    const hicolor_rgb* rgb
);

#endif /* HICOLOR_H */

#ifdef HICOLOR_IMPLEMENTATION

#endif /* HICOLOR_IMPLEMENTATION */
