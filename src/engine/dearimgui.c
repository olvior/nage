#include "dearimgui.h"
#include "utils.h"

#include "renderer/image.h"

#include <vulkan/vulkan.h>

#include <imgui/dcimgui_impl_glfw.h>
#include <imgui/dcimgui_impl_vulkan.h>

void imgui_initialise(Renderer* renderer, GLFWwindow* window, ImGuiIO** io_out)
{
    // oversized descriptor pools for imgui
    VkDescriptorPoolSize pool_sizes[11] = {
        { VK_DESCRIPTOR_TYPE_SAMPLER, 1000 },
        { VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1000 },
        { VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, 1000 },
        { VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 1000 },
        { VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER, 1000 },
        { VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER, 1000 },
        { VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1000 },
        { VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1000 },
        { VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, 1000 },
        { VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC, 1000 },
        { VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT, 1000 }
    };

    VkDescriptorPoolCreateInfo pool_info = {
        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
        .flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT,
        .maxSets = 1000,
        .poolSizeCount = 11,
        .pPoolSizes = pool_sizes,
    };

    VkDescriptorPool imgui_pool;

    VK_CHECK(vkCreateDescriptorPool(renderer->device, &pool_info, NULL, &imgui_pool));

    // create imgui's image

    // VkFormat format = VK_FORMAT_R8G8B8A8_UNORM;
    //
    // VkExtent3D extent = {
    //     .width = renderer->swapchain.extent.width,
    //     .height = renderer->swapchain.extent.height,
    //     .depth = 1,
    // };
    //
    // VkImageUsageFlags flags = VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
    //
    // VkImageCreateInfo image_create_info = get_image_create_info(format, flags, extent);
    //
    // VmaAllocationCreateInfo image_alloc_info = {
    //     .usage = VMA_MEMORY_USAGE_GPU_ONLY,
    //     .requiredFlags = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
    // };
    //
    // vmaCreateImage(renderer->allocator, &image_create_info, &image_alloc_info,
    //         &renderer->imgui_image.image, &renderer->imgui_image.allocation, NULL);
    //
    // VkImageViewCreateInfo image_view_info = get_image_view_create_info(format,
    //         renderer->imgui_image.image, VK_IMAGE_ASPECT_COLOR_BIT);
    //
    // VK_CHECK(vkCreateImageView(renderer->device, &image_view_info, NULL, &renderer->imgui_image.view));

    // init imgui
    ImGui_CreateContext(NULL);

    *io_out = ImGui_GetIO();
    ImGuiIO* io = *io_out;
    io->ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;     // Enable Keyboard Controls
    io->ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;      // Enable Gamepad Controls
    io->ConfigFlags |= ImGuiConfigFlags_DockingEnable;         // Enable Docking
    io->ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;

    cImGui_ImplGlfw_InitForVulkan(window, true);

    ImGui_ImplVulkan_InitInfo init_info = {
        .Instance = renderer->instance,
        .PhysicalDevice = renderer->gpu,
        .Device = renderer->device,
        .Queue = renderer->graphics_queue,
        .DescriptorPool = imgui_pool,
        .MinImageCount = 3,
        .ImageCount = 3,
        .UseDynamicRendering = true,

        // dynamic rendering pipelines
        .PipelineRenderingCreateInfo = {
            .sType = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO,
            .colorAttachmentCount = 1,
            .pColorAttachmentFormats = &renderer->swapchain.image_format,
        },

        .MSAASamples = VK_SAMPLE_COUNT_1_BIT,
    };

    cImGui_ImplVulkan_Init(&init_info);

    renderer->imgui_pool = imgui_pool;
}

void imgui_cleanup(Renderer* renderer)
{
    cImGui_ImplVulkan_Shutdown();
    vkDestroyDescriptorPool(renderer->device, renderer->imgui_pool, NULL);

    // vkDestroyImageView(renderer->device, renderer->imgui_image.view, NULL);
    // vmaDestroyImage(renderer->allocator, renderer->imgui_image.image, renderer->imgui_image.allocation);
}

void imgui_draw(Renderer* renderer, VkCommandBuffer cmd_buf, VkImageView target_image_view)
{
    VkRenderingAttachmentInfo colour_attachment = {
        .sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO,
        .pNext = NULL,

        .imageView = target_image_view,
        .imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,

        .loadOp = VK_ATTACHMENT_LOAD_OP_LOAD,
        .storeOp = VK_ATTACHMENT_STORE_OP_STORE,
    };

    VkRenderingInfoKHR rendering_info = {
        .sType = VK_STRUCTURE_TYPE_RENDERING_INFO,
        .pNext = NULL,

        .flags = 0,
        .renderArea = {
            .offset = {0, 0},
            .extent = renderer->swapchain.extent,
        },

        .layerCount = 1,
        .viewMask = 0,
        .colorAttachmentCount = 1,
        .pColorAttachments = &colour_attachment,
    };

    PFN_vkCmdBeginRenderingKHR func_begin = (PFN_vkCmdBeginRenderingKHR)
        vkGetDeviceProcAddr(renderer->device, "vkCmdBeginRenderingKHR");

    PFN_vkCmdEndRendering func_end = (PFN_vkCmdEndRenderingKHR)
        vkGetDeviceProcAddr(renderer->device, "vkCmdEndRenderingKHR");

    if (func_begin == NULL)
        FATAL("Could not find address of dynamic rendering begin function!\n");

    if (func_end == NULL)
        FATAL("Could not find address of dynamic rendering end function!\n");

    func_begin(cmd_buf, &rendering_info);

    cImGui_ImplVulkan_RenderDrawData(ImGui_GetDrawData(), cmd_buf);

    func_end(cmd_buf);
}

void imgui_frame(ImGuiIO* io, Renderer* renderer)
{
    cImGui_ImplVulkan_NewFrame();
    cImGui_ImplGlfw_NewFrame();
    ImGui_NewFrame();


    if (ImGui_Begin("Settings", NULL, 0))
    {
        ImGui_ColorPicker4("Ambient", renderer->scene_data.ambient_colour, 0, NULL);
        ImGui_SliderFloat("Dir x", &renderer->scene_data.sunlight_direction[0], -5, 5);
        ImGui_SliderFloat("Dir y", &renderer->scene_data.sunlight_direction[1], -5, 5);
        ImGui_SliderFloat("Dir z", &renderer->scene_data.sunlight_direction[2], -5, 5);
        ImGui_SliderFloat("fov", &renderer->fov, 0, 180);
    }
    ImGui_End();

    ImGui_Render();

    // Update and Render additional Platform Windows
    if (io->ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
    {
        ImGui_UpdatePlatformWindows();
        ImGui_RenderPlatformWindowsDefault();
    }
}
