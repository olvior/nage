#pragma once

#include <vulkan/vulkan.h>
#include "renderer.h"

void transition_image(VkCommandBuffer cmd_buf, VkDevice device, VkImage image,
        VkImageLayout current_layout, VkImageLayout new_layout);

VkImageCreateInfo get_image_create_info(VkFormat format, VkImageUsageFlags usage_flags,
        VkExtent3D extent);
VkImageViewCreateInfo get_image_view_create_info(VkFormat format, VkImage image,
        VkImageAspectFlags aspectFlags);
void copy_image(VkCommandBuffer cmd_buf, VkImage src, VkImage dst,
        VkExtent2D src_size, VkExtent2D dst_size);
void image_destroy(VkDevice device, VmaAllocator allocator, Image image);
Image image_create(VmaAllocator allocator, VkDevice device, VkExtent3D size, VkFormat format,
        VkImageUsageFlags usage, bool mipmapped);
Image image_create_textured(Renderer* renderer, void* data, VkExtent3D size,
        VkFormat format, VkImageUsageFlags usage, bool mipmapped);

