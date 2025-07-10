#include "engine.h"

#include <GLFW/glfw3.h>

void engine_initialise(Engine* engine)
{
    window_initialise(&engine->window);
}

void framebuffer_resize_callback()
{
    printf("We got resized!");
}

void window_initialise(Window* window)
{
    window->width = 800;
    window->height = 800;
    window->title = "Engineing";

    glfwInit();

    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);

    window->window = glfwCreateWindow(
            window->width,
            window->height,
            window->title,
            NULL,
            NULL
    );

    glfwSetFramebufferSizeCallback(window->window, framebuffer_resize_callback);
}

void engine_run(Engine* engine)
{
    while(!glfwWindowShouldClose(engine->window.window))
    {
        glfwPollEvents();
    }
}
