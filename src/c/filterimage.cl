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

__kernel void
filter_image_separable(__global unsigned char *filtered,
                       __global unsigned char *horizontally_filtered,
                       __global unsigned char *image, int w, int h,
                       __global float *filter_kernel, int kernel_radius,
                       int channel_count) {
  return;
}
