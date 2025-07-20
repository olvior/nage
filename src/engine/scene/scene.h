#pragma once

#include <cglm/cglm.h>
#include <vulkan/vulkan.h>
#include <renderer/renderer.h>

typedef struct Entity {
    size_t id;
} Entity;

typedef struct RenderComponent {
    Mesh* mesh;
    mat4 transformation;
} RenderComponent;

typedef struct ECS {
    Entity* entities;
    RenderComponent* render_components;

    size_t count;
    size_t capacity;
} ECS;


void ecs_intitialise(ECS* ecs);
void ecs_cleanup(ECS* ecs);
Entity ecs_add_entity(ECS* ecs);
void ecs_add_renderable(ECS* ecs, Mesh* mesh, mat4 transformation);
void ecs_render_component_draw(ECS* ecs);
void ecs_renderable_collect(ECS* ecs, Renderer* renderer, DrawContext* context_out);

