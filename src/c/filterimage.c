#include <string.h>
#include <stdlib.h>
#include <math.h>

/* Convolves a horizontal filter kernel across an image region.*/
unsigned char filter_region_one_channel_horizontal(unsigned char **image, int width, int start, int end, float **kernel, int kernel_radius, int channel_count)
{
    int midpoint = (start + end) / 2;
    double start_row = floor((double)start / width);
    double midpoint_row = floor((double)midpoint / width);
    double end_row = floor((double)end / width);

    int end_overflowing = end_row > midpoint_row;
    int curr_row_last_index = width * (midpoint_row + 1) - 1;
    if (end_overflowing)
    {
        end = curr_row_last_index;
    }

    int start_underflowing = start_row < midpoint_row;
    int curr_row_first_index = width * midpoint_row;
    int curr_channel = end % channel_count;
    if (start_underflowing)
    {
        start = curr_row_first_index + curr_channel;
    }

    float result = 0;
    for (int i = start; i <= end; i += channel_count)
    {
        result += (*image)[i] * (*kernel)[(i - start) / channel_count];
    }

    return result;
}

/* Convolves a vertical filter kernel across an image region.*/
unsigned char filter_region_one_channel_vertical(unsigned char **image, int width, int height, int start, int end, float **kernel, int kernel_radius)
{
    double start_row = floor((double)start / width);
    double end_row = ceil((double)end / width);

    int start_col = start % width;
    int end_col = end % width;

    int end_overflowing = end_row > height;
    int curr_col_last_index = width * (height - 1) + start_col;
    if (end_overflowing)
    {
        end = curr_col_last_index;
    }

    int start_underflowing = start_row < 0;
    if (start_underflowing)
    {
        start = end_col;
    }

    float result = 0;
    for (int i = start; i <= end; i += width)
    {
        result += (*image)[i] * (*kernel)[(i - start) / width];
    }

    return result;
}

void filter_image_separable(unsigned char **filtered, unsigned char **horizontally_filtered, unsigned char **image, int w, int h, float **kernel, int kernel_radius, int channel_count)
{
    int width = w * channel_count;
    int height = h;
    for (int y = kernel_radius; y < height - kernel_radius; y++)
    {
        for (int x = kernel_radius * channel_count; x < width - kernel_radius * channel_count; x++)
        {
            int i = x + y * width;
            int start = i - kernel_radius * channel_count;
            int end = i + kernel_radius * channel_count;
            (*horizontally_filtered)[i] = filter_region_one_channel_horizontal(
                image, width, start, end, kernel, kernel_radius, channel_count);
        }
    }

    for (int y = kernel_radius; y < height - kernel_radius; y++)
    {
        for (int x = kernel_radius * channel_count; x < width - kernel_radius * channel_count; x++)
        {
            int i = x + y * width;
            int start = i - kernel_radius * width;
            int end = i + kernel_radius * width;
            (*filtered)[i] = filter_region_one_channel_vertical(
                horizontally_filtered, width, height, start, end, kernel, kernel_radius);
        }
    }
}

void create_1d_filter_kernel(float **kernel, float (*f)(int, int), int radius)
{
    for (int i = -radius; i <= radius; i++)
    {
        (*kernel)[i + radius] = f(i, radius);
        // printf("%f%s ", (*kernel)[i + radius], i == radius - 1 ? "" : ", ");
    }
}

void pad_image(unsigned char **image, unsigned char **padded, int original_w, int original_h, int padding, int channel_count)
{
    for (unsigned char *image_ptr = *image; image_ptr < *image + original_w * channel_count * original_h; image_ptr += original_w * channel_count)
    {
        int row = (image_ptr - *image) / (original_w * channel_count);
        int padded_width = (original_w + padding) * channel_count;
        int padded_image_start = padded_width * padding / 2 + padding * channel_count / 2;
        memcpy(
            *padded + padded_image_start + padded_width * row,
            image_ptr,
            original_w * channel_count);
    }
}

void unpad_image(unsigned char **image, unsigned char **padded, int original_w, int original_h, int padding, int channel_count)
{
    for (unsigned char *image_ptr = *image; image_ptr < *image + original_w * channel_count * original_h; image_ptr += original_w * channel_count)
    {
        int row = (image_ptr - *image) / (original_w * channel_count);
        int padded_width = (original_w + padding) * channel_count;
        int padded_image_start = padded_width * padding / 2 + channel_count * padding / 2;
        memcpy(
            image_ptr,
            *padded + padded_image_start + padded_width * row,
            original_w * channel_count);
    }
}

float box_kernel_fun(int i, int radius)
{
    return 1.0 / (2 * radius + 1);
}

float gaussian_kernel_fun(int i, int radius)
{
    int std_dev = ceilf((float)radius / 3);
    if (std_dev == 0)
    {
        return 1;
    }

    return (1 / sqrtf(2 * M_PI * powf(std_dev, 2))) * powf(M_E, -powf(i, 2) / (2 * powf(std_dev, 2)));
}