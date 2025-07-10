#pragma once

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>

#include <vulkan/vulkan.h>
#include <vulkan/vk_enum_string_helper.h>


#define CHECK_VK_FATAL(x)                                             \
    if (x) {                                                          \
        fprintf(stderr, "%s:%d@%s:\n", __FILE__, __LINE__, __func__); \
        fprintf(stderr, "FATAL: VULKAN %s\n", string_VkResult(x));    \
        exit(x);                                                      \
    }



