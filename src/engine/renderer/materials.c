#include "materials.h"
#include "pipeline.h"
#include "shaders.h"
#include "../utils.h"

void material_metallic_initialise_pipelines(MaterialMetallic* mat, Renderer* renderer)
{
    material_metallic_build_descriptors(mat, renderer);
    material_metallic_build_layouts(mat, renderer);
    material_metallic_build_pipelines(mat, renderer);
}

void material_metallic_cleanup(MaterialMetallic* mat, Renderer* renderer)
{
    vkDestroyPipeline(renderer->device, mat->pipeline_opaque.pipeline, NULL);
    vkDestroyPipeline(renderer->device, mat->pipeline_transparent.pipeline, NULL);
    vkDestroyPipelineLayout(renderer->device, mat->pipeline_opaque.layout, NULL);
    vkDestroyDescriptorSetLayout(renderer->device, mat->material_layout, NULL);
}

void material_metallic_build_descriptors(MaterialMetallic* mat, Renderer* renderer)
{
    const int n = 3;
    VkDescriptorSetLayoutBinding bindings[n] = {
        {
            .binding = 0,
            .descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
            .descriptorCount = 1,
        },
        {
            .binding = 1,
            .descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
            .descriptorCount = 1,
        },
        {
            .binding = 2,
            .descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
            .descriptorCount = 1,
        }
    };
    mat->material_layout = create_descriptor_set_layout(bindings, n, renderer->device,
            VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 0);
}

void material_metallic_build_layouts(MaterialMetallic* mat, Renderer* renderer)
{
    VkPushConstantRange push_constant = {
        .offset = 0,
        .size = sizeof(PushConstants),
        .stageFlags = VK_SHADER_STAGE_VERTEX_BIT,
    };

    VkDescriptorSetLayout layouts[] = {renderer->scene_data_desc_set_layout, mat->material_layout};

    VkPipelineLayoutCreateInfo pipeline_layout = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
        .pNext = NULL,
        .pSetLayouts = layouts,
        .setLayoutCount = 2,

        .pushConstantRangeCount = 1,
        .pPushConstantRanges = &push_constant,
    };

    VkPipelineLayout new_layout;
    VK_CHECK(vkCreatePipelineLayout(renderer->device, &pipeline_layout, NULL, &new_layout));

    // give it to them
    mat->pipeline_opaque.layout = new_layout;
    mat->pipeline_transparent.layout = new_layout;
}

void material_metallic_build_pipelines(MaterialMetallic* mat, Renderer* renderer)
{
    VkPipelineShaderStageCreateInfo frag_shader = make_shader_info(renderer->device, "mesh.frag.spv",
            VK_SHADER_STAGE_FRAGMENT_BIT);
    VkPipelineShaderStageCreateInfo vert_shader = make_shader_info(renderer->device, "mesh.vert.spv",
            VK_SHADER_STAGE_VERTEX_BIT);

    PipelineBuilder pb = {0};
    pb.layout = mat->pipeline_opaque.layout;
    pb.shader_stages[0] = vert_shader;
    pb.shader_stages[1] = frag_shader;
    pipeline_builder_set_input_topology(&pb, VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST);
    pipeline_builder_set_polygon_mode(&pb, VK_POLYGON_MODE_FILL);
    pipeline_builder_set_cull_mode(&pb, VK_CULL_MODE_BACK_BIT, VK_FRONT_FACE_COUNTER_CLOCKWISE);
    pipeline_builder_set_multisampling_none(&pb);
    pipeline_builder_disable_blending(&pb);
    // pipeline_builder_enable_blending(&pb, false);
    pipeline_builder_set_depthtest(&pb, true, VK_COMPARE_OP_GREATER_OR_EQUAL);

    pipeline_builder_set_color_attachment_format(&pb, renderer->draw_image.format);
    pipeline_builder_set_depth_format(&pb, renderer->depth_image.format);

    mat->pipeline_opaque.pipeline = pipeline_builder_build(&pb, renderer->device);

    pipeline_builder_enable_blending(&pb, true);
    pipeline_builder_set_depthtest(&pb, false, VK_COMPARE_OP_GREATER_OR_EQUAL);

    mat->pipeline_transparent.pipeline = pipeline_builder_build(&pb, renderer->device);

    vkDestroyShaderModule(renderer->device, frag_shader.module, NULL);
    vkDestroyShaderModule(renderer->device, vert_shader.module, NULL);
}

MaterialInstance material_metallic_write_material(MaterialMetallic* mat, VkDevice device,
        enum MaterialPass pass_type, const MaterialMetallicResources* resources,
        DescriptorAllocatorGrowable* dag)
{
    MaterialInstance instance = {
        .pass_type = pass_type,
    };

    if (pass_type == MAT_PASS_TRANSPARENT)
        instance.pipeline = &mat->pipeline_transparent;
    else
        instance.pipeline = &mat->pipeline_opaque;

    instance.material_set = descriptor_allocator_growable_allocate(dag, device, mat->material_layout,
            NULL);

    VkDescriptorBufferInfo info_buf = descriptor_writer_get_buffer_info(resources->data_buffer,
            sizeof(MaterialMetallicConstants), resources->data_buffer_offset);
    VkDescriptorImageInfo info_image1 = descriptor_writer_get_image_info(resources->colour_image.view,
            resources->colour_sampler, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
    VkDescriptorImageInfo info_image2 = descriptor_writer_get_image_info(
            resources->metal_rough_image.view, resources->metal_rough_sampler,
            VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

    VkWriteDescriptorSet write_infos[] = {
        descriptor_writer_get_write(0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, &info_buf, NULL),
        descriptor_writer_get_write(1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, NULL, &info_image1),
        descriptor_writer_get_write(2, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, NULL, &info_image2),
    };

    descriptor_writer_update_set(device, instance.material_set, write_infos, 3);

    return instance;
}
