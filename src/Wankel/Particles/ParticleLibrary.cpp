#include "wkpch.h"
#include "Wankel/Particles/ParticleLibrary.h"

#include <filesystem>
#include <fstream>
#include <unordered_map>

namespace Wankel::ParticleLibrary {

using json = nlohmann::json;

namespace {

std::unordered_map<std::string, Ref<ParticleEffect>> s_Cache;

json Vec(const glm::vec3& v) {
    return json::array({v.x, v.y, v.z});
}
json Vec(const glm::vec4& v) {
    return json::array({v.x, v.y, v.z, v.w});
}

template <typename T>
void Read(const json& j, const char* key, T& out) {
    if (j.contains(key))
        out = j.at(key).get<T>();
}
void Read(const json& j, const char* key, glm::vec3& out) {
    if (j.contains(key) && j.at(key).is_array() && j.at(key).size() == 3)
        out = {j[key][0].get<float>(), j[key][1].get<float>(), j[key][2].get<float>()};
}
void Read(const json& j, const char* key, glm::vec4& out) {
    if (j.contains(key) && j.at(key).is_array() && j.at(key).size() == 4)
        out = {j[key][0].get<float>(), j[key][1].get<float>(), j[key][2].get<float>(), j[key][3].get<float>()};
}

// Enums as readable strings - the file stays meaningful (and diffable) without the C++ header at hand.
template <typename E, size_t N>
const char* EnumName(E value, const char* const (&names)[N]) {
    size_t i = (size_t)value;
    return i < N ? names[i] : names[0];
}
template <typename E, size_t N>
void ReadEnum(const json& j, const char* key, E& out, const char* const (&names)[N]) {
    if (!j.contains(key) || !j.at(key).is_string())
        return;
    std::string name = j.at(key).get<std::string>();
    for (size_t i = 0; i < N; i++)
        if (name == names[i])
            out = (E)i;
}

constexpr const char* kShapeNames[] = {"Point", "Cone", "Sphere", "Box"};
constexpr const char* kBlendNames[] = {"Alpha", "Additive"};
constexpr const char* kOrientationNames[] = {"Billboard", "VelocityAligned", "VelocityStretched"};

// Curves/gradients as [[t, v], ...] / [[t, r, g, b, a], ...] - compact and easy to hand-edit.
json CurveJson(const ParticleCurve& curve) {
    json keys = json::array();
    for (const auto& k : curve.Keys)
        keys.push_back({k.Time, k.Value});
    return keys;
}
void ReadCurve(const json& j, const char* key, ParticleCurve& out) {
    if (!j.contains(key) || !j.at(key).is_array())
        return;
    out.Keys.clear();
    for (const json& k : j.at(key))
        if (k.is_array() && k.size() == 2)
            out.Keys.push_back({k[0].get<float>(), k[1].get<float>()});
}
json GradientJson(const ParticleGradient& gradient) {
    json keys = json::array();
    for (const auto& k : gradient.Keys)
        keys.push_back({k.Time, k.Color.r, k.Color.g, k.Color.b, k.Color.a});
    return keys;
}
void ReadGradient(const json& j, const char* key, ParticleGradient& out) {
    if (!j.contains(key) || !j.at(key).is_array())
        return;
    out.Keys.clear();
    for (const json& k : j.at(key))
        if (k.is_array() && k.size() == 5)
            out.Keys.push_back(
                {k[0].get<float>(), {k[1].get<float>(), k[2].get<float>(), k[3].get<float>(), k[4].get<float>()}});
}

json LayerJson(const ParticleLayer& layer) {
    const ParticleEmission& em = layer.Emission;
    const ParticleMotion& mo = layer.Motion;
    const ParticleAppearance& ap = layer.Appearance;
    const ParticleMaterial& ma = layer.Material;
    const ParticleFlipbook& fb = ma.Flipbook;
    return {
        {"Name", layer.Name},
        {"Enabled", layer.Enabled},
        {"Emission",
         {{"Rate", em.Rate},
          {"BurstCount", em.BurstCount},
          {"StartDelay", em.StartDelay},
          {"Shape", EnumName(em.Shape, kShapeNames)},
          {"ConeAngleDegrees", em.ConeAngleDegrees},
          {"BoxHalfExtents", Vec(em.BoxHalfExtents)},
          {"LocalOffset", Vec(em.LocalOffset)},
          {"LifetimeMin", em.LifetimeMin},
          {"LifetimeMax", em.LifetimeMax},
          {"SpeedMin", em.SpeedMin},
          {"SpeedMax", em.SpeedMax},
          {"InheritVelocity", em.InheritVelocity},
          {"AddedVelocity", Vec(em.AddedVelocity)}}},
        {"Motion",
         {{"Gravity", Vec(mo.Gravity)},
          {"Drag", mo.Drag},
          {"TurbulenceStrength", mo.TurbulenceStrength},
          {"TurbulenceFrequency", mo.TurbulenceFrequency},
          {"TurbulenceScroll", mo.TurbulenceScroll}}},
        {"Appearance",
         {{"SizeMin", ap.SizeMin},
          {"SizeMax", ap.SizeMax},
          {"SizeOverLife", CurveJson(ap.SizeOverLife)},
          {"ColorOverLife", GradientJson(ap.ColorOverLife)},
          {"AlphaOverLife", CurveJson(ap.AlphaOverLife)},
          {"StartRotationJitter", ap.StartRotationJitter},
          {"AngularVelocityMin", ap.AngularVelocityMin},
          {"AngularVelocityMax", ap.AngularVelocityMax},
          {"Orientation", EnumName(ap.Orientation, kOrientationNames)},
          {"Aspect", ap.Aspect},
          {"StretchFactor", ap.StretchFactor}}},
        {"Material",
         {{"TexturePath", ma.TexturePath},
          {"AtlasRect", Vec(ma.AtlasRect)},
          {"Blend", EnumName(ma.Blend, kBlendNames)},
          {"EmissiveStrength", ma.EmissiveStrength},
          {"SoftParticles", ma.SoftParticles},
          {"SoftDistance", ma.SoftDistance},
          {"Erosion", ma.Erosion},
          {"ErosionOverLife", CurveJson(ma.ErosionOverLife)},
          {"ErosionSoftness", ma.ErosionSoftness},
          {"Distortion", ma.Distortion},
          {"DistortionStrength", ma.DistortionStrength},
          {"DistortionScale", ma.DistortionScale},
          {"DistortionScroll", ma.DistortionScroll},
          {"Flipbook",
           {{"Enabled", fb.Enabled},
            {"Columns", fb.Columns},
            {"Rows", fb.Rows},
            {"FrameCount", fb.FrameCount},
            {"FramesPerSecond", fb.FramesPerSecond},
            {"Loop", fb.Loop},
            {"RandomStartFrame", fb.RandomStartFrame}}}}},
    };
}

ParticleLayer LayerFromJson(const json& j) {
    ParticleLayer layer;
    Read(j, "Name", layer.Name);
    Read(j, "Enabled", layer.Enabled);

    if (j.contains("Emission")) {
        const json& e = j["Emission"];
        ParticleEmission& em = layer.Emission;
        Read(e, "Rate", em.Rate);
        Read(e, "BurstCount", em.BurstCount);
        Read(e, "StartDelay", em.StartDelay);
        ReadEnum(e, "Shape", em.Shape, kShapeNames);
        Read(e, "ConeAngleDegrees", em.ConeAngleDegrees);
        Read(e, "BoxHalfExtents", em.BoxHalfExtents);
        Read(e, "LocalOffset", em.LocalOffset);
        Read(e, "LifetimeMin", em.LifetimeMin);
        Read(e, "LifetimeMax", em.LifetimeMax);
        Read(e, "SpeedMin", em.SpeedMin);
        Read(e, "SpeedMax", em.SpeedMax);
        Read(e, "InheritVelocity", em.InheritVelocity);
        Read(e, "AddedVelocity", em.AddedVelocity);
    }
    if (j.contains("Motion")) {
        const json& m = j["Motion"];
        ParticleMotion& mo = layer.Motion;
        Read(m, "Gravity", mo.Gravity);
        Read(m, "Drag", mo.Drag);
        Read(m, "TurbulenceStrength", mo.TurbulenceStrength);
        Read(m, "TurbulenceFrequency", mo.TurbulenceFrequency);
        Read(m, "TurbulenceScroll", mo.TurbulenceScroll);
    }
    if (j.contains("Appearance")) {
        const json& a = j["Appearance"];
        ParticleAppearance& ap = layer.Appearance;
        Read(a, "SizeMin", ap.SizeMin);
        Read(a, "SizeMax", ap.SizeMax);
        ReadCurve(a, "SizeOverLife", ap.SizeOverLife);
        ReadGradient(a, "ColorOverLife", ap.ColorOverLife);
        ReadCurve(a, "AlphaOverLife", ap.AlphaOverLife);
        Read(a, "StartRotationJitter", ap.StartRotationJitter);
        Read(a, "AngularVelocityMin", ap.AngularVelocityMin);
        Read(a, "AngularVelocityMax", ap.AngularVelocityMax);
        ReadEnum(a, "Orientation", ap.Orientation, kOrientationNames);
        Read(a, "Aspect", ap.Aspect);
        Read(a, "StretchFactor", ap.StretchFactor);
    }
    if (j.contains("Material")) {
        const json& m = j["Material"];
        ParticleMaterial& ma = layer.Material;
        Read(m, "TexturePath", ma.TexturePath);
        Read(m, "AtlasRect", ma.AtlasRect);
        ReadEnum(m, "Blend", ma.Blend, kBlendNames);
        Read(m, "EmissiveStrength", ma.EmissiveStrength);
        Read(m, "SoftParticles", ma.SoftParticles);
        Read(m, "SoftDistance", ma.SoftDistance);
        Read(m, "Erosion", ma.Erosion);
        ReadCurve(m, "ErosionOverLife", ma.ErosionOverLife);
        Read(m, "ErosionSoftness", ma.ErosionSoftness);
        Read(m, "Distortion", ma.Distortion);
        Read(m, "DistortionStrength", ma.DistortionStrength);
        Read(m, "DistortionScale", ma.DistortionScale);
        Read(m, "DistortionScroll", ma.DistortionScroll);
        if (m.contains("Flipbook")) {
            const json& f = m["Flipbook"];
            ParticleFlipbook& fb = ma.Flipbook;
            Read(f, "Enabled", fb.Enabled);
            Read(f, "Columns", fb.Columns);
            Read(f, "Rows", fb.Rows);
            Read(f, "FrameCount", fb.FrameCount);
            Read(f, "FramesPerSecond", fb.FramesPerSecond);
            Read(f, "Loop", fb.Loop);
            Read(f, "RandomStartFrame", fb.RandomStartFrame);
        }
    }
    return layer;
}

bool ReadFile(const std::string& path, ParticleEffect& out) {
    std::ifstream file(path);
    if (!file) {
        WK_CORE_ERROR("ParticleLibrary - can't open '{0}'", path);
        return false;
    }
    try {
        out = FromJson(json::parse(file));
    } catch (const std::exception& e) {
        WK_CORE_ERROR("ParticleLibrary - failed to parse '{0}': {1}", path, e.what());
        return false;
    }
    out.SourcePath = path;
    return true;
}

} // namespace

json ToJson(const ParticleEffect& effect) {
    json layers = json::array();
    for (const ParticleLayer& layer : effect.Layers)
        layers.push_back(LayerJson(layer));
    return {{"Name", effect.Name}, {"Duration", effect.Duration}, {"Layers", layers}};
}

ParticleEffect FromJson(const json& j) {
    ParticleEffect effect;
    Read(j, "Name", effect.Name);
    Read(j, "Duration", effect.Duration);
    if (j.contains("Layers") && j["Layers"].is_array())
        for (const json& layer : j["Layers"])
            effect.Layers.push_back(LayerFromJson(layer));
    effect.Validate();
    return effect;
}

Ref<ParticleEffect> Load(const std::string& path) {
    auto it = s_Cache.find(path);
    if (it != s_Cache.end())
        return it->second;

    auto effect = CreateRef<ParticleEffect>();
    if (!ReadFile(path, *effect))
        return nullptr; // not cached, so a file fixed on disk loads next time
    s_Cache[path] = effect;
    return effect;
}

bool Save(const ParticleEffect& effect, const std::string& path) {
    std::error_code ec;
    std::filesystem::path parent = std::filesystem::path(path).parent_path();
    if (!parent.empty())
        std::filesystem::create_directories(parent, ec);

    std::ofstream file(path);
    if (!file) {
        WK_CORE_ERROR("ParticleLibrary - can't write '{0}'", path);
        return false;
    }
    file << ToJson(effect).dump(2) << '\n';
    return (bool)file;
}

bool Reload(const std::string& path) {
    auto it = s_Cache.find(path);
    if (it == s_Cache.end())
        return Load(path) != nullptr;

    ParticleEffect fresh;
    if (!ReadFile(path, fresh))
        return false;
    *it->second = std::move(fresh);
    return true;
}

void Clear() {
    s_Cache.clear();
}

} // namespace Wankel::ParticleLibrary
