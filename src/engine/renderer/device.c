#include "device.h"

#ifdef __APPLE__
    const char* device_extensions[] = {
        VK_KHR_SWAPCHAIN_EXTENSION_NAME,
        VK_KHR_PORTABILITY_SUBSET_EXTENSION_NAME,
    };
    const int device_extension_count = 2;
#else
    static const char* device_extensions[] = {
        VK_KHR_SWAPCHAIN_EXTENSION_NAME,
    };
    const int device_extension_count = 1;
#endif

