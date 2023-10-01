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

#define HICOLOR_CLI_ERROR "error: "
#define HICOLOR_CLI_NO_MEMORY_EXIT_CODE 255

#define HICOLOR_CLI_CMD_ENCODE "encode"
#define HICOLOR_CLI_CMD_QUANTIZE "quantize"
#define HICOLOR_CLI_CMD_DECODE "decode"
#define HICOLOR_CLI_CMD_INFO "info"
#define HICOLOR_CLI_CMD_VERSION "version"
#define HICOLOR_CLI_CMD_HELP "help"

bool check_and_report_error(char* step, hicolor_result res)
{
    if (res == HICOLOR_OK) return false;

    fprintf(
        stderr,
        HICOLOR_CLI_ERROR "%s: %s\n",
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

bool check_src_exists(
    const char* src
) {
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

    cp_image_t png_img = cp_load_png(src);
    if (png_img.pix == 0) {
        fprintf(
            stderr,
            HICOLOR_CLI_ERROR "can't load PNG file \"%s\": %s\n",
            src,
            cp_error_reason
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
        fprintf(
            stderr,
            HICOLOR_CLI_ERROR "can't allocate memory for RGB image\n"
        );
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

    cp_image_t png_img = cp_load_png(src);
    if (png_img.pix == 0) {
        fprintf(
            stderr,
            HICOLOR_CLI_ERROR "can't load PNG file \"%s\": %s\n",
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
        fprintf(
            stderr,
            HICOLOR_CLI_ERROR "can't allocate memory for RGB image\n"
        );
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
        fprintf(stderr, HICOLOR_CLI_ERROR "can't save PNG\n");
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
        fprintf(stderr, HICOLOR_CLI_ERROR "can't save PNG\n");
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

void usage(FILE* output)
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

void version() {
    uint32_t version = HICOLOR_LIBRARY_VERSION;
    printf(
        "%u.%u.%u\n",
        version / 10000,
        version % 10000 / 100,
        version % 100
    );
}

void help()
{
    printf(
        "HiColor "
    );
    version();
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
        "  version          print program version\n"
        "  help             print this help message\n"
        "\noptions:\n"
        "  -5, --15-bit     15-bit color\n"
        "  -6, --16-bit     16-bit color\n"
        "  -a, --a-dither   dither image with \"a dither\"\n"
        "  -b, --bayer      dither image with Bayer algorithm (default)\n"
        "  -n, --no-dither  do not dither image\n"
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
    ENCODE, DECODE, QUANTIZE, INFO, VERSION, HELP
} command;

int main(int argc, char** argv)
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
        if (arg_dest == NULL) return HICOLOR_CLI_NO_MEMORY_EXIT_CODE;
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
        version();
        return 0;
    case HELP:
        help();
        return 0;
    }
}
