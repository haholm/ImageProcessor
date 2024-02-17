#include <string.h>
#include <stdlib.h>
#include <math.h>
#include <sys/time.h>
#include <stdio.h>
#include "filterimage.h"

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
    for (int y = 0; y < height; y++)
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

void pad_image(unsigned char **image, unsigned char **padded, int original_w, int original_h, int padding, int channel_count, OverflowMode overflow_mode)
{
    int padding_left = padding * channel_count / 2;
    int padding_top = padding / 2;
    int padded_width = (original_w + padding) * channel_count;
    int padded_height = original_h + padding;
    int padded_image_start = padded_width * padding_top + padding_left;

    for (int row = 0; row < original_h; row++)
    {
        unsigned char *image_row = *image + row * original_w * channel_count;

        for (int i = 0; i < original_w * channel_count; i++)
        {
            (*padded)[padded_image_start + padded_width * row + i] = image_row[i];
        }

        if (overflow_mode == REPEAT)
        {
            for (int i = 0; i < padding / 2; i++)
            {
                for (int c = 0; c < channel_count; c++)
                {
                    int color_offset = i * channel_count + c;
                    (*padded)[padded_image_start - padding_left + padded_width * row + color_offset] = image_row[c];
                    (*padded)[padded_image_start - padding_left + padded_width * (row + 1) - padding_left + color_offset] = image_row[original_w * channel_count - channel_count + c];
                }
            }

            if (row > 0 && row < original_h - 1)
            {
                continue; // Only repeat pixels horizontally on top and bottom.
            }

            for (int col = 0; col < padded_width; col++)
            {
                for (int i = 0; i < padding / 2; i++)
                {
                    (*padded)[col + padded_width * i] = (*padded)[padded_image_start - padding_left + col];
                    (*padded)[padded_width * padded_height - col - padded_width * i] = (*padded)[padded_width * padded_height - padded_image_start - col];
                }
            }
        }
    }
}

void unpad_image(unsigned char **padded, unsigned char **image, int original_w, int original_h, int padding, int channel_count)
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

void create_1d_filter_kernel(float **kernel, float (*f)(int, int), int radius)
{
    for (int i = -radius; i <= radius; i++)
    {
        (*kernel)[i + radius] = f(i, radius);
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

    // Normalization factor based on the integral of the gaussian function.
    double normal_factor = 1 / (sqrt((double)pow(std_dev, 2) * M_PI / 2.0) * (erf(radius / sqrt(2 * pow(std_dev, 2))) - erf(-radius / sqrt(2 * pow(std_dev, 2)))));
    return normal_factor * pow(M_E, -pow(i, 2) / (2 * pow(std_dev, 2)));
}

unsigned char **filter(unsigned char **image, int width, int height, int channel_count, int kernel_radius, float (*filter_fun)(int i, int radius), OverflowMode overflow_mode)
{
    int padding = kernel_radius * 2;
    int padded_width = width + padding,
        padded_height = height + padding;

    struct timeval start, end;
    double cpu_time_used;
    gettimeofday(&start, NULL);

    unsigned char *padded_image = malloc(padded_width * padded_height * channel_count * sizeof(unsigned char));
    pad_image(image, &padded_image, width, height, padding, channel_count, overflow_mode);

    float *kernel = malloc((2 * kernel_radius + 1) * sizeof(float));
    create_1d_filter_kernel(&kernel, filter_fun, kernel_radius);

    unsigned char *filtered = malloc(padded_width * padded_height * channel_count * sizeof(unsigned char));
    unsigned char *horizontally_filtered = malloc(padded_width * padded_height * channel_count * sizeof(unsigned char));
    filter_image_separable(&filtered, &horizontally_filtered, &padded_image, padded_width, padded_height, &kernel, kernel_radius, channel_count);

    unpad_image(&filtered, image, width, height, padding, channel_count);

    gettimeofday(&end, NULL);
    cpu_time_used = (end.tv_sec - start.tv_sec) * 1000.0;    // sec to ms
    cpu_time_used += (end.tv_usec - start.tv_usec) / 1000.0; // us to ms
    printf("cpu_time_used: %f\n", cpu_time_used);

    free(horizontally_filtered);
    free(filtered);
    free(kernel);
    free(padded_image);
    return image;
}

#ifdef CL
void filter_cl(cl_handle *handle, unsigned char **image, int width, int height, int channel_count, int kernel_radius, float (*filter_fun)(int i, int radius), OverflowMode overflow_mode)
{
    int padding = kernel_radius * 2;
    int padded_width = width + padding,
        padded_height = height + padding;

    struct timeval start, end;
    double gpu_time_used;
    gettimeofday(&start, NULL);

    size_t image_size = width * height * channel_count * sizeof(unsigned char);
    cl_mem image_d = cl_alloc(image_size, handle, *image);

    size_t padded_image_size = padded_width * padded_height * channel_count * sizeof(unsigned char);
    cl_mem padded_image_d = cl_alloc(padded_image_size, handle, NULL);

    cl_kernel pad_image_cl;
    cl_execute_kernel(
        handle,
        &pad_image_cl,
        "pad_image_cl",
        7,
        (void *[]){
            &image_d, &padded_image_d, &width, &height, &padding, &channel_count, &overflow_mode},
        (int[]){
            sizeof(cl_mem), sizeof(cl_mem), sizeof(int), sizeof(int), sizeof(int), sizeof(int), sizeof(OverflowMode)},
        height);

    size_t kernel_size = (2 * kernel_radius + 1) * sizeof(float);
    float *kernel = malloc(kernel_size);
    create_1d_filter_kernel(&kernel, filter_fun, kernel_radius);
    cl_mem kernel_d = cl_alloc(kernel_size, handle, kernel);

    size_t filtered_size = padded_width * padded_height * channel_count * sizeof(unsigned char);
    cl_mem filtered_d = cl_alloc(filtered_size, handle, NULL);
    cl_mem horizontally_filtered_d = cl_alloc(filtered_size, handle, NULL);

    cl_kernel filter_image_horizontal_cl;
    cl_execute_kernel(
        handle,
        &filter_image_horizontal_cl,
        "filter_image_horizontal_cl",
        7,
        (void *[]){
            &horizontally_filtered_d,
            &padded_image_d,
            &padded_width,
            &padded_height,
            &kernel_d,
            &kernel_radius,
            &channel_count},
        (int[]){
            sizeof(cl_mem), sizeof(cl_mem), sizeof(int), sizeof(int), sizeof(cl_mem), sizeof(int), sizeof(int)},
        filtered_size);

    cl_kernel filter_image_vertical_cl;
    cl_execute_kernel(
        handle,
        &filter_image_vertical_cl,
        "filter_image_vertical_cl",
        7,
        (void *[]){
            &filtered_d,
            &horizontally_filtered_d,
            &padded_width,
            &padded_height,
            &kernel_d,
            &kernel_radius,
            &channel_count},
        (int[]){
            sizeof(cl_mem), sizeof(cl_mem), sizeof(int), sizeof(int), sizeof(cl_mem), sizeof(int), sizeof(int)},
        filtered_size);

    cl_event event;
    unsigned char *filtered = malloc(filtered_size);
    cl_int ret = clEnqueueReadBuffer(handle->command_queue, filtered_d, CL_TRUE, 0, filtered_size, filtered, 0, NULL, &event);
    cl_handle_err(ret, 11);
    clWaitForEvents(1, &event);

    unpad_image(&filtered, image, width, height, padding, channel_count);

    gettimeofday(&end, NULL);
    gpu_time_used = (end.tv_sec - start.tv_sec) * 1000.0;    // sec to ms
    gpu_time_used += (end.tv_usec - start.tv_usec) / 1000.0; // us to ms
    printf("gpu_time_used: %f\n", gpu_time_used);

    free(filtered);
    free(kernel);

    ret = clReleaseKernel(pad_image_cl);
    ret = clReleaseMemObject(image_d);
    ret = clReleaseMemObject(horizontally_filtered_d);
    ret = clReleaseMemObject(filtered_d);
    ret = clReleaseMemObject(kernel_d);
    ret = clReleaseMemObject(padded_image_d);
}

#endif