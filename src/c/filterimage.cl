#include "filterimage_types.h"

__kernel void pad_image_cl(__global unsigned char *image,
                           __global unsigned char *padded, int original_w,
                           int original_h, int padding, int channel_count,
                           OverflowMode overflow_mode) {
  int padding_left = padding * channel_count / 2;
  int padding_top = padding / 2;
  int padded_width = (original_w + padding) * channel_count;
  int padded_height = original_h + padding;
  int padded_image_start = padded_width * padding_top + padding_left;

  int thread_id = get_global_id(0);
  int row = thread_id;
  if (row > original_h)
    return;

  __global unsigned char *image_row = image + row * original_w * channel_count;

  for (int i = 0; i < original_w * channel_count; i++) {
    padded[padded_image_start + padded_width * row + i] = image_row[i];
  }

  if (overflow_mode == REPEAT) {
    for (int i = 0; i < padding / 2; i++) {
      for (int c = 0; c < channel_count; c++) {
        int color_offset = i * channel_count + c;
        padded[padded_image_start - padding_left + padded_width * row +
               color_offset] = image_row[c];
        padded[padded_image_start - padding_left + padded_width * (row + 1) -
               padding_left + color_offset] =
            image_row[original_w * channel_count - channel_count + c];
      }
    }

    if (row > 0 && row < original_h - 1) {
      return; // Only repeat pixels horizontally on top and bottom.
    }

    for (int col = 0; col < padded_width; col++) {
      for (int i = 0; i < padding / 2; i++) {
        padded[col + padded_width * i] =
            padded[padded_image_start - padding_left + col];
        padded[padded_width * padded_height - col - padded_width * i] =
            padded[padded_width * padded_height - padded_image_start - col];
      }
    }
  }
}

unsigned char filter_region_one_channel_horizontal(
    __global unsigned char *image, int width, int start, int end,
    __global float *filter_kernel, int kernel_radius, int channel_count) {
  int midpoint = (start + end) / 2;
  float start_row = (float)floor((float)start / width);
  float midpoint_row = (float)floor((float)midpoint / width);
  float end_row = (float)floor((float)end / width);

  int end_overflowing = end_row > midpoint_row;
  int curr_row_last_index = width * (midpoint_row + 1) - 1;
  if (end_overflowing) {
    end = curr_row_last_index;
  }

  int start_underflowing = start_row < midpoint_row;
  int curr_row_first_index = width * midpoint_row;
  int curr_channel = end % channel_count;
  if (start_underflowing) {
    start = curr_row_first_index + curr_channel;
  }

  float result = 0;
  int ki = 0;
  for (int i = start; i <= end; i += channel_count) {
    result += image[i] * filter_kernel[ki++];
  }

  return result;
}

unsigned char filter_region_one_channel_vertical(__global unsigned char *image,
                                                 int width, int height,
                                                 int start, int end,
                                                 __global float *filter_kernel,
                                                 int kernel_radius) {
  float start_row = (float)floor((float)start / width);
  float end_row = (float)ceil((float)end / width);

  int start_col = start % width;
  int end_col = end % width;

  int end_overflowing = end_row > height;
  int curr_col_last_index = width * (height - 1) + start_col;
  if (end_overflowing) {
    end = curr_col_last_index;
  }

  int start_underflowing = start_row < 0;
  if (start_underflowing) {
    start = end_col;
  }

  float result = 0;
  int ki = 0;
  for (int i = start; i <= end; i += width) {
    result += image[i] * filter_kernel[ki++];
  }

  return result;
}

__kernel void filter_image_horizontal_cl(__global unsigned char *filtered,
                                         __global unsigned char *image, int w,
                                         int h, __global float *filter_kernel,
                                         int kernel_radius, int channel_count) {
  const int width = w * channel_count;
  const int height = h;
  int tid = get_global_id(0);

  int x = tid % width;

  int start;
  int end;
  if (x >= kernel_radius * channel_count &&
      x < width - kernel_radius * channel_count) {
    start = tid - kernel_radius * channel_count;
    end = tid + kernel_radius * channel_count;

    filtered[tid] = filter_region_one_channel_horizontal(
        image, width, start, end, filter_kernel, kernel_radius, channel_count);
  }
}

__kernel void filter_image_vertical_cl(__global unsigned char *filtered,
                                       __global unsigned char *image, int w,
                                       int h, __global float *filter_kernel,
                                       int kernel_radius, int channel_count) {
  const int width = w * channel_count;
  const int height = h;
  int tid = get_global_id(0);

  int x = tid % width;
  int y = tid / width;

  int start;
  int end;
  if (y >= kernel_radius - 1 && y <= height - kernel_radius &&
      x >= kernel_radius * channel_count &&
      x < width - kernel_radius * channel_count) {
    start = tid - kernel_radius * width;
    end = tid + kernel_radius * width;

    filtered[tid] = filter_region_one_channel_vertical(
        image, width, height, start, end, filter_kernel, kernel_radius);
  }
}
