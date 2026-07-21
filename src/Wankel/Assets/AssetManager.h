#pragma once

#include <string>
#include <unordered_map>
#include <functional>

#include "Wankel/Core/Base.h"

namespace Wankel {

class Mesh;
class Shader;
class Font;

// Path-keyed cache for engine assets. Every call site used to do its own
// raw MeshLoader::Load/Font::Load/Shader construction with its own
// hardcoded path string and its own (or no) error handling - as more
// assets get added that only gets worse. AssetManager centralizes it: load
// each distinct asset exactly once, hand out a shared Ref<> to every
// caller after that.
//
// GetMesh/GetShader intentionally let a load failure propagate as an
// exception (matches today's actual behavior for both - there's no
// fallback/placeholder mesh or shader, so a missing file is genuinely
// startup-fatal, caught by EntryPoint.h's top-level handler). GetFont
// instead catches, logs, and returns nullptr - matching the one ad hoc
// try/catch that already existed around Font::Load before this, since a
// missing font is meant to degrade gracefully (HUD text just doesn't
// render) rather than take down the whole app.
class AssetManager {
public:
    static Ref<Mesh> GetMesh(const std::string& path);
    static Ref<Shader> GetShader(const std::string& vertexPath, const std::string& fragmentPath);
    static Ref<Font> GetFont(const std::string& ttfPath, float pixelHeight = 48.0f);

    // For meshes that aren't loaded from a file path at all (procedurally built,
    // or derived from another mesh) but still need to live as long as any entity's
    // Mesh* points at them - same cache, same Clear() lifetime, arbitrary key.
    static Ref<Mesh> GetOrCreateMesh(const std::string& key, const std::function<Ref<Mesh>()>& factory);

    // A mirrored variant of an already-loadable mesh (see Mesh::CreateMirrored),
    // cached under a key derived from the source path + mirror flags so repeated
    // requests for the same mirrored mesh share one instance instead of deriving
    // a fresh (and, without this, un-owned) copy each time.
    static Ref<Mesh> GetMirroredMesh(const std::string& sourcePath, bool mirrorX, bool mirrorY, bool mirrorZ);

    // Drops every cached asset (and the GPU resources they own) - call
    // before the GL context goes away.
    static void Clear();

private:
    static std::unordered_map<std::string, Ref<Mesh>> s_Meshes;
    static std::unordered_map<std::string, Ref<Shader>> s_Shaders;
    static std::unordered_map<std::string, Ref<Font>> s_Fonts;
};

} // namespace Wankel
