#include "scene.h"
#include "../utils.h"

static const mat4 MAT4_IDENTITY = GLM_MAT4_IDENTITY_INIT;

void ecs_intitialise(ECS* ecs)
{
    ecs->capacity = 50;
    ecs->entities = malloc(sizeof(Entity) * ecs->capacity);

    // calloc because NULL means no component for that entity
    ecs->render_components = calloc(ecs->capacity, sizeof(RenderComponent));

    ecs->count = 0;
}

void ecs_cleanup(ECS* ecs)
{
    free(ecs->entities);
    free(ecs->render_components);
}

void ecs_add_renderable(ECS* ecs, Mesh* mesh, mat4 transformation)
{
    // add an entity
    Entity e = ecs_add_entity(ecs);
    // set the component
    ecs->render_components[e.id].mesh = mesh;
    memcpy(ecs->render_components[e.id].transformation, transformation, sizeof(mat4));
}

Entity ecs_add_entity(ECS* ecs)
{
    Entity e = { ecs->count };
    ecs->entities[ecs->count] = e;
    ecs->count += 1;

    return e;
}

void ecs_renderable_collect(ECS* ecs, Renderer* renderer, DrawContext* context_out)
{
    int counter = 0;
    for (int i = 0; i < ecs->count; ++i)
    {
        if (ecs->render_components[i].mesh == NULL)
        {
            continue;
        }

        Mesh* mesh = ecs->render_components[counter].mesh;
        for (int j = 0; j < mesh->n_surfaces; ++j)
        {
            RenderObject object = {
                .transform = MAT4_UNPACK(ecs->render_components[counter].transformation),
                // .material = mesh->surfaces[j].material == NULL ?
                //     &renderer->default_material_instance : mesh->surfaces[j].material,
                .material = &renderer->default_material_instance,

                .vertex_buffer_address = mesh->mesh_buffers.vertex_buffer_address,
                .index_buffer = mesh->mesh_buffers.index_buffer.buffer,
                .index_count = mesh->surfaces[j].count,
                .first_index = mesh->surfaces[j].start_index,
            };


            glm_translate(object.transform, (vec4){0, 1, 1, 0});
            context_out->opaque_surfaces[counter] = object;
        }

        counter++;
    }
    context_out->n = counter;
}

