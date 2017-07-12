#version 420 core
#extension GL_ARB_shading_language_420pack : require
const vec2 quadVertices[4] = { vec2(-1.0, -1.0), vec2(1.0, -1.0), vec2(-1.0, 1.0), vec2(1.0, 1.0) };
void main() { gl_Position = vec4(quadVertices[gl_VertexID], 0.0, 1.0); }
