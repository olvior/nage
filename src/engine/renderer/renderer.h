#pragma once

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
} Renderer;


void renderer_initialise(Renderer* renderer, GLFWwindow* window);
void renderer_cleanup(Renderer* renderer);

int validation_layers_count();
const char** validation_layers();

// internal
void renderer_create_instance(Renderer* renderer);
void renderer_create_surface(Renderer* renderer, GLFWwindow* window);
