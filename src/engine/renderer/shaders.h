#pragma once
#include "../utils.h"

VkShaderModule create_shader_module(VkDevice device, File* f);
VkShaderModule create_shader(VkDevice device, char file_path[]);
VkPipelineShaderStageCreateInfo make_shader_info(VkDevice device, char file_path[],
        VkShaderStageFlagBits stage);

