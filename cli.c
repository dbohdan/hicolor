#include <unistd.h>

#define CUTE_PNG_IMPLEMENTATION
#include "vendor/cute_png.h"

#define HICOLOR_IMPLEMENTATION
#include "hicolor.h"

bool verbose = true;

void exit_if_error(char* step, hicolor_result res)
{
    if (res == HICOLOR_OK) return;

    fprintf(
        stderr,
        "%s: %s\n",
        step,
        hicolor_error_message(res)
    );
    exit(1);
}

void png_to_hicolor(
    hicolor_version version,
    const char* src,
    const char* dest
)
{
    hicolor_result res;

    if (access(src, F_OK) != 0) {
        fprintf(stderr, "source image \"%s\" doesn't exist", src);
        exit(1);
    }
    
    cp_image_t png_img = cp_load_png(src);

    if (verbose) {
        fprintf(stderr, "PNG image size: %i x %i\n", png_img.w, png_img.h);
    }

    FILE* hi_file = fopen(dest, "wb");
    if (hi_file == NULL) {
        fprintf(stderr, "can't open destination \"%s\" for writing\n", dest);
        exit(1);
    }
    
    hicolor_metadata meta = {
        .version = version,
        .width = png_img.w,
        .height = png_img.h
    };
    res = hicolor_write_header(hi_file, meta);
    exit_if_error("can't write header", res);

    hicolor_rgb* rgb_img = malloc(sizeof(hicolor_rgb) * png_img.w * png_img.h);
    for (uint32_t i = 0; i < (uint32_t) png_img.w * (uint32_t) png_img.h; i++) {
        rgb_img[i].r = png_img.pix[i].r;
        rgb_img[i].g = png_img.pix[i].g;
        rgb_img[i].b = png_img.pix[i].b;
    }
    free(png_img.pix);
    CUTE_PNG_MEMSET(&png_img, 0, sizeof(png_img));

    res = hicolor_quantize_rgb_image(meta, rgb_img);
    exit_if_error("can't quantize image", res);

    res = hicolor_write_rgb_image(hi_file, meta, rgb_img);
    exit_if_error("can't write image data", res);
    free(rgb_img);

    fclose(hi_file);
}

void hicolor_to_png(
    const char* src,
    const char* dest
)
{
    hicolor_result res;

    FILE* hi_file = fopen(src, "rb");
    if (hi_file == NULL) {
        fprintf(stderr, "can't open source image \"%s\" for reading\n", src);
        exit(1);
    }

    hicolor_metadata meta;
    res = hicolor_read_header(hi_file, &meta);
    exit_if_error("can't read header", res);

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
    exit_if_error("can't read image data", res);

    hicolor_rgb* pix =
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
    free(rgb_img);

    cp_save_png(dest, &png_img);
    free(pix);

    fclose(hi_file);
}

int main()
{
    hicolor_result res;

    png_to_hicolor(HICOLOR_VERSION_5, "photo.png", "photo.hi5");
    hicolor_to_png("photo.hi5", "photo.hi5.png");

    png_to_hicolor(HICOLOR_VERSION_6, "photo.png", "photo.hi6");
    hicolor_to_png("photo.hi6", "photo.hi6.png");
    
    return 0;
}
