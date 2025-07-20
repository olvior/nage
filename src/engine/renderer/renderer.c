#define CGLM_FORCE_DEPTH_ZERO_TO_ONE
#define CGLM_FORCE_LEFT_HANDED

#include "renderer.h"
#include "debug.h"
#include "device.h"
#include "swapchain.h"
#include "image.h"
#include "pipeline.h"
#include "buffers.h"
#include "materials.h"
#include "../dearimgui.h"
#include "../utils.h"
#include "../scene/loader.h"

// #include <math.h>

#define M_I 0

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

    renderer->scene_data_buffer = buffer_create(renderer->allocator, sizeof(GPUSceneData),
            VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VMA_MEMORY_USAGE_CPU_TO_GPU);

    renderer->frame_in_flight = 0;
    renderer->frame = 0;
}

void renderer_cleanup(Renderer* renderer)
{
    material_metallic_cleanup(&renderer->metalic_material, renderer);
    buffer_destroy(&renderer->scene_data_buffer, renderer->allocator);

    for (int i = 0; i < renderer->n_buf_destroy; ++i)
    {
        Buffer* buf = &renderer->buf_destroy[i];
        vmaUnmapMemory(renderer->allocator, buf->allocation);
        buffer_destroy(&renderer->buf_destroy[i], renderer->allocator);
    }
    free(renderer->buf_destroy);

    vkDestroySampler(renderer->device, renderer->sampler_nearest, NULL);
    vkDestroySampler(renderer->device, renderer->sampler_linear, NULL);
    image_destroy(renderer->device, renderer->allocator, renderer->error_image);

    meshes_destroy(renderer->meshes, renderer->n_meshes, renderer->allocator);

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

void renderer_draw(Renderer* renderer, DrawContext* context)
{
    int frame = renderer->frame_in_flight;

    // first we wait
    VK_CHECK(vkWaitForFences(renderer->device, 1, &renderer->fences[frame], true, ONE_SEC));

    descriptor_allocator_growable_clear_pools(&renderer->frame_descriptors[frame], renderer->device);


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

    draw_geometry(renderer, cmd_buf, context);

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

void draw_geometry(Renderer* renderer, VkCommandBuffer cmd_buf, DrawContext* context)
{
    VkClearValue clear_value = { .color = { {0.1f, 0.2f, 0.3f, 1.0f} } };
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

    Buffer scene_data_buffer = renderer->scene_data_buffer;

    // copy data to gpu
    void* data;
    vmaMapMemory(renderer->allocator, scene_data_buffer.allocation, &data);
    memcpy(data, &renderer->scene_data, sizeof(GPUSceneData));
    vmaUnmapMemory(renderer->allocator, scene_data_buffer.allocation);

    // uniform buffer
    VkDescriptorSet global_descriptor = descriptor_allocator_growable_allocate(
            &renderer->frame_descriptors[renderer->frame_in_flight], renderer->device,
            renderer->scene_data_desc_set_layout, NULL);

    VkDescriptorBufferInfo buffer_info = descriptor_writer_get_buffer_info(scene_data_buffer.buffer,
            sizeof(GPUSceneData), 0);
    VkWriteDescriptorSet write_info = descriptor_writer_get_write(0,
            VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, &buffer_info, NULL);
    descriptor_writer_update_set(renderer->device, global_descriptor, &write_info, 1);

    // image descriptor
    VkDescriptorSet image_set = descriptor_allocator_growable_allocate(
            &renderer->frame_descriptors[renderer->frame_in_flight], renderer->device,
            renderer->single_image_desc_layout, NULL);

    VkDescriptorImageInfo image_info = descriptor_writer_get_image_info(renderer->error_image.view,
            renderer->sampler_nearest, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
    write_info = descriptor_writer_get_write(0, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
            NULL, &image_info);
    descriptor_writer_update_set(renderer->device, image_set, &write_info, 1);

    // begin rendering
    void* func = get_device_proc_adr(renderer->device, "vkCmdBeginRenderingKHR");
    ((PFN_vkCmdBeginRenderingKHR)(func))(cmd_buf, &render_info);

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


    for (int i = 0; i < context->n; ++i)
    {
        RenderObject* render_object = &context->opaque_surfaces[i];
        MaterialInstance* mat = render_object->material;
        vkCmdBindPipeline(cmd_buf, VK_PIPELINE_BIND_POINT_GRAPHICS, mat->pipeline->pipeline);
        vkCmdBindDescriptorSets(cmd_buf, VK_PIPELINE_BIND_POINT_GRAPHICS, mat->pipeline->layout, 0, 1,
                &global_descriptor, 0, NULL);
        vkCmdBindDescriptorSets(cmd_buf, VK_PIPELINE_BIND_POINT_GRAPHICS, mat->pipeline->layout, 1, 1,
                &mat->material_set, 0, NULL);

        vkCmdBindIndexBuffer(cmd_buf, render_object->index_buffer, 0, VK_INDEX_TYPE_UINT32);


        mat4 mvp = GLM_MAT4_IDENTITY_INIT;
        mat4* model = &render_object->transform;
        glm_mat4_mulN((mat4* [3]){&proj, &view, model}, 3, mvp);

        PushConstants push_constants = {
            .world_matrix = MAT4_UNPACK(mvp),
            .vertex_buffer = render_object->vertex_buffer_address,
        };

        vkCmdPushConstants(cmd_buf, mat->pipeline->layout, VK_SHADER_STAGE_VERTEX_BIT, 0,
                sizeof(PushConstants), &push_constants);

        vkCmdDrawIndexed(cmd_buf, render_object->index_count, 1, render_object->first_index, 0, 0);
    }

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
    renderer->translation[0] = 0;
    renderer->translation[1] = 0;
    renderer->translation[1] = 0;
    renderer->fov = 90;

    uint32_t checkerboard[16*16];
    for (int x = 0; x < 16; ++x)
    {
        for (int y = 0; y < 16; ++y)
        {
            checkerboard[16 * y + x] = ((x % 2) ^ (y % 2)) ? 0xff00ff : 0x00000000;
        }
    }
    renderer->error_image = image_create_textured(renderer, &checkerboard,
            (VkExtent3D) { 16, 16, 1 }, VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_USAGE_SAMPLED_BIT, false);

    VkSamplerCreateInfo sampler_info = {
        .sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO,
        .magFilter = VK_FILTER_NEAREST,
        .minFilter = VK_FILTER_NEAREST
    };
    vkCreateSampler(renderer->device, &sampler_info, NULL, &renderer->sampler_nearest);

    VkSamplerCreateInfo sampler2_info = {
        .sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO,
        .magFilter = VK_FILTER_LINEAR,
        .minFilter = VK_FILTER_LINEAR
    };
    vkCreateSampler(renderer->device, &sampler2_info, NULL, &renderer->sampler_linear);

    MaterialMetallicResources mat_resources = {
        .colour_image = renderer->error_image,
        .colour_sampler = renderer->sampler_linear,
        .metal_rough_image = renderer->error_image,
        .metal_rough_sampler = renderer->sampler_linear,
    };

    Buffer mat_constants = buffer_create(renderer->allocator, sizeof(MaterialMetallicConstants),
            VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VMA_MEMORY_USAGE_CPU_TO_GPU);


    renderer->buf_destroy = malloc(sizeof(Buffer) * 10);
    renderer->buf_destroy[renderer->n_buf_destroy] = mat_constants;
    renderer->n_buf_destroy = 1;

    MaterialMetallicConstants* data;
    vmaMapMemory(renderer->allocator, mat_constants.allocation, (void**) &data);

    // set all colour factors to 1
    for (int i = 0; i < 4; ++i)
        data->colour_factors[i] = 1;

    data->metal_rough_factors[0] = 1;
    data->metal_rough_factors[1] = 0.5;
    data->metal_rough_factors[2] = 0;
    data->metal_rough_factors[3] = 0;

    mat_resources.data_buffer = mat_constants.buffer;
    mat_resources.data_buffer_offset = 0;


    renderer->default_material_instance = material_metallic_write_material(&renderer->metalic_material,
            renderer->device, MAT_PASS_MAIN_COLOUR, &mat_resources,
            &renderer->global_descriptor_allocator);

    vec4 ambient_colour = {0.1, 0.1, 0.1, 1};
    memcpy(renderer->scene_data.ambient_colour, ambient_colour, sizeof(vec4));
    vec4 sunlight_dir = {2, -5, -2, 0.2};
    memcpy(renderer->scene_data.sunlight_direction, sunlight_dir, sizeof(vec4));
    vec4 sunlight_colour = {0.1, 0.1, 1, 1};
    memcpy(renderer->scene_data.sunlight_colour, sunlight_colour, sizeof(vec4));


    printf("%f %f %f\n", ambient_colour[0], ambient_colour[1], ambient_colour[2]);
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

