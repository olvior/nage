#version 450
#extension GL_EXT_buffer_reference : require

layout (location = 0) out vec3 outColor;
layout (location = 1) out vec2 outUV;

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
    mat4 render_matrix;
    VertexBuffer vertexBuffer;
} PushConstants;

void main()
{
    // load the vertex
    Vertex v = PushConstants.vertexBuffer.vertices[gl_VertexIndex];


    //output the position of each vertex
    gl_Position = PushConstants.render_matrix * vec4(v.position, 1);

    outColor = v.position.xyz;
    outUV.x = v.uv_x;
    outUV.y = v.uv_y;
}

