#pragma once

#include <vulkan/vulkan.h>
#include <GLFW/glfw3.h>

typedef struct {
    VkInstance instance;
    VkDebugUtilsMessengerEXT debug_messenger;
    VkSurfaceKHR surface;
    VkDevice device;
    VkPhysicalDevice gpu;
} Renderer;


void renderer_initialise(Renderer* renderer, GLFWwindow* window);
void renderer_cleanup(Renderer* renderer);


// internal
void renderer_create_instance(Renderer* renderer);
void renderer_create_surface(Renderer* renderer, GLFWwindow* window);
