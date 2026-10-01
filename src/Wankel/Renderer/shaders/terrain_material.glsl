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
};

uniform sampler2DArray u_TerrainBaseColor;   // sRGB storage - sampled values are already linear
uniform sampler2DArray u_TerrainNormal;      // linear, OpenGL (+Y) tangent space
uniform sampler2DArray u_TerrainSurface;     // linear: AO, roughness, metallic, height
uniform sampler2DArray u_TerrainOpacityMask; // linear: opacity, mask
uniform sampler2DArray u_TerrainEmissive;    // sRGB storage - sampled values are already linear
uniform TerrainSlot u_TerrainSlots[TERRAIN_MATERIAL_SLOTS];
uniform float u_TriplanarSharpness;
uniform float u_TriplanarFaceNormal; // 0 = project along the smooth vertex normal, 1 = along the true triangle normal

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

// Textured vertices bake Color=0 + a one-hot slot weight, so vertexColor + sum(w * base color) is exactly the
// plain vertex-color blend with that material's color swapped for its texture; every other channel blends
// from its untextured default the same way. weights[k] = slots 4k..4k+3; viewDir = surface to camera, normalized.
TerrainSurface BlendTerrainMaterials(vec3 vertexColor, vec3 N, float baseRoughness, float baseMetallic,
                                     vec4 weights[TERRAIN_WEIGHT_VEC4S], vec3 worldPos, vec3 viewDir) {
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

        vec3 baseColor = slot.BaseColorTint;
        if ((maps & TERRAIN_MAP_BASECOLOR) != 0)
            baseColor *= SampleTriplanar(u_TerrainBaseColor, c, blend).rgb;

        vec4 surf = vec4(1.0, slot.Roughness, slot.Metallic, 0.5); // AO, roughness, metallic, height defaults
        if ((maps & TERRAIN_MAPS_SURFACE) != 0) {
            vec4 t = SampleTriplanar(u_TerrainSurface, c, blend);
            surf = vec4((maps & TERRAIN_MAP_AO) != 0 ? t.r : surf.r, (maps & TERRAIN_MAP_ROUGHNESS) != 0 ? t.g : surf.g,
                        (maps & TERRAIN_MAP_METALLIC) != 0 ? t.b : surf.b, (maps & TERRAIN_MAP_HEIGHT) != 0 ? t.a : surf.a);
        }

        vec2 opacityMask = vec2(slot.Opacity, 0.0);
        if ((maps & TERRAIN_MAPS_OPACITY_MASK) != 0) {
            vec2 t = SampleTriplanar(u_TerrainOpacityMask, c, blend).rg;
            opacityMask = vec2((maps & TERRAIN_MAP_OPACITY) != 0 ? t.r : opacityMask.r,
                               (maps & TERRAIN_MAP_MASK) != 0 ? t.g : opacityMask.g);
        }

        vec3 emissive = slot.Emissive;
        if ((maps & TERRAIN_MAP_EMISSIVE) != 0)
            emissive = SampleTriplanar(u_TerrainEmissive, c, blend).rgb;
        emissive *= slot.EmissiveStrength;

        // Normal: each plane's tangent normal is reoriented into world space around the geometric normal
        // (whiteout blend) BEFORE the planes are blended - never an average of raw normal-map colors.
        vec3 normal = N;
        if ((maps & TERRAIN_MAP_NORMAL) != 0) {
            vec3 tnX = textureGrad(u_TerrainNormal, c.X, c.DxX, c.DyX).xyz * 2.0 - 1.0;
            vec3 tnY = textureGrad(u_TerrainNormal, c.Y, c.DxY, c.DyY).xyz * 2.0 - 1.0;
            vec3 tnZ = textureGrad(u_TerrainNormal, c.Z, c.DxZ, c.DyZ).xyz * 2.0 - 1.0;
            tnX.xy *= slot.NormalStrength;
            tnY.xy *= slot.NormalStrength;
            tnZ.xy *= slot.NormalStrength;
            tnX.x *= axisSign.x;
            tnY.x *= axisSign.y;
            tnZ.x *= -axisSign.z;

            tnX = vec3(tnX.xy + N.zy, abs(tnX.z) * N.x);
            tnY = vec3(tnY.xy + N.xz, abs(tnY.z) * N.y);
            tnZ = vec3(tnZ.xy + N.xy, abs(tnZ.z) * N.z);
            normal = normalize(tnX.zyx * blend.x + tnY.xzy * blend.y + tnZ.xyz * blend.z);
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
    return surface;
}
