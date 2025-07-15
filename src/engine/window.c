#include "window.h"
#include "utils.h"

#include "GLFW/glfw3.h"

void framebuffer_resize_callback(GLFWwindow* window, int width, int height);

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

void window_cleanup(Window* window)
{
    glfwDestroyWindow(window->window);
    glfwTerminate();
}

void framebuffer_resize_callback(GLFWwindow* window, int width, int height)
{
    LOG("We got resized! ");
}

