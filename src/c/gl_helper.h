#ifdef __APPLE__
#include "lib/include/glfw3.h"
// #include <GLUT/glut.h>
#include <OpenGL/gl.h>

#else
// #include <GL/glut.h>
#endif

void gl_init_window(GLFWwindow **window, char *title, int width, int height);

void gl_loop(GLFWwindow **window, void (*render_callback)(GLFWwindow **window), void (*window_size_callback)(GLFWwindow **window, int w, int h));

void gl_draw(GLFWwindow **window, unsigned char *image, int width, int height, int channel_count);

void gl_draw_fixed(GLFWwindow **window, unsigned char *image, int width, int height, int channel_count);