#include "utils.h"

void print_string_list(const char* b[], int n)
{
    for (int i = 0; i < n; ++i)
        printf("%s\n", b[i]);
}

uint32_t clamp(uint32_t a, uint32_t min, uint32_t max)
{
    if (a > max) { return max; }
    if (a < min) { return min; }
    return a;
}

QueueFamilyIndices find_queue_families(VkPhysicalDevice* gpu, VkSurfaceKHR* surface)
{
    QueueFamilyIndices indices = {INVALID_IDX, INVALID_IDX};

    uint32_t queue_family_count = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(*gpu, &queue_family_count, NULL);

    VkQueueFamilyProperties queue_families[queue_family_count];
    vkGetPhysicalDeviceQueueFamilyProperties(*gpu, &queue_family_count, queue_families);

    for (int i = 0; i < queue_family_count; ++i)
    {
        VkQueueFamilyProperties queue_family = queue_families[i];
        if (queue_family.queueFlags & VK_QUEUE_GRAPHICS_BIT)
        {
            indices.graphics_family = i;
        }

        VkBool32 present_support = 0;
        vkGetPhysicalDeviceSurfaceSupportKHR(*gpu, i, *surface, &present_support);
        if (present_support)
            indices.present_family = i;

        if (indices_complete(&indices))
            break;
    }

    return indices;
}

bool indices_complete(QueueFamilyIndices* indices)
{
    return indices->graphics_family != INVALID_IDX && indices->present_family != INVALID_IDX;
}

