#include "swapchain.h"
#include "image.h"
#include "pipeline.h"
#include <vk_mem_alloc.h>
#include "../utils.h"

void swapchain_initialise(Renderer* renderer, GLFWwindow* window)
{
    create_swap_chain(renderer, window);
    create_image_views(renderer);
    create_drawing_image(renderer);
    create_depth_image(renderer);
}

void swapchain_cleanup(Renderer* renderer)
{
    Swapchain* swapchain = &renderer->swapchain;

    vkDestroyImageView(renderer->device, renderer->draw_image.view, NULL);
    vmaDestroyImage(renderer->allocator, renderer->draw_image.image, renderer->draw_image.allocation);
    vkDestroyImageView(renderer->device, renderer->depth_image.view, NULL);
    vmaDestroyImage(renderer->allocator, renderer->depth_image.image, renderer->depth_image.allocation);


    for (int i = 0; i < swapchain->image_count; ++i)
        vkDestroyImageView(renderer->device, swapchain->image_views[i], NULL);

    vkDestroySwapchainKHR(renderer->device, swapchain->swapchain, NULL);
    free(swapchain->image_views);
    free(swapchain->images);
}

void create_swap_chain(Renderer* renderer, GLFWwindow* window)
{
    Swapchain* swapchain = &renderer->swapchain;

    SwapChainSupportDetails details = {0};

    query_swap_chain_support(renderer, &details);
    VkSurfaceFormatKHR formats_buf[details.format_count];
    VkPresentModeKHR present_buf[details.present_mode_count];
    details.formats = formats_buf;
    details.present_modes = present_buf;

    query_swap_chain_support(renderer, &details);


    VkSurfaceFormatKHR surface_format = choose_swap_surface_format(&details);
    VkPresentModeKHR present_mode = choose_swap_present_mode(&details);
    VkExtent2D extent = choose_swap_extent(window, &details.capabilities);
    uint32_t image_count = details.capabilities.minImageCount + 1;
    uint32_t max_images = details.capabilities.maxImageCount;

    if (max_images > 0 && image_count > max_images)
        image_count = max_images;

    VkSwapchainCreateInfoKHR swapchain_create_info = {
        .sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR,
        .surface = renderer->surface,

        .minImageCount = image_count,
        .imageFormat = surface_format.format,
        .imageColorSpace = surface_format.colorSpace,
        .imageExtent = extent,
        .imageArrayLayers = 1,
        .imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT,

        .preTransform = details.capabilities.currentTransform,
        .compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR,
        .presentMode = present_mode,
        .clipped = VK_TRUE,
        .oldSwapchain = VK_NULL_HANDLE,
    };



    QueueFamilyIndices indices = find_queue_families(&renderer->gpu, &renderer->surface);
    uint32_t indices_array[2] = {indices.graphics_family, indices.present_family};
    if (indices.graphics_family != indices.present_family)
    {
        swapchain_create_info.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
        swapchain_create_info.queueFamilyIndexCount = 2;
        swapchain_create_info.pQueueFamilyIndices = indices_array;
    }
    else
    {
        swapchain_create_info.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
        swapchain_create_info.queueFamilyIndexCount = 0;
        swapchain_create_info.pQueueFamilyIndices = NULL;
    }

    VK_CHECK(
            vkCreateSwapchainKHR(
                renderer->device,
                &swapchain_create_info,
                NULL,
                &swapchain->swapchain
            )
    );

    vkGetSwapchainImagesKHR(
            renderer->device,
            swapchain->swapchain,
            &swapchain->image_count,
            NULL
    );
    swapchain->images = malloc(sizeof(VkImage) * swapchain->image_count);
    vkGetSwapchainImagesKHR(
            renderer->device,
            swapchain->swapchain,
            &swapchain->image_count,
            swapchain->images
    );

    swapchain->image_format = surface_format.format;
    swapchain->extent = extent;
}

void swapchain_resize(Renderer* renderer, Window* window)
{
    vkDeviceWaitIdle(renderer->device);
    swapchain_cleanup(renderer);

    int w, h;
    glfwGetFramebufferSize(window->window, &w, &h);

    window->width = w;
    window->height = h;

    create_swap_chain(renderer, window->window);

    renderer->resize_requested = false;
}

void query_swap_chain_support(Renderer* renderer, SwapChainSupportDetails* details)
{
    vkGetPhysicalDeviceSurfaceCapabilitiesKHR(
            renderer->gpu,
            renderer->surface,
            &details->capabilities
    );

    vkGetPhysicalDeviceSurfaceFormatsKHR(
            renderer->gpu,
            renderer->surface,
            &details->format_count,
            NULL
    );

    if (details->formats != NULL)
    {
        vkGetPhysicalDeviceSurfaceFormatsKHR(
                renderer->gpu,
                renderer->surface,
                &details->format_count,
                details->formats
        );
    }

    vkGetPhysicalDeviceSurfacePresentModesKHR(
            renderer->gpu,
            renderer->surface,
            &details->present_mode_count,
            NULL
    );
    if (details->present_modes != NULL)
    {
        vkGetPhysicalDeviceSurfacePresentModesKHR(
                renderer->gpu,
                renderer->surface,
                &details->present_mode_count,
                details->present_modes
        );
    }
}

VkSurfaceFormatKHR choose_swap_surface_format(SwapChainSupportDetails* details)
{
    for (int i = 0; i < details->format_count; ++i)
    {
        VkSurfaceFormatKHR f = details->formats[i];
        if (f.format == VK_FORMAT_B8G8R8A8_SRGB &&
            f.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR)
            return f;
    }

    return details->formats[0];
}

VkPresentModeKHR choose_swap_present_mode(SwapChainSupportDetails* details)
{
    for (int i = 0; i < details->present_mode_count; ++i)
    {
        VkPresentModeKHR m = details->present_modes[i];
        if (m == VK_PRESENT_MODE_MAILBOX_KHR)
        // if (m == VK_PRESENT_MODE_IMMEDIATE_KHR)
            return m;
    }

    return details->present_modes[0];
}


VkExtent2D choose_swap_extent(GLFWwindow* window, VkSurfaceCapabilitiesKHR* capabilities)
{
    if (capabilities->currentExtent.width != UINT32_MAX)
    {
        return capabilities->currentExtent;
    }
    else
    {
        int width, height;
        glfwGetFramebufferSize(window, &width, &height);

        VkExtent2D actual_extent = { (uint32_t) width, (uint32_t) height };
        actual_extent.width = clamp(width,
                capabilities->minImageExtent.width, capabilities->maxImageExtent.width);
        actual_extent.height = clamp(height,
                capabilities->minImageExtent.height, capabilities->maxImageExtent.height);

        return actual_extent;
    }
}

void create_image_views(Renderer* renderer)
{
    Swapchain* swapchain = &renderer->swapchain;
    swapchain->image_views = malloc(sizeof(VkImageView) * swapchain->image_count);

    for (int i = 0; i < swapchain->image_count; ++i)
    {
        VkImageViewCreateInfo image_view_create_info = {
            .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
            .image = swapchain->images[i],
            .viewType = VK_IMAGE_VIEW_TYPE_2D,
            .format = swapchain->image_format,
            .components = {VK_COMPONENT_SWIZZLE_IDENTITY, VK_COMPONENT_SWIZZLE_IDENTITY,
                VK_COMPONENT_SWIZZLE_IDENTITY, VK_COMPONENT_SWIZZLE_IDENTITY},
            .subresourceRange = {
                .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
                .baseMipLevel = 0,
                .levelCount = 1,
                .baseArrayLayer = 0,
                .layerCount = 1
            }
        };

        VK_CHECK(
                vkCreateImageView(
                    renderer->device,
                    &image_view_create_info,
                    NULL,
                    &swapchain->image_views[i]
                )
        );
    }
}

void create_drawing_image(Renderer* renderer)
{
    Swapchain* sc = &renderer->swapchain;
    Image* draw_image = &renderer->draw_image;

    VkExtent3D extent = {
        .width = sc->extent.width,
        .height = sc->extent.height,
        .depth = 1,
    };

    VkFormat format = VK_FORMAT_R16G16B16A16_SFLOAT;

    draw_image->format = format;
    draw_image->extent = extent;

    VkImageUsageFlags usage = VK_IMAGE_USAGE_TRANSFER_SRC_BIT |
        VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_STORAGE_BIT |
        VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

    VkImageCreateInfo image_create_info = get_image_create_info(format, usage, extent);

    VmaAllocationCreateInfo image_alloc_info = {
        .usage = VMA_MEMORY_USAGE_GPU_ONLY,
        .requiredFlags = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
    };

    vmaCreateImage(renderer->allocator, &image_create_info, &image_alloc_info,
            &draw_image->image, &draw_image->allocation, NULL);

    VkImageViewCreateInfo image_view_info = get_image_view_create_info(format,
            draw_image->image, VK_IMAGE_ASPECT_COLOR_BIT);

    VK_CHECK(vkCreateImageView(renderer->device, &image_view_info, NULL, &draw_image->view));

}

void create_depth_image(Renderer* renderer)
{
    renderer->depth_image.format = VK_FORMAT_D32_SFLOAT;
    renderer->depth_image.extent = renderer->draw_image.extent;

    VkImageUsageFlags usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;

    VkImageCreateInfo depth_img_info = get_image_create_info(renderer->depth_image.format, usage,
            renderer->depth_image.extent);

    VmaAllocationCreateInfo image_alloc_info = {
        .usage = VMA_MEMORY_USAGE_GPU_ONLY,
        .requiredFlags = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
    };

    vmaCreateImage(renderer->allocator, &depth_img_info, &image_alloc_info, &renderer->depth_image.image,
            &renderer->depth_image.allocation, NULL);

    VkImageViewCreateInfo image_view_info = get_image_view_create_info(renderer->depth_image.format,
            renderer->depth_image.image, VK_IMAGE_ASPECT_DEPTH_BIT);

    VK_CHECK(vkCreateImageView(renderer->device, &image_view_info, NULL, &renderer->depth_image.view));
}

