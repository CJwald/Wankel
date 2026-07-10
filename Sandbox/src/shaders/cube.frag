#version 330 core

in vec4 v_Color;
in vec3 v_WorldPos;
in vec3 v_Normal;

out vec4 FragColor;

uniform vec3 u_CameraPos;

// DIRECTIONAL LIGHT (single "sun" - no attenuation, distance-independent)
uniform vec3 u_LightDir;   // direction the light travels (points FROM light TOWARD the scene)
uniform vec3 u_LightColor;
uniform float u_AmbientStrength;
uniform float u_SpecularStrength;
uniform float u_Shininess;

uniform vec3 u_FogColor;
uniform float u_FogDensity;

uniform int u_FogNoiseEnabled;

uniform float u_Time;
uniform float u_FogNoiseScale;
uniform float u_FogNoiseStrength;
uniform float u_FogNoiseSpeed;
uniform int u_FogNoiseOctaves;

// WIND (VECTORISED)
uniform vec3 u_FogWindDir;
uniform float u_FogWindSpeed;

// 3D HASH
float hash(vec3 p) {
    p = fract(p * 0.3183099 + vec3(0.1, 0.2, 0.3));
    p *= 17.0;
    return fract(p.x * p.y * p.z * (p.x + p.y + p.z));
}

// 3D VALUE NOISE
float noise(vec3 p) {
    vec3 i = floor(p);
    vec3 f = fract(p);

    float n000 = hash(i + vec3(0,0,0));
    float n100 = hash(i + vec3(1,0,0));
    float n010 = hash(i + vec3(0,1,0));
    float n110 = hash(i + vec3(1,1,0));
    float n001 = hash(i + vec3(0,0,1));
    float n101 = hash(i + vec3(1,0,1));
    float n011 = hash(i + vec3(0,1,1));
    float n111 = hash(i + vec3(1,1,1));

    vec3 u = f * f * (3.0 - 2.0 * f);

    float nx00 = mix(n000, n100, u.x);
    float nx10 = mix(n010, n110, u.x);
    float nx01 = mix(n001, n101, u.x);
    float nx11 = mix(n011, n111, u.x);

    float nxy0 = mix(nx00, nx10, u.y);
    float nxy1 = mix(nx01, nx11, u.y);

    return mix(nxy0, nxy1, u.z);
}

// 3D FBM
float fbm(vec3 p) {
    float value = 0.0;
    float amplitude = 0.5;
    float frequency = 1.0;

    for(int i = 0; i < u_FogNoiseOctaves; i++) {
        value += amplitude * noise(p * frequency);
        frequency *= 2.0;
        amplitude *= 0.5;
    }

    return value;
}

// MAIN
void main() {
    // BLINN-PHONG LIGHTING
    vec3 N = normalize(v_Normal);
    vec3 L = normalize(-u_LightDir);
    vec3 V = normalize(u_CameraPos - v_WorldPos);
    vec3 H = normalize(L + V);

    float diff = max(dot(N, L), 0.0);
    float spec = pow(max(dot(N, H), 0.0), u_Shininess);

    vec3 ambient = u_AmbientStrength * u_LightColor;
    vec3 diffuse = diff * u_LightColor;
    vec3 specular = u_SpecularStrength * spec * u_LightColor;

    vec3 color = v_Color.rgb * (ambient + diffuse) + specular;

    float dist = length(u_CameraPos - v_WorldPos);

    float fogDensity = u_FogDensity;

    // 3D NOISE FOG
    if(u_FogNoiseEnabled == 1) {
        vec3 pos = v_WorldPos * u_FogNoiseScale;

        // VECTORISED WIND
        vec3 windOffset = u_FogWindDir * (u_Time * u_FogWindSpeed);
        vec3 samplePos = pos + windOffset;
        float n = fbm(samplePos);
        float noiseMod = mix(0.75, 1.25, n);

        fogDensity *= mix(1.0, noiseMod, u_FogNoiseStrength);
    }

    // DISTANCE FOG
    float fogFactor = exp(-pow(dist * fogDensity, 2.0));
    fogFactor = clamp(fogFactor, 0.0, 1.0);
    vec3 finalColor = mix(u_FogColor, color, fogFactor);

    FragColor = vec4(finalColor, v_Color.a);
}
