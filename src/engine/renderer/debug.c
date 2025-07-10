#include "debug.h"

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>


bool validation_layers_supported(const char* layers[], int n)
{
    uint32_t layer_count;
    vkEnumerateInstanceLayerProperties(&layer_count, NULL);

    VkLayerProperties available_layers[layer_count];
    vkEnumerateInstanceLayerProperties(&layer_count, available_layers);

    for (int i = 0; i < n; ++i)
    {
        const char* layer_name = layers[i];
        bool layer_found = false;

        for (int j = 0; j < layer_count; ++j)
        {
            if (strcmp(layer_name, available_layers[j].layerName) == 0)
                layer_found = true;
        }

        if (!layer_found)
            return false;
    }

    return true;
}

void get_required_extensions(uint32_t* count, const char** extensions)
{
    const char** extensions_original = glfwGetRequiredInstanceExtensions(count);
    int count_original = *count;
    #ifdef __APPLE__
    *count += 2;
    #endif
    #ifdef VALIDATION_LAYERS_ENABLED
    *count += 1;
    #endif

    if (extensions == NULL)
        return;


    for (int i = 0; i < count_original; ++i)
        extensions[i] = extensions_original[i];

    #ifdef __APPLE__
    extensions[count_original] = VK_KHR_PORTABILITY_ENUMERATION_EXTENSION_NAME;
    extensions[count_original + 1] = "VK_KHR_get_physical_device_properties2";
    #endif

    #ifdef VALIDATION_LAYERS_ENABLED
    extensions[*count - 1] = VK_EXT_DEBUG_UTILS_EXTENSION_NAME;
    #endif

    printf("a, %d", *count);
}

VKAPI_ATTR VkBool32 VKAPI_CALL debug_callback(
    VkDebugUtilsMessageSeverityFlagBitsEXT m_severity,
    VkDebugUtilsMessageTypeFlagsEXT m_type,
    const VkDebugUtilsMessengerCallbackDataEXT* p_data,
    void* p_user_data)
{
    if (m_severity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT)
        LOG_W("validation layers - %s: %s\n", p_data->pMessageIdName, p_data->pMessage);
    else if (m_severity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT)
        LOG_W("validation layers - %s: %s\n", p_data->pMessageIdName, p_data->pMessage);

    return VK_FALSE;
}

void populate_debug_messenger_info(VkDebugUtilsMessengerCreateInfoEXT* info)
{
    info->sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
    info->messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT
        | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
    info->messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT
        | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT
        | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
    info->pfnUserCallback = debug_callback;
    info->flags = 0;
}

void create_debug_messenger(VkInstance* instance, VkDebugUtilsMessengerEXT* debug_msgr)
{
    VkDebugUtilsMessengerCreateInfoEXT debug_create_info = {0};
    populate_debug_messenger_info(&debug_create_info);

    VkResult result;
    // weird address things
    PFN_vkCreateDebugUtilsMessengerEXT func = (PFN_vkCreateDebugUtilsMessengerEXT)
        vkGetInstanceProcAddr(*instance, "vkCreateDebugUtilsMessengerEXT");

    if (func != NULL)
        result = func(*instance, &debug_create_info, NULL, debug_msgr);
    else
        result = VK_ERROR_EXTENSION_NOT_PRESENT;

    CHECK_VK_FATAL(result);
}

void destroy_debug_messenger(VkInstance* instance, VkDebugUtilsMessengerEXT* debug_msgr)
{
    PFN_vkDestroyDebugUtilsMessengerEXT func = (PFN_vkDestroyDebugUtilsMessengerEXT)
        vkGetInstanceProcAddr(*instance, "vkDestroyDebugUtilsMessengerEXT");

    if (func != NULL)
        func(*instance, *debug_msgr, NULL);
}

