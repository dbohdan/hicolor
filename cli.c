/* HiColor CLI.
 *
 * Copyright (c) 2021, 2023-2024 D. Bohdan and contributors listed in AUTHORS.
 * License: MIT.
 */

#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <png.h>
#include <zlib.h>

#define HICOLOR_IMPLEMENTATION
#include "hicolor.h"

#define HICOLOR_CLI_ERROR "error: "
#define HICOLOR_CLI_LIB_NAME_FORMAT "%-9s"
#define HICOLOR_CLI_LIBPNG_COMPRESSION_LEVEL 6
#define HICOLOR_CLI_NO_MEMORY_EXIT_CODE 255

#define HICOLOR_CLI_CMD_ENCODE "encode"
#define HICOLOR_CLI_CMD_QUANTIZE "quantize"
#define HICOLOR_CLI_CMD_DECODE "decode"
#define HICOLOR_CLI_CMD_INFO "info"
#define HICOLOR_CLI_CMD_VERSION "version"
#define HICOLOR_CLI_CMD_HELP "help"

const char* png_error_msg = "no error recorded";

void libpng_error_handler(
    png_structp png_ptr,
    png_const_charp error_msg
)
{
    png_error_msg = error_msg;
    longjmp(png_jmpbuf(png_ptr), 1);
}

bool load_png(
    const char* filename,
    int* width,
    int* height,
    hicolor_rgb** rgb_img,
    uint8_t** alpha
)
{
    FILE* fp = fopen(filename, "rb");
    if (!fp) {
        png_error_msg = "failed to open for reading";
        return false;
    }

    png_structp png = png_create_read_struct(PNG_LIBPNG_VER_STRING, NULL, libpng_error_handler, NULL);
    if (png == NULL) {
        png_error_msg = "`png_create_read_struct` returned null";
        fclose(fp);
        return false;
    }

    png_infop info = png_create_info_struct(png);
    if (info == NULL) {
        png_error_msg = "`png_create_info_struct` returned null";
        png_destroy_read_struct(&png, NULL, NULL);
        fclose(fp);
        return false;
    }

    if (setjmp(png_jmpbuf(png))) {
        /* Do not overwrite `png_error_msg` set by the handler. */
        png_destroy_read_struct(&png, &info, NULL);
        fclose(fp);
        return false;
    }

    png_init_io(png, fp);
    png_read_info(png, info);

    *width = png_get_image_width(png, info);
    *height = png_get_image_height(png, info);
    png_byte color_type = png_get_color_type(png, info);
    png_byte bit_depth = png_get_bit_depth(png, info);

    if (bit_depth == 16) {
        png_set_strip_16(png);
    }

    if (color_type == PNG_COLOR_TYPE_PALETTE) {
        png_set_palette_to_rgb(png);
    }

    if (color_type == PNG_COLOR_TYPE_GRAY && bit_depth < 8) {
        png_set_expand_gray_1_2_4_to_8(png);
    }

    if (png_get_valid(png, info, PNG_INFO_tRNS)) {
        png_set_tRNS_to_alpha(png);
    }

    if (color_type == PNG_COLOR_TYPE_RGB
        || color_type == PNG_COLOR_TYPE_GRAY
        || color_type == PNG_COLOR_TYPE_PALETTE) {
        png_set_filler(png, 0xFF, PNG_FILLER_AFTER);
    }

    if (color_type == PNG_COLOR_TYPE_GRAY
        || color_type == PNG_COLOR_TYPE_GRAY_ALPHA) {
        png_set_gray_to_rgb(png);
    }

    png_read_update_info(png, info);

    *rgb_img = malloc(sizeof(hicolor_rgb) * *width * *height);
    *alpha = malloc(sizeof(uint8_t) * *width * *height);

    if (*rgb_img == NULL || *alpha == NULL) {
        png_error_msg = "failed to allocate memory for `rgb_img` or `alpha`";
        png_destroy_read_struct(&png, &info, NULL);
        fclose(fp);
        return false;
    }

    png_bytep row = malloc(png_get_rowbytes(png, info));
    if (row == NULL) {
        png_error_msg = "failed to allocate memory for `row`";
        free(*rgb_img);
        free(*alpha);
        png_destroy_read_struct(&png, &info, NULL);
        fclose(fp);
        return false;
    }

    for (int y = 0; y < *height; y++) {
        png_read_row(png, row, NULL);

        for (int x = 0; x < *width; x++) {
            png_bytep pixel = &(row[x * 4]);
            (*rgb_img)[y * (*width) + x].r = pixel[0];
            (*rgb_img)[y * (*width) + x].g = pixel[1];
            (*rgb_img)[y * (*width) + x].b = pixel[2];
            (*alpha)[y * (*width) + x] = pixel[3];
        }
    }

    free(row);
    png_destroy_read_struct(&png, &info, NULL);
    fclose(fp);

    return true;
}

bool save_png(
    const char* filename,
    int width,
    int height,
    const hicolor_rgb* rgb_img,
    const uint8_t* alpha
)
{
    FILE* fp = fopen(filename, "wb");
    if (!fp) {
        png_error_msg = "failed to open for writing";
        return false;
    }

    png_structp png = png_create_write_struct(PNG_LIBPNG_VER_STRING, NULL, libpng_error_handler, NULL);
    if (png == NULL) {
        png_error_msg = "`png_create_write_struct` returned null";
        fclose(fp);
        return false;
    }

    png_infop info = png_create_info_struct(png);
    if (info == NULL) {
        png_error_msg = "`png_create_info_struct` returned null";
        png_destroy_write_struct(&png, NULL);
        fclose(fp);
        return false;
    }

    if (setjmp(png_jmpbuf(png))) {
        /* Do not overwrite `png_error_msg` set by the handler. */
        png_destroy_write_struct(&png, &info);
        fclose(fp);
        return false;
    }

    png_init_io(png, fp);

    png_set_IHDR(
        png,
        info,
        width,
        height,
        8,
        PNG_COLOR_TYPE_RGBA,
        PNG_INTERLACE_NONE,
        PNG_COMPRESSION_TYPE_DEFAULT,
        PNG_FILTER_TYPE_DEFAULT
    );
    png_set_compression_level(png, HICOLOR_CLI_LIBPNG_COMPRESSION_LEVEL);
    png_write_info(png, info);

    png_bytep row = malloc(png_get_rowbytes(png, info));
    if (row == NULL) {
        png_error_msg = "failed to allocate memory for `row`";
        png_destroy_write_struct(&png, &info);
        fclose(fp);
        return false;
    }

    for (int y = 0; y < height; y++) {
        for (int x = 0; x < width; x++) {
            png_bytep pixel = &(row[x * 4]);
            pixel[0] = rgb_img[y * width + x].r;
            pixel[1] = rgb_img[y * width + x].g;
            pixel[2] = rgb_img[y * width + x].b;
            pixel[3] = alpha == NULL ? 255 : alpha[y * width + x];
        }

        png_write_row(png, row);
    }

    free(row);
    png_write_end(png, NULL);
    png_destroy_write_struct(&png, &info);
    fclose(fp);

    return true;
}

bool check_and_report_error(
    char* step,
    hicolor_result res
)
{
    if (res == HICOLOR_OK) {
        return false;
    }

    fprintf(
        stderr,
        HICOLOR_CLI_ERROR "%s: %s\n",
        step,
        hicolor_error_message(res)
    );

    return true;
}

bool check_src_exists(
    const char* src
)
{
    if (access(src, F_OK) != 0) {
        fprintf(
            stderr,
            HICOLOR_CLI_ERROR "source image \"%s\" doesn't exist\n",
            src
        );
        return false;
    }

    return true;
}

bool png_to_hicolor(
    hicolor_version version,
    hicolor_dither dither,
    const char* src,
    const char* dest
)
{
    hicolor_result res;

    bool exists = check_src_exists(src);
    if (!exists) {
        return false;
    }

    int width, height;
    hicolor_rgb* rgb_img = NULL;
    uint8_t* alpha = NULL;
    if (!load_png(src, &width, &height, &rgb_img, &alpha)) {
        fprintf(
            stderr,
            HICOLOR_CLI_ERROR "can't load PNG file \"%s\": %s\n",
            src,
            png_error_msg
        );
        return false;
    }

    FILE* hi_file = fopen(dest, "wb");
    if (hi_file == NULL) {
        fprintf(
            stderr,
            HICOLOR_CLI_ERROR "can't open file \"%s\" for writing\n",
            dest
        );

        free(rgb_img);
        return false;
    }

    hicolor_metadata meta = {
        .version = version,
        .width = width,
        .height = height
    };
    res = hicolor_write_header(hi_file, meta);

    bool success = false;
    if (check_and_report_error("can't write header", res)) {
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
    free(rgb_img);

clean_up_file:
    fclose(hi_file);

    return success;
}

bool png_quantize(
    hicolor_version version,
    hicolor_dither dither,
    const char* src,
    const char* dest
)
{
    hicolor_result res;

    bool exists = check_src_exists(src);
    if (!exists) {
        return false;
    }

    int width, height;
    hicolor_rgb* rgb_img = NULL;
    uint8_t* alpha = NULL;
    if (!load_png(src, &width, &height, &rgb_img, &alpha)) {
        fprintf(
            stderr,
            HICOLOR_CLI_ERROR "can't load PNG file \"%s\": %s\n",
            src,
            png_error_msg
        );
        return false;
    }

    hicolor_metadata meta = {
        .version = version,
        .width = width,
        .height = height
    };

    res = hicolor_quantize_rgb_image(meta, dither, rgb_img);
    bool success = false;
    if (check_and_report_error("can't quantize image", res)) {
        goto clean_up_images;
    }

    if (!save_png(dest, width, height, rgb_img, alpha)) {
        fprintf(
            stderr,
            HICOLOR_CLI_ERROR "can't save PNG: %s\n",
            png_error_msg
        );
        goto clean_up_images;
    }

    success = true;

clean_up_images:
    free(rgb_img);

    return success;
}

bool hicolor_to_png(
    const char* src,
    const char* dest
)
{
    hicolor_result res;

    bool exists = check_src_exists(src);
    if (!exists) {
        return false;
    }

    FILE* hi_file = fopen(src, "rb");
    if (hi_file == NULL) {
        fprintf(stderr, HICOLOR_CLI_ERROR "can't open source image \"%s\" for reading\n", src);
        return false;
    }

    hicolor_metadata meta;
    res = hicolor_read_header(hi_file, &meta);
    bool success = false;
    if (check_and_report_error("can't read header", res)) {
        goto clean_up_file;
    }

    hicolor_rgb* rgb_img = malloc(sizeof(hicolor_rgb) * meta.width * meta.height);
    if (rgb_img == NULL) {
        goto clean_up_file;
    }
    res = hicolor_read_rgb_image(hi_file, meta, rgb_img);
    if (check_and_report_error("can't read image data", res)) {
        goto clean_up_rgb_img;
    }

    if (!save_png(dest, meta.width, meta.height, rgb_img, NULL)) {
        fprintf(
            stderr,
            HICOLOR_CLI_ERROR "can't save PNG: %s\n",
            png_error_msg
        );
        goto clean_up_rgb_img;
    }

    success = true;

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

    bool exists = check_src_exists(src);
    if (!exists) {
        return false;
    }

    FILE* hi_file = fopen(src, "rb");
    if (hi_file == NULL) {
        fprintf(
            stderr,
            HICOLOR_CLI_ERROR "can't open source image \"%s\" for reading\n",
            src
        );
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

void usage(
    FILE* output
)
{
    fprintf(
        output,
        "usage:\n"
        "  hicolor (encode|quantize) [-5|-6] [-a|-b|-n] [--] <src> [<dest>]\n"
        "  hicolor decode <src> [<dest>]\n"
        "  hicolor info <file>\n"
        "  hicolor (version|help|-h|--help)\n"
    );
}

void version(
    bool full
)
{
    if (full) {
        printf(
            HICOLOR_CLI_LIB_NAME_FORMAT,
            "HiColor"
        );
    }

    uint32_t program_version = HICOLOR_LIBRARY_VERSION;
    printf(
        "%u.%u.%u\n",
        program_version / 10000,
        program_version % 10000 / 100,
        program_version % 100
    );

    if (!full) {
        return;
    }

    png_uint_32 libpng_version = png_access_version_number();
    printf(
        HICOLOR_CLI_LIB_NAME_FORMAT "%u.%u.%u\n",
        "libpng",
        libpng_version / 10000,
        libpng_version % 10000 / 100,
        libpng_version % 100
    );

    printf(
        HICOLOR_CLI_LIB_NAME_FORMAT "%s\n",
        "zlib",
        ZLIB_VERSION
    );
}

void help()
{
    printf(
        "HiColor "
    );
    version(false);
    printf(
        "Create 15/16-bit color RGB images.\n\n"
    );
    usage(stdout);
    printf(
        "\ncommands:\n"
        "  encode           convert PNG to HiColor\n"
        "  decode           convert HiColor to PNG\n"
        "  quantize         quantize PNG to PNG\n"
        "  info             print HiColor image version and resolution\n"
        "  version          print version of HiColor, libpng, and zlib\n"
        "  help             print this help message\n"
        "\noptions:\n"
        "  -5, --15-bit     15-bit color\n"
        "  -6, --16-bit     16-bit color\n"
        "  -a, --a-dither   dither image with \"a dither\"\n"
        "  -b, --bayer      dither image with Bayer algorithm (default)\n"
        "  -n, --no-dither  do not dither image\n"
    );
}

bool str_prefix(
    const char* ref,
    const char* str
)
{
    size_t i;

    for (i = 0; str[i] != '\0'; i++) {
        if (str[i] != ref[i]) {
            return false;
        }
    }

    if (i == 0) {
        return false;
    }

    return true;
}

typedef enum command {
    ENCODE, DECODE, QUANTIZE, INFO, VERSION, HELP
} command;

int main(
    int argc,
    char** argv
)
{
    command opt_command = ENCODE;
    hicolor_dither opt_dither = HICOLOR_BAYER;
    hicolor_version opt_version = HICOLOR_VERSION_6;
    const char* command_name;
    char* arg_src;
    char* arg_dest;
    bool allow_opts = true;
    int min_pos_args = 1;
    int max_pos_args = 2;

    if (argc <= 1) {
        help();
        return 1;
    }

    /* The regular "help" command is handled later with the rest. */
    for (int i = 0; i < argc; i++) {
        if (strcmp(argv[i], "-h") == 0
            || strcmp(argv[i], "--help") == 0) {
            help();
            return 0;
        }
    }

    int i = 1;

    if (str_prefix(HICOLOR_CLI_CMD_ENCODE, argv[i])) {
        command_name = HICOLOR_CLI_CMD_ENCODE;
        opt_command = ENCODE;
    } else if (str_prefix(HICOLOR_CLI_CMD_DECODE, argv[i])) {
        allow_opts = false;
        command_name = HICOLOR_CLI_CMD_DECODE;
        opt_command = DECODE;
    } else if (str_prefix(HICOLOR_CLI_CMD_QUANTIZE, argv[i])) {
        command_name = HICOLOR_CLI_CMD_QUANTIZE;
        opt_command = QUANTIZE;
    } else if (str_prefix(HICOLOR_CLI_CMD_INFO, argv[i])) {
        allow_opts = false;
        command_name = HICOLOR_CLI_CMD_INFO;
        max_pos_args = 1;
        opt_command = INFO;
    } else if (str_prefix(HICOLOR_CLI_CMD_VERSION, argv[i])) {
        allow_opts = false;
        command_name = HICOLOR_CLI_CMD_VERSION;
        min_pos_args = 0;
        max_pos_args = 0;
        opt_command = VERSION;
    } else if (str_prefix(HICOLOR_CLI_CMD_HELP, argv[i])) {
        allow_opts = false;
        command_name = HICOLOR_CLI_CMD_HELP;
        min_pos_args = 0;
        max_pos_args = 0;
        opt_command = HELP;
    } else {
        fprintf(
            stderr,
            HICOLOR_CLI_ERROR "unknown command \"%s\"\n",
            argv[i]
        );
        usage(stderr);
        return 1;
    }

    i++;

    if (allow_opts) {
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
            } else if (strcmp(argv[i], "-a") == 0
                || strcmp(argv[i], "--a-dither") == 0) {
                opt_dither = HICOLOR_A_DITHER;
            } else if (strcmp(argv[i], "-b") == 0
                || strcmp(argv[i], "--bayer") == 0) {
                opt_dither = HICOLOR_BAYER;
            } else if (strcmp(argv[i], "-n") == 0
                || strcmp(argv[i], "--no-dither") == 0) {
                opt_dither = HICOLOR_NO_DITHER;
            } else {
                fprintf(
                    stderr,
                    HICOLOR_CLI_ERROR "unknown option \"%s\"\n",
                    argv[i]
                );
                usage(stderr);
                return 1;
            }

            i++;
        }
    }

    int rem_args = argc - i;

    if (rem_args < min_pos_args) {
        fprintf(
            stderr,
            HICOLOR_CLI_ERROR "no source image given to command \"%s\"\n",
            command_name
        );
        usage(stderr);
        return 1;
    }

    if (rem_args > max_pos_args) {
        fprintf(
            stderr,
            HICOLOR_CLI_ERROR "too many arguments to command \"%s\"\n",
            command_name
        );
        usage(stderr);
        return 1;
    }

    arg_src = argv[i];
    i++;

    if (i == argc) {
        arg_dest = malloc(strlen(arg_src) + 5);
        if (arg_dest == NULL) {
            return HICOLOR_CLI_NO_MEMORY_EXIT_CODE;
        }
        sprintf(
            arg_dest,
            opt_command == ENCODE ? "%s.hic" : "%s.png",
            arg_src
        );
    } else {
        arg_dest = argv[i];
    }
    i++;

    switch (opt_command) {
    case ENCODE:
        return !png_to_hicolor(opt_version, opt_dither, arg_src, arg_dest);
    case DECODE:
        return !hicolor_to_png(arg_src, arg_dest);
    case QUANTIZE:
        return !png_quantize(opt_version, opt_dither, arg_src, arg_dest);
    case INFO:
        return !hicolor_print_info(arg_src);
    case VERSION:
        version(true);
        return 0;
    case HELP:
        help();
        return 0;
    }
}
