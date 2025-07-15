#include "device.h"

#define UNSUPPORTED_GPU -1

// macos just needs one extra extension :)
#ifdef __APPLE__
    const int DEVICE_EXTENSION_COUNT = 4;
    const char* DEVICE_EXTENSIONS[DEVICE_EXTENSION_COUNT] = {
        VK_KHR_SWAPCHAIN_EXTENSION_NAME,
        VK_KHR_DYNAMIC_RENDERING_EXTENSION_NAME,
        "VK_KHR_portability_subset",
        "VK_KHR_push_descriptor",
    };
#else
    const int DEVICE_EXTENSION_COUNT = 3;
    static const char* DEVICE_EXTENSIONS[DEVICE_EXTENSION_COUNT] = {
        VK_KHR_SWAPCHAIN_EXTENSION_NAME,
        VK_KHR_DYNAMIC_RENDERING_EXTENSION_NAME,
        "VK_KHR_push_descriptor",
    };
#endif


void device_initialise(Renderer* renderer)
{
    pick_gpu(renderer);
    create_device(renderer);
}

void pick_gpu(Renderer* renderer)
{
    uint32_t device_count = 0;
    vkEnumeratePhysicalDevices(renderer->instance, &device_count, NULL);

    if (device_count == 0)
        FATAL("No gpu could be found");

    VkPhysicalDevice devices_found[device_count];
    int scores[device_count];
    vkEnumeratePhysicalDevices(renderer->instance, &device_count, devices_found);

    LOG_V("Devices list:\n");
    int max_score = UNSUPPORTED_GPU;
    int max_idx = UNSUPPORTED_GPU;
    for (int i = 0; i < device_count; ++i)
    {
        scores[i] = rate_device(devices_found[i], &renderer->surface);
        if (scores[i] > max_score)
        {
            max_score = scores[i];
            max_idx = i;
        }

        VkPhysicalDeviceProperties p;
        vkGetPhysicalDeviceProperties(devices_found[i], &p);
        LOG_V("%s %d\n", p.deviceName, scores[i]);
    }


    if (max_score == UNSUPPORTED_GPU)
        FATAL("Could not find gpu with supported features");

    renderer->gpu = devices_found[max_idx];

    VkPhysicalDeviceProperties p;
    vkGetPhysicalDeviceProperties(renderer->gpu, &p);
    uint32_t v = p.apiVersion;
    LOG_V("Picked: %s version %d.%d.%d\n", p.deviceName, VK_API_VERSION_MAJOR(v),
            VK_API_VERSION_MINOR(v), VK_API_VERSION_PATCH(v));
}

void create_device(Renderer* renderer)
{
    QueueFamilyIndices indices = find_queue_families(&renderer->gpu, &renderer->surface);

    float queue_priority = 1;

    VkDeviceQueueCreateInfo queue_create_info = {
        .sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
        .queueFamilyIndex = indices.graphics_family,
        .pQueuePriorities = &queue_priority,
        .queueCount = 1,
    };

    VkPhysicalDeviceFeatures device_features = {0};

    // enable the feature
    VkPhysicalDeviceBufferDeviceAddressFeatures buffer_address_feature = {
        .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_BUFFER_DEVICE_ADDRESS_FEATURES,
        .bufferDeviceAddress = VK_TRUE,
    };

    VkPhysicalDeviceDynamicRenderingFeaturesKHR dynamic_rendering_feature = {
        .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DYNAMIC_RENDERING_FEATURES_KHR,
        .pNext = &buffer_address_feature,
        .dynamicRendering = VK_TRUE,
    };

    VkPhysicalDeviceFeatures2 device_features2 = {
        .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2,
        .pNext = &dynamic_rendering_feature,
        .features = device_features,
    };

    VkDeviceCreateInfo device_create_info = {
        .sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
        .pNext = &device_features2,
        .pQueueCreateInfos = &queue_create_info,
        .queueCreateInfoCount = 1,

        // .pEnabledFeatures = &device_features,

        .ppEnabledExtensionNames = DEVICE_EXTENSIONS,
        .enabledExtensionCount = DEVICE_EXTENSION_COUNT,

        #ifdef VALIDATION_LAYERS_ENABLED
        .enabledLayerCount = validation_layers_count(),
        .ppEnabledLayerNames = validation_layers(),
        #endif
    };


    VK_CHECK(
            vkCreateDevice(renderer->gpu, &device_create_info, NULL, &renderer->device)
    );

    vkGetDeviceQueue(renderer->device, indices.graphics_family, 0, &renderer->graphics_queue);
}

int rate_device(VkPhysicalDevice gpu, VkSurfaceKHR* surface)
{
    VkPhysicalDeviceProperties device_properties;
    VkPhysicalDeviceFeatures device_features;
    vkGetPhysicalDeviceProperties(gpu, &device_properties);
    vkGetPhysicalDeviceFeatures(gpu, &device_features);

    // starts as doable
    int score = 0;

    // discrete gpu = better
    if (device_properties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU)
        score += 1000;

    QueueFamilyIndices indices = find_queue_families(&gpu, surface);

    // not good
    if (!indices_complete(&indices))
        return UNSUPPORTED_GPU;

    int extensions_supported = check_extention_device_support(gpu);
    if (!extensions_supported)
        return UNSUPPORTED_GPU;

    return score;
}

bool check_extention_device_support(VkPhysicalDevice gpu)
{
    uint32_t extension_count;
    vkEnumerateDeviceExtensionProperties(gpu, NULL, &extension_count, NULL);

    VkExtensionProperties available_extensions[extension_count];
    vkEnumerateDeviceExtensionProperties(gpu, NULL, &extension_count, available_extensions);

    // #ifdef DEBUG
    // printf("Extensions found:\n");
    // for (int i = 0; i < extension_count; ++i) {
    //     printf("%s\n", available_extensions[i].extensionName);
    // }
    // #endif

    int all_found = true;
    for (int i = 0; i < DEVICE_EXTENSION_COUNT && all_found; ++i)
    {
        int found = 0;
        for (int j = 0; j < extension_count && !found; ++j)
        {
            VkExtensionProperties p = available_extensions[j];
            if (strcmp(DEVICE_EXTENSIONS[i], p.extensionName) == 0)
                found = true;
        }

        if (!found)
            all_found = false;

    }

    return all_found;
}

