#include "image.h"
#include "renderer.h"
#include "buffers.h"
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

Image image_create(VmaAllocator allocator, VkDevice device, VkExtent3D size, VkFormat format,
        VkImageUsageFlags usage, bool mipmapped)
{
    Image image;
    image.format = format;
    image.extent = size;

    VkImageCreateInfo image_info = get_image_create_info(format, usage, size);

    if (mipmapped) {
		image_info.mipLevels = floorf(log2f(max(size.width, size.height))) + 1;
	}

    VmaAllocationCreateInfo alloc_info = {
        .usage = VMA_MEMORY_USAGE_GPU_ONLY,
        .requiredFlags = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
    };

    VK_CHECK(vmaCreateImage(allocator, &image_info, &alloc_info, &image.image, &image.allocation,
                NULL));

    VkImageAspectFlags aspect_flag = VK_IMAGE_ASPECT_COLOR_BIT;
	if (format == VK_FORMAT_D32_SFLOAT) {
		aspect_flag = VK_IMAGE_ASPECT_DEPTH_BIT;
	}

    VkImageViewCreateInfo view_info = get_image_view_create_info(format, image.image, aspect_flag);
    view_info.subresourceRange.levelCount = image_info.mipLevels;

    VK_CHECK(vkCreateImageView(device, &view_info, NULL, &image.view));

    return image;
}

Image image_create_textured(Renderer* renderer, void* data, VkExtent3D size,
        VkFormat format, VkImageUsageFlags usage, bool mipmapped)
{
    size_t data_size = size.depth * size.width * size.height * 4;

    Buffer upload_buffer = buffer_create(renderer->allocator, data_size,
            VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VMA_MEMORY_USAGE_CPU_TO_GPU);

    // copy it in
    void* buf_data;
    vmaMapMemory(renderer->allocator, upload_buffer.allocation, &buf_data);
    memcpy(buf_data, data, data_size);
    vmaUnmapMemory(renderer->allocator, upload_buffer.allocation);

    Image image = image_create(renderer->allocator, renderer->device, size, format, usage
            | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT, mipmapped);

    VkBufferImageCopy copy_region = {
        .bufferOffset = 0,
        .bufferRowLength = 0,
        .bufferImageHeight = 0,

        .imageExtent = size,
        .imageSubresource = {
            .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
            .mipLevel = 0,
            .baseArrayLayer = 0,
            .layerCount = 1,
        },
    };

    {
        immediate_begin(renderer);

        transition_image(renderer->imm_cmd_buf, renderer->device, image.image,
                VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);

        vkCmdCopyBufferToImage(renderer->imm_cmd_buf, upload_buffer.buffer, image.image,
                VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &copy_region);

        transition_image(renderer->imm_cmd_buf, renderer->device, image.image,
                VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
        immediate_end(renderer);
    }

    buffer_destroy(&upload_buffer, renderer->allocator);

    return image;
}

void image_destroy(VkDevice device, VmaAllocator allocator, Image image)
{
    vkDestroyImageView(device, image.view, NULL);
    vmaDestroyImage(allocator, image.image, image.allocation);
}
