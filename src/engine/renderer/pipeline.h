#pragma once

#include "renderer.h"


void pipeline_initialise(Renderer* renderer);
void pipeline_cleanup(Renderer* renderer);

// internal
VkDescriptorSetLayout create_descriptor_set_layout(VkDescriptorSetLayoutBinding* bindings,
        int n, VkDevice device, VkShaderStageFlags shader_stages,
        VkDescriptorSetLayoutCreateFlags flags);

VkDescriptorPool create_descriptor_pool(VkDevice device, uint32_t max_sets,
        VkDescriptorType* types, uint32_t* ratios, int n);

VkDescriptorSet allocate_descriptor_set(VkDescriptorPool pool, VkDevice device,
        VkDescriptorSetLayout layout);

void clear_descriptor_pool(VkDescriptorPool pool, VkDevice device);
void destroy_pool(VkDescriptorPool pool, VkDevice device);
void descriptors_initialise(Renderer* renderer);

