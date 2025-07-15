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
void pipeline_builder_disable_depthtest(PipelineBuilder* pb);

