#include "pipeline.h"
#include "shaders.h"
#include "../utils.h"

#include <vulkan/vulkan.h>


void pipeline_initialise(Renderer* renderer)
{
    descriptors_initialise(renderer);
    create_pipeline_layout(renderer);
    create_pipeline(renderer);
}

void pipeline_cleanup(Renderer* renderer)
{
    vkDestroyPipeline(renderer->device, renderer->pipeline, NULL);
    vkDestroyPipelineLayout(renderer->device, renderer->pipeline_layout, NULL);

    destroy_pool(renderer->descriptor_pool, renderer->device);
    vkDestroyDescriptorSetLayout(renderer->device, renderer->draw_image_desc_layout, NULL);
}

void descriptors_initialise(Renderer* renderer)
{
    int n = 1;
    VkDescriptorType types[] = {VK_DESCRIPTOR_TYPE_STORAGE_IMAGE};
    uint32_t ratios[] = {1};
    renderer->descriptor_pool = create_descriptor_pool(renderer->device, 10, types, ratios, n);

    VkDescriptorSetLayoutBinding bindings[] = {
        {
            .binding = 0,
            .descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,
            .descriptorCount = 1,
        }
    };

    renderer->draw_image_desc_layout = create_descriptor_set_layout(bindings, 1,
            renderer->device, VK_SHADER_STAGE_COMPUTE_BIT, 0);

    // allocate descriptor

    renderer->draw_image_desc_set = allocate_descriptor_set(renderer->descriptor_pool,
            renderer->device, renderer->draw_image_desc_layout);

    VkDescriptorImageInfo image_info = {
        .imageLayout = VK_IMAGE_LAYOUT_GENERAL,
        .imageView = renderer->draw_image.view,
    };

    VkWriteDescriptorSet draw_image_write = {
        .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
        .pNext = NULL,

        .dstBinding = 0,
        .dstSet = renderer->draw_image_desc_set,
        .descriptorCount = 1,
        .descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,
        .pImageInfo = &image_info,
    };

    vkUpdateDescriptorSets(renderer->device, 1, &draw_image_write, 0, NULL);
}

VkDescriptorSetLayout create_descriptor_set_layout(VkDescriptorSetLayoutBinding* bindings,
        int n, VkDevice device, VkShaderStageFlags shader_stages,
        VkDescriptorSetLayoutCreateFlags flags)
{
    for (int i = 0; i < n; ++i)
    {
        bindings[i].stageFlags |= shader_stages;
    }

    VkDescriptorSetLayoutCreateInfo info = {
        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
        .pBindings = bindings,
        .bindingCount = n,
        .flags = flags
    };

    VkDescriptorSetLayout set_layout;

    VK_CHECK(vkCreateDescriptorSetLayout(device, &info, NULL, &set_layout));

    return set_layout;
}

VkDescriptorPool create_descriptor_pool(VkDevice device, uint32_t max_sets,
        VkDescriptorType* types, uint32_t* ratios, int n)
{
    VkDescriptorPoolSize pool_sizes[n];

    for (int i = 0; i < n; ++i)
    {
        pool_sizes[i].type = types[i];
        pool_sizes[i].descriptorCount = (uint32_t) (ratios[i] * max_sets);
    }

    VkDescriptorPoolCreateInfo pool_create_info = {
        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
        .flags = 0,
        .maxSets = max_sets,
        .poolSizeCount = (uint32_t) n,
        .pPoolSizes = pool_sizes,
    };

    VkDescriptorPool pool;
    vkCreateDescriptorPool(device, &pool_create_info, NULL, &pool);

    return pool;
}

void clear_descriptor_pool(VkDescriptorPool pool, VkDevice device)
{
    vkResetDescriptorPool(device, pool, 0);
}

void destroy_pool(VkDescriptorPool pool, VkDevice device)
{
    vkDestroyDescriptorPool(device, pool, NULL);
}

VkDescriptorSet allocate_descriptor_set(VkDescriptorPool pool, VkDevice device,
        VkDescriptorSetLayout layout)
{
    VkDescriptorSetAllocateInfo alloc_info = {
        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
        .pNext = 0,
        .descriptorSetCount = 1,
        .descriptorPool = pool,
        .pSetLayouts = &layout,
    };

    VkDescriptorSet descriptor_set;
    VK_CHECK(vkAllocateDescriptorSets(device, &alloc_info, &descriptor_set));

    return descriptor_set;
}

void create_pipeline_layout(Renderer* renderer)
{
    VkPipelineLayoutCreateInfo pipeline_layout = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
        .pNext = NULL,
        .pSetLayouts = &renderer->draw_image_desc_layout,
        .setLayoutCount = 1,
    };

    VK_CHECK(vkCreatePipelineLayout(renderer->device, &pipeline_layout, NULL,
                &renderer->pipeline_layout));
}

void create_pipeline(Renderer* renderer)
{
    VkShaderModule compute_shader = create_shader(renderer->device, "comp.spv");

    VkPipelineShaderStageCreateInfo stage_info = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
        .pNext = NULL,

        .stage = VK_SHADER_STAGE_COMPUTE_BIT,
        .module = compute_shader,
        .pName = "main",
    };

    VkComputePipelineCreateInfo pipeline_create_info = {
        .sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO,
        .pNext = NULL,

        .layout = renderer->pipeline_layout,
        .stage = stage_info,
    };

    VK_CHECK(vkCreateComputePipelines(renderer->device, VK_NULL_HANDLE, 1, &pipeline_create_info,
                NULL, &renderer->pipeline));

    vkDestroyShaderModule(renderer->device, compute_shader, NULL);
}

