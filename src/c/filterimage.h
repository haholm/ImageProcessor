
/* Convolves a horizontal filter kernel across an image region.*/
unsigned char filter_region_one_channel_horizontal(unsigned char **image, int width, int start, int end, float **kernel, int kernel_radius, int channel_count);

/* Convolves a vertical filter kernel across an image region.*/
unsigned char filter_region_one_channel_vertical(unsigned char **image, int width, int height, int start, int end, float **kernel, int kernel_radius);

void filter_image_separable(unsigned char **filtered, unsigned char **horizontally_filtered, unsigned char **image, int w, int h, float **kernel, int kernel_radius, int channel_count);

void create_1d_filter_kernel(float **kernel, float (*f)(int, int), int radius);

void pad_image(unsigned char **image, unsigned char **padded, int original_w, int original_h, int padding, int channel_amount);

void unpad_image(unsigned char **image, unsigned char **padded, int original_w, int original_h, int padding, int channel_amount);

float box_kernel_fun(int i, int radius);

float gaussian_kernel_fun(int i, int radius);
