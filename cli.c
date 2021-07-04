#include <unistd.h>

#define CUTE_PNG_IMPLEMENTATION
#include "vendor/cute_png.h"

#define HICOLOR_IMPLEMENTATION
#include "hicolor.h"

bool verbose = false;

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

bool png_to_hicolor(
    hicolor_version version,
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

    if (verbose) {
        fprintf(stderr, "PNG image size: %i x %i\n", png_img.w, png_img.h);
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
        goto cleanup_file;
    }

    hicolor_rgb* rgb_img = malloc(sizeof(hicolor_rgb) * png_img.w * png_img.h);
    for (uint32_t i = 0; i < (uint32_t) png_img.w * (uint32_t) png_img.h; i++) {
        rgb_img[i].r = png_img.pix[i].r;
        rgb_img[i].g = png_img.pix[i].g;
        rgb_img[i].b = png_img.pix[i].b;
    }

    res = hicolor_quantize_rgb_image(meta, rgb_img);
    if (check_and_report_error("can't quantize image", res)) {
        goto cleanup_images;
    }

    res = hicolor_write_rgb_image(hi_file, meta, rgb_img);
    if (check_and_report_error("can't write image data", res)) {
        goto cleanup_images;
    }

    success = true;

cleanup_images:
    free(png_img.pix);
    CUTE_PNG_MEMSET(&png_img, 0, sizeof(png_img));

    free(rgb_img);

cleanup_file:
    fclose(hi_file);

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
        goto cleanup_file;
    }

    if (verbose) {
        fprintf(
            stderr,
            "HiColor image size: %i x %i\n",
            meta.width,
            meta.height
        );
    }

    hicolor_rgb* rgb_img =
        malloc(sizeof(hicolor_rgb) * meta.width * meta.height);
    res = hicolor_read_rgb_image(hi_file, meta, rgb_img);
    if (check_and_report_error("can't read image data", res)) {
        goto cleanup_rgb_img;
    }

    cp_pixel_t* pix =
        malloc(sizeof(cp_pixel_t) * meta.width * meta.height);
    cp_image_t png_img = {
        .w = meta.width,
        .h = meta.height,
        .pix = pix
    };
    
    for (uint32_t i = 0; i < (uint32_t) png_img.w * (uint32_t) png_img.h; i++) {
        png_img.pix[i].r = rgb_img[i].r;
        png_img.pix[i].g = rgb_img[i].g;
        png_img.pix[i].b = rgb_img[i].b;
        png_img.pix[i].a = 255;
    }
    cp_save_png(dest, &png_img);
    free(pix);

    success = true;

cleanup_rgb_img:
    free(rgb_img);

cleanup_file:
    fclose(hi_file);

    return success;
}

bool print_info(
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
        goto cleanup_file;
    }

    uint8_t vch;
    res = hicolor_version_to_char(meta.version, &vch);
    if (check_and_report_error("can't decode version", res)) {
        goto cleanup_file;
    }

    printf(
        "%c %i %i\n",
        vch,
        meta.width,
        meta.height
    );

    success = true;

cleanup_file:
    fclose(hi_file);

    return success;
}

void usage()
{
    fprintf(
        stderr,
        "usage: hicolor (encode|decode) [(-5|-6|--15-bit|--16-bit)] "
        "src [dest]\n"
        "       hicolor info file\n"
        "       hicolor version\n"
        "       hicolor help\n"
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

typedef enum command {
    ENCODE, DECODE
} command;

int main(int argc, char** argv)
{
    command opt_command = ENCODE;
    hicolor_version opt_version = HICOLOR_VERSION_6;
    char* opt_src;
    char* opt_dest;

    if (argc == 2 && strcmp(argv[1], "version") == 0) {
        version();
        return 0;
    }

    if (argc == 2 && strcmp(argv[1], "help") == 0) {
        usage();
        return 0;
    }

    if (argc == 3 && strcmp(argv[1], "info") == 0) {
        return !print_info(argv[2]);
    }

    if (argc < 3 || argc > 5) {
        fprintf(
            stderr,
            argc < 3 ? "too few arguments\n" : "too many arguments\n"
        );
        usage();
        return 1;
    }

    int i = 1;

    if (strcmp(argv[i], "encode") == 0) {
        opt_command = ENCODE;
    } else if (strcmp(argv[i], "decode") == 0) {
        opt_command = DECODE;
    } else {
        fprintf(stderr, "invalid command\n");
        usage();
        return 1;
    }
    i++;

    if (strcmp(argv[i], "-5") == 0 || strcmp(argv[i], "--15-bit") == 0) {
        opt_version = HICOLOR_VERSION_5;
        i++;
    } else if (strcmp(argv[i], "-6") == 0 || strcmp(argv[i], "--16-bit") == 0) {
        opt_version = HICOLOR_VERSION_6;
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
        sprintf(
            opt_dest,
            opt_command == ENCODE ? "%s.hic" : "%s.png",
            opt_src
        );
    } else {
        opt_dest = argv[i];
    }
    i++;

    if (opt_command == ENCODE) {
        return !png_to_hicolor(opt_version, opt_src, opt_dest);
    } else {
        return !hicolor_to_png(opt_src, opt_dest);
    }
}
