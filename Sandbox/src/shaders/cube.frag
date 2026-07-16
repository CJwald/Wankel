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

// MATERIAL (solid-color PBR, metallic-roughness workflow - no textures/UVs yet)
uniform vec3 u_Albedo;
uniform float u_Roughness;
uniform float u_Metallic;
uniform vec3 u_Emissive;

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

// PBR (Cook-Torrance: GGX/Trowbridge-Reitz normal distribution, Smith
// geometry term, Schlick Fresnel approximation), metallic-roughness
// workflow. Reference: Karis 2013 "Real Shading in Unreal Engine 4" /
// LearnOpenGL's PBR tutorial / the glTF 2.0 metallic-roughness spec.
const float PI = 3.14159265359;

float DistributionGGX(vec3 N, vec3 H, float roughness) {
    float a = roughness * roughness;
    float a2 = a * a;
    float NdotH = max(dot(N, H), 0.0);
    float NdotH2 = NdotH * NdotH;
    float denom = (NdotH2 * (a2 - 1.0) + 1.0);
    denom = PI * denom * denom;
    return a2 / max(denom, 1e-4);
}

float GeometrySchlickGGX(float NdotV, float roughness) {
    float r = roughness + 1.0;
    float k = (r * r) / 8.0; // direct-lighting Schlick-GGX k
    return NdotV / (NdotV * (1.0 - k) + k);
}

float GeometrySmith(vec3 N, vec3 V, vec3 L, float roughness) {
    float ggx2 = GeometrySchlickGGX(max(dot(N, V), 0.0), roughness);
    float ggx1 = GeometrySchlickGGX(max(dot(N, L), 0.0), roughness);
    return ggx1 * ggx2;
}

vec3 FresnelSchlick(float cosTheta, vec3 F0) {
    return F0 + (1.0 - F0) * pow(clamp(1.0 - cosTheta, 0.0, 1.0), 5.0);
}

// MAIN
void main() {
    // PBR LIGHTING - Cook-Torrance (GGX + Smith + Schlick), metallic-roughness workflow.
    vec3 N = normalize(v_Normal);
    vec3 V = normalize(u_CameraPos - v_WorldPos);
    vec3 L = normalize(-u_LightDir);
    vec3 H = normalize(V + L);

    // Clamp away from 0 - roughness=0 makes the GGX denominator's a2 term hit
    // exactly 0 at N==H, producing a 0/0 (NaN/black-pixel) singularity that a
    // user dragging a slider to its minimum would trivially hit.
    float roughness = clamp(u_Roughness, 0.045, 1.0);
    float metallic = clamp(u_Metallic, 0.0, 1.0);

    // Dielectrics get a fixed ~4% F0 (standard non-metal reflectance); metals use Albedo as F0.
    vec3 F0 = mix(vec3(0.04), u_Albedo, metallic);

    float NdotL = max(dot(N, L), 0.0);
    float NdotV = max(dot(N, V), 0.0);

    vec3 F = FresnelSchlick(max(dot(H, V), 0.0), F0);
    float NDF = DistributionGGX(N, H, roughness);
    float G = GeometrySmith(N, V, L, roughness);

    vec3 specular = (NDF * G * F) / max(4.0 * NdotV * NdotL, 1e-4);

    // Energy conservation: kD is what's left after specular reflectance F, then
    // zeroed for metals entirely (metals have no diffuse subsurface term).
    vec3 kD = (vec3(1.0) - F) * (1.0 - metallic);
    vec3 diffuse = kD * u_Albedo / PI;

    // NOTE: u_SpecularStrength now scales total direct-light intensity (both
    // diffuse and specular), not just specular as it did under Blinn-Phong -
    // PBR ties the two together, so keeping the old "Specular" slider name but
    // widening what it controls keeps it meaningful instead of dead.
    vec3 radiance = u_LightColor * u_SpecularStrength;
    vec3 directLight = (diffuse + specular) * radiance * NdotL;

    // Crude flat ambient (no IBL/environment map in this scope - see TODO.md);
    // metals get dimmer ambient since real metals have no true diffuse response.
    vec3 ambient = u_AmbientStrength * u_LightColor * u_Albedo * (1.0 - metallic * 0.5);

    vec3 color = ambient + directLight + u_Emissive;

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
