#include "engine.h"
#include "utils.h"
#include "renderer/swapchain.h"
#include "scene/loader.h"

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

    ecs_intitialise(&engine->ecs);
    // renderer->mesh = upload_mesh(renderer, indices, n_indices, vertices, n_vertices);
    uint8_t n_meshes;
    engine->renderer.meshes = load_glft_meshes(&engine->renderer, "basicmesh.glb",
            &engine->renderer.n_meshes);
    // Mesh* meshes = load_glft_meshes(&engine->renderer, "basicmesh.glb", &n_meshes);
    Mesh* meshes = engine->renderer.meshes;
    mat4 transform = GLM_MAT4_IDENTITY_INIT;
    ecs_add_renderable(&engine->ecs, &meshes[0], transform);
    glm_translate(transform, (vec4){ 4, 0, 0, 0 });
    ecs_add_renderable(&engine->ecs, &meshes[1], transform);
    glm_translate(transform, (vec4){ -8, 0, 2, 0 });
    ecs_add_renderable(&engine->ecs, &meshes[2], transform);
    // renderer->mesh = meshes[0].mesh_buffers;
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

        if (engine->renderer.resize_requested) {
            swapchain_resize(&engine->renderer, &engine->window);
        }

        glfwPollEvents();

        process_inputs(engine);

        imgui_frame(engine->io, &engine->renderer);

        RenderObject opaque_surfaces[10] = {0};
        DrawContext context = {opaque_surfaces, 0};
        ecs_renderable_collect(&engine->ecs, &engine->renderer, &context);
        renderer_draw(&engine->renderer, &context);
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
