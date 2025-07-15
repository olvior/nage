#include "buffers.h"
#include "../utils.h"

Buffer buffer_create(VmaAllocator allocator, size_t alloc_size, VkBufferUsageFlags usage,
        VmaMemoryUsage memory_usage)
{
    VkBufferCreateInfo buffer_info = {
        .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
        .pNext = NULL,
        .size = alloc_size,
        .usage = usage,
    };

    VmaAllocationCreateInfo vma_alloc_info = {
        .usage = memory_usage,
        .flags = VMA_ALLOCATION_CREATE_MAPPED_BIT,
    };

    Buffer buffer;
    VK_CHECK(vmaCreateBuffer(allocator, &buffer_info, &vma_alloc_info, &buffer.buffer,
                &buffer.allocation, &buffer.info));

    return buffer;
}

void buffer_destroy(Buffer* buffer, VmaAllocator allocator)
{
    vmaDestroyBuffer(allocator, buffer->buffer, buffer->allocation);
}

MeshBuffers upload_mesh(Renderer* renderer, uint32_t* indices, int n_indices,
        Vertex* vertices, int n_vertices)
{
    const size_t vertex_buffer_size = n_vertices * sizeof(Vertex);
    const size_t index_buffer_size = n_indices * sizeof(uint32_t);

    MeshBuffers new_mesh;

    new_mesh.vertex_buffer = buffer_create(renderer->allocator, vertex_buffer_size,
            VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT
            | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT, VMA_MEMORY_USAGE_GPU_ONLY);

    VkBufferDeviceAddressInfo device_address_info = {
        .sType = VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO,
        .buffer = new_mesh.vertex_buffer.buffer,
    };

    new_mesh.vertex_buffer_address = vkGetBufferDeviceAddress(renderer->device, &device_address_info);

    new_mesh.index_buffer = buffer_create(renderer->allocator, index_buffer_size,
            VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
            VMA_MEMORY_USAGE_GPU_ONLY);


    Buffer staging_buffer = buffer_create(renderer->allocator, vertex_buffer_size + index_buffer_size,
            VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VMA_MEMORY_USAGE_CPU_ONLY);

    void* data;

    vmaMapMemory(renderer->allocator, staging_buffer.allocation, &data);
    // copy vertex buffer
    memcpy(data, vertices, vertex_buffer_size);
    // copy index buffer
    memcpy((char*)data + vertex_buffer_size, indices, index_buffer_size);
    vmaUnmapMemory(renderer->allocator, staging_buffer.allocation);

    // submit the command in immediate mode

    immediate_begin(renderer);

    VkBufferCopy vertex_copy = {
        .dstOffset = 0,
        .srcOffset = 0,
        .size = vertex_buffer_size,
    };

    vkCmdCopyBuffer(renderer->imm_cmd_buf, staging_buffer.buffer, new_mesh.vertex_buffer.buffer,
            1, &vertex_copy);

    VkBufferCopy index_copy = {
        .dstOffset = 0,
        .srcOffset = vertex_buffer_size,
        .size = index_buffer_size,
    };

    vkCmdCopyBuffer(renderer->imm_cmd_buf, staging_buffer.buffer, new_mesh.index_buffer.buffer,
            1, &index_copy);

    immediate_end(renderer);

    buffer_destroy(&staging_buffer, renderer->allocator);

    return new_mesh;
}

