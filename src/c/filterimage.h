#include "filterimage_types.h"

/* Convolves a horizontal filter kernel across an image region.*/
unsigned char
filter_region_one_channel_horizontal(unsigned char **image, int width, int start, int end, float **filter_kernel, int kernel_radius, int channel_count);

/* Convolves a vertical filter kernel across an image region.*/
unsigned char filter_region_one_channel_vertical(unsigned char **image, int width, int height, int start, int end, float **filter_kernel, int kernel_radius);

void filter_image_separable(unsigned char **filtered, unsigned char **horizontally_filtered, unsigned char **image, int w, int h, float **filter_kernel, int kernel_radius, int channel_count);

void create_1d_filter_kernel(float **filter_kernel, float (*f)(int, int), int radius);

void pad_image(unsigned char **image, unsigned char **padded, int original_w, int original_h, int padding, int channel_count, OverflowMode overflow_mode);

void unpad_image(unsigned char **padded, unsigned char **image, int original_w, int original_h, int padding, int channel_count);

float box_kernel_fun(int i, int radius);

float gaussian_kernel_fun(int i, int radius);

unsigned char **filter(unsigned char **image, int width, int height, int channel_count, int kernel_radius, float (*filter_fun)(int i, int radius), OverflowMode overflow_mode);

#ifdef CL
#include "cl_helper.h"
#include "gl_helper.h"

void filter_cl(cl_handle *handle, unsigned char **image, int width, int height, int channel_count, int kernel_radius, float (*filter_fun)(int i, int radius), OverflowMode overflow_mode);

static const char *cl_string = "#include \"filterimage_types.h\"\n"
                               "\n"
                               "__kernel void pad_image_cl(__global unsigned char *image,\n"
                               "                           __global unsigned char *padded, int original_w,\n"
                               "                           int original_h, int padding, int channel_count,\n"
                               "                           OverflowMode overflow_mode) {\n"
                               "  int padding_left = padding * channel_count / 2;\n"
                               "  int padding_top = padding / 2;\n"
                               "  int padded_width = (original_w + padding) * channel_count;\n"
                               "  int padded_height = original_h + padding;\n"
                               "  int padded_image_start = padded_width * padding_top + padding_left;\n"
                               "\n"
                               "  int thread_id = get_global_id(0);\n"
                               "  int row = thread_id;\n"
                               "  if (row > original_h)\n"
                               "    return;\n"
                               "\n"
                               "  __global unsigned char *image_row = image + row * original_w * channel_count;\n"
                               "\n"
                               "  for (int i = 0; i < original_w * channel_count; i++) {\n"
                               "    padded[padded_image_start + padded_width * row + i] = image_row[i];\n"
                               "  }\n"
                               "\n"
                               "  if (overflow_mode == REPEAT) {\n"
                               "    for (int i = 0; i < padding / 2; i++) {\n"
                               "      for (int c = 0; c < channel_count; c++) {\n"
                               "        int color_offset = i * channel_count + c;\n"
                               "        padded[padded_image_start - padding_left + padded_width * row +\n"
                               "               color_offset] = image_row[c];\n"
                               "        padded[padded_image_start - padding_left + padded_width * (row + 1) -\n"
                               "               padding_left + color_offset] =\n"
                               "            image_row[original_w * channel_count - channel_count + c];\n"
                               "      }\n"
                               "    }\n"
                               "\n"
                               "    if (row > 0 && row < original_h - 1) {\n"
                               "      return; // Only repeat pixels horizontally on top and bottom.\n"
                               "    }\n"
                               "\n"
                               "    for (int col = 0; col < padded_width; col++) {\n"
                               "      for (int i = 0; i < padding / 2; i++) {\n"
                               "        padded[col + padded_width * i] =\n"
                               "            padded[padded_image_start - padding_left + col];\n"
                               "        padded[padded_width * padded_height - col - padded_width * i] =\n"
                               "            padded[padded_width * padded_height - padded_image_start - col];\n"
                               "      }\n"
                               "    }\n"
                               "  }\n"
                               "}\n"
                               "\n"
                               "unsigned char filter_region_one_channel_horizontal(\n"
                               "    __global unsigned char *image, int width, int start, int end,\n"
                               "    __global float *filter_kernel, int kernel_radius, int channel_count) {\n"
                               "  int midpoint = (start + end) / 2;\n"
                               "  float start_row = (float)floor((float)start / width);\n"
                               "  float midpoint_row = (float)floor((float)midpoint / width);\n"
                               "  float end_row = (float)floor((float)end / width);\n"
                               "\n"
                               "  int end_overflowing = end_row > midpoint_row;\n"
                               "  int curr_row_last_index = width * (midpoint_row + 1) - 1;\n"
                               "  if (end_overflowing) {\n"
                               "    end = curr_row_last_index;\n"
                               "  }\n"
                               "\n"
                               "  int start_underflowing = start_row < midpoint_row;\n"
                               "  int curr_row_first_index = width * midpoint_row;\n"
                               "  int curr_channel = end % channel_count;\n"
                               "  if (start_underflowing) {\n"
                               "    start = curr_row_first_index + curr_channel;\n"
                               "  }\n"
                               "\n"
                               "  float result = 0;\n"
                               "  int ki = 0;\n"
                               "  for (int i = start; i <= end; i += channel_count) {\n"
                               "    result += image[i] * filter_kernel[ki++];\n"
                               "  }\n"
                               "\n"
                               "  return result;\n"
                               "}\n"
                               "\n"
                               "unsigned char filter_region_one_channel_vertical(__global unsigned char *image,\n"
                               "                                                 int width, int height,\n"
                               "                                                 int start, int end,\n"
                               "                                                 __global float *filter_kernel,\n"
                               "                                                 int kernel_radius) {\n"
                               "  float start_row = (float)floor((float)start / width);\n"
                               "  float end_row = (float)ceil((float)end / width);\n"
                               "\n"
                               "  int start_col = start % width;\n"
                               "  int end_col = end % width;\n"
                               "\n"
                               "  int end_overflowing = end_row > height;\n"
                               "  int curr_col_last_index = width * (height - 1) + start_col;\n"
                               "  if (end_overflowing) {\n"
                               "    end = curr_col_last_index;\n"
                               "  }\n"
                               "\n"
                               "  int start_underflowing = start_row < 0;\n"
                               "  if (start_underflowing) {\n"
                               "    start = end_col;\n"
                               "  }\n"
                               "\n"
                               "  float result = 0;\n"
                               "  int ki = 0;\n"
                               "  for (int i = start; i <= end; i += width) {\n"
                               "    result += image[i] * filter_kernel[ki++];\n"
                               "  }\n"
                               "\n"
                               "  return result;\n"
                               "}\n"
                               "\n"
                               "__kernel void filter_image_horizontal_cl(__global unsigned char *filtered,\n"
                               "                                         __global unsigned char *image, int w,\n"
                               "                                         int h, __global float *filter_kernel,\n"
                               "                                         int kernel_radius, int channel_count) {\n"
                               "  const int width = w * channel_count;\n"
                               "  const int height = h;\n"
                               "  int tid = get_global_id(0);\n"
                               "\n"
                               "  int x = tid % width;\n"
                               "\n"
                               "  int start;\n"
                               "  int end;\n"
                               "  if (x >= kernel_radius * channel_count &&\n"
                               "      x < width - kernel_radius * channel_count) {\n"
                               "    start = tid - kernel_radius * channel_count;\n"
                               "    end = tid + kernel_radius * channel_count;\n"
                               "\n"
                               "    filtered[tid] = filter_region_one_channel_horizontal(\n"
                               "        image, width, start, end, filter_kernel, kernel_radius, channel_count);\n"
                               "  }\n"
                               "}\n"
                               "\n"
                               "__kernel void filter_image_vertical_cl(__global unsigned char *filtered,\n"
                               "                                       __global unsigned char *image, int w,\n"
                               "                                       int h, __global float *filter_kernel,\n"
                               "                                       int kernel_radius, int channel_count) {\n"
                               "  const int width = w * channel_count;\n"
                               "  const int height = h;\n"
                               "  int tid = get_global_id(0);\n"
                               "\n"
                               "  int x = tid % width;\n"
                               "  int y = tid / width;\n"
                               "\n"
                               "  int start;\n"
                               "  int end;\n"
                               "  if (y >= kernel_radius - 1 && y <= height - kernel_radius &&\n"
                               "      x >= kernel_radius * channel_count &&\n"
                               "      x < width - kernel_radius * channel_count) {\n"
                               "    start = tid - kernel_radius * width;\n"
                               "    end = tid + kernel_radius * width;\n"
                               "\n"
                               "    filtered[tid] = filter_region_one_channel_vertical(\n"
                               "        image, width, height, start, end, filter_kernel, kernel_radius);\n"
                               "  }\n"
                               "}\n"
                               "\n";
#endif