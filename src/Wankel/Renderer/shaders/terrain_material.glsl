// Triplanar terrain materials (Wankel::TerrainMaterials) - #include from a terrain fragment shader.
#define TERRAIN_MATERIAL_SLOTS 8

struct TerrainSlot {
    vec3 Tint;
    float Roughness;
    float Metallic;
    float Scale;
    float NormalStrength;
    int Active;
};

uniform sampler2DArray u_TerrainAlbedo;
uniform sampler2DArray u_TerrainNormal;
uniform sampler2DArray u_TerrainRoughness;
uniform TerrainSlot u_TerrainSlots[TERRAIN_MATERIAL_SLOTS];
uniform float u_TriplanarSharpness;
uniform float u_TriplanarFaceNormal; // 0 = project along the smooth vertex normal, 1 = along the true triangle normal

struct TerrainSurface {
    vec3 Albedo;
    vec3 Normal;
    float Roughness;
    float Metallic;
};

// Textured vertices bake Color=0 + a one-hot slot weight, so vertexColor + sum(w * texture) is exactly the
// plain vertex-color blend with that material's color swapped for its texture. w0/w1 = slots 0-3 / 4-7.
TerrainSurface BlendTerrainMaterials(vec3 vertexColor, vec3 N, float baseRoughness, float baseMetallic, vec4 w0,
                                     vec4 w1, vec3 worldPos) {
    TerrainSurface surface = TerrainSurface(vertexColor, N, baseRoughness, baseMetallic);

    // Derivatives before any per-fragment branch - they're undefined in divergent control flow.
    vec3 dPdx = dFdx(worldPos);
    vec3 dPdy = dFdy(worldPos);

    float total = dot(w0, vec4(1.0)) + dot(w1, vec4(1.0));
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

    vec3 blend = pow(abs(projN), vec3(u_TriplanarSharpness));
    blend /= max(dot(blend, vec3(1.0)), 1e-5);
    vec3 axisSign = vec3(projN.x < 0.0 ? -1.0 : 1.0, projN.y < 0.0 ? -1.0 : 1.0, projN.z < 0.0 ? -1.0 : 1.0);

    vec3 albedoSum = vec3(0.0);
    vec3 normalSum = vec3(0.0);
    float roughnessSum = 0.0;
    float metallicSum = 0.0;

    for (int i = 0; i < TERRAIN_MATERIAL_SLOTS; i++) {
        float w = i < 4 ? w0[i] : w1[i - 4];
        if (w < 1e-3 || u_TerrainSlots[i].Active == 0)
            continue;

        float layer = float(i);
        vec3 p = worldPos * u_TerrainSlots[i].Scale;
        // Flip u on back-facing planes so the texture isn't mirrored (Golus, "Normal Mapping for a Triplanar Shader").
        vec3 uvX = vec3(p.z * axisSign.x, p.y, layer);
        vec3 uvY = vec3(p.x * axisSign.y, p.z, layer);
        vec3 uvZ = vec3(-p.x * axisSign.z, p.y, layer);

        // Explicit gradients - implicit ones are undefined inside this per-fragment `continue` branch.
        vec3 gx = dPdx * u_TerrainSlots[i].Scale;
        vec3 gy = dPdy * u_TerrainSlots[i].Scale;
        vec2 gxX = vec2(gx.z * axisSign.x, gx.y), gyX = vec2(gy.z * axisSign.x, gy.y);
        vec2 gxY = vec2(gx.x * axisSign.y, gx.z), gyY = vec2(gy.x * axisSign.y, gy.z);
        vec2 gxZ = vec2(-gx.x * axisSign.z, gx.y), gyZ = vec2(-gy.x * axisSign.z, gy.y);

        vec3 albedo = textureGrad(u_TerrainAlbedo, uvX, gxX, gyX).rgb * blend.x +
                      textureGrad(u_TerrainAlbedo, uvY, gxY, gyY).rgb * blend.y +
                      textureGrad(u_TerrainAlbedo, uvZ, gxZ, gyZ).rgb * blend.z;
        float roughness = textureGrad(u_TerrainRoughness, uvX, gxX, gyX).r * blend.x +
                          textureGrad(u_TerrainRoughness, uvY, gxY, gyY).r * blend.y +
                          textureGrad(u_TerrainRoughness, uvZ, gxZ, gyZ).r * blend.z;

        vec3 tnX = textureGrad(u_TerrainNormal, uvX, gxX, gyX).xyz * 2.0 - 1.0;
        vec3 tnY = textureGrad(u_TerrainNormal, uvY, gxY, gyY).xyz * 2.0 - 1.0;
        vec3 tnZ = textureGrad(u_TerrainNormal, uvZ, gxZ, gyZ).xyz * 2.0 - 1.0;
        float strength = u_TerrainSlots[i].NormalStrength;
        tnX.xy *= strength;
        tnY.xy *= strength;
        tnZ.xy *= strength;
        tnX.x *= axisSign.x;
        tnY.x *= axisSign.y;
        tnZ.x *= -axisSign.z;

        // Whiteout blend: per-plane tangent normal swizzled into world space around the geometric normal.
        tnX = vec3(tnX.xy + N.zy, abs(tnX.z) * N.x);
        tnY = vec3(tnY.xy + N.xz, abs(tnY.z) * N.y);
        tnZ = vec3(tnZ.xy + N.xy, abs(tnZ.z) * N.z);
        vec3 normal = normalize(tnX.zyx * blend.x + tnY.xzy * blend.y + tnZ.xyz * blend.z);

        albedoSum += w * albedo * u_TerrainSlots[i].Tint;
        normalSum += w * normal;
        roughnessSum += w * roughness * u_TerrainSlots[i].Roughness;
        metallicSum += w * u_TerrainSlots[i].Metallic;
    }

    surface.Albedo = vertexColor + albedoSum;
    surface.Normal = normalize(N * (1.0 - total) + normalSum);
    surface.Roughness = baseRoughness * (1.0 - total) + roughnessSum;
    surface.Metallic = baseMetallic * (1.0 - total) + metallicSum;
    return surface;
}
