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

    // --- FILTERING ---
    int padding = kernel_radius * 2;
    int padded_width = w + padding,
        padded_height = h + padding;
    unsigned char *padded_image = malloc(padded_width * padded_height * channel_count * sizeof(unsigned char));
    pad_image(&image_buffer, &padded_image, w, h, padding, channel_count);

    float *kernel = malloc((2 * kernel_radius + 1) * sizeof(float));
    create_1d_filter_kernel(&kernel, &gaussian_kernel_fun, kernel_radius);

    unsigned char *filtered = malloc(padded_width * padded_height * channel_count * sizeof(unsigned char));
    unsigned char *horizontally_filtered = malloc(padded_width * padded_height * sizeof(unsigned char));
    filter_image_separable(&filtered, &horizontally_filtered, &padded_image, padded_width, padded_height, &kernel, kernel_radius, channel_count);

    unpad_image(&image_buffer, &filtered, w, h, padding, channel_count);
    // --- FILTERING ---

    printf("Filter options:\n");
    printf("\tKernel radius: %li\n", kernel_radius);
    printf("\tKernel type: %s\n", "gaussian_kernel_1d");
    printf("\tOverflow behaviour: %s\n", "ignore (dark edges)");

    char *new_filename;
    if (asprintf(&new_filename, "%s.filtered.png", filename) == -1)
    {
        free(horizontally_filtered);
        free(filtered);
        free(kernel);
        free(padded_image);
        free(image_buffer);
        exit(-1);
    }

    unsigned char *png;
    size_t pngsize;
    error = lodepng_encode24(&png, &pngsize, image_buffer, w, h);
    if (!error)
        lodepng_save_file(png, pngsize, new_filename);
    else
        printf("Could not save file. Error %u.\n", error);

    printf("Out: %s\n", new_filename);

    free(png);
    free(new_filename);
    free(filtered);
    free(horizontally_filtered);
    free(kernel);
    free(padded_image);
    free(image_buffer);

    return 0;
}