// Directional-light shadows + sky occlusion (Wankel::ShadowSettings) - #include from a lit fragment shader.

uniform sampler2DShadow u_SunShadowMap;
uniform sampler2DShadow u_SkyShadowMap;

uniform int u_SunShadowEnabled;
uniform mat4 u_SunViewProj;
uniform float u_SunDepthRange;
uniform float u_ShadowDepthBias;
uniform float u_ShadowNormalBias;
uniform float u_ShadowPcfRadius;
uniform float u_ShadowStrength;

uniform int u_SkyOcclusionEnabled;
uniform mat4 u_SkyViewProj;
uniform float u_SkyDepthRange;
uniform float u_SkyFadeDepth;
uniform float u_SkySoftness;
uniform float u_SkyNormalOffset;
uniform float u_CaveAmbient;

uniform float u_WorldHalfY;

// Vertically tiled copies of the world shade exactly like the real (base) tile the maps were rendered from.
vec3 WrapToBaseTileY(vec3 p) {
    if (u_WorldHalfY > 0.0)
        p.y = mod(p.y + u_WorldHalfY, 2.0 * u_WorldHalfY) - u_WorldHalfY;
    return p;
}

// Light-space [0,1] coords, or z < 0 when outside the map's box.
vec3 ShadowCoords(mat4 viewProj, vec3 p) {
    vec3 c = (viewProj * vec4(p, 1.0)).xyz * 0.5 + 0.5;
    bool inside = all(greaterThanEqual(c, vec3(0.0))) && all(lessThanEqual(c, vec3(1.0)));
    return inside ? c : vec3(-1.0);
}

#ifndef SHADOW_FILTER_TAPS
#define SHADOW_FILTER_TAPS 9 // ShadowSettings::FilterTaps
#endif

// Tap offsets in units of the filter spacing: a 3x3 grid, or 2x2 at +-0.75 - each tap is a hardware-filtered
// 2x2 compare, so four of those cover about the same footprint as the 3x3 for less than half the lookups.
vec2 ShadowTapOffset(int tap) {
    if (SHADOW_FILTER_TAPS == 9)
        return vec2(tap % 3 - 1, tap / 3 - 1);
    return (vec2(tap & 1, tap >> 1) - 0.5) * 1.5;
}

// 0 = no sky light reaches this point (under terrain), 1 = open to the top world border. Blurs over
// u_SkySoftness texels and fades over u_SkyFadeDepth below the topmost surface, one depth step per tap.
float SkyVisibility(vec3 worldPos, vec3 N) {
    if (u_SkyOcclusionEnabled == 0)
        return 1.0;
    vec3 p = WrapToBaseTileY(worldPos) + vec3(N.x, 0.0, N.z) * u_SkyNormalOffset;
    vec3 c = ShadowCoords(u_SkyViewProj, p);
    if (c.z < 0.0)
        return 1.0;

    vec2 texel = u_SkySoftness / vec2(textureSize(u_SkyShadowMap, 0));
    float fade = u_SkyFadeDepth / max(u_SkyDepthRange, 1e-4);
    float surfaceBias = 0.1 / max(u_SkyDepthRange, 1e-4);
    float sum = 0.0;
    for (int tap = 0; tap < SHADOW_FILTER_TAPS; tap++) {
        float depthStep = float(tap) / float(SHADOW_FILTER_TAPS - 1); // spread depth offsets 0..fade across the taps
        sum += texture(u_SkyShadowMap, vec3(c.xy + ShadowTapOffset(tap) * texel, c.z - surfaceBias - fade * depthStep));
    }
    return sum / float(SHADOW_FILTER_TAPS);
}

// Direct sunlight visibility; `fallback` is used outside the sun map's box (e.g. sky visibility, so far
// caves stay dark).
float SunVisibility(vec3 worldPos, vec3 N, float fallback) {
    if (u_SunShadowEnabled == 0)
        return fallback;
    vec3 p = WrapToBaseTileY(worldPos) + N * u_ShadowNormalBias;
    vec3 c = ShadowCoords(u_SunViewProj, p);
    if (c.z < 0.0)
        return fallback;

    vec2 texel = u_ShadowPcfRadius / vec2(textureSize(u_SunShadowMap, 0));
    float ref = c.z - u_ShadowDepthBias / max(u_SunDepthRange, 1e-4);
    float lit = 0.0;
    for (int tap = 0; tap < SHADOW_FILTER_TAPS; tap++)
        lit += texture(u_SunShadowMap, vec3(c.xy + ShadowTapOffset(tap) * texel, ref));
    lit /= float(SHADOW_FILTER_TAPS);

    // Ease into the fallback near the map's edge instead of popping.
    vec2 edge = min(c.xy, 1.0 - c.xy);
    float edgeWeight = smoothstep(0.0, 0.05, min(edge.x, edge.y));
    return mix(fallback, mix(1.0, lit, u_ShadowStrength), edgeWeight);
}
