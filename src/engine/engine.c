#include "engine.h"
#include "utils.h"

#include <GLFW/glfw3.h>

void engine_initialise(Engine* engine)
{
    window_initialise(&engine->window);
    renderer_initialise(&engine->renderer, engine->window.window);
}

void engine_run(Engine* engine)
{
    while(!glfwWindowShouldClose(engine->window.window))
    {
        glfwPollEvents();
    }
}

void engine_cleanup(Engine* engine)
{
    window_cleanup(&engine->window);
    renderer_cleanup(&engine->renderer);
}

