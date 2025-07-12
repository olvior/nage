#pragma once

#define FRAMES_IN_FLIGHT 3

#include <vulkan/vulkan.h>
#include <GLFW/glfw3.h>

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

    Swapchain swapchain;
    VkQueue graphics_queue;

    VkCommandPool command_pools[FRAMES_IN_FLIGHT];
    VkCommandBuffer command_buffers[FRAMES_IN_FLIGHT];

    VkSemaphore semaphores_swapchain[FRAMES_IN_FLIGHT];
    VkSemaphore semaphores_render[FRAMES_IN_FLIGHT];
    VkFence fences[FRAMES_IN_FLIGHT];

    int current_frame;
    int total_frames;
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

