#pragma once

#define FRAMES_IN_FLIGHT 3

#include <vulkan/vulkan.h>
#include <GLFW/glfw3.h>

typedef struct VmaAllocator_T* VmaAllocator;
typedef struct VmaAllocation_T* VmaAllocation;

typedef struct {
    VkImage image;
    VkImageView view;
    VmaAllocation allocation;
    VkExtent3D extent;
    VkFormat format;
} Image;

typedef struct {
    VkSwapchainKHR swapchain;
    VkExtent2D extent;

    VkFormat image_format;
    uint32_t image_count;

    // these need to be heap allocated since we don't know how many there will be
    VkImage* images;
    VkImageView* image_views;
} Swapchain;

typedef struct {
    VkInstance instance;
    VkDebugUtilsMessengerEXT debug_messenger;
    VkSurfaceKHR surface;
    VkDevice device;
    VkPhysicalDevice gpu;
    VmaAllocator allocator;

    VkDescriptorPool descriptor_pool;

    Swapchain swapchain;
    VkQueue graphics_queue;

    Image draw_image;
    VkExtent2D draw_extent;
    VkDescriptorSet draw_image_desc_set;
    VkDescriptorSetLayout draw_image_desc_layout;


    VkCommandPool command_pools[FRAMES_IN_FLIGHT];
    VkCommandBuffer command_buffers[FRAMES_IN_FLIGHT];

    VkSemaphore semaphores_swapchain[FRAMES_IN_FLIGHT];
    VkSemaphore semaphores_render[FRAMES_IN_FLIGHT];
    VkFence fences[FRAMES_IN_FLIGHT];

    int frame_in_flight;
    int frame;
} Renderer;


void renderer_initialise(Renderer* renderer, GLFWwindow* window);
void renderer_draw(Renderer* renderer);
void renderer_cleanup(Renderer* renderer);

int validation_layers_count();
const char** validation_layers();

// internal
void renderer_create_instance(Renderer* renderer);
void renderer_create_surface(Renderer* renderer, GLFWwindow* window);
void create_command_buffers(Renderer* renderer);
void cleanup_command_buffers(Renderer* renderer);
VkCommandBufferSubmitInfo get_command_buffer_submit_info(VkCommandBuffer cmd);
VkSubmitInfo get_submit_info(Renderer* renderer);
void renderer_inc_frame(Renderer* renderer);
void vma_allocator_initialise(Renderer* renderer);
void sync_initialise(Renderer* renderer);
void sync_cleanup(Renderer* renderer);

