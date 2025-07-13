#include "image.h"
#include "../utils.h"

void transition_image(VkCommandBuffer cmd_buf, VkDevice device, VkImage image,
        VkImageLayout current_layout, VkImageLayout new_layout)
{
    VkImageAspectFlags aspect_mask = (new_layout == VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL)
        ? VK_IMAGE_ASPECT_DEPTH_BIT : VK_IMAGE_ASPECT_COLOR_BIT;

    VkImageSubresourceRange sub_image = {
        .aspectMask = aspect_mask,
        .levelCount = VK_REMAINING_MIP_LEVELS,
        .layerCount = VK_REMAINING_ARRAY_LAYERS,
    };

    VkImageMemoryBarrier barrier = {
        .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
        .pNext = NULL,

        .srcAccessMask = VK_ACCESS_2_MEMORY_WRITE_BIT,
        .dstAccessMask = VK_ACCESS_2_MEMORY_WRITE_BIT | VK_ACCESS_2_MEMORY_READ_BIT,

        .oldLayout = current_layout,
        .newLayout = new_layout,

        .subresourceRange = sub_image,
        .image = image,
    };


    vkCmdPipelineBarrier(cmd_buf, VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT, VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT,
            0, 0, NULL, 0, NULL, 1, &barrier);
}

VkImageCreateInfo get_image_create_info(
        VkFormat format,
        VkImageUsageFlags usage_flags,
        VkExtent3D extent
)
{
    VkImageCreateInfo image_create_info = {
        .sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
        .pNext = NULL,

        .imageType = VK_IMAGE_TYPE_2D,

        .format = format,
        .extent = extent,

        .mipLevels = 1,
        .arrayLayers = 1,

        .samples = VK_SAMPLE_COUNT_1_BIT,

        .tiling = VK_IMAGE_TILING_OPTIMAL,
        .usage = usage_flags,
    };

    return image_create_info;
}

VkImageViewCreateInfo get_image_view_create_info(VkFormat format, VkImage image,
        VkImageAspectFlags aspectFlags)
{
    VkImageViewCreateInfo info = {
        .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,

        .viewType = VK_IMAGE_VIEW_TYPE_2D,
        .image = image,
        .format = format,
        .subresourceRange = {
            .baseMipLevel = 0,
            .levelCount = 1,
            .baseArrayLayer = 0,
            .layerCount = 1,
            .aspectMask = aspectFlags,
        }
    };

    return info;
}

void copy_image(VkCommandBuffer cmd_buf, VkImage src, VkImage dst,
        VkExtent2D src_size, VkExtent2D dst_size)
{
    VkImageBlit blit_region = {
        .srcOffsets = {
            { 0, 0, 0},
            { src_size.width, src_size.height, 1 },
        },

        .dstOffsets = {
            { 0, 0, 0},
            { dst_size.width, dst_size.height, 1 },
        },

        .srcSubresource = {
            .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
            .baseArrayLayer = 0,
            .layerCount = 1,
            .mipLevel = 0,
        },

        .dstSubresource = {
            .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
            .baseArrayLayer = 0,
            .layerCount = 1,
            .mipLevel = 0,
        },
    };

    vkCmdBlitImage(
            cmd_buf,
            src,
            VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
            dst,
            VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
            1,
            &blit_region,
            VK_FILTER_LINEAR
    );
}
