#version 330 core

in vec2 v_UV;
in vec4 v_Color;

out vec4 FragColor;

// RGBA particle texture (or the built-in white soft sprite) - multiplied by the per-particle color.
uniform sampler2D u_Texture;
uniform float u_Emissive; // rgb multiplier, > 1 for bright additive effects

void main() {
    vec4 tex = texture(u_Texture, v_UV);
    FragColor = vec4(tex.rgb * v_Color.rgb * u_Emissive, tex.a * v_Color.a);
}
