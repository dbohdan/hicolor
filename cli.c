/* HiColor CLI.
 *
 * Copyright (c) 2021, 2023 D. Bohdan and contributors listed in AUTHORS.
 * License: MIT.
 */

#include <unistd.h>

#define CUTE_PNG_IMPLEMENTATION
#include "vendor/cute_png.h"

#define HICOLOR_IMPLEMENTATION
#include "hicolor.h"

#define HICOLOR_CLI_NO_MEMORY_EXIT_CODE 255

bool check_and_report_error(char* step, hicolor_result res)
{
    if (res == HICOLOR_OK) return false;

    fprintf(
        stderr,
        "%s: %s\n",
        step,
        hicolor_error_message(res)
    );

    return true;
}

hicolor_rgb* cp_to_rgb(const cp_image_t img)
{
    hicolor_rgb* rgb_img = malloc(sizeof(hicolor_rgb) * img.w * img.h);
    if (rgb_img == NULL) return NULL;

    for (uint32_t i = 0; i < (uint32_t) img.w * (uint32_t) img.h; i++) {
        rgb_img[i].r = img.pix[i].r;
        rgb_img[i].g = img.pix[i].g;
        rgb_img[i].b = img.pix[i].b;
    }

    return rgb_img;
}

cp_image_t rgb_to_cp(const hicolor_metadata meta, const hicolor_rgb* rgb_img)
{
    cp_pixel_t* pix =
        malloc(sizeof(cp_pixel_t) * meta.width * meta.height);
    cp_image_t img = {
        .w = meta.width,
        .h = meta.height,
        .pix = pix
    };
    if (pix == NULL) {
        img.w = 0;
        img.h = 0;
    }

    for (uint32_t i = 0; i < (uint32_t) img.w * (uint32_t) img.h; i++) {
        img.pix[i].r = rgb_img[i].r;
        img.pix[i].g = rgb_img[i].g;
        img.pix[i].b = rgb_img[i].b;
        img.pix[i].a = 255;
    }

    return img;
}

bool png_to_hicolor(
    hicolor_version version,
    bool dither,
    const char* src,
    const char* dest
)
{
    hicolor_result res;

    if (access(src, F_OK) != 0) {
        fprintf(stderr, "source image \"%s\" doesn't exist\n", src);
        return false;
    }

    cp_image_t png_img = cp_load_png(src);
    if (png_img.pix == 0) {
        fprintf(
            stderr,
            "can't load PNG file \"%s\": %s\n",
            src,
            cp_error_reason
        );
        return false;
    }

    FILE* hi_file = fopen(dest, "wb");
    if (hi_file == NULL) {
        fprintf(stderr, "can't open destination \"%s\" for writing\n", dest);
        return false;
    }

    hicolor_metadata meta = {
        .version = version,
        .width = png_img.w,
        .height = png_img.h
    };
    res = hicolor_write_header(hi_file, meta);
    bool success = false;
    if (check_and_report_error("can't write header", res)) {
        goto clean_up_file;
    }

    hicolor_rgb* rgb_img = cp_to_rgb(png_img);
    if (rgb_img == NULL) {
        fprintf(stderr, "can't allocate memory for RGB image\n");
        goto clean_up_file;
    }

    res = hicolor_quantize_rgb_image(meta, dither, rgb_img);
    if (check_and_report_error("can't quantize image", res)) {
        goto clean_up_images;
    }

    res = hicolor_write_rgb_image(hi_file, meta, rgb_img);
    if (check_and_report_error("can't write image data", res)) {
        goto clean_up_images;
    }

    success = true;

clean_up_images:
    free(png_img.pix);
    CUTE_PNG_MEMSET(&png_img, 0, sizeof(png_img));

    free(rgb_img);

clean_up_file:
    fclose(hi_file);

    return success;
}

bool png_quantize(
    hicolor_version version,
    bool dither,
    const char* src,
    const char* dest
)
{
    hicolor_result res;

    if (access(src, F_OK) != 0) {
        fprintf(stderr, "source image \"%s\" doesn't exist\n", src);
        return false;
    }

    cp_image_t png_img = cp_load_png(src);
    if (png_img.pix == 0) {
        fprintf(
            stderr,
            "can't load PNG file \"%s\": %s\n",
            src,
            cp_error_reason
        );
        return false;
    }

    hicolor_metadata meta = {
        .version = version,
        .width = png_img.w,
        .height = png_img.h
    };

    hicolor_rgb* rgb_img = cp_to_rgb(png_img);
    if (rgb_img == NULL) {
        fprintf(stderr, "can't allocate memory for RGB image\n");
        return false;
    }

    res = hicolor_quantize_rgb_image(meta, dither, rgb_img);
    bool success = false;
    if (check_and_report_error("can't quantize image", res)) {
        goto clean_up_images;
    }

    cp_image_t quant_png_img = rgb_to_cp(meta, rgb_img);
    /* Restore the alpha channel. */
    for (uint32_t i = 0; i < (uint32_t) png_img.w * (uint32_t) png_img.h; i++) {
        quant_png_img.pix[i].a = png_img.pix[i].a;
    }
    if (!cp_save_png(dest, &quant_png_img)) {
        fprintf(stderr, "can't save PNG\n");
        goto clean_up_quant_image;
    }

    success = true;

clean_up_quant_image:
    free(quant_png_img.pix);

clean_up_images:
    free(png_img.pix);
    CUTE_PNG_MEMSET(&png_img, 0, sizeof(png_img));

    free(rgb_img);

    return success;
}

bool hicolor_to_png(
    const char* src,
    const char* dest
)
{
    hicolor_result res;

    FILE* hi_file = fopen(src, "rb");
    if (hi_file == NULL) {
        fprintf(stderr, "can't open source image \"%s\" for reading\n", src);
        return false;
    }

    hicolor_metadata meta;
    res = hicolor_read_header(hi_file, &meta);
    bool success = false;
    if (check_and_report_error("can't read header", res)) {
        goto clean_up_file;
    }

    hicolor_rgb* rgb_img =
        malloc(sizeof(hicolor_rgb) * meta.width * meta.height);
    if (rgb_img == NULL) {
        goto clean_up_file;
    }
    res = hicolor_read_rgb_image(hi_file, meta, rgb_img);
    if (check_and_report_error("can't read image data", res)) {
        goto clean_up_rgb_img;
    }

    cp_image_t png_img = rgb_to_cp(meta, rgb_img);
    if (!cp_save_png(dest, &png_img)) {
        fprintf(stderr, "can't save PNG\n");
        goto clean_up_png_image;
    }

    success = true;

clean_up_png_image:
    free(png_img.pix);

clean_up_rgb_img:
    free(rgb_img);

clean_up_file:
    fclose(hi_file);

    return success;
}

bool hicolor_print_info(
    const char* src
)
{
    hicolor_result res;

    FILE* hi_file = fopen(src, "rb");
    if (hi_file == NULL) {
        fprintf(stderr, "can't open source image \"%s\" for reading\n", src);
        return false;
    }

    hicolor_metadata meta;
    res = hicolor_read_header(hi_file, &meta);
    bool success = false;
    if (check_and_report_error("can't read header", res)) {
        goto clean_up_file;
    }

    uint8_t vch = '\0';
    res = hicolor_version_to_char(meta.version, &vch);
    if (check_and_report_error("can't decode version", res)) {
        goto clean_up_file;
    }

    printf(
        "%c %i %i\n",
        vch,
        meta.width,
        meta.height
    );

    success = true;

clean_up_file:
    fclose(hi_file);

    return success;
}

void usage()
{
    fprintf(
        stderr,
        "usage:\n"
        "  hicolor (encode|decode|quantize) [options] src [dest]\n"
        "  hicolor info file\n"
        "  hicolor version\n"
        "  hicolor help\n"
    );
}

void help()
{
    fprintf(
        stderr,
        "HiColor\n"
        "Create 15/16-bit color RGB images.\n\n"
    );
    usage();
    fprintf(stderr,
        "\noptions:\n"
        "  -5, --15-bit     15-bit color\n"
        "  -6, --16-bit     16-bit color\n"
        "  -n, --no-dither  Do not dither the image\n"
    );
}

void version() {
    uint32_t version = HICOLOR_LIBRARY_VERSION;
    printf(
        "%u.%u.%u\n",
        version / 10000,
        version % 10000 / 100,
        version % 100
    );
}

bool str_prefix(const char* ref, const char* str)
{
    size_t i;

    for (i = 0; str[i] != '\0'; i++) {
        if (str[i] != ref[i]) return false;
    }

    if (i == 0) return false;

    return true;
}

typedef enum command {
    ENCODE, DECODE, QUANTIZE
} command;

int main(int argc, char** argv)
{
    command opt_command = ENCODE;
    bool opt_dither = true;
    hicolor_version opt_version = HICOLOR_VERSION_6;
    char* opt_src;
    char* opt_dest;

    if (argc == 2 && str_prefix("version", argv[1])) {
        version();
        return 0;
    }

    if (argc == 2 && str_prefix("help", argv[1])) {
        help();
        return 0;
    }

    if (argc == 3 && str_prefix("info", argv[1])) {
        return !hicolor_print_info(argv[2]);
    }

    if (argc < 3) {
        fprintf(stderr, "too few arguments\n");
        usage();
        return 1;
    }

    int i = 1;

    if (str_prefix("encode", argv[i])) {
        opt_command = ENCODE;
    } else if (str_prefix("decode", argv[i])) {
        opt_command = DECODE;
    } else if (str_prefix("quantize", argv[i])) {
        opt_command = QUANTIZE;
    } else {
        fprintf(stderr, "invalid command\n");
        usage();
        return 1;
    }
    i++;

    while (i < argc && argv[i][0] == '-') {
        if (strcmp(argv[i], "--") == 0) {
            i++;
            break;
        } else if (strcmp(argv[i], "-5") == 0
            || strcmp(argv[i], "--15-bit") == 0) {
            opt_version = HICOLOR_VERSION_5;
        } else if (strcmp(argv[i], "-6") == 0
            || strcmp(argv[i], "--16-bit") == 0) {
            opt_version = HICOLOR_VERSION_6;
        } else if (strcmp(argv[i], "-n") == 0
            || strcmp(argv[i], "--no-dither") == 0) {
            opt_dither = false;
        }

        i++;
    }

    if (i >= argc) {
        fprintf(stderr, "too few arguments\n");
        usage();
        return 1;
    }

    opt_src = argv[i];
    i++;

    if (i == argc) {
        opt_dest = malloc(strlen(opt_src) + 5);
        if (opt_dest == NULL) return HICOLOR_CLI_NO_MEMORY_EXIT_CODE;
        sprintf(
            opt_dest,
            opt_command == ENCODE ? "%s.hic" : "%s.png",
            opt_src
        );
    } else {
        opt_dest = argv[i];
    }
    i++;

    if (i < argc) {
        fprintf(stderr, "too many arguments\n");
        usage();
        return 1;
    }

    switch (opt_command) {
    case ENCODE:
        return !png_to_hicolor(opt_version, opt_dither, opt_src, opt_dest);
    case DECODE:
        return !hicolor_to_png(opt_src, opt_dest);
    case QUANTIZE:
        return !png_quantize(opt_version, opt_dither, opt_src, opt_dest);
    }
}
