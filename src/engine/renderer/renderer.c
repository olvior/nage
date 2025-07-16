#define CGLM_FORCE_DEPTH_ZERO_TO_ONE
#define CGLM_FORCE_LEFT_HANDED

#include "renderer.h"
#include "debug.h"
#include "device.h"
#include "swapchain.h"
#include "image.h"
#include "pipeline.h"
#include "buffers.h"
#include "../dearimgui.h"
#include "../utils.h"
#include "../scene/loader.h"

// #include <math.h>

const int VALIDATION_LAYERS_COUNT = 1;
const char* VALIDATION_LAYERS[VALIDATION_LAYERS_COUNT] = {
    "VK_LAYER_KHRONOS_validation",
};


int validation_layers_count()
{
    return VALIDATION_LAYERS_COUNT;
}

const char** validation_layers()
{
    return VALIDATION_LAYERS;
}

// Main code
void renderer_initialise(Renderer* renderer, GLFWwindow* window)
{
    renderer_create_instance(renderer);
    #ifdef VALIDATION_LAYERS_ENABLED
    create_debug_messenger(&renderer->instance, &renderer->debug_messenger);
    #endif
    renderer_create_surface(renderer, window);
    device_initialise(renderer);

    vma_allocator_initialise(renderer);

    swapchain_initialise(renderer, window);
    create_command_buffers(renderer);
    sync_initialise(renderer);

    pipeline_initialise(renderer);

    initialise_data(renderer);

    renderer->frame_in_flight = 0;
    renderer->frame = 0;
}

void renderer_cleanup(Renderer* renderer)
{
    buffer_destroy(&renderer->mesh.mesh_buffers.index_buffer, renderer->allocator);
    buffer_destroy(&renderer->mesh.mesh_buffers.vertex_buffer, renderer->allocator);

    pipeline_cleanup(renderer);

    sync_cleanup(renderer);
    cleanup_command_buffers(renderer);
    swapchain_cleanup(renderer);
    vmaDestroyAllocator(renderer->allocator);
    vkDestroyDevice(renderer->device, NULL);
    vkDestroySurfaceKHR(renderer->instance, renderer->surface, NULL);
    destroy_debug_messenger(&renderer->instance, &renderer->debug_messenger);
    vkDestroyInstance(renderer->instance, NULL);
}

void renderer_draw(Renderer* renderer)
{
    int frame = renderer->frame_in_flight;

    // first we wait
    VK_CHECK(vkWaitForFences(renderer->device, 1, &renderer->fences[frame], true, ONE_SEC));

    VK_CHECK(vkResetFences(renderer->device, 1, &renderer->fences[frame]));

    uint32_t image_index;
    VkResult e = vkAcquireNextImageKHR(renderer->device, renderer->swapchain.swapchain, ONE_SEC,
            renderer->semaphores_swapchain[frame], NULL, &image_index);

   if (e == VK_ERROR_OUT_OF_DATE_KHR) {
        renderer->resize_requested = true;
        return;
    }

    // init the command buffer
    VkCommandBuffer cmd_buf = renderer->command_buffers[frame];
    VK_CHECK(vkResetCommandBuffer(cmd_buf, 0));

    VkCommandBufferBeginInfo begin_info = {
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
        .flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT
    };
    VK_CHECK(vkBeginCommandBuffer(cmd_buf, &begin_info));

    transition_image(cmd_buf, renderer->device, renderer->draw_image.image,
            VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);
    transition_image(cmd_buf, renderer->device, renderer->depth_image.image,
            VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL);

    draw_geometry(renderer, cmd_buf);

    // vkCmdBindPipeline(cmd_buf, VK_PIPELINE_BIND_POINT_COMPUTE, renderer->pipeline);
    //
    // vkCmdBindDescriptorSets(cmd_buf, VK_PIPELINE_BIND_POINT_COMPUTE, renderer->pipeline_layout,
    //         0, 1, &renderer->draw_image_desc_set, 0, NULL);
    //
    // vkCmdPushConstants(cmd_buf, renderer->pipeline_layout, VK_SHADER_STAGE_COMPUTE_BIT, 0,
    //         sizeof(PushConstants), &renderer->push_constants);
    //
    // vkCmdDispatch(cmd_buf, ceilf(renderer->swapchain.extent.width / 16.0),
    //         ceilf(renderer->swapchain.extent.height / 16.0), 1);


    transition_image(cmd_buf, renderer->device, renderer->draw_image.image,
            VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL);
    transition_image(cmd_buf, renderer->device, renderer->swapchain.images[image_index],
            VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
    // transition_image(cmd_buf, renderer->device, renderer->imgui_image.image,
    //         VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);
    // imgui_draw(renderer, cmd_buf, renderer->imgui_image.view);

    // transition_image(cmd_buf, renderer->device, renderer->imgui_image.image,
    //         VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL);

    VkExtent2D draw_extent = {renderer->draw_image.extent.width, renderer->draw_image.extent.height};
    copy_image(cmd_buf, renderer->draw_image.image, renderer->swapchain.images[image_index],
            draw_extent, renderer->swapchain.extent);
    // copy_image(cmd_buf, renderer->imgui_image.image, renderer->swapchain.images[image_index],
    //         draw_extent, renderer->swapchain.extent);
    transition_image(cmd_buf, renderer->device, renderer->swapchain.images[image_index],
            VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);

    imgui_draw(renderer, cmd_buf, renderer->swapchain.image_views[image_index]);


    transition_image(cmd_buf, renderer->device, renderer->swapchain.images[image_index],
            VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, VK_IMAGE_LAYOUT_PRESENT_SRC_KHR);
    // transition_image(cmd_buf, renderer->device, renderer->swapchain.images[image_index],
    //         VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_PRESENT_SRC_KHR);

    VK_CHECK(vkEndCommandBuffer(cmd_buf));

    VkSubmitInfo submit_info = get_submit_info(renderer);

    VK_CHECK(vkQueueSubmit(renderer->graphics_queue, 1, &submit_info, renderer->fences[frame]));

    // now we need to present

    VkPresentInfoKHR present_info = {
        .sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR,

        .pSwapchains = &renderer->swapchain.swapchain,
        .swapchainCount = 1,

        .pWaitSemaphores = &renderer->semaphores_render[frame],
        .waitSemaphoreCount = 1,

        .pImageIndices = &image_index,
    };

    e = vkQueuePresentKHR(renderer->graphics_queue, &present_info);
    if (e == VK_ERROR_OUT_OF_DATE_KHR)
        renderer->resize_requested = true;

    renderer_inc_frame(renderer);
}

void draw_geometry(Renderer* renderer, VkCommandBuffer cmd_buf)
{
    VkClearValue clear_value = {
        .color = { {0.1f, 0.2f, 0.3f, 1.0f} }
    };
    VkRenderingAttachmentInfoKHR colour_attachment = {
        .sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO,
        .pNext = NULL,
        .imageView = renderer->draw_image.view,
        .imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
        .loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
        .storeOp = VK_ATTACHMENT_STORE_OP_STORE,
        .clearValue = clear_value,
    };

    VkRenderingAttachmentInfo depth_attachment = {
        .sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO,
        .pNext = NULL,
        .imageView = renderer->depth_image.view,
        .imageLayout = VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL,
        .loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
        .storeOp = VK_ATTACHMENT_STORE_OP_STORE,
        .clearValue.depthStencil.depth = 0,
    };

    VkRenderingInfoKHR render_info = {
        .sType = VK_STRUCTURE_TYPE_RENDERING_INFO,
        .renderArea = {
            .offset = {0, 0},
            .extent = { renderer->draw_image.extent.width, renderer->draw_image.extent.height },
        },

        .layerCount = 1,
        .viewMask = 0,
        .colorAttachmentCount = 1,
        .pColorAttachments = &colour_attachment,
        .pDepthAttachment = &depth_attachment,
    };


    void* func = get_device_proc_adr(renderer->device, "vkCmdBeginRenderingKHR");
    ((PFN_vkCmdBeginRenderingKHR)(func))(cmd_buf, &render_info);

    vkCmdBindPipeline(cmd_buf, VK_PIPELINE_BIND_POINT_GRAPHICS, renderer->pipeline);

    // dynamic viewport and scissor
    VkViewport viewport = {
        .x = 0,
        .y = 0,
        .width = (float)renderer->draw_image.extent.width,
        .height = (float)renderer->draw_image.extent.height,

        .minDepth = 0.f,
        .maxDepth = 1.f,
    };

    vkCmdSetViewport(cmd_buf, 0, 1, &viewport);

    VkRect2D scissor = {
        .offset = { 0, 0 },
        .extent = {renderer->draw_image.extent.width, renderer->draw_image.extent.height},
    };

    vkCmdSetScissor(cmd_buf, 0, 1, &scissor);


    VkExtent3D draw_extent = renderer->draw_image.extent;
    vec3 translation = {renderer->translation[0], renderer->translation[1], renderer->translation[2]};

    vec3 camera_pos = { 0, -1, -6};
    vec3 forward_dir = { 0, 0, 1 };
    vec3 up_dir = { 0, -1, 0 };
    mat4 view = GLM_MAT4_IDENTITY_INIT;

    vec3 upped;
    glm_vec3_add(camera_pos, forward_dir, upped);
    glm_lookat(camera_pos, upped, up_dir, view);

    mat4 proj = GLM_MAT4_IDENTITY_INIT;
    float aspect = 1.0;

    glm_perspective(glm_rad(renderer->fov), aspect, 0.1, 1000, proj);

    mat4 model = GLM_MAT4_IDENTITY_INIT;
    glm_translate(model, translation);

    mat4 mvp = GLM_MAT4_IDENTITY_INIT;
    glm_mat4_mulN((mat4* [3]){&proj, &view, &model}, 3, mvp);


    PushConstants push_constants = {
        .world_matrix = MAT4_UNPACK(mvp),
        .vertex_buffer = renderer->mesh.mesh_buffers.vertex_buffer_address,
    };

    vkCmdPushConstants(cmd_buf, renderer->pipeline_layout, VK_SHADER_STAGE_VERTEX_BIT, 0,
            sizeof(PushConstants), &push_constants);
    vkCmdBindIndexBuffer(cmd_buf, renderer->mesh.mesh_buffers.index_buffer.buffer, 0, VK_INDEX_TYPE_UINT32);
    vkCmdDrawIndexed(cmd_buf, renderer->mesh.surfaces[0].count, 1, renderer->mesh.surfaces[0].start_index, 0, 0);
    // vkCmdDraw(cmd_buf, 102, 1, 0, 0);

    func = get_device_proc_adr(renderer->device, "vkCmdEndRenderingKHR");
    ((PFN_vkCmdEndRenderingKHR)(func))(cmd_buf);
}

void renderer_create_instance(Renderer* renderer)
{
    #ifdef VALIDATION_LAYERS_ENABLED
    if (!validation_layers_supported(VALIDATION_LAYERS, VALIDATION_LAYERS_COUNT))
        FATAL("Validation layers not found\n");
    #endif

    VkApplicationInfo app_info = {
        .sType = VK_STRUCTURE_TYPE_APPLICATION_INFO,
        .pApplicationName = "Test app",
        .pEngineName = "Engine name here",
        .engineVersion = VK_MAKE_VERSION(0, 1, 0),
        .apiVersion = VK_API_VERSION_1_2,
    };

    uint32_t extension_count;
    get_required_extensions(&extension_count, NULL);

    const char* extensions[extension_count];
    get_required_extensions(&extension_count, extensions);

    print_string_list(extensions, extension_count);

    #ifdef VALIDATION_LAYERS_ENABLED
        VkDebugUtilsMessengerCreateInfoEXT debug_create_info = {0};
        populate_debug_messenger_info(&debug_create_info);
    #endif

    VkInstanceCreateInfo instance_create_info = {
        .sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
        .pApplicationInfo = &app_info,
        #ifdef __APPLE__
        .flags = VK_INSTANCE_CREATE_ENUMERATE_PORTABILITY_BIT_KHR,
        #endif

        .enabledExtensionCount = extension_count,
        .ppEnabledExtensionNames = extensions,

        #ifdef VALIDATION_LAYERS_ENABLED
        .enabledLayerCount = VALIDATION_LAYERS_COUNT,
        .ppEnabledLayerNames = VALIDATION_LAYERS,
        .pNext = &debug_create_info,
        #endif
    };

    VkResult result = vkCreateInstance(&instance_create_info, NULL, &renderer->instance);
    VK_CHECK(result);

    LOG_W("oh no\n");
}

void vma_allocator_initialise(Renderer* renderer)
{
    VmaAllocatorCreateInfo allocator_info = {
        .physicalDevice = renderer->gpu,
        .device = renderer->device,
        .instance = renderer->instance,
        .flags = VMA_ALLOCATOR_CREATE_BUFFER_DEVICE_ADDRESS_BIT,
    };

    vmaCreateAllocator(&allocator_info, &renderer->allocator);
}

void renderer_create_surface(Renderer* renderer, GLFWwindow* window)
{
    VK_CHECK(glfwCreateWindowSurface(renderer->instance, window, NULL, &renderer->surface));
}

void create_command_buffers(Renderer* renderer)
{
    QueueFamilyIndices indices = find_queue_families(&renderer->gpu, &renderer->surface);

    VkCommandPoolCreateInfo command_pool_info = {
        .sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
        .flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT,
        .queueFamilyIndex = indices.graphics_family
    };

    for (int i = 0; i < FRAMES_IN_FLIGHT; ++i)
    {
        VK_CHECK(
                vkCreateCommandPool(
                    renderer->device,
                    &command_pool_info,
                    NULL,
                    &renderer->command_pools[i]
                )
        );

        VkCommandBufferAllocateInfo cmd_alloc_info = {
            .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
            .commandPool = renderer->command_pools[i],
            .commandBufferCount = 1,
            .level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
        };

        VK_CHECK(
                vkAllocateCommandBuffers(
                    renderer->device,
                    &cmd_alloc_info,
                    &renderer->command_buffers[i]
                )
        );
    }

    VK_CHECK(vkCreateCommandPool(renderer->device, &command_pool_info, NULL, &renderer->imm_cmd_pool));

    VkCommandBufferAllocateInfo cmd_alloc_info = {
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
        .commandPool = renderer->imm_cmd_pool,
        .commandBufferCount = 1,
        .level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
    };

    VK_CHECK(vkAllocateCommandBuffers(renderer->device, &cmd_alloc_info, &renderer->imm_cmd_buf));
}

void cleanup_command_buffers(Renderer* renderer)
{
    for (int i = 0; i < FRAMES_IN_FLIGHT; ++i)
    {
        vkDestroyCommandPool(renderer->device, renderer->command_pools[i], NULL);
    }

    vkDestroyCommandPool(renderer->device, renderer->imm_cmd_pool, NULL);
}

void sync_initialise(Renderer* renderer)
{
    VkFenceCreateInfo fence_info = {
        .sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO,
        .flags = VK_FENCE_CREATE_SIGNALED_BIT
    };

    VkSemaphoreCreateInfo semaphore_info = {
        .sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO
    };

    for (int i = 0; i < FRAMES_IN_FLIGHT; i++) {
        VK_CHECK(vkCreateFence(renderer->device, &fence_info, NULL, &renderer->fences[i]));

        VK_CHECK(
            vkCreateSemaphore(
                renderer->device,
                &semaphore_info,
                NULL,
                &renderer->semaphores_render[i]
            )
        );

        VK_CHECK(
            vkCreateSemaphore(
                renderer->device,
                &semaphore_info,
                NULL,
                &renderer->semaphores_swapchain[i]
            )
        );
    }


    // immediate mode
    VK_CHECK(vkCreateFence(renderer->device, &fence_info, NULL, &renderer->imm_fence));
}

void sync_cleanup(Renderer* renderer)
{
    for (int i = 0; i < FRAMES_IN_FLIGHT; ++i)
    {
        vkDestroySemaphore(renderer->device, renderer->semaphores_render[i], NULL);
        vkDestroySemaphore(renderer->device, renderer->semaphores_swapchain[i], NULL);
        vkDestroyFence(renderer->device, renderer->fences[i], NULL);
    }

    vkDestroyFence(renderer->device, renderer->imm_fence, NULL);
}

void initialise_data(Renderer* renderer)
{
    // renderer->mesh = upload_mesh(renderer, indices, n_indices, vertices, n_vertices);
    Mesh* meshes = load_glft_meshes(renderer, "basicmesh.glb");
    renderer->mesh = meshes[2];

    renderer->translation[0] = 0;
    renderer->translation[1] = 0;
    renderer->translation[1] = 0;
    renderer->fov = 90;
}

VkSubmitInfo get_submit_info(Renderer* renderer)
{
    VkSemaphore* wait_semaphore = &renderer->semaphores_swapchain[renderer->frame_in_flight];
    VkSemaphore* signal_semaphore = &renderer->semaphores_render[renderer->frame_in_flight];
    VkPipelineStageFlags wait_stages[] = {VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};


    VkSubmitInfo submit_info = {
        .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,

        .pWaitDstStageMask = wait_stages,

        .waitSemaphoreCount = wait_semaphore == NULL ? 0 : 1,
        .pWaitSemaphores = wait_semaphore,

        .signalSemaphoreCount = signal_semaphore == NULL ? 0 : 1,
        .pSignalSemaphores = signal_semaphore,

        .commandBufferCount = 1,
        .pCommandBuffers = &renderer->command_buffers[renderer->frame_in_flight],
    };


    return submit_info;
}

void immediate_begin(Renderer* renderer)
{
    VK_CHECK(vkResetFences(renderer->device, 1, &renderer->imm_fence));
    VK_CHECK(vkResetCommandBuffer(renderer->imm_cmd_buf, 0));

    VkCommandBufferBeginInfo cmd_begin_info = {
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
        .flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT
    };

    VK_CHECK(vkBeginCommandBuffer(renderer->imm_cmd_buf, &cmd_begin_info));
}

void immediate_end(Renderer* renderer)
{
    VK_CHECK(vkEndCommandBuffer(renderer->imm_cmd_buf));

    VkPipelineStageFlags waits[] = {VK_PIPELINE_STAGE_ALL_GRAPHICS_BIT};

    VkSubmitInfo submit_info = {
        .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
        .pWaitDstStageMask = waits,
        .commandBufferCount = 1,
        .pCommandBuffers = &renderer->imm_cmd_buf,
    };

    VK_CHECK(vkQueueSubmit(renderer->graphics_queue, 1, &submit_info, renderer->imm_fence));
    VK_CHECK(vkWaitForFences(renderer->device, 1, &renderer->imm_fence, true, ONE_SEC));
}

void renderer_inc_frame(Renderer* renderer)
{
    renderer->frame_in_flight += 1;
    if (renderer->frame_in_flight >= FRAMES_IN_FLIGHT)
        renderer->frame_in_flight = 0;

    renderer->frame += 1;
}

