#version 330 core

in vec2 v_UV;

uniform sampler2D u_Texture;
uniform vec3 u_Step; // xy = UV offset between taps along the blur axis; zero = plain copy

out vec4 FragColor;

const float kWeights[5] = float[](0.227027, 0.1945946, 0.1216216, 0.054054, 0.016216);

void main() {
    vec3 color = texture(u_Texture, v_UV).rgb * kWeights[0];
    for (int i = 1; i < 5; i++) {
        vec2 offset = u_Step.xy * float(i);
        color += texture(u_Texture, v_UV + offset).rgb * kWeights[i];
        color += texture(u_Texture, v_UV - offset).rgb * kWeights[i];
    }
    if (u_Step.x == 0.0 && u_Step.y == 0.0)
        color = texture(u_Texture, v_UV).rgb;
    FragColor = vec4(color, 1.0);
}
