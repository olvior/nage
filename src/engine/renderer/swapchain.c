#include "swapchain.h"
#include "../utils.h"

void swapchain_initialise(Renderer* renderer, GLFWwindow* window)
{
    create_swap_chain(renderer, window);
    create_image_views(renderer);
}

void swapchain_cleanup(Renderer* renderer)
{
    Swapchain* swapchain = &renderer->swapchain;

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
        .imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,

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

    CHECK_VK_FATAL(
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

        CHECK_VK_FATAL(
                vkCreateImageView(
                    renderer->device,
                    &image_view_create_info,
                    NULL,
                    &swapchain->image_views[i]
                )
        );
    }
}
