#include <stdlib.h>
#include <stdio.h>

#include "gl_helper.h"

void gl_handle_err(int i)
{
    const char *error_description;
    int err_nr = glfwGetError(&error_description);

    if (error_description)
    {
        printf("GL Error %i %i %s\n", err_nr, i, error_description);
        exit(err_nr);
    }
}

/**
 * Updates glPointSize to match window and texture width. Fills in
 * dark spots when texture is larger than frame.
 *
 * @returns The amount of pixels covered by one point. Useful for
 * eliminating unnecessary draw calls.
 */
int gl_update_point_size(GLFWwindow **window, int texture_width)
{
    int fbsw, fbsh;
    glfwGetFramebufferSize(*window, &fbsw, &fbsh);
    GLfloat frame_to_texture_ratio = (GLfloat)fbsw / texture_width;
    if (frame_to_texture_ratio < 1)
    {
        glPointSize(1.0f);
        return 1 / frame_to_texture_ratio;
    }

    glPointSize((GLfloat)fbsw / texture_width);
    return 1;
}

void gl_draw(GLFWwindow **window, unsigned char *image, int width, int height, int channel_count)
{
    int point_per_pixel = gl_update_point_size(window, width);
    glBegin(GL_POINTS);
    for (int x = 0; x < width * channel_count; x += channel_count * point_per_pixel)
    {
        for (int y = 0; y < height; y += point_per_pixel)
        {
            // int x = (i / channel_count) % width, y = i / (width * channel_count);
            int i = x + y * width * channel_count;
            glColor3f(image[i] / 255.0f, image[i + 1] / 255.0f, image[i + 2] / 255.0f);
            // glVertex2f(0, 0);
            glVertex2f(2 / 3.0 * (float)x / width - 1, -(2 * (float)y / height - 1));
        }
    }
    glEnd();
}

void gl_draw_fixed(GLFWwindow **window, unsigned char *image, int width, int height, int channel_count)
{
    gl_draw(window, image, width, height, channel_count);
    glfwSwapBuffers(*window);
    gl_draw(window, image, width, height, channel_count);
}

void gl_init_window(GLFWwindow **window, char *title, int width, int height)
{
    if (!glfwInit())
    {
        gl_handle_err(1);
    }

    *window = glfwCreateWindow(width, height, title, NULL, NULL);
    if (!*window)
    {
        glfwTerminate();
        gl_handle_err(2);
    }

    glfwMakeContextCurrent(*window);
    gl_handle_err(3);

    glfwSetWindowAspectRatio(*window, width, height);
    gl_handle_err(4);
    glfwSwapInterval(1);
}

void gl_loop(GLFWwindow **window, void (*render_callback)(GLFWwindow **window), void (*window_size_callback)(GLFWwindow **window, int w, int h))
{

    // glClearColor(0.0f, 0.0f, 0.0f, 1.0f);

    // glClear(GL_COLOR_BUFFER_BIT);

    int prev_fbsw = 0, prev_fbsh = 0;
    while (!glfwWindowShouldClose(*window))
    {
        int fbsw, fbsh;
        glfwGetFramebufferSize(*window, &fbsw, &fbsh);
        if (fbsw != prev_fbsw || fbsh != prev_fbsh)
        {
            window_size_callback(window, fbsw, fbsh);
            prev_fbsw = fbsw, prev_fbsh = fbsh;
        }

        render_callback(window);

        // Enforces vsync set by glfwSwapInterval
        glfwSwapBuffers(*window);

        /* Poll for and process events */
        glfwPollEvents();

        glFinish();
    }

    glfwTerminate();
}
