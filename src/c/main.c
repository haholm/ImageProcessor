#include "lodepng.h"
#include "filterimage.h"
#include <stdlib.h>
#include <stdio.h>

int main(int argc, const char *argv[])
{
    const char *filename = argv[1];
    printf("In: %s\n", filename);

    const char *kernel_radius_str = argv[2];
    long kernel_radius = strtol(kernel_radius_str, NULL, 10);

    unsigned char *image_buffer = 0;
    unsigned int w, h;
    unsigned int error;
    error = lodepng_decode_file(&image_buffer, &w, &h, filename, LCT_RGB, 8);
    if (error)
    {
        if (image_buffer != 0)
            free(image_buffer);

        exit(error);
    }

    int channel_count = 3;

    printf("Image details:\n");
    printf("\tDimensions: (%i, %i)\n", w, h);
    printf("\tColor channels: 3 (RGB)\n");
    printf("\tBit depth: 8\n");

    image_buffer = *filter_cl(&image_buffer, w, h, channel_count, kernel_radius, &gaussian_kernel_fun, REPEAT);

    printf("Filter options:\n");
    printf("\tKernel radius: %li\n", kernel_radius);
    printf("\tKernel type: %s\n", "box_kernel_fun");
    printf("\tOverflow behaviour: %i\n", REPEAT);

    char *new_filename;
    if (asprintf(&new_filename, "%s.filtered.png", filename) == -1)
    {
        free(image_buffer);
        exit(-1);
    }

    unsigned char *png;
    size_t pngsize;
    error = lodepng_encode24(&png, &pngsize, image_buffer, w, h);
    if (!error)
    {
        lodepng_save_file(png, pngsize, new_filename);
        printf("Out: %s\n", new_filename);
    }
    else
        printf("Could not save file. Error %u.\n", error);

    free(png);
    free(new_filename);
    free(image_buffer);
    return 0;
}