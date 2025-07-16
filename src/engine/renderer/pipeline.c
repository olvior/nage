#include "pipeline.h"
#include "shaders.h"
#include "../utils.h"

#include <vulkan/vulkan.h>

const int SHADER_STAGES = 2;

typedef struct PipelineBuilder {
    VkPipelineLayout layout;

    VkPipelineShaderStageCreateInfo shader_stages[SHADER_STAGES];
    VkPipelineInputAssemblyStateCreateInfo input_assembly;
    VkPipelineRasterizationStateCreateInfo rasteriser;
    VkPipelineColorBlendAttachmentState colour_blend_attachment;
    VkPipelineMultisampleStateCreateInfo multisampling;
    VkPipelineLayout pipeline_layout;
    VkPipelineDepthStencilStateCreateInfo depth_stencil;
    VkPipelineRenderingCreateInfo rendering_info;
    VkFormat colour_attachment_format;
} PipelineBuilder;


void pipeline_initialise(Renderer* renderer)
{
    descriptors_initialise(renderer);
    create_pipeline_layout(renderer);
    create_pipeline(renderer);

    // PushConstants a = {{100}};
    // renderer->push_constants = a;
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
    VkPushConstantRange push_constant = {
        .offset = 0,
        .size = sizeof(PushConstants),
        .stageFlags = VK_SHADER_STAGE_VERTEX_BIT,
    };

    VkPipelineLayoutCreateInfo pipeline_layout = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
        .pNext = NULL,
        .pSetLayouts = &renderer->draw_image_desc_layout,
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
    pipeline_builder_set_cull_mode(&pb, VK_CULL_MODE_NONE, VK_FRONT_FACE_COUNTER_CLOCKWISE);
    pipeline_builder_set_multisampling_none(&pb);
    pipeline_builder_disable_blending(&pb);
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
    if (vkCreateGraphicsPipelines(device, VK_NULL_HANDLE, 1, &pipeline_info, NULL, &pipeline)
            != VK_SUCCESS)
    {
        LOG_E("Could not create vk pipeline");
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

