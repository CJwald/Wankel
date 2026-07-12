#version 330 core

in vec2 v_UV;
out vec4 FragColor;

uniform sampler2D u_FontAtlas;
uniform vec3 u_Color;

void main() {
    float alpha = texture(u_FontAtlas, v_UV).r;
    FragColor = vec4(u_Color, alpha);
}
