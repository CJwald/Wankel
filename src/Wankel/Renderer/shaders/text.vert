#version 330 core
layout(location = 0) in vec2 aPos;
layout(location = 1) in vec2 aUV;

out vec2 v_UV;

uniform mat4 u_Projection;

void main() {
    v_UV = aUV;
    gl_Position = u_Projection * vec4(aPos, 0.0, 1.0);
}
