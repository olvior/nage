#pragma once

#include <vulkan/vulkan.h>
#include <GLFW/glfw3.h>

#include "renderer.h"

typedef struct {
    VkSurfaceCapabilitiesKHR capabilities;
    VkSurfaceFormatKHR* formats;
    VkPresentModeKHR* present_modes;

    uint32_t format_count;
    uint32_t present_mode_count;
} SwapChainSupportDetails;

void swapchain_initialise(Renderer* renderer, GLFWwindow* window);
void swapchain_cleanup(Renderer* renderer);

// internal
void create_swap_chain(Renderer* renderer, GLFWwindow* window);
void create_image_views(Renderer* renderer);
void query_swap_chain_support(Renderer* renderer, SwapChainSupportDetails* details);
VkSurfaceFormatKHR choose_swap_surface_format(SwapChainSupportDetails* details);
VkPresentModeKHR choose_swap_present_mode(SwapChainSupportDetails* details);
VkExtent2D choose_swap_extent(GLFWwindow* window, VkSurfaceCapabilitiesKHR* capabilities);
void create_drawing_image(Renderer* renderer);

