#include "shaders.h"
#include "../utils.h"

VkShaderModule create_shader_module(VkDevice device, File* f)
{
    VkShaderModuleCreateInfo info = {0};

    info.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    info.codeSize = f->size;
    info.pCode = (uint32_t*) f->buf;

    VkShaderModule shader_module;
    if (vkCreateShaderModule(device, &info, NULL, &shader_module) != VK_SUCCESS)
    {
        FATAL("Could not create shader module!");
    }

    return shader_module;
}

VkShaderModule create_shader(VkDevice device, char file_path[])
{
    File f = read_file(file_path);
    VkShaderModule shader = create_shader_module(device, &f);
    free(f.buf);

    return shader;
}

VkPipelineShaderStageCreateInfo make_shader_info(VkDevice device, char file_path[],
        VkShaderStageFlagBits stage)
{
    VkShaderModule shader = create_shader(device, file_path);

    VkPipelineShaderStageCreateInfo stage_info = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
        .pNext = NULL,

        .stage = stage,
        .module = shader,
        .pName = "main",
    };

    return stage_info;
}

