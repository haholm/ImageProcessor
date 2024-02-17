#include "lodepng.h"
#include "filterimage.h"
#include <stdlib.h>
#include <stdio.h>
#include <math.h>

static int render_count = 0;
static int channel_count = 0;
static unsigned char *original_image_buffer = 0;
static unsigned char *image_buffer = 0;
static int image_w = 0, image_h = 0, image_size = 0;
static double kernel_radius = 0;

#ifdef CL
static cl_handle *handle = 0;

void render(GLFWwindow **window)
{
    gl_draw_fixed(window, image_buffer, image_w, image_h, channel_count);
}

void window_size_changed(GLFWwindow **window, int width, int height)
{
    printf("Window size changed to %ix%i\n", width, height);
}

void scroll_callback(GLFWwindow *window, double xoffset, double yoffset)
{
    kernel_radius = kernel_radius + yoffset;
    if (kernel_radius < 0)
        kernel_radius = 0;

    memcpy(image_buffer, original_image_buffer, image_size);
    filter_cl(handle, &image_buffer, image_w, image_h, channel_count, kernel_radius, &gaussian_kernel_fun, REPEAT);
}
#endif

int main(int argc, const char *argv[])
{
    const char *filename = argv[1];
    printf("In: %s\n", filename);

    const char *kernel_radius_str = argv[2];
    kernel_radius = strtol(kernel_radius_str, NULL, 10);

    unsigned int error;
    error = lodepng_decode_file(&image_buffer, &image_w, &image_h, filename, LCT_RGB, 8);
    if (error)
    {
        if (image_buffer != 0)
            free(image_buffer);

        exit(error);
    }

    channel_count = 3;
    image_size = image_w * image_h * channel_count;

    original_image_buffer = malloc(image_size);
    memcpy(original_image_buffer, image_buffer, image_size);

    printf("Image details:\n");
    printf("\tDimensions: (%i, %i)\n", image_w, image_h);
    printf("\tColor channels: 3 (RGB)\n");
    printf("\tBit depth: 8\n");

#ifdef CL
    GLFWwindow *window;
    gl_init_window(&window, "Blur inspector - Close to save", image_w, image_h);

    glfwSetScrollCallback(window, scroll_callback);

    handle = malloc(sizeof(cl_handle));
    cl_init(handle, cl_string);
    filter_cl(handle, &image_buffer, image_w, image_h, channel_count, kernel_radius, &gaussian_kernel_fun, REPEAT);

    gl_loop(&window, &render, &window_size_changed);

    cl_terminate(handle);
#else
    image_buffer = *filter(&image_buffer, image_w, image_h, channel_count, kernel_radius, &gaussian_kernel_fun, REPEAT);
#endif

    printf("Filter options:\n");
    printf("\tKernel radius: %li\n", kernel_radius);
    printf("\tOverflow behaviour: %i\n", REPEAT);

    char *new_filename;
    if (asprintf(&new_filename, "%s.filtered.png", filename) == -1)
    {
        free(image_buffer);
        exit(-1);
    }

    unsigned char *png;
    size_t pngsize;
    error = lodepng_encode24(&png, &pngsize, image_buffer, image_w, image_h);
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