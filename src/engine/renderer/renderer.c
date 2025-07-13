#include "renderer.h"
#include "debug.h"
#include "device.h"
#include "swapchain.h"
#include "image.h"
#include "../utils.h"

#include <math.h>
#include <vk_mem_alloc.h>

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

    renderer->frame_in_flight = 0;
    renderer->frame = 0;
}

void renderer_cleanup(Renderer* renderer)
{
    vkDeviceWaitIdle(renderer->device);

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
    VK_CHECK(
            vkAcquireNextImageKHR(renderer->device, renderer->swapchain.swapchain, ONE_SEC,
                renderer->semaphores_swapchain[frame], NULL, &image_index)
    );

    // init the command buffer
    VkCommandBuffer cmd_buf = renderer->command_buffers[frame];
    VK_CHECK(vkResetCommandBuffer(cmd_buf, 0));

    VkCommandBufferBeginInfo begin_info = {
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
        .flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT
    };
    VK_CHECK(vkBeginCommandBuffer(cmd_buf, &begin_info));

    // printf("%d, a\n", image_index);
    transition_image(cmd_buf, renderer->device, renderer->draw_image.image,
            VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_GENERAL);


    float flash = fabsf(sinf(renderer->frame / 20.0f));
    VkClearColorValue clear_value = { {0.0f, 0.0f, flash, 1.0f} };

    VkImageSubresourceRange clear_range = {
        .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
        .levelCount = VK_REMAINING_MIP_LEVELS,
        .layerCount = VK_REMAINING_ARRAY_LAYERS,
    };

    // clear image
    vkCmdClearColorImage(cmd_buf, renderer->draw_image.image,
            VK_IMAGE_LAYOUT_GENERAL, &clear_value, 1, &clear_range);

    transition_image(cmd_buf, renderer->device, renderer->draw_image.image,
            VK_IMAGE_LAYOUT_GENERAL, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL);
    transition_image(cmd_buf, renderer->device, renderer->swapchain.images[image_index],
            VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);

    VkExtent2D draw_extent = {renderer->draw_image.extent.width, renderer->draw_image.extent.height};
    copy_image(cmd_buf, renderer->draw_image.image, renderer->swapchain.images[image_index],
            draw_extent, renderer->swapchain.extent);

    transition_image(cmd_buf, renderer->device, renderer->swapchain.images[image_index],
            VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_PRESENT_SRC_KHR);


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

    VK_CHECK(vkQueuePresentKHR(renderer->graphics_queue, &present_info));

    renderer_inc_frame(renderer);
}

void draw_background(Renderer* renderer)
{
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
        // .flags = VMA_ALLOCATOR_CREATE_BUFFER_DEVICE_ADDRESS_BIT,
        // tutorials wants this but probably not needed
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
}

void cleanup_command_buffers(Renderer* renderer)
{
    for (int i = 0; i < FRAMES_IN_FLIGHT; ++i)
    {
        vkDestroyCommandPool(renderer->device, renderer->command_pools[i], NULL);
    }
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
}

void sync_cleanup(Renderer* renderer)
{
    for (int i = 0; i < FRAMES_IN_FLIGHT; ++i)
    {
        vkDestroySemaphore(renderer->device, renderer->semaphores_render[i], NULL);
        vkDestroySemaphore(renderer->device, renderer->semaphores_swapchain[i], NULL);
        vkDestroyFence(renderer->device, renderer->fences[i], NULL);
    }
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

void renderer_inc_frame(Renderer* renderer)
{
    renderer->frame_in_flight += 1;
    if (renderer->frame_in_flight >= FRAMES_IN_FLIGHT)
        renderer->frame_in_flight = 0;

    renderer->frame += 1;
}

