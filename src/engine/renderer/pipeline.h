#pragma once

#include "renderer.h"

void pipeline_initialise(Renderer* renderer);
void pipeline_cleanup(Renderer* renderer);

// descriptor allocator growable
void descriptor_allocator_growable_alloc_lists(DescriptorAllocatorGrowable* dag);
void descriptor_allocator_growable_free_lists(DescriptorAllocatorGrowable* dag);
VkDescriptorPool descriptor_allocator_growable_create_pool(DescriptorAllocatorGrowable* dag,
        VkDevice device);
VkDescriptorPool descriptor_allocator_growable_get_pool(DescriptorAllocatorGrowable* dag,
        VkDevice device);
void descriptor_allocator_growable_init(DescriptorAllocatorGrowable* dag, VkDevice device);
void descriptor_allocator_growable_clear_pools(DescriptorAllocatorGrowable* dag, VkDevice device);
void descriptor_allocator_growable_destroy_pools(DescriptorAllocatorGrowable* dag, VkDevice device);
VkDescriptorSet descriptor_allocator_growable_allocate(DescriptorAllocatorGrowable* dag,
        VkDevice device, VkDescriptorSetLayout layout, void* pNext);

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
void create_pipeline_layout(Renderer* renderer);
void create_pipeline(Renderer* renderer);

typedef struct PipelineBuilder PipelineBuilder;

void pipeline_builder_clear(PipelineBuilder* pb);
VkPipeline pipeline_builder_build(PipelineBuilder* pb, VkDevice device);
void pipeline_builder_set_input_topology(PipelineBuilder* pb, VkPrimitiveTopology topology);
void pipeline_builder_set_polygon_mode(PipelineBuilder* pb, VkPolygonMode mode);
void pipeline_builder_set_cull_mode(PipelineBuilder* pb, VkCullModeFlags cull_mode,
        VkFrontFace front_face);
void pipeline_builder_set_multisampling_none(PipelineBuilder* pb);
void pipeline_builder_disable_blending(PipelineBuilder* pb);
void pipeline_builder_set_color_attachment_format(PipelineBuilder* pb, VkFormat format);
void pipeline_builder_set_depth_format(PipelineBuilder* pb, VkFormat format);
void pipeline_builder_set_depthtest(PipelineBuilder* pb, bool enable_depth_write, VkCompareOp op);
void pipeline_builder_enable_blending(PipelineBuilder* pb, bool additive);
VkDescriptorBufferInfo descriptor_writer_get_buffer_info(VkBuffer buffer, size_t size, size_t offset);
VkDescriptorImageInfo descriptor_writer_get_image_info(VkImageView image_view, VkSampler sampler,
        VkImageLayout layout);
VkWriteDescriptorSet descriptor_writer_get_write(int binding, VkDescriptorType type,
        VkDescriptorBufferInfo* buffer_info, VkDescriptorImageInfo* image_info);
void descriptor_writer_update_set(VkDevice device, VkDescriptorSet set,
        VkWriteDescriptorSet* write_sets, int n_write_sets);

