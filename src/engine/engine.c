#include "engine.h"
#include <stdio.h>

#include <vulkan/vulkan.h>
#include <GLFW/glfw3.h>

void engine_initialise(Engine* engine)
{
    window_initialise(&engine->window);
    renderer_initialise(&engine->renderer, engine->window.window);
}

void engine_run(Engine* engine)
{
    double old_time = glfwGetTime();
    int i = 0;
    while(!glfwWindowShouldClose(engine->window.window))
    {
        i++;

        if (i >= 100) {
            double current_time = glfwGetTime();
            double delta = current_time - old_time;
            printf("100 frames took %lf ", delta);
            printf("(%.1lf fps)\n", 100 / delta);
            old_time = glfwGetTime();
            i = 0;
        }

        glfwPollEvents();
        renderer_draw(&engine->renderer);
    }
}

void engine_cleanup(Engine* engine)
{
    window_cleanup(&engine->window);
    renderer_cleanup(&engine->renderer);
}

