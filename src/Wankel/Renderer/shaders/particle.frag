#version 330 core

in vec2 v_UV;
in vec2 v_QuadUV;
flat in vec4 v_UVRect;
flat in vec2 v_Extra; // erosion threshold, noise seed
in vec4 v_Color;

out vec4 FragColor;

// RGBA particle texture (or the built-in white soft sprite) - multiplied by the per-particle color.
uniform sampler2D u_Texture;
uniform sampler2D u_Noise;      // tiling value noise (r)
uniform sampler2D u_SceneDepth; // opaque scene depth, only bound when a batch uses soft particles
uniform float u_Emissive;       // rgb multiplier, > 1 for bright additive effects
uniform float u_Time;
uniform vec3 u_NearFar; // camera near, far

// Per batch (one layer) - each feature is skipped entirely unless the layer's material enables it.
uniform int u_Soft;
uniform float u_SoftDistance;
uniform int u_Erosion;
uniform float u_ErosionSoftness;
uniform int u_Distortion;
uniform vec3 u_DistortionParams; // strength, scale, scroll speed

float LinearDepth(float depth) {
    float z = depth * 2.0 - 1.0;
    float n = u_NearFar.x, f = u_NearFar.y;
    return 2.0 * n * f / (f + n - z * (f - n));
}

void main() {
    float seed = v_Extra.y;
    vec2 uv = v_UV;
    if (u_Distortion == 1) {
        vec2 q = v_QuadUV * u_DistortionParams.y + vec2(seed * 7.31, seed * 3.17) +
                 vec2(0.3, 1.0) * u_Time * u_DistortionParams.z;
        vec2 wobble = vec2(texture(u_Noise, q).r, texture(u_Noise, q + vec2(0.37, 0.61)).r) - 0.5;
        // Offset is in quad space, scaled into the cell and clamped so it never samples a neighboring frame.
        uv = clamp(uv + wobble * 2.0 * u_DistortionParams.x * v_UVRect.zw, v_UVRect.xy, v_UVRect.xy + v_UVRect.zw);
    }

    vec4 tex = texture(u_Texture, uv);
    float alpha = tex.a * v_Color.a;

    if (u_Erosion == 1) {
        float n = texture(u_Noise, v_QuadUV * 0.75 + vec2(seed * 5.13, seed * 9.71)).r;
        float threshold = v_Extra.x;
        alpha *= smoothstep(threshold - u_ErosionSoftness, threshold + u_ErosionSoftness, n);
    }

    if (u_Soft == 1) {
        float sceneDepth = LinearDepth(texelFetch(u_SceneDepth, ivec2(gl_FragCoord.xy), 0).r);
        alpha *= clamp((sceneDepth - LinearDepth(gl_FragCoord.z)) / u_SoftDistance, 0.0, 1.0);
    }

    FragColor = vec4(tex.rgb * v_Color.rgb * u_Emissive, alpha);
}
