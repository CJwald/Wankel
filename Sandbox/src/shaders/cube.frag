#version 330 core
in vec4 v_Color;
in vec3 v_WorldPos;

out vec4 FragColor;

uniform vec3 u_CameraPos;
uniform vec3 u_FogColor;
uniform float u_FogDensity;

void main()
{
    vec3 color = v_Color.rgb;

    float dist = length(u_CameraPos - v_WorldPos);

    // Exponential squared fog (good choice)
    float fogFactor = exp(-pow(dist * u_FogDensity, 2.0));
    fogFactor = clamp(fogFactor, 0.0, 1.0);

    vec3 finalColor = mix(u_FogColor, color, fogFactor);

    FragColor = vec4(finalColor, v_Color.a);
}
