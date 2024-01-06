#include "filterimage_types.h"

#ifdef __APPLE__
#include <OpenCL/opencl.h>
// #include <GLUT/glut.h>
// #include <OpenGL/gl.h>
#else
#include <CL/cl.h>
// #include <GL/glut.h>
#endif

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
                               "}\n";

void init_cl(cl_context *, cl_command_queue *, cl_program *);

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

unsigned char **filter_cl(unsigned char **image, int width, int height, int channel_count, int kernel_radius, float (*filter_fun)(int i, int radius), OverflowMode overflow_mode);
