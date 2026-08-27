#version 330 core

in vec2 v_UV;
in vec4 v_Color;

out vec4 FragColor;

// Built-in soft round sprite - single channel (R8), supplies only the alpha shape. Per-particle rgba
// comes from v_Color.
uniform sampler2D u_Sprite;

void main() {
    float mask = texture(u_Sprite, v_UV).r;
    FragColor = vec4(v_Color.rgb, v_Color.a * mask);
}
