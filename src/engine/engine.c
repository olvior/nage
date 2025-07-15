#include "engine.h"
#include "utils.h"

#include <vulkan/vulkan.h>
#include <GLFW/glfw3.h>

#include <imgui/dcimgui_impl_vulkan.h>
#include <imgui/dcimgui_impl_glfw.h>

#include "dearimgui.h"

void engine_initialise(Engine* engine)
{
    window_initialise(&engine->window);
    renderer_initialise(&engine->renderer, engine->window.window);
    imgui_initialise(&engine->renderer, engine->window.window, &engine->io);
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
            LOG_V("100 frames took %lf ", delta);
            LOG_B("(%.1lf fps)\n", 100 / delta);
            old_time = glfwGetTime();
            i = 0;
        }

        glfwPollEvents();

        process_inputs(engine);

        imgui_frame(engine->io, &engine->renderer);

        renderer_draw(&engine->renderer);
    }
}

void engine_cleanup(Engine* engine)
{
    window_cleanup(&engine->window);

    vkDeviceWaitIdle(engine->renderer.device);
    imgui_cleanup(&engine->renderer);
    renderer_cleanup(&engine->renderer);
}

void process_inputs(Engine* engine)
{
    // PushConstants* pc = &engine->renderer.push_constants;
    // float zoom = powf(2, pc->data2[0]);
    // if (glfwGetKey(engine->window.window, GLFW_KEY_LEFT) == GLFW_PRESS) {
    //     pc->data2[1] -= 0.1 / zoom;
    // }
    // if (glfwGetKey(engine->window.window, GLFW_KEY_RIGHT) == GLFW_PRESS) {
    //     pc->data2[1] += 0.1 / zoom;
    // }
    // if (glfwGetKey(engine->window.window, GLFW_KEY_UP) == GLFW_PRESS) {
    //     pc->data2[2] -= 0.1 / zoom;
    // }
    // if (glfwGetKey(engine->window.window, GLFW_KEY_DOWN) == GLFW_PRESS) {
    //     pc->data2[2] += 0.1 / zoom;
    // }
    // if (glfwGetKey(engine->window.window, GLFW_KEY_PERIOD) == GLFW_PRESS) {
    //     pc->data2[0] += 0.1;
    // }
    // if (glfwGetKey(engine->window.window, GLFW_KEY_COMMA) == GLFW_PRESS) {
    //     pc->data2[0] -= 0.1;
    // }
}
