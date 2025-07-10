#pragma once

#include "../utils.h"

#ifdef DEBUG
    #define VALIDATION_LAYERS_ENABLED 1
#endif

void create_debug_messenger(VkInstance* instance, VkDebugUtilsMessengerEXT* debug_msgr);
void destroy_debug_messenger(VkInstance* instance, VkDebugUtilsMessengerEXT* debug_msgr);


void populate_debug_messenger_info(VkDebugUtilsMessengerCreateInfoEXT* info);
bool validation_layers_supported(const char* layers[], int n);
void get_required_extensions(uint32_t* count, const char** extensions);

static VKAPI_ATTR VkBool32 VKAPI_CALL debug_callback(
    VkDebugUtilsMessageSeverityFlagBitsEXT m_severity,
    VkDebugUtilsMessageTypeFlagsEXT m_type,
    const VkDebugUtilsMessengerCallbackDataEXT* p_data,
    void* p_user_data);

