#version 330 core

// Fullscreen triangle from gl_VertexID - no vertex attributes.
uniform vec3 u_UVScale; // xy = used fraction of the (grow-only) source texture

out vec2 v_UV;

void main() {
    vec2 p = vec2((gl_VertexID << 1) & 2, gl_VertexID & 2);
    v_UV = p * u_UVScale.xy;
    gl_Position = vec4(p * 2.0 - 1.0, 0.0, 1.0);
}
