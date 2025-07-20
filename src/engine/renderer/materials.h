#pragma once
#include "renderer.h"

void material_metallic_initialise_pipelines(MaterialMetallic* mat, Renderer* renderer);
MaterialInstance material_metallic_write_material(MaterialMetallic* mat, VkDevice device,
        enum MaterialPass pass_type, const MaterialMetallicResources* resources,
        DescriptorAllocatorGrowable* dag);
void material_metallic_cleanup(MaterialMetallic* mat, Renderer* renderer);


// internal

void material_metallic_build_descriptors(MaterialMetallic* mat, Renderer* renderer);
void material_metallic_build_layouts(MaterialMetallic* mat, Renderer* renderer);
void material_metallic_build_pipelines(MaterialMetallic* mat, Renderer* renderer);


