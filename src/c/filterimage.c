#include <string.h>
#include <stdlib.h>
#include <math.h>
#include <stdio.h>
#include <time.h>
#include "filterimage.h"

void handle_cl_err(cl_int err_nr, int i)
{
    if (err_nr != CL_SUCCESS)
    {
        printf("CL Error %i %i\n", i, err_nr);
        exit(err_nr);
    }
}

void init_cl(cl_context *context, cl_command_queue *command_queue, cl_program *program)
{
    cl_platform_id platform_id = NULL;
    cl_device_id device_id = NULL;
    cl_uint ret_num_devices;
    cl_uint ret_num_platforms;
    cl_int ret = clGetPlatformIDs(1, &platform_id, &ret_num_platforms); // TODO: Multiple platforms/devices?
    handle_cl_err(ret, 0);
    ret = clGetDeviceIDs(platform_id, CL_DEVICE_TYPE_GPU, 1, &device_id, &ret_num_devices);
    handle_cl_err(ret, 1);

    *context = clCreateContext(NULL, 1, &device_id, NULL, NULL, &ret);
    handle_cl_err(ret, 2);

    *command_queue = clCreateCommandQueue(*context, device_id, 0, &ret);
    handle_cl_err(ret, 3);

    size_t cl_string_len = strlen(cl_string);
    *program = clCreateProgramWithSource(*context, 1, (const char **)&cl_string, (const size_t *)&cl_string_len, &ret);
    handle_cl_err(ret, 4);

    ret = clBuildProgram(*program, 1, &device_id, "-I. -g", NULL, NULL);
    size_t len = 0;
    ret = clGetProgramBuildInfo(*program, device_id, CL_PROGRAM_BUILD_LOG, 0, NULL, &len);
    char *buffer = calloc(len, sizeof(char));
    ret = clGetProgramBuildInfo(*program, device_id, CL_PROGRAM_BUILD_LOG, len, buffer, NULL);
    if (len > 1)
        printf("OpenCL build info:\n%s\n", buffer);
    handle_cl_err(ret, 5);
}

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

    return (1 / sqrtf(2 * M_PI * powf(std_dev, 2))) * powf(M_E, -powf(i, 2) / (2 * powf(std_dev, 2)));
}

unsigned char **filter(unsigned char **image, int width, int height, int channel_count, int kernel_radius, float (*filter_fun)(int i, int radius), OverflowMode overflow_mode)
{
    int padding = kernel_radius * 2;
    int padded_width = width + padding,
        padded_height = height + padding;

    clock_t start, end;
    double cpu_time_used;
    start = clock();
    unsigned char *padded_image = malloc(padded_width * padded_height * channel_count * sizeof(unsigned char));
    pad_image(image, &padded_image, width, height, padding, channel_count, overflow_mode);

    float *kernel = malloc((2 * kernel_radius + 1) * sizeof(float));
    create_1d_filter_kernel(&kernel, filter_fun, kernel_radius);

    unsigned char *filtered = malloc(padded_width * padded_height * channel_count * sizeof(unsigned char));
    unsigned char *horizontally_filtered = malloc(padded_width * padded_height * channel_count * sizeof(unsigned char));
    filter_image_separable(&filtered, &horizontally_filtered, &padded_image, padded_width, padded_height, &kernel, kernel_radius, channel_count);

    unpad_image(&filtered, image, width, height, padding, channel_count);

    end = clock();
    cpu_time_used = ((double)(end - start)) / CLOCKS_PER_SEC;
    printf("cpu_time_used: %f\n", cpu_time_used);

    free(horizontally_filtered);
    free(filtered);
    free(kernel);
    free(padded_image);
    return image;
}

// Rounds to closest power of 2. Taken from: https://stackoverflow.com/a/9194117
int round_to_closest_p2(int num_to_round, int multiple)
{
    if (multiple && ((multiple & (multiple - 1)) == 0))
    {
        return (num_to_round + multiple - 1) & -multiple;
    }

    exit(-1);
}

cl_mem cl_alloc(size_t size, cl_context context, cl_command_queue command_queue, void *data)
{
    cl_int ret;
    cl_mem buffer = clCreateBuffer(context, CL_MEM_READ_WRITE, size, NULL, &ret);
    handle_cl_err(ret, 7);
    if (data != NULL)
    {
        ret = clEnqueueWriteBuffer(command_queue, buffer, CL_TRUE, 0, size, data, 0, NULL, NULL);
        handle_cl_err(ret, 8);
    }

    return buffer;
}

void execute_kernel(cl_program program, cl_command_queue command_queue, cl_kernel *kernel, char *name, int num_args, void **args, int *args_sizes, int num_threads)
{
    cl_int ret;
    if (name != NULL)
    {
        *kernel = clCreateKernel(program, name, &ret);
        handle_cl_err(ret, 6);
    }

    for (int i = 0; i < num_args; i++)
    {
        ret = clSetKernelArg(*kernel, i, args_sizes[i], args[i]);
        handle_cl_err(ret, 9);
    }

    size_t local_item_size = 256;
    size_t global_item_size = round_to_closest_p2(num_threads, local_item_size);
    cl_event event;
    ret = clEnqueueNDRangeKernel(command_queue, *kernel, 1, NULL, &global_item_size, &local_item_size, 0, NULL, &event);
    handle_cl_err(ret, 10);
    clWaitForEvents(1, &event);
}

unsigned char **filter_cl(unsigned char **image, int width, int height, int channel_count, int kernel_radius, float (*filter_fun)(int i, int radius), OverflowMode overflow_mode)
{
    int padding = kernel_radius * 2;
    int padded_width = width + padding,
        padded_height = height + padding;

    clock_t start, end;
    double gpu_time_used;
    start = clock();

    cl_context context;
    cl_command_queue command_queue;
    cl_program program;
    init_cl(&context, &command_queue, &program);

    size_t image_size = width * height * channel_count * sizeof(unsigned char);
    cl_mem image_d = cl_alloc(image_size, context, command_queue, *image);

    size_t padded_image_size = padded_width * padded_height * channel_count * sizeof(unsigned char);
    cl_mem padded_image_d = cl_alloc(padded_image_size, context, command_queue, NULL);

    cl_kernel pad_image_cl;
    execute_kernel(
        program,
        command_queue,
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
    cl_mem kernel_d = cl_alloc(kernel_size, context, command_queue, kernel);

    size_t filtered_size = padded_width * padded_height * channel_count * sizeof(unsigned char);
    cl_mem filtered_d = cl_alloc(filtered_size, context, command_queue, NULL);
    cl_mem horizontally_filtered_d = cl_alloc(filtered_size, context, command_queue, NULL);

    cl_kernel filter_image_horizontal_cl;
    execute_kernel(
        program,
        command_queue,
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
    execute_kernel(
        program,
        command_queue,
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
    cl_int ret = clEnqueueReadBuffer(command_queue, filtered_d, CL_TRUE, 0, filtered_size, filtered, 0, NULL, &event);
    handle_cl_err(ret, 11);
    clWaitForEvents(1, &event);

    unpad_image(&filtered, image, width, height, padding, channel_count);

    end = clock();
    gpu_time_used = ((double)(end - start)) / CLOCKS_PER_SEC;
    printf("gpu_time_used: %f\n", gpu_time_used);

    free(filtered);
    free(kernel);

    ret = clFlush(command_queue);
    ret = clFinish(command_queue);
    ret = clReleaseKernel(pad_image_cl);
    ret = clReleaseProgram(program);
    ret = clReleaseMemObject(image_d);
    ret = clReleaseMemObject(horizontally_filtered_d);
    ret = clReleaseMemObject(filtered_d);
    ret = clReleaseMemObject(kernel_d);
    ret = clReleaseMemObject(padded_image_d);
    ret = clReleaseCommandQueue(command_queue);
    ret = clReleaseContext(context);
    return image;
}