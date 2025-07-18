#include "loader.h"
#include "../utils.h"
#include "../renderer/buffers.h"

#include <cgltf.h>
#include <fast_obj.h>

static cgltf_result LoadFileGLTFCallback(const struct cgltf_memory_options *memoryOptions, const struct cgltf_file_options *fileOptions, const char *path, cgltf_size *size, void **data)
{
    File a = read_file(path);
    int filesize = a.size;
    unsigned char *filedata = (unsigned char*)a.buf;

    if (filedata == NULL) return cgltf_result_io_error;

    *size = filesize;
    *data = filedata;

    return cgltf_result_success;
}

static void ReleaseFileGLTFCallback(const struct cgltf_memory_options *memoryOptions, const struct cgltf_file_options *fileOptions, void *data)
{
    free(data);
}

Mesh* load_glft_meshes(Renderer* renderer, char* file_path, int* out_n)
{
    LOG_V("Loading GLTF %s\n", file_path);

    File file = read_file(file_path);

    cgltf_options options = { 0 };
    options.file.read = LoadFileGLTFCallback;
    options.file.release = ReleaseFileGLTFCallback;
    cgltf_data *data = NULL;
    cgltf_result result = cgltf_parse(&options, file.buf, file.size, &data);

    if (result != cgltf_result_success)
        FATAL("Could not load GLTF file %s %d\n", file_path, result);

    result = cgltf_load_buffers(&options, data, file_path);

    Mesh* meshes = malloc(sizeof(Mesh) * data->meshes_count);
    *out_n = data->meshes_count;

    for (int i = 0; i < data->meshes_count; ++i)
    {
        LOG_V("Loading mesh\n");
        cgltf_mesh gltf_mesh = data->meshes[i];
        Mesh new_mesh = {0};

        new_mesh.name = gltf_mesh.name;
        new_mesh.surfaces = malloc(sizeof(GeoSurface) * gltf_mesh.primitives_count);

        uint32_t index_count = 0;
        uint32_t indices[9999];
        uint32_t vertex_count = 0;
        Vertex vertices[9999];
        int extra_vertices_found = 0;

        for (int j = 0; j < gltf_mesh.primitives_count; ++j)
        {
            LOG_V("Loading primitive\n");
            cgltf_primitive primitive = gltf_mesh.primitives[j];

            GeoSurface new_surface = {0};

            new_surface.start_index = index_count;
            new_surface.count = primitive.indices->count;

            // uint32_t* new_indices = malloc(sizeof(uint32_t) * (index_count + primitive.indices->count));
            // memcpy(new_indices, indices, sizeof(uint32_t) * index_count);
            // free(indices);
            // indices = new_indices;

            // loading the indices
            int primitive_index_count = cgltf_accessor_unpack_indices(primitive.indices, NULL, 4, 0);

            cgltf_accessor_unpack_indices(primitive.indices, indices + index_count,
                    4, primitive_index_count);

            for (int a = 0; a < primitive_index_count; ++a)
                indices[index_count + a] += vertex_count;

            index_count += primitive_index_count;

            // loading the vertices
            for (int a = 0; a < primitive.attributes_count; a++)
            {
                if (primitive.attributes[a].type == cgltf_attribute_type_position)
                {
                    LOG_V("Loading position attributes");
                    cgltf_accessor* attribute = primitive.attributes[a].data;

                    int primitive_vertex_count = cgltf_accessor_unpack_floats(attribute, NULL, 0);
                    float raw_floats[primitive_vertex_count * 3 + 10];
                    int success = cgltf_accessor_unpack_floats(attribute, raw_floats, primitive_vertex_count * 3);
                    extra_vertices_found = success;

                    LOG_B(" %d\n", success);
                    for (int b = 0; b < primitive_vertex_count; ++b)
                    {
                        vertices[vertex_count + b].position[0] = raw_floats[b * 3];
                        vertices[vertex_count + b].position[1] = raw_floats[b * 3 + 1];
                        vertices[vertex_count + b].position[2] = raw_floats[b * 3 + 2];
                        // LOG_V("%f\n", raw_floats[b*3+2]);
                    }
                }
                if (primitive.attributes[a].type == cgltf_attribute_type_texcoord)
                {
                    LOG_V("Loading texcoord");
                    cgltf_accessor* attribute = primitive.attributes[a].data;
                    int primitive_uv_count = cgltf_accessor_unpack_floats(attribute, NULL, 0);
                    float raw_floats[primitive_uv_count * 2 + 10];
                    int success = cgltf_accessor_unpack_floats(attribute, raw_floats, primitive_uv_count * 2);
                    LOG_B(" %d\n", success);
                    for (int b = 0; b < primitive_uv_count; ++b)
                    {
                        vertices[vertex_count + b].uv_x = raw_floats[b * 2];
                        vertices[vertex_count + b].uv_y = raw_floats[b * 2 + 1];
                        vertices[vertex_count + b].colour[0] = raw_floats[b * 2];
                        vertices[vertex_count + b].colour[1] = raw_floats[b * 2 + 1];
                    }
                }
                if (primitive.attributes[a].type == cgltf_attribute_type_color)
                {
                    cgltf_accessor* attribute = primitive.attributes[a].data;
                    int primitive_colour_count = cgltf_accessor_unpack_floats(attribute, NULL, 0);
                    float raw_floats[primitive_colour_count * 3 + 10];
                    int success = cgltf_accessor_unpack_floats(attribute, raw_floats, primitive_colour_count * 3);
                    for (int b = 0; b < primitive_colour_count; ++b)
                    {
                        vertices[vertex_count + b].colour[0] = raw_floats[b * 3];
                        vertices[vertex_count + b].colour[1] = raw_floats[b * 3 + 1];
                        vertices[vertex_count + b].colour[2] = raw_floats[b * 3 + 2];
                    }
                }
            }
            vertex_count += extra_vertices_found;
            new_mesh.surfaces[j] = new_surface;
        }

        new_mesh.mesh_buffers = upload_mesh(renderer, indices, index_count, vertices, vertex_count);
        meshes[i] = new_mesh;

        // free(vertices);
        // free(indices);
    }

    cgltf_free(data);

    return meshes;
}

Mesh load_obj_mesh(Renderer* renderer, char* file_path)
{
    LOG_V("Loading OBJ %s\n", file_path);


    Mesh mesh = {0};
    mesh.name = "hi";
    mesh.n_surfaces = 1;
    GeoSurface* a = malloc(sizeof(GeoSurface));
    mesh.surfaces = a;
    fastObjMesh* obj_mesh = fast_obj_read("room.obj");

    Vertex vertices[obj_mesh->position_count];
    uint32_t indices[obj_mesh->index_count * 3];
    LOG_V("Found %d positions\n", obj_mesh->position_count / 3);

    for (int i = 1; i < obj_mesh->position_count / 3; ++i)
    {
        vertices[i].position[0] = obj_mesh->positions[i * 3 + 0];
        vertices[i].position[1] = obj_mesh->positions[i * 3 + 1];
        vertices[i].position[2] = obj_mesh->positions[i * 3 + 2];
    }

    for (int i = 0; i < obj_mesh->index_count; ++i)
    {
        indices[i * 3 + 0] = obj_mesh->indices[i].p;
        indices[i * 3 + 1] = obj_mesh->indices[i].t;
        indices[i * 3 + 2] = obj_mesh->indices[i].n;
    }

    mesh.mesh_buffers = upload_mesh(renderer, indices, obj_mesh->index_count * 3, vertices,
            obj_mesh->position_count / 3);

    a->count = obj_mesh->index_count * 3;
    fast_obj_destroy(obj_mesh);

    LOG_V("Finished loading\n");
    return mesh;
}


void meshes_destroy(Mesh* meshes, int n, VmaAllocator allocator)
{
    for (int i = 0; i < n; ++i)
    {
        buffer_destroy(&meshes[i].mesh_buffers.index_buffer, allocator);
        buffer_destroy(&meshes[i].mesh_buffers.vertex_buffer, allocator);

        free(meshes[i].surfaces);
    }
    free(meshes);
}

