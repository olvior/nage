#include "renderer.h"
#include "debug.h"
#include "device.h"
#include "swapchain.h"
#include "sync.h"
#include "../utils.h"

#include <math.h>

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
    swapchain_initialise(renderer, window);
    create_command_buffers(renderer);
    sync_initialise(renderer);

    renderer->current_frame = 0;
    renderer->total_frames = 0;
}

void renderer_cleanup(Renderer* renderer)
{
    vkDeviceWaitIdle(renderer->device);

    sync_cleanup(renderer);
    cleanup_command_buffers(renderer);
    swapchain_cleanup(renderer);
    vkDestroyDevice(renderer->device, NULL);
    vkDestroySurfaceKHR(renderer->instance, renderer->surface, NULL);
    destroy_debug_messenger(&renderer->instance, &renderer->debug_messenger);
    vkDestroyInstance(renderer->instance, NULL);
}

void renderer_draw(Renderer* renderer)
{
    int frame = renderer->current_frame;

    // first we wait
    CHECK_VK_FATAL(
            vkWaitForFences(renderer->device, 1, &renderer->fences[frame], true, ONE_SEC)
    );

    CHECK_VK_FATAL(
            vkResetFences(renderer->device, 1, &renderer->fences[frame])
    );

    uint32_t image_index;
    CHECK_VK_FATAL(
            vkAcquireNextImageKHR(renderer->device, renderer->swapchain.swapchain, ONE_SEC,
                renderer->semaphores_swapchain[frame], NULL, &image_index)
    );

    // init the command buffer
    VkCommandBuffer cmd_buf = renderer->command_buffers[frame];
    CHECK_VK_FATAL(vkResetCommandBuffer(cmd_buf, 0));

    VkCommandBufferBeginInfo begin_info = {
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
        .flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT
    };
    CHECK_VK_FATAL(vkBeginCommandBuffer(cmd_buf, &begin_info));

    // printf("%d, a\n", image_index);
    transition_image(cmd_buf, renderer->device, renderer->swapchain.images[image_index],
            VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_GENERAL);


    float flash = fabsf(sinf(renderer->total_frames / 120.0f));
    VkClearColorValue clear_value = { {0.0f, 0.0f, flash, 1.0f} };

    VkImageSubresourceRange clear_range = {
        .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
        .levelCount = VK_REMAINING_MIP_LEVELS,
        .layerCount = VK_REMAINING_ARRAY_LAYERS,
    };

    // clear image
    vkCmdClearColorImage(cmd_buf, renderer->swapchain.images[image_index],
            VK_IMAGE_LAYOUT_GENERAL, &clear_value, 1, &clear_range);

    // make it presentable
    transition_image(cmd_buf, renderer->device, renderer->swapchain.images[image_index],
            VK_IMAGE_LAYOUT_GENERAL, VK_IMAGE_LAYOUT_PRESENT_SRC_KHR);

    CHECK_VK_FATAL(vkEndCommandBuffer(cmd_buf));

    VkSubmitInfo submit_info = get_submit_info(renderer);

    CHECK_VK_FATAL(
        vkQueueSubmit(renderer->graphics_queue, 1, &submit_info, renderer->fences[frame])
    );

    // now we need to present

    VkPresentInfoKHR present_info = {
        .sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR,

        .pSwapchains = &renderer->swapchain.swapchain,
        .swapchainCount = 1,

        .pWaitSemaphores = &renderer->semaphores_render[frame],
        .waitSemaphoreCount = 1,

        .pImageIndices = &image_index,
    };

    CHECK_VK_FATAL(vkQueuePresentKHR(renderer->graphics_queue, &present_info));

    renderer_inc_frame(renderer);
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
        .apiVersion = VK_API_VERSION_1_3,
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
    CHECK_VK_FATAL(result);

    LOG_W("oh no\n");
}

void renderer_create_surface(Renderer* renderer, GLFWwindow* window)
{
    CHECK_VK_FATAL(
            glfwCreateWindowSurface(renderer->instance, window, NULL, &renderer->surface)
    );
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
        CHECK_VK_FATAL(
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

        CHECK_VK_FATAL(
                vkAllocateCommandBuffers(
                    renderer->device,
                    &cmd_alloc_info,
                    &renderer->command_buffers[i]
                )
        );
    }
}

void cleanup_command_buffers(Renderer* renderer)
{
    for (int i = 0; i < FRAMES_IN_FLIGHT; ++i)
    {
        vkDestroyCommandPool(renderer->device, renderer->command_pools[i], NULL);
    }
}

VkSubmitInfo get_submit_info(Renderer* renderer)
{
    VkSemaphore* wait_semaphore = &renderer->semaphores_swapchain[renderer->current_frame];
    VkSemaphore* signal_semaphore = &renderer->semaphores_render[renderer->current_frame];
    VkPipelineStageFlags wait_stages[] = {VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};


    VkSubmitInfo submit_info = {
        .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,

        .pWaitDstStageMask = wait_stages,

        .waitSemaphoreCount = wait_semaphore == NULL ? 0 : 1,
        .pWaitSemaphores = wait_semaphore,

        .signalSemaphoreCount = signal_semaphore == NULL ? 0 : 1,
        .pSignalSemaphores = signal_semaphore,

        .commandBufferCount = 1,
        .pCommandBuffers = &renderer->command_buffers[renderer->current_frame],
    };


    return submit_info;
}

void renderer_inc_frame(Renderer* renderer)
{
    renderer->current_frame += 1;
    if (renderer->current_frame >= FRAMES_IN_FLIGHT)
        renderer->current_frame = 0;

    renderer->total_frames += 1;
}
