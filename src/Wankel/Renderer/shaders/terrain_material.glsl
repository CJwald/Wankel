// Triplanar PBR terrain materials (Wankel::TerrainMaterials) - #include from a terrain fragment shader.
#define TERRAIN_MATERIAL_SLOTS 16
#define TERRAIN_WEIGHT_VEC4S 4 // per-vertex slot weights, 4 slots per vec4 (Wankel::kTerrainMaterialWeightVec4s)

// Must match Wankel::TerrainMap.
#define TERRAIN_MAP_BASECOLOR 1
#define TERRAIN_MAP_NORMAL 2
#define TERRAIN_MAP_ROUGHNESS 4
#define TERRAIN_MAP_METALLIC 8
#define TERRAIN_MAP_HEIGHT 16
#define TERRAIN_MAP_AO 32
#define TERRAIN_MAP_EMISSIVE 64
#define TERRAIN_MAP_OPACITY 128
#define TERRAIN_MAP_MASK 256
#define TERRAIN_MAPS_SURFACE (TERRAIN_MAP_AO | TERRAIN_MAP_ROUGHNESS | TERRAIN_MAP_METALLIC | TERRAIN_MAP_HEIGHT)
#define TERRAIN_MAPS_OPACITY_MASK (TERRAIN_MAP_OPACITY | TERRAIN_MAP_MASK)

// Each param is the channel's value when its map is missing; BaseColorTint and EmissiveStrength also scale maps.
struct TerrainSlot {
    vec3 BaseColorTint;
    float Roughness;
    float Metallic;
    vec3 Emissive;
    float EmissiveStrength;
    float Opacity;
    float Scale;
    float NormalStrength;
    float HeightScale;
    int Maps; // TERRAIN_MAP_* bits of the maps that loaded - only those are sampled
    int Active;
    int Layer; // texture array layer - its own slot index, or the source slot's when shared
    int VariantBase;  // first texture-variant layer (variants 1..VariantCount-1)
    int VariantCount; // 1 = just the main maps
};

uniform sampler2DArray u_TerrainBaseColor;   // sRGB storage - sampled values are already linear
uniform sampler2DArray u_TerrainNormal;      // linear, OpenGL (+Y) tangent space
uniform sampler2DArray u_TerrainSurface;     // linear: AO, roughness, metallic, height
uniform sampler2DArray u_TerrainOpacityMask; // linear: opacity, mask
uniform sampler2DArray u_TerrainEmissive;    // sRGB storage - sampled values are already linear
uniform TerrainSlot u_TerrainSlots[TERRAIN_MATERIAL_SLOTS];
uniform float u_TriplanarSharpness;
uniform float u_TriplanarFaceNormal; // 0 = project along the smooth vertex normal, 1 = along the true triangle normal

#include "terrain_variation.glsl"
uniform float u_TexVarTransformScale;
uniform float u_TexVarEdgeBlend;
uniform int u_TexVarRotation;
uniform int u_TexVarMirror;
uniform int u_TexVarTransformSeed;
uniform int u_TexVarVariantCount;
uniform float u_TexVarVariantScale;
uniform int u_TexVarVariantSeed;
uniform float u_TexVarMacroStrength;
uniform float u_TexVarDetailScale;
uniform float u_TexVarDetailStrength;
uniform int u_TexVarDetailChannels; // 1 = base color, 2 = normal, 4 = roughness
uniform float u_TexVarMaterialAlbedo;
uniform float u_TexVarMaterialRoughness;
uniform float u_TexVarMaterialNormal;

struct TerrainSurface {
    vec3 BaseColor;
    vec3 Normal;
    float Roughness;
    float Metallic;
    float AO;
    vec3 Emissive;
    float Opacity;
    float Mask;   // generic procedural control mask - exposed for effects, not consumed here
    float Height; // 0.5 = surface level
};

// The three planar projections for one slot: xy = uv, z = array layer, plus explicit gradients (implicit
// ones are undefined inside the per-slot branch below). Every channel samples through the same coords,
// so all maps line up.
struct TriplanarCoords {
    vec3 X, Y, Z;
    vec2 DxX, DyX, DxY, DyY, DxZ, DyZ;
};

vec4 SampleTriplanar(sampler2DArray tex, TriplanarCoords c, vec3 blend) {
    return textureGrad(tex, c.X, c.DxX, c.DyX) * blend.x + textureGrad(tex, c.Y, c.DxY, c.DyY) * blend.y +
           textureGrad(tex, c.Z, c.DxZ, c.DyZ) * blend.z;
}

// ---- Variation helpers ----

// One of the 8 rotate/mirror transforms (bit 2 = mirror u, bits 0-1 = quarter turns), applied to a uv/gradient.
vec2 TexVarApply(int id, vec2 v) {
    if (id >= 4)
        v.x = -v.x;
    int r = id & 3;
    return r == 0 ? v : r == 1 ? vec2(-v.y, v.x) : r == 2 ? -v : vec2(v.y, -v.x);
}

// Inverse of TexVarApply - takes a tangent-space normal sampled in transformed texture space back to the plane's.
vec2 TexVarApplyInverse(int id, vec2 v) {
    int r = id & 3;
    v = r == 0 ? v : r == 1 ? vec2(v.y, -v.x) : r == 2 ? -v : vec2(-v.y, v.x);
    if (id >= 4)
        v.x = -v.x;
    return v;
}

int TexVarTransformId(uint h) {
    if (u_TexVarRotation != 0 && u_TexVarMirror != 0)
        return int(h & 7u);
    if (u_TexVarRotation != 0)
        return int(h & 3u);
    int mirrorOnly[4] = int[4](0, 4, 2, 6); // identity, mirror u, mirror v, both
    return mirrorOnly[int(h & 3u)];
}

// One plane's samples: up to four cells (near a cell edge they blend), each with its own transform.
struct PlaneTaps {
    vec3 UV[4];
    vec2 Dx[4], Dy[4];
    float W[4]; // 0 = tap unused; fixed slots (no runtime indexing) keep the arrays in registers on Intel
    int Id[4];
    uint CellHash; // the fragment's own cell, for the debug views
};

// q = this plane's two world coordinates (cell lattice), uv/dx/dy = its texture coords and gradients, period2 = the
// world period along q's two axes, planeId keeps the three planes' patterns independent.
PlaneTaps BuildPlaneTaps(vec2 q, vec3 uv, vec2 dx, vec2 dy, vec2 period2, int planeId) {
    PlaneTaps t;
    for (int k = 0; k < 4; k++) {
        t.UV[k] = uv;
        t.Dx[k] = dx;
        t.Dy[k] = dy;
        t.W[k] = 0.0;
        t.Id[k] = 0;
    }
    vec3 period = vec3(period2, 0.0);
    vec3 size = TexVarCellSize(u_TexVarTransformScale, period);
    vec2 cellF = q / size.xy;
    ivec2 cell = ivec2(floor(cellF));
    vec2 f = fract(cellF);
    int seed = u_TexVarTransformSeed + planeId * 7919;
    t.CellHash = TexVarHash(TexVarWrap(ivec3(cell, 0), size, period), seed);

    if (TEXVAR_TRANSFORM == 0) {
        t.W[0] = 1.0;
        return t;
    }

    // Weight toward the neighbouring cell across the nearer edge, 0.5 right at the edge, 0 past the band.
    float band = u_TexVarEdgeBlend;
    vec2 toward = vec2(f.x < 0.5 ? -1.0 : 1.0, f.y < 0.5 ? -1.0 : 1.0);
    vec2 edgeDist = min(f, 1.0 - f);
    vec2 n = band > 0.0 ? 0.5 * (1.0 - smoothstep(vec2(0.0), vec2(band), edgeDist)) : vec2(0.0);

    for (int k = 0; k < 4; k++) {
        ivec2 offset = ivec2(k & 1, k >> 1);
        float w = (offset.x == 1 ? n.x : 1.0 - n.x) * (offset.y == 1 ? n.y : 1.0 - n.y);
        if (w < 1e-4)
            continue;
        ivec2 c = cell + offset * ivec2(toward);
        int id = TexVarTransformId(TexVarHash(TexVarWrap(ivec3(c, 0), size, period), seed));
        t.UV[k] = vec3(TexVarApply(id, uv.xy), uv.z);
        t.Dx[k] = TexVarApply(id, dx);
        t.Dy[k] = TexVarApply(id, dy);
        t.W[k] = w;
        t.Id[k] = id;
    }
    return t;
}

vec4 SamplePlaneTaps(sampler2DArray tex, PlaneTaps t) {
    vec4 sum = vec4(0.0);
    for (int k = 0; k < 4; k++)
        if (t.W[k] > 0.0)
            sum += textureGrad(tex, t.UV[k], t.Dx[k], t.Dy[k]) * t.W[k];
    return sum;
}

// A plane's tangent normal, each tap's sample turned back from its transform before blending.
vec3 SamplePlaneTapsNormal(PlaneTaps t, float strength) {
    vec3 sum = vec3(0.0);
    for (int k = 0; k < 4; k++) {
        if (t.W[k] <= 0.0)
            continue;
        vec3 tn = textureGrad(u_TerrainNormal, t.UV[k], t.Dx[k], t.Dy[k]).xyz * 2.0 - 1.0;
        tn.xy = TexVarApplyInverse(t.Id[k], tn.xy) * strength;
        sum += tn * t.W[k];
    }
    return sum;
}

vec4 SampleTriplanarTaps(sampler2DArray tex, PlaneTaps tx, PlaneTaps ty, PlaneTaps tz, vec3 blend) {
    vec4 sum = vec4(0.0);
    if (blend.x > 0.0)
        sum += SamplePlaneTaps(tex, tx) * blend.x;
    if (blend.y > 0.0)
        sum += SamplePlaneTaps(tex, ty) * blend.y;
    if (blend.z > 0.0)
        sum += SamplePlaneTaps(tex, tz) * blend.z;
    return sum;
}

vec3 TexVarDebugColor(uint h) {
    return vec3(float(h & 255u), float((h >> 8u) & 255u), float((h >> 16u) & 255u)) / 255.0 * 0.8 + 0.2;
}

// Textured vertices bake Color=0 + a one-hot slot weight, so vertexColor + sum(w * base color) is exactly the
// plain vertex-color blend with that material's color swapped for its texture; every other channel blends
// from its untextured default the same way. weights[k] = slots 4k..4k+3; viewDir = surface to camera, normalized;
// variationNoise = the interpolated TexVarVertexNoise of the vertex shader.
TerrainSurface BlendTerrainMaterials(vec3 vertexColor, vec3 N, float baseRoughness, float baseMetallic,
                                     vec4 weights[TERRAIN_WEIGHT_VEC4S], vec3 worldPos, vec3 viewDir,
                                     vec4 variationNoise) {
    TerrainSurface surface = TerrainSurface(vertexColor, N, baseRoughness, baseMetallic, 1.0, vec3(0.0), 1.0, 0.0, 0.5);

    // Derivatives before any per-fragment branch - they're undefined in divergent control flow.
    vec3 dPdx = dFdx(worldPos);
    vec3 dPdy = dFdy(worldPos);

    float total = 0.0;
    for (int k = 0; k < TERRAIN_WEIGHT_VEC4S; k++)
        total += dot(weights[k], vec4(1.0));
    if (total < 1e-3)
        return surface;
    total = min(total, 1.0);

    // Smoothed MC normals tilt away from the real surface near edges; projecting along them puts weight on
    // planes the surface is nearly edge-on to, which smears the texture. The triangle normal doesn't.
    vec3 faceN = cross(dPdx, dPdy);
    float faceLen = length(faceN);
    faceN = faceLen > 1e-12 ? faceN / faceLen : N;
    faceN *= dot(faceN, N) < 0.0 ? -1.0 : 1.0;
    vec3 projN = normalize(mix(N, faceN, clamp(u_TriplanarFaceNormal, 0.0, 1.0)));

    // |normal| components, sharpened then normalized to sum to 1 (sharpness 1 = the plain weights).
    vec3 blend = pow(abs(projN), vec3(u_TriplanarSharpness));
    blend /= max(dot(blend, vec3(1.0)), 1e-5);
    vec3 axisSign = vec3(projN.x < 0.0 ? -1.0 : 1.0, projN.y < 0.0 ? -1.0 : 1.0, projN.z < 0.0 ? -1.0 : 1.0);

    // Fragment-wide variation terms, shared by every slot.
    const bool variation = TEXVAR_ENABLED != 0;
    float macro = variationNoise.x;
    vec3 materialNoise = variationNoise.yzw;
    // Debug view inputs from the most heavily weighted slot's dominant plane.
    float debugWeight = -1.0;
    uint debugCell = 0u;
    int debugId = 0;
    int debugVariant = 0;
    int dominantPlane = blend.x >= blend.y && blend.x >= blend.z ? 0 : blend.y >= blend.z ? 1 : 2;

    vec3 baseColorSum = vec3(0.0);
    vec3 normalSum = vec3(0.0);
    vec3 emissiveSum = vec3(0.0);
    float roughnessSum = 0.0, metallicSum = 0.0, aoSum = 0.0, heightSum = 0.0, opacitySum = 0.0, maskSum = 0.0;

    for (int i = 0; i < TERRAIN_MATERIAL_SLOTS; i++) {
        float w = weights[i / 4][i % 4];
        if (w < 1e-3 || u_TerrainSlots[i].Active == 0)
            continue;

        TerrainSlot slot = u_TerrainSlots[i];
        int maps = slot.Maps;
        float layer = float(slot.Layer);
        vec3 p = worldPos * slot.Scale;
        vec3 gx = dPdx * slot.Scale;
        vec3 gy = dPdy * slot.Scale;

        // Flip u on back-facing planes so the texture isn't mirrored (Golus, "Normal Mapping for a Triplanar Shader").
        TriplanarCoords c;
        c.X = vec3(p.z * axisSign.x, p.y, layer);
        c.Y = vec3(p.x * axisSign.y, p.z, layer);
        c.Z = vec3(-p.x * axisSign.z, p.y, layer);
        c.DxX = vec2(gx.z * axisSign.x, gx.y);
        c.DyX = vec2(gy.z * axisSign.x, gy.y);
        c.DxY = vec2(gx.x * axisSign.y, gx.z);
        c.DyY = vec2(gy.x * axisSign.y, gy.z);
        c.DxZ = vec2(-gx.x * axisSign.z, gx.y);
        c.DyZ = vec2(-gy.x * axisSign.z, gy.y);

        // Height: per-plane parallax offset (offset-limited, no mesh displacement) - each plane shifts its uv
        // along the view direction expressed in that plane's own u/v axes.
        if ((maps & TERRAIN_MAP_HEIGHT) != 0 && slot.HeightScale > 0.0) {
            float depth = slot.HeightScale * slot.Scale;
            float hX = textureGrad(u_TerrainSurface, c.X, c.DxX, c.DyX).a;
            float hY = textureGrad(u_TerrainSurface, c.Y, c.DxY, c.DyY).a;
            float hZ = textureGrad(u_TerrainSurface, c.Z, c.DxZ, c.DyZ).a;
            c.X.xy += vec2(viewDir.z * axisSign.x, viewDir.y) * (hX - 0.5) * depth;
            c.Y.xy += vec2(viewDir.x * axisSign.y, viewDir.z) * (hY - 0.5) * depth;
            c.Z.xy += vec2(-viewDir.x * axisSign.z, viewDir.y) * (hZ - 0.5) * depth;
        }

        // Per-cell transforms and/or texture variants switch to per-plane sample taps; otherwise the plain path.
        int variantCount = min(u_TexVarVariantCount, slot.VariantCount);
        bool useVariants = variation && TEXVAR_VARIANTS != 0 && variantCount > 1;
        bool useTaps = variation && (TEXVAR_TRANSFORM != 0 || useVariants);
        PlaneTaps tx, ty, tz;
        int planeVariant[3] = int[3](0, 0, 0);
        if (useTaps) {
            vec3 period = u_TexVarPeriod;
            vec2 qs[3] = vec2[3](worldPos.zy, worldPos.xz, worldPos.xy);
            vec2 periods[3] = vec2[3](period.zy, period.xz, period.xy);
            if (useVariants) {
                for (int pl = 0; pl < 3; pl++) {
                    vec3 size = TexVarCellSize(u_TexVarVariantScale, vec3(periods[pl], 0.0));
                    ivec3 cell = ivec3(ivec2(floor(qs[pl] / size.xy)), pl);
                    uint h = TexVarHash(TexVarWrap(cell, size, vec3(periods[pl], 0.0)), u_TexVarVariantSeed);
                    planeVariant[pl] = int(h % uint(variantCount));
                }
            }
            vec3 layers = vec3(planeVariant[0] == 0 ? layer : float(slot.VariantBase + planeVariant[0] - 1),
                               planeVariant[1] == 0 ? layer : float(slot.VariantBase + planeVariant[1] - 1),
                               planeVariant[2] == 0 ? layer : float(slot.VariantBase + planeVariant[2] - 1));
            tx = BuildPlaneTaps(qs[0], vec3(c.X.xy, layers.x), c.DxX, c.DyX, periods[0], 0);
            ty = BuildPlaneTaps(qs[1], vec3(c.Y.xy, layers.y), c.DxY, c.DyY, periods[1], 1);
            tz = BuildPlaneTaps(qs[2], vec3(c.Z.xy, layers.z), c.DxZ, c.DyZ, periods[2], 2);
        }

        vec3 baseColor = slot.BaseColorTint;
        if ((maps & TERRAIN_MAP_BASECOLOR) != 0)
            baseColor *= useTaps ? SampleTriplanarTaps(u_TerrainBaseColor, tx, ty, tz, blend).rgb
                                 : SampleTriplanar(u_TerrainBaseColor, c, blend).rgb;

        vec4 surf = vec4(1.0, slot.Roughness, slot.Metallic, 0.5); // AO, roughness, metallic, height defaults
        if ((maps & TERRAIN_MAPS_SURFACE) != 0) {
            vec4 t = useTaps ? SampleTriplanarTaps(u_TerrainSurface, tx, ty, tz, blend)
                             : SampleTriplanar(u_TerrainSurface, c, blend);
            surf = vec4((maps & TERRAIN_MAP_AO) != 0 ? t.r : surf.r, (maps & TERRAIN_MAP_ROUGHNESS) != 0 ? t.g : surf.g,
                        (maps & TERRAIN_MAP_METALLIC) != 0 ? t.b : surf.b, (maps & TERRAIN_MAP_HEIGHT) != 0 ? t.a : surf.a);
        }

        vec2 opacityMask = vec2(slot.Opacity, 0.0);
        if ((maps & TERRAIN_MAPS_OPACITY_MASK) != 0) {
            vec2 t = useTaps ? SampleTriplanarTaps(u_TerrainOpacityMask, tx, ty, tz, blend).rg
                             : SampleTriplanar(u_TerrainOpacityMask, c, blend).rg;
            opacityMask = vec2((maps & TERRAIN_MAP_OPACITY) != 0 ? t.r : opacityMask.r,
                               (maps & TERRAIN_MAP_MASK) != 0 ? t.g : opacityMask.g);
        }

        vec3 emissive = slot.Emissive;
        if ((maps & TERRAIN_MAP_EMISSIVE) != 0)
            emissive = useTaps ? SampleTriplanarTaps(u_TerrainEmissive, tx, ty, tz, blend).rgb
                               : SampleTriplanar(u_TerrainEmissive, c, blend).rgb;
        emissive *= slot.EmissiveStrength;

        float normalStrength = slot.NormalStrength;
        if (variation && TEXVAR_MATERIAL != 0)
            normalStrength *= mix(1.0 - u_TexVarMaterialNormal, 1.0 + u_TexVarMaterialNormal, materialNoise.z);

        // Multi-scale detail: the slot's own maps again at another scale, so channels don't all repeat together.
        bool useDetail = variation && TEXVAR_DETAIL != 0;
        TriplanarCoords d;
        if (useDetail) {
            float k = u_TexVarDetailScale / max(slot.Scale, 1e-4);
            d = c;
            d.X.xy *= k;
            d.Y.xy *= k;
            d.Z.xy *= k;
            d.DxX *= k;
            d.DyX *= k;
            d.DxY *= k;
            d.DyY *= k;
            d.DxZ *= k;
            d.DyZ *= k;
            if ((u_TexVarDetailChannels & 1) != 0 && (maps & TERRAIN_MAP_BASECOLOR) != 0) {
                vec3 detail = SampleTriplanar(u_TerrainBaseColor, d, blend).rgb;
                vec3 average = textureLod(u_TerrainBaseColor, vec3(0.5, 0.5, layer), 16.0).rgb; // top mip = mean
                float ratio = dot(detail, vec3(0.299, 0.587, 0.114)) /
                              max(dot(average, vec3(0.299, 0.587, 0.114)), 1e-3);
                baseColor *= mix(1.0, ratio, u_TexVarDetailStrength);
            }
            if ((u_TexVarDetailChannels & 4) != 0 && (maps & TERRAIN_MAP_ROUGHNESS) != 0) {
                float detail = SampleTriplanar(u_TerrainSurface, d, blend).g;
                float average = textureLod(u_TerrainSurface, vec3(0.5, 0.5, layer), 16.0).g;
                surf.g *= mix(1.0, detail / max(average, 1e-3), u_TexVarDetailStrength);
            }
        }

        // Large-scale and low-frequency variation of the final channels.
        if (variation && TEXVAR_MACRO != 0)
            baseColor *= mix(1.0 - u_TexVarMacroStrength, 1.0 + u_TexVarMacroStrength, macro);
        if (variation && TEXVAR_MATERIAL != 0) {
            baseColor *= mix(1.0 - u_TexVarMaterialAlbedo, 1.0 + u_TexVarMaterialAlbedo, materialNoise.x);
            surf.g = clamp(surf.g * mix(1.0 - u_TexVarMaterialRoughness, 1.0 + u_TexVarMaterialRoughness, materialNoise.y),
                           0.0, 1.0);
        }

        // Normal: each plane's tangent normal is reoriented into world space around the geometric normal
        // (whiteout blend) BEFORE the planes are blended - never an average of raw normal-map colors.
        vec3 normal = N;
        if ((maps & TERRAIN_MAP_NORMAL) != 0) {
            vec3 tnX, tnY, tnZ;
            if (useTaps) {
                tnX = SamplePlaneTapsNormal(tx, normalStrength);
                tnY = SamplePlaneTapsNormal(ty, normalStrength);
                tnZ = SamplePlaneTapsNormal(tz, normalStrength);
            } else {
                tnX = textureGrad(u_TerrainNormal, c.X, c.DxX, c.DyX).xyz * 2.0 - 1.0;
                tnY = textureGrad(u_TerrainNormal, c.Y, c.DxY, c.DyY).xyz * 2.0 - 1.0;
                tnZ = textureGrad(u_TerrainNormal, c.Z, c.DxZ, c.DyZ).xyz * 2.0 - 1.0;
                tnX.xy *= normalStrength;
                tnY.xy *= normalStrength;
                tnZ.xy *= normalStrength;
            }
            if (useDetail && (u_TexVarDetailChannels & 2) != 0) {
                float s = u_TexVarDetailStrength * normalStrength;
                vec3 dX = textureGrad(u_TerrainNormal, d.X, d.DxX, d.DyX).xyz * 2.0 - 1.0;
                vec3 dY = textureGrad(u_TerrainNormal, d.Y, d.DxY, d.DyY).xyz * 2.0 - 1.0;
                vec3 dZ = textureGrad(u_TerrainNormal, d.Z, d.DxZ, d.DyZ).xyz * 2.0 - 1.0;
                tnX = vec3(tnX.xy + dX.xy * s, tnX.z);
                tnY = vec3(tnY.xy + dY.xy * s, tnY.z);
                tnZ = vec3(tnZ.xy + dZ.xy * s, tnZ.z);
            }
            tnX.x *= axisSign.x;
            tnY.x *= axisSign.y;
            tnZ.x *= -axisSign.z;

            tnX = vec3(tnX.xy + N.zy, abs(tnX.z) * N.x);
            tnY = vec3(tnY.xy + N.xz, abs(tnY.z) * N.y);
            tnZ = vec3(tnZ.xy + N.xy, abs(tnZ.z) * N.z);
            normal = normalize(tnX.zyx * blend.x + tnY.xzy * blend.y + tnZ.xyz * blend.z);
        }

        if (variation && TEXVAR_DEBUG_VIEW != 0 && w > debugWeight) {
            debugWeight = w;
            if (useTaps) {
                debugCell = dominantPlane == 0 ? tx.CellHash : dominantPlane == 1 ? ty.CellHash : tz.CellHash;
                debugId = dominantPlane == 0 ? tx.Id[0] : dominantPlane == 1 ? ty.Id[0] : tz.Id[0];
            }
            debugVariant = planeVariant[dominantPlane];
        }

        baseColorSum += w * baseColor;
        normalSum += w * normal;
        emissiveSum += w * emissive;
        aoSum += w * surf.r;
        roughnessSum += w * surf.g;
        metallicSum += w * surf.b;
        heightSum += w * surf.a;
        opacitySum += w * opacityMask.r;
        maskSum += w * opacityMask.g;
    }

    float rest = 1.0 - total; // the untextured share of this fragment keeps its defaults
    surface.BaseColor = vertexColor + baseColorSum;
    surface.Normal = normalize(N * rest + normalSum);
    surface.Roughness = baseRoughness * rest + roughnessSum;
    surface.Metallic = baseMetallic * rest + metallicSum;
    surface.AO = rest + aoSum;
    surface.Emissive = emissiveSum;
    surface.Opacity = rest + opacitySum;
    surface.Mask = maskSum;
    surface.Height = 0.5 * rest + heightSum;

    // Debug views: a flat, unlit color so the pattern reads regardless of lighting.
    if (variation && TEXVAR_DEBUG_VIEW != 0) {
        vec3 color = TEXVAR_DEBUG_VIEW == 1   ? vec3(macro)
                     : TEXVAR_DEBUG_VIEW == 2 ? TexVarDebugColor(debugCell)
                     : TEXVAR_DEBUG_VIEW == 3 ? TexVarDebugColor(uint(debugVariant + 1) * 2654435761u)
                                              : TexVarDebugColor(uint(debugId + 1) * 2654435761u);
        surface.BaseColor = vec3(0.0);
        surface.Emissive = color;
        surface.Normal = N;
        surface.AO = 1.0;
    }
    return surface;
}
