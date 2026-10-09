// Texture repetition mitigation (Wankel::TextureVariationSettings) - the stage-agnostic part, #included by
// terrain_material.glsl and by terrain vertex shaders, which evaluate TexVarVertexNoise and pass it down.

// On/off switches are compile-time (TerrainMaterials::SyncShaderDefines), so disabled features cost nothing.
#ifndef TEXVAR_ENABLED
#define TEXVAR_ENABLED 0
#endif
#ifndef TEXVAR_TRANSFORM
#define TEXVAR_TRANSFORM 0
#endif
#ifndef TEXVAR_VARIANTS
#define TEXVAR_VARIANTS 0
#endif
#ifndef TEXVAR_MACRO
#define TEXVAR_MACRO 0
#endif
#ifndef TEXVAR_DETAIL
#define TEXVAR_DETAIL 0
#endif
#ifndef TEXVAR_MATERIAL
#define TEXVAR_MATERIAL 0
#endif
#ifndef TEXVAR_DEBUG_VIEW
#define TEXVAR_DEBUG_VIEW 0 // Wankel::TextureVariationSettings::DebugView
#endif

// Every pattern comes from world position and wraps at u_TexVarPeriod (0 = no wrap), so chunk borders and a
// tiled world's seam never show.
uniform vec3 u_TexVarPeriod;
uniform float u_TexVarMacroScale;
uniform float u_TexVarMacroContrast;
uniform int u_TexVarMacroSeed;
uniform float u_TexVarMaterialScale;
uniform int u_TexVarMaterialSeed;

// pcg3d (Jarzynski & Olano) over a lattice cell plus seed.
uint TexVarHash(ivec3 cell, int seed) {
    uvec3 v = uvec3(cell) * 1664525u + 1013904223u + uint(seed) * 0x9E3779B9u;
    v.x += v.y * v.z;
    v.y += v.z * v.x;
    v.z += v.x * v.y;
    v ^= v >> 16u;
    v.x += v.y * v.z;
    v.y += v.z * v.x;
    v.z += v.x * v.y;
    return v.x ^ v.y ^ v.z;
}

// Lattice spacing along each axis: `scale`, nudged so a whole number of cells fits the world period.
vec3 TexVarCellSize(float scale, vec3 period) {
    vec3 cells = max(floor(period / scale + 0.5), vec3(1.0));
    return mix(vec3(scale), period / cells, step(vec3(1e-4), period));
}

// A lattice cell wrapped into the world period, so tiled copies of the world hash identically.
ivec3 TexVarWrap(ivec3 cell, vec3 size, vec3 period) {
    ivec3 count = ivec3(floor(period / size + 0.5));
    ivec3 wrapped = ivec3(mod(vec3(cell), vec3(max(count, ivec3(1)))));
    return ivec3(count.x > 0 ? wrapped.x : cell.x, count.y > 0 ? wrapped.y : cell.y, count.z > 0 ? wrapped.z : cell.z);
}

float TexVarHash01(ivec3 cell, vec3 size, int seed) {
    return float(TexVarHash(TexVarWrap(cell, size, u_TexVarPeriod), seed) & 0xFFFFu) / 65535.0;
}

// Trilinear value noise in [0,1] at `scale` world units per cell, periodic with the world.
float TexVarValueNoise(vec3 p, float scale, int seed) {
    vec3 size = TexVarCellSize(scale, u_TexVarPeriod);
    vec3 q = p / size;
    ivec3 i = ivec3(floor(q));
    vec3 f = fract(q);
    f = f * f * (3.0 - 2.0 * f);
    float n000 = TexVarHash01(i, size, seed), n100 = TexVarHash01(i + ivec3(1, 0, 0), size, seed);
    float n010 = TexVarHash01(i + ivec3(0, 1, 0), size, seed), n110 = TexVarHash01(i + ivec3(1, 1, 0), size, seed);
    float n001 = TexVarHash01(i + ivec3(0, 0, 1), size, seed), n101 = TexVarHash01(i + ivec3(1, 0, 1), size, seed);
    float n011 = TexVarHash01(i + ivec3(0, 1, 1), size, seed), n111 = TexVarHash01(i + ivec3(1, 1, 1), size, seed);
    return mix(mix(mix(n000, n100, f.x), mix(n010, n110, f.x), f.y), mix(mix(n001, n101, f.x), mix(n011, n111, f.x), f.y),
               f.z);
}

// Two octaves - broad shapes without visible lattice blockiness.
float TexVarFbm(vec3 p, float scale, int seed) {
    return TexVarValueNoise(p, scale, seed) * 0.65 + TexVarValueNoise(p, scale * 0.5, seed + 101) * 0.35;
}

// x = macro pattern, yzw = material noise (albedo, roughness, normal); 0.5 = neutral. Both are many metres across,
// so per-vertex is indistinguishable from per-pixel on terrain - and dozens of hashes cheaper per pixel.
vec4 TexVarVertexNoise(vec3 worldPos) {
    vec4 noise = vec4(0.5);
    if (TEXVAR_ENABLED != 0 && (TEXVAR_MACRO != 0 || TEXVAR_DEBUG_VIEW == 1)) {
        float macro = TexVarFbm(worldPos, u_TexVarMacroScale, u_TexVarMacroSeed);
        noise.x = clamp((macro - 0.5) * u_TexVarMacroContrast + 0.5, 0.0, 1.0);
    }
    if (TEXVAR_ENABLED != 0 && TEXVAR_MATERIAL != 0)
        noise.yzw = vec3(TexVarFbm(worldPos, u_TexVarMaterialScale, u_TexVarMaterialSeed),
                         TexVarFbm(worldPos, u_TexVarMaterialScale, u_TexVarMaterialSeed + 1),
                         TexVarFbm(worldPos, u_TexVarMaterialScale, u_TexVarMaterialSeed + 2));
    return noise;
}
