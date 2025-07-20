#pragma once

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>

#include <vulkan/vulkan.h>
#include <vulkan/vk_enum_string_helper.h>

#define INVALID_IDX -1

#define ONE_SEC 1000000000

#define ANSI_DEFAULT 0
#define ANSI_RED 31
#define ANSI_YELLOW 33

#define max(a, b) a > b ? a : b

// interesting set of macros
#define VK_CHECK(x)                                   \
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

#define LOG_V(msg, ...)                        \
    do {                                       \
        printf("%s:%d: ", __FILE__, __LINE__); \
        printf(msg, ##__VA_ARGS__);            \
    } while (0)

#define LOG(msg, ...)                                                  \
    do {                                                               \
        fprintf(stderr, msg, ##__VA_ARGS__);                           \
        fprintf(stderr, "- %s:%d@%s\n", __FILE__, __LINE__, __func__); \
    } while (0)

#define LOG_B(msg, ...)             \
    do {                            \
        printf(msg, ##__VA_ARGS__); \
    } while (0)

#define SET_COLOUR(x, place)             \
    do {                                 \
        fprintf(place, "%c[%dm", 27, x); \
    } while (0)

#define MAT4_UNPACK(m) {{m[0][0], m[0][1], m[0][2], m[0][3]}, {m[1][0], m[1][1], m[1][2], m[1][3]}, {m[2][0], m[2][1], m[2][2], m[2][3]}, {m[3][0], m[3][1], m[3][2], m[3][3]}}

#define MAT4_FSTR(m) "%f\t%f\t%f\t%f\n%f\t%f\t%f\t%f\n%f\t%f\t%f\t%f\n%f\t%f\t%f\t%f\n", m[0][0], m[0][1], m[0][2], m[0][3], m[1][0], m[1][1], m[1][2], m[1][3], m[2][0], m[2][1], m[2][2], m[2][3], m[3][0], m[3][1], m[3][2], m[3][3]



typedef struct {
    uint32_t graphics_family;
    uint32_t present_family;
} QueueFamilyIndices;

typedef struct {
    long size;
    char* buf;
} File;

File read_file(const char path[]);

void print_string_list(const char* b[], int n);
uint32_t clamp(uint32_t a, uint32_t min, uint32_t max);

QueueFamilyIndices find_queue_families(VkPhysicalDevice* gpu, VkSurfaceKHR* surface);
bool indices_complete(QueueFamilyIndices* indeces);

void* get_device_proc_adr(VkDevice device, char name[]);

