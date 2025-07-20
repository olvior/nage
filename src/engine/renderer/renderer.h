#pragma once

#define CGLM_FORCE_LEFT_HANDED
#define CGLM_FORCE_DEPTH_ZERO_TO_ONE

#define FRAMES_IN_FLIGHT 3

#include <vulkan/vulkan.h>
#include <GLFW/glfw3.h>
#include <cglm/cglm.h>
#include <vk_mem_alloc.h>

typedef struct Vertex {
    vec3 position;
    float uv_x;
    vec3 normal;
    float uv_y;
    vec4 colour;
} Vertex;

typedef struct GPUSceneData {
    vec4 ambient_colour;
    vec4 sunlight_direction; // [3] is for sun power
    vec4 sunlight_colour;
} GPUSceneData;

typedef struct PushConstants {
    mat4 world_matrix;
    VkDeviceAddress vertex_buffer;
} PushConstants;

typedef struct Buffer {
    VkBuffer buffer;
    VmaAllocation allocation;
    VmaAllocationInfo info;
} Buffer;

typedef struct Image {
    VkImage image;
    VkImageView view;
    VmaAllocation allocation;
    VkExtent3D extent;
    VkFormat format;
} Image;

typedef struct DescriptorAllocatorGrowable {
    VkDescriptorPoolSize* ratios;
    VkDescriptorPool* full_pools;
    VkDescriptorPool* ready_pools;

    uint16_t n_ratios;
    uint16_t n_full_pools;
    uint16_t n_ready_pools;
    uint16_t sets_per_pool;
} DescriptorAllocatorGrowable;


typedef struct Swapchain {
    VkSwapchainKHR swapchain;
    VkExtent2D extent;

    VkFormat image_format;
    uint32_t image_count;

    // these need to be heap allocated since we don't know how many there will be
    VkImage* images;
    VkImageView* image_views;
} Swapchain;

enum MaterialPass {
    MAT_PASS_MAIN_COLOUR, MAT_PASS_TRANSPARENT
};

typedef struct MaterialPipeline {
    VkPipeline pipeline;
    VkPipelineLayout layout;
} MaterialPipeline;

typedef struct MaterialInstance {
    MaterialPipeline* pipeline;
    VkDescriptorSet material_set;
    enum MaterialPass pass_type;
} MaterialInstance;

typedef struct MaterialMetallic {
    MaterialPipeline pipeline_opaque;
    MaterialPipeline pipeline_transparent;

    VkDescriptorSetLayout material_layout;
} MaterialMetallic;

typedef struct {
    vec4 colour_factors;
    vec4 metal_rough_factors;

    // padding to reach 256 bytes
    vec4 pad[14];
} MaterialMetallicConstants;

typedef struct MaterialMetallicResources {
    Image colour_image;
    VkSampler colour_sampler;
    Image metal_rough_image;
    VkSampler metal_rough_sampler;
    VkBuffer data_buffer;
    uint32_t data_buffer_offset;
} MaterialMetallicResources;

typedef struct RenderObject {
    mat4 transform;

    MaterialInstance* material;

    VkDeviceAddress vertex_buffer_address;
    VkBuffer index_buffer;
    uint32_t index_count;
    uint32_t first_index;
} RenderObject;

typedef struct MeshBuffers {
    Buffer index_buffer;
    Buffer vertex_buffer;
    VkDeviceAddress vertex_buffer_address;
} MeshBuffers;

typedef struct GeoSurface {
    uint32_t start_index;
    uint32_t count;
    MaterialInstance* material;
} GeoSurface;

typedef struct Mesh {
    char* name;

    int n_surfaces;
    GeoSurface* surfaces;
    MeshBuffers mesh_buffers;
} Mesh;

typedef struct DrawContext {
    RenderObject* opaque_surfaces;
    int n;
} DrawContext;

typedef struct Renderer {
    VkInstance instance;
    VkDebugUtilsMessengerEXT debug_messenger;
    VkSurfaceKHR surface;
    VkDevice device;
    VkPhysicalDevice gpu;
    VmaAllocator allocator;

    VkDescriptorPool descriptor_pool;
    VkPipeline pipeline;
    VkPipelineLayout pipeline_layout;

    Swapchain swapchain;
    VkQueue graphics_queue;

    Image draw_image;
    Image depth_image;
    VkExtent2D draw_extent;
    VkDescriptorSet draw_image_desc_set;
    VkDescriptorSetLayout draw_image_desc_layout;

    VkCommandPool command_pools[FRAMES_IN_FLIGHT];
    VkCommandBuffer command_buffers[FRAMES_IN_FLIGHT];

    VkSemaphore semaphores_swapchain[FRAMES_IN_FLIGHT];
    VkSemaphore semaphores_render[FRAMES_IN_FLIGHT];
    VkFence fences[FRAMES_IN_FLIGHT];

    DescriptorAllocatorGrowable frame_descriptors[FRAMES_IN_FLIGHT];
    DescriptorAllocatorGrowable global_descriptor_allocator;

    VkCommandPool imm_cmd_pool;
    VkCommandBuffer imm_cmd_buf;
    VkFence imm_fence;
    VkDescriptorPool imgui_pool;

    GPUSceneData scene_data;
    VkDescriptorSetLayout scene_data_desc_set_layout;
    VkDescriptorSetLayout single_image_desc_layout;

    MaterialInstance default_material_instance;
    MaterialMetallic metalic_material;

    VkSampler sampler_nearest;
    VkSampler sampler_linear;
    Image error_image;
    vec3 translation;
    float fov;

    Buffer scene_data_buffer;

    Mesh* meshes;
    Buffer* buf_destroy;

    int frame;

    uint8_t n_meshes;
    uint8_t frame_in_flight;
    uint8_t n_buf_destroy;

    bool resize_requested;
} Renderer;


void renderer_initialise(Renderer* renderer, GLFWwindow* window);
void renderer_draw(Renderer* renderer, DrawContext* context);
void renderer_cleanup(Renderer* renderer);

int validation_layers_count();
const char** validation_layers();

void immediate_begin(Renderer* renderer);
void immediate_end(Renderer* renderer);

// internal
void renderer_create_instance(Renderer* renderer);
void renderer_create_surface(Renderer* renderer, GLFWwindow* window);
void create_command_buffers(Renderer* renderer);
void cleanup_command_buffers(Renderer* renderer);
VkCommandBufferSubmitInfo get_command_buffer_submit_info(VkCommandBuffer cmd);
VkSubmitInfo get_submit_info(Renderer* renderer);
void renderer_inc_frame(Renderer* renderer);
void vma_allocator_initialise(Renderer* renderer);
void sync_initialise(Renderer* renderer);
void sync_cleanup(Renderer* renderer);

void draw_geometry(Renderer* renderer, VkCommandBuffer cmd_buf, DrawContext* context);
void initialise_data(Renderer* renderer);

