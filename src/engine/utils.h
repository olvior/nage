#pragma once

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>

#include <vulkan/vulkan.h>
#include <vulkan/vk_enum_string_helper.h>

#define INVALID_IDX -1

#define ANSI_DEFAULT 0
#define ANSI_RED 31
#define ANSI_YELLOW 33

// interesting set of macros
#define CHECK_VK_FATAL(x)                             \
    do {                                              \
        if (x)                                        \
            FATAL("VULKAN %s\n", string_VkResult(x)); \
    } while (0)

#define FATAL(msg, ...)                   \
    do {                                  \
        SET_COLOUR(ANSI_RED, stderr);     \
        fprintf(stderr, "FATAL: ");       \
        LOG(msg, ##__VA_ARGS__);          \
        SET_COLOUR(ANSI_DEFAULT, stderr); \
        exit(-1);                         \
    } while (0)

#define LOG_W(msg, ...)                   \
    do {                                  \
        SET_COLOUR(ANSI_YELLOW, stderr);  \
        fprintf(stderr, "WARNING: ");     \
        SET_COLOUR(ANSI_DEFAULT, stderr); \
        LOG(msg, ##__VA_ARGS__);          \
    } while (0)

#define LOG_E(msg, ...)                   \
    do {                                  \
        SET_COLOUR(ANSI_RED, stderr);     \
        fprintf(stderr, "ERROR: ");       \
        SET_COLOUR(ANSI_DEFAULT, stderr); \
        LOG(msg, ##__VA_ARGS__);          \
    } while (0)

#define LOG(msg, ...)                                                  \
    do {                                                               \
        fprintf(stderr, msg, ##__VA_ARGS__);                           \
        fprintf(stderr, "- %s:%d@%s\n", __FILE__, __LINE__, __func__); \
    } while (0)

#define SET_COLOUR(x, place)             \
    do {                                 \
        fprintf(place, "%c[%dm", 27, x); \
    } while (0)


void print_string_list(const char* b[], int n);

