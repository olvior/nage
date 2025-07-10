#include "renderer.h"
#include "debug.h"
#include "../utils.h"


const char* VALIDATION_LAYERS = {
    "VK_LAYER_KHRONOS_validation",
};
const int VALIDATION_LAYERS_COUNT = 1;

// Main code
void renderer_initialise(Renderer* renderer, GLFWwindow* window)
{
    renderer_create_instance(renderer);
    create_debug_messenger(&renderer->instance, &renderer->debug_messenger);
    renderer_create_surface(renderer, window);
    device_initialise(&renderer->device, &renderer->gpu);
}

void renderer_cleanup(Renderer* renderer)
{
    vkDestroySurfaceKHR(renderer->instance, renderer->surface, NULL);
    destroy_debug_messenger(&renderer->instance, &renderer->debug_messenger);
    vkDestroyInstance(renderer->instance, NULL);
}

void renderer_create_instance(Renderer* renderer)
{
    #ifdef VALIDATION_LAYERS_ENABLED
    if (!validation_layers_supported(&VALIDATION_LAYERS, VALIDATION_LAYERS_COUNT))
        FATAL("Validation layers not found\n");
    #endif

    VkApplicationInfo app_info = {
        .sType = VK_STRUCTURE_TYPE_APPLICATION_INFO,
        .pApplicationName = "Test app",
        .pEngineName = "Engine name here",
        .engineVersion = VK_MAKE_VERSION(0, 1, 0),
        .apiVersion = VK_API_VERSION_1_3,
    };

    uint32_t extension_count;
    get_required_extensions(&extension_count, NULL);
    const char* extensions[extension_count];
    get_required_extensions(&extension_count, extensions);

    print_string_list(extensions, extension_count);

    #ifdef VALIDATION_LAYERS_ENABLED
        VkDebugUtilsMessengerCreateInfoEXT debug_create_info = {0};
        populate_debug_messenger_info(&debug_create_info);
    #endif

    VkInstanceCreateInfo instance_create_info = {
        .sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
        .pApplicationInfo = &app_info,
        #ifdef __APPLE__
        .flags = VK_INSTANCE_CREATE_ENUMERATE_PORTABILITY_BIT_KHR,
        #endif

        .enabledExtensionCount = extension_count,
        .ppEnabledExtensionNames = extensions,

        #ifdef VALIDATION_LAYERS_ENABLED
        .enabledLayerCount = VALIDATION_LAYERS_COUNT,
        .ppEnabledLayerNames = &VALIDATION_LAYERS,
        .pNext = &debug_create_info,
        #endif
    };

    VkResult result = vkCreateInstance(&instance_create_info, NULL, &renderer->instance);
    CHECK_VK_FATAL(result);

    LOG_W("oh no\n");
}

void renderer_create_surface(Renderer* renderer, GLFWwindow* window)
{
    CHECK_VK_FATAL(
            glfwCreateWindowSurface(renderer->instance, window, NULL, &renderer->surface)
    );
}
