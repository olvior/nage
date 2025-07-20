#version 450
#extension GL_GOOGLE_include_directive : require
#extension GL_EXT_buffer_reference : require

#include "input_structures.glsl"

layout (location = 0) out vec3 outNormal;
layout (location = 1) out vec3 outColor;
layout (location = 2) out vec2 outUV;

struct Vertex {
    vec3 position;
    float uv_x;
    vec3 normal;
    float uv_y;
    vec4 color;
};

layout(buffer_reference, std430) readonly buffer VertexBuffer{
    Vertex vertices[];
};

layout( push_constant ) uniform constants
{
    mat4 mvp;
    VertexBuffer vertexBuffer;
} PushConstants;

void main()
{
    // load the vertex
    Vertex v = PushConstants.vertexBuffer.vertices[gl_VertexIndex];


    vec4 pos = vec4(v.position, 1);

    //output the position of each vertex
    gl_Position = PushConstants.mvp * pos;

    outNormal = (PushConstants.mvp * vec4(v.normal, 0.f)).xyz;
    outColor = material_data.color_factors.xyz;
    outUV.x = v.uv_x;
    outUV.y = v.uv_y;
}

