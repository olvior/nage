#pragma once

#include "renderer.h"

Buffer buffer_create(VmaAllocator allocator, size_t alloc_size, VkBufferUsageFlags usage,
        VmaMemoryUsage memory_usage);

MeshBuffers upload_mesh(Renderer* renderer, uint32_t* indices, int n_indices,
        Vertex* vertices, int n_vertices);

void buffer_destroy(Buffer* buffer, VmaAllocator allocator);

