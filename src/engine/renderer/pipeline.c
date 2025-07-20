#include "pipeline.h"
#include "shaders.h"
#include "materials.h"
#include "../utils.h"

#include <vulkan/vulkan.h>


void pipeline_initialise(Renderer* renderer)
{
    descriptors_initialise(renderer);
    // create_pipeline_layout(renderer);
    // create_pipeline(renderer);

    material_metallic_initialise_pipelines(&renderer->metalic_material, renderer);
}

void pipeline_cleanup(Renderer* renderer)
{
    vkDestroyPipeline(renderer->device, renderer->pipeline, NULL);
    vkDestroyPipelineLayout(renderer->device, renderer->pipeline_layout, NULL);

    for (int i = 0; i < FRAMES_IN_FLIGHT; ++i)
    {
        descriptor_allocator_growable_destroy_pools(&renderer->frame_descriptors[i], renderer->device);
        descriptor_allocator_growable_free_lists(&renderer->frame_descriptors[i]);
    }
    descriptor_allocator_growable_destroy_pools(&renderer->global_descriptor_allocator, renderer->device);
    descriptor_allocator_growable_free_lists(&renderer->global_descriptor_allocator);

    destroy_pool(renderer->descriptor_pool, renderer->device);
    vkDestroyDescriptorSetLayout(renderer->device, renderer->draw_image_desc_layout, NULL);
    vkDestroyDescriptorSetLayout(renderer->device, renderer->scene_data_desc_set_layout, NULL);
    vkDestroyDescriptorSetLayout(renderer->device, renderer->single_image_desc_layout, NULL);
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
            .descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
            .descriptorCount = 1,
        }
    };

    renderer->single_image_desc_layout = create_descriptor_set_layout(bindings, 1,
            renderer->device, VK_SHADER_STAGE_FRAGMENT_BIT, 0);

    // allocate descriptor
    // renderer->single_image_desc_layout = create_descriptor_set_layout(bindings, 1,
    //         renderer->device, VK_SHADER_STAGE_FRAGMENT_BIT, 0);
    //
    // renderer->draw_image_desc_set = allocate_descriptor_set(renderer->descriptor_pool,
    //         renderer->device, renderer->draw_image_desc_layout);


    // VkDescriptorImageInfo image_info = descriptor_writer_get_image_info(renderer->draw_image.view,
    //         VK_NULL_HANDLE, VK_IMAGE_LAYOUT_GENERAL);
    // VkWriteDescriptorSet write_info = descriptor_writer_get_write(0, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,
    //         NULL, &image_info);
    // descriptor_writer_update_set(renderer->device, renderer->draw_image_desc_set, &write_info, 1);


    for (int i = 0; i < FRAMES_IN_FLIGHT; ++i)
    {
        DescriptorAllocatorGrowable* dag = &renderer->frame_descriptors[i];
        descriptor_allocator_growable_alloc_lists(dag);
        dag->ratios[0] = (VkDescriptorPoolSize) { VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 3 };
        dag->ratios[1] = (VkDescriptorPoolSize) { VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 3 };
        dag->ratios[2] = (VkDescriptorPoolSize) { VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 3 };
        dag->ratios[3] = (VkDescriptorPoolSize) { VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 4 };
        dag->n_ratios = 4;
        dag->sets_per_pool = 1000;

        descriptor_allocator_growable_init(dag, renderer->device);
    }
    DescriptorAllocatorGrowable* dag = &renderer->global_descriptor_allocator;
    descriptor_allocator_growable_alloc_lists(dag);
    dag->ratios[0] = (VkDescriptorPoolSize) { VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 3 };
    dag->ratios[1] = (VkDescriptorPoolSize) { VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 3 };
    dag->ratios[2] = (VkDescriptorPoolSize) { VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 3 };
    dag->ratios[3] = (VkDescriptorPoolSize) { VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 4 };
    dag->n_ratios = 4;
    dag->sets_per_pool = 100;

    descriptor_allocator_growable_init(dag, renderer->device);

    // scene data
    VkDescriptorSetLayoutBinding scene_bindings[] = {
        {
            .binding = 0,
            .descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
            .descriptorCount = 1,
        }
    };

    renderer->scene_data_desc_set_layout = create_descriptor_set_layout(scene_bindings, 1,
            renderer->device, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 0);
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

void descriptor_allocator_growable_alloc_lists(DescriptorAllocatorGrowable* dag)
{
    const uint16_t default_size = 20;
    dag->ratios = malloc(sizeof(VkDescriptorPoolSize) * default_size);
    dag->full_pools = malloc(sizeof(VkDescriptorPool) * default_size);
    dag->ready_pools = malloc(sizeof(VkDescriptorPool) * default_size);

    dag->n_ratios = 0;
    dag->n_full_pools = 0;
    dag->n_ready_pools = 0;
}

void descriptor_allocator_growable_free_lists(DescriptorAllocatorGrowable* dag)
{
    free(dag->ratios);
    free(dag->full_pools);
    free(dag->ready_pools);
}

VkDescriptorPool descriptor_allocator_growable_create_pool(DescriptorAllocatorGrowable* dag,
        VkDevice device)
{
    VkDescriptorPoolSize pool_sizes[dag->n_ratios];
    for (int i = 0; i < dag->n_ratios; ++i)
    {
        pool_sizes[i].type = dag->ratios[i].type;
        pool_sizes[i].descriptorCount = dag->ratios[i].descriptorCount * dag->sets_per_pool;
    }

    VkDescriptorPoolCreateInfo pool_create_info = {
        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
        .flags = 0,
        .maxSets = dag->sets_per_pool,
        .poolSizeCount = dag->n_ratios,
        .pPoolSizes = pool_sizes,
    };

    VkDescriptorPool new_pool;
    vkCreateDescriptorPool(device, &pool_create_info, NULL, &new_pool);

    return new_pool;
}

VkDescriptorPool descriptor_allocator_growable_get_pool(DescriptorAllocatorGrowable* dag,
        VkDevice device)
{
    VkDescriptorPool new_pool;

    if (dag->n_ready_pools != 0)
    {
        new_pool = dag->ready_pools[dag->n_ready_pools - 1];
        dag->n_ready_pools -= 1;
    }
    else
    {
        new_pool = descriptor_allocator_growable_create_pool(dag, device);

        dag->sets_per_pool *= 2;
        if (dag->sets_per_pool > 4092)
            dag->sets_per_pool = 4092;
    }

    return new_pool;
}

void descriptor_allocator_growable_init(DescriptorAllocatorGrowable* dag, VkDevice device)
{
    VkDescriptorPool new_pool = descriptor_allocator_growable_create_pool(dag, device);
    dag->sets_per_pool *= 2;

    dag->ready_pools[dag->n_ready_pools] = new_pool;
    dag->n_ready_pools += 1;
}

void descriptor_allocator_growable_clear_pools(DescriptorAllocatorGrowable* dag, VkDevice device)
{
    for (int i = 0; i < dag->n_ready_pools; ++i)
        vkResetDescriptorPool(device, dag->ready_pools[i], 0);

    for (int i = 0; i < dag->n_full_pools; ++i)
    {
        vkResetDescriptorPool(device, dag->full_pools[i], 0);
        dag->ready_pools[dag->n_ready_pools] = dag->full_pools[i];
        dag->n_full_pools -= 1;
    }
}

void descriptor_allocator_growable_destroy_pools(DescriptorAllocatorGrowable* dag, VkDevice device)
{
    for (int i = 0; i < dag->n_ready_pools; ++i)
        vkDestroyDescriptorPool(device, dag->ready_pools[i], 0);

    for (int i = 0; i < dag->n_full_pools; ++i)
        vkDestroyDescriptorPool(device, dag->full_pools[i], 0);

    dag->n_ready_pools = 0;
    dag->n_full_pools = 0;
}

VkDescriptorSet descriptor_allocator_growable_allocate(DescriptorAllocatorGrowable* dag,
        VkDevice device, VkDescriptorSetLayout layout, void* pNext)
{
    VkDescriptorPool pool_to_use = descriptor_allocator_growable_get_pool(dag, device);

    VkDescriptorSetAllocateInfo alloc_info = {
        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
        .pNext = pNext,

        .descriptorPool = pool_to_use,
        .descriptorSetCount = 1,
        .pSetLayouts = &layout,
    };

    VkDescriptorSet desc_set;
    VkResult e = vkAllocateDescriptorSets(device, &alloc_info, &desc_set);

    if (e == VK_ERROR_OUT_OF_POOL_MEMORY || e == VK_ERROR_FRAGMENTED_POOL)
    {
        dag->full_pools[dag->n_full_pools] = pool_to_use;
        dag->n_full_pools += 1;

        pool_to_use = descriptor_allocator_growable_get_pool(dag, device);
        VK_CHECK(vkAllocateDescriptorSets(device, &alloc_info, &desc_set));
    }

    dag->ready_pools[dag->n_ready_pools] = pool_to_use;
    dag->n_ready_pools += 1;

    return desc_set;
}

VkDescriptorBufferInfo descriptor_writer_get_buffer_info(VkBuffer buffer, size_t size, size_t offset)
{
    VkDescriptorBufferInfo info = {
        .buffer = buffer,
        .offset = offset,
        .range = size,
    };

    return info;
}

VkDescriptorImageInfo descriptor_writer_get_image_info(VkImageView image_view, VkSampler sampler,
        VkImageLayout layout)
{
    VkDescriptorImageInfo info = {
        .sampler = sampler,
        .imageView = image_view,
        .imageLayout = layout,
    };

    return info;
}

VkWriteDescriptorSet descriptor_writer_get_write(int binding, VkDescriptorType type,
        VkDescriptorBufferInfo* buffer_info, VkDescriptorImageInfo* image_info)
{
    VkWriteDescriptorSet write = {
        .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
        .dstBinding = binding,
        .dstSet = VK_NULL_HANDLE, // null for now
        .descriptorCount = 1,
        .descriptorType = type,
    };

    if (buffer_info != NULL)
        write.pBufferInfo = buffer_info;
    else if (image_info != NULL)
        write.pImageInfo = image_info;
    else
        FATAL("Could not setup descriptor write info\n");

    return write;
}

void descriptor_writer_update_set(VkDevice device, VkDescriptorSet set,
        VkWriteDescriptorSet* write_sets, int n_write_sets)
{
    if (set == NULL)
        LOG_E("Destination set was null\n");
    for (int i = 0; i < n_write_sets; ++i)
        write_sets[i].dstSet = set;

    vkUpdateDescriptorSets(device, n_write_sets, write_sets, 0, NULL);
}

void create_pipeline_layout(Renderer* renderer)
{
    VkPushConstantRange push_constant = {
        .offset = 0,
        .size = sizeof(PushConstants),
        .stageFlags = VK_SHADER_STAGE_VERTEX_BIT,
    };

    VkPipelineLayoutCreateInfo pipeline_layout = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
        .pNext = NULL,
        .pSetLayouts = &renderer->single_image_desc_layout,
        .setLayoutCount = 1,

        .pushConstantRangeCount = 1,
        .pPushConstantRanges = &push_constant,
    };

    VK_CHECK(vkCreatePipelineLayout(renderer->device, &pipeline_layout, NULL,
                &renderer->pipeline_layout));
}

void create_pipeline(Renderer* renderer)
{
    VkPipelineShaderStageCreateInfo frag_shader = make_shader_info(renderer->device, "frag.spv",
            VK_SHADER_STAGE_FRAGMENT_BIT);
    VkPipelineShaderStageCreateInfo vert_shader = make_shader_info(renderer->device, "vert.spv",
            VK_SHADER_STAGE_VERTEX_BIT);

    PipelineBuilder pb = {0};
    pb.layout = renderer->pipeline_layout;
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

    renderer->pipeline = pipeline_builder_build(&pb, renderer->device);

    vkDestroyShaderModule(renderer->device, frag_shader.module, NULL);
    vkDestroyShaderModule(renderer->device, vert_shader.module, NULL);
}

// void create_pipeline(Renderer* renderer)
// {
//     // VkShaderModule compute_shader = create_shader(renderer->device, "comp.spv");
//
//     VkPipelineShaderStageCreateInfo stage_info = make_shader_info(renderer->device, "comp.spv",
//             VK_SHADER_STAGE_COMPUTE_BIT);
//
//     VkComputePipelineCreateInfo pipeline_create_info = {
//         .sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO,
//         .pNext = NULL,
//
//         .layout = renderer->pipeline_layout,
//         .stage = stage_info,
//     };
//
//     VK_CHECK(vkCreateComputePipelines(renderer->device, VK_NULL_HANDLE, 1, &pipeline_create_info,
//                 NULL, &renderer->pipeline));
//
//     vkDestroyShaderModule(renderer->device, stage_info.module, NULL);
// }

void pipeline_builder_clear(PipelineBuilder* pb)
{
    FATAL("NO dont clear");
}

VkPipeline pipeline_builder_build(PipelineBuilder* pb, VkDevice device)
{
    VkPipelineViewportStateCreateInfo viewport_state = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO,
        .pNext = NULL,
        .viewportCount = 1,
        .scissorCount = 1,
    };

    VkPipelineColorBlendStateCreateInfo colour_blending = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO,
        .pNext = NULL,

        .logicOpEnable = VK_FALSE,
        .logicOp = VK_LOGIC_OP_COPY,
        .attachmentCount = 1,
        .pAttachments = &pb->colour_blend_attachment,
    };

    VkPipelineVertexInputStateCreateInfo vertex_input_info = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO,
    };

    VkDynamicState state[] = { VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR };
    VkPipelineDynamicStateCreateInfo dynamic_info = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO,
        .pDynamicStates = state,
        .dynamicStateCount = 2,
    };

    // building the actual pipeline
    VkGraphicsPipelineCreateInfo pipeline_info = {
        .sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO,
        .pNext = &pb->rendering_info,

        .stageCount = SHADER_STAGES,
        .pStages = pb->shader_stages,

        .pVertexInputState = &vertex_input_info,
        .pInputAssemblyState = &pb->input_assembly,
        .pViewportState = &viewport_state,
        .pRasterizationState = &pb->rasteriser,
        .pMultisampleState = &pb->multisampling,
        .pColorBlendState = &colour_blending,
        .pDepthStencilState = &pb->depth_stencil,
        .layout = pb->layout,
        .pDynamicState = &dynamic_info,

        .subpass = 1,
    };

    VkPipeline pipeline;
    VkResult e;
    if ((e = vkCreateGraphicsPipelines(device, VK_NULL_HANDLE, 1, &pipeline_info, NULL,
                    &pipeline) != VK_SUCCESS))
    {
        LOG_E("Could not create vk pipeline, error code %d\n", e);
        return VK_NULL_HANDLE;
    }

    return pipeline;
}

void pipeline_builder_set_input_topology(PipelineBuilder* pb, VkPrimitiveTopology topology)
{
    pb->input_assembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    pb->input_assembly.topology = topology;
    pb->input_assembly.primitiveRestartEnable = VK_FALSE;
}

void pipeline_builder_set_polygon_mode(PipelineBuilder* pb, VkPolygonMode mode)
{
    pb->rasteriser.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    pb->rasteriser.polygonMode = mode;
    pb->rasteriser.lineWidth = 1.0f;
}

void pipeline_builder_set_cull_mode(PipelineBuilder* pb, VkCullModeFlags cull_mode,
        VkFrontFace front_face)
{
    pb->rasteriser.cullMode = cull_mode;
    pb->rasteriser.frontFace = front_face;
}

void pipeline_builder_set_multisampling_none(PipelineBuilder* pb)
{
    pb->multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    pb->multisampling.sampleShadingEnable = VK_FALSE;
    pb->multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
    pb->multisampling.minSampleShading = 1.0f;
    pb->multisampling.pSampleMask = NULL;
    // no alpha to coverage either
    pb->multisampling.alphaToCoverageEnable = VK_FALSE;
    pb->multisampling.alphaToOneEnable = VK_FALSE;
}

void pipeline_builder_disable_blending(PipelineBuilder* pb)
{
    pb->colour_blend_attachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT
        | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
    pb->colour_blend_attachment.blendEnable = VK_FALSE;
}

void pipeline_builder_enable_blending(PipelineBuilder* pb, bool additive)
{
    pb->colour_blend_attachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT |
        VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
    pb->colour_blend_attachment.blendEnable = VK_TRUE;
    pb->colour_blend_attachment.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;

    pb->colour_blend_attachment.dstColorBlendFactor = additive ?
        VK_BLEND_FACTOR_ONE : VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;

    pb->colour_blend_attachment.colorBlendOp = VK_BLEND_OP_ADD;
    pb->colour_blend_attachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
    pb->colour_blend_attachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
    pb->colour_blend_attachment.alphaBlendOp = VK_BLEND_OP_ADD;

}

void pipeline_builder_set_color_attachment_format(PipelineBuilder* pb, VkFormat format)
{
    pb->colour_attachment_format = format;

    pb->rendering_info.sType = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO;
    pb->rendering_info.colorAttachmentCount = 1;
    pb->rendering_info.pColorAttachmentFormats = &pb->colour_attachment_format;
}

void pipeline_builder_set_depth_format(PipelineBuilder* pb, VkFormat format)
{
    pb->rendering_info.depthAttachmentFormat = format;
}

void pipeline_builder_set_depthtest(PipelineBuilder* pb, bool enable_depth_write, VkCompareOp op)
{
    pb->depth_stencil.depthTestEnable = VK_TRUE;
    pb->depth_stencil.depthWriteEnable = enable_depth_write;
    pb->depth_stencil.depthCompareOp = op;
    pb->depth_stencil.depthBoundsTestEnable = VK_FALSE;
    pb->depth_stencil.stencilTestEnable = VK_FALSE;
    pb->depth_stencil.minDepthBounds = 0.f;
    pb->depth_stencil.maxDepthBounds = 1.f;
}

