#include "wkpch.h"
#include "AssetManager.h"

#include "MeshLoader.h"
#include "Wankel/Renderer/Mesh.h"
#include "Wankel/Renderer/Shader.h"
#include "Wankel/Renderer/Font.h"

namespace Wankel {

std::unordered_map<std::string, Ref<Mesh>> AssetManager::s_Meshes;
std::unordered_map<std::string, Ref<Shader>> AssetManager::s_Shaders;
std::unordered_map<std::string, Ref<Font>> AssetManager::s_Fonts;

Ref<Mesh> AssetManager::GetMesh(const std::string& path) {
    auto it = s_Meshes.find(path);
    if (it != s_Meshes.end())
        return it->second;

    // MeshLoader::Load returns a unique_ptr; release() + wrap is the
    // standard way to hand a uniquely-owned resource over to a shared_ptr
    // without a redundant copy/move-then-copy of the underlying Mesh.
    Ref<Mesh> mesh(MeshLoader::Load(path).release());
    s_Meshes[path] = mesh;
    return mesh;
}

Ref<Shader> AssetManager::GetShader(const std::string& vertexPath, const std::string& fragmentPath) {
    std::string key = vertexPath + "|" + fragmentPath;

    auto it = s_Shaders.find(key);
    if (it != s_Shaders.end())
        return it->second;

    Ref<Shader> shader = CreateRef<Shader>(vertexPath, fragmentPath);
    s_Shaders[key] = shader;
    return shader;
}

Ref<Font> AssetManager::GetFont(const std::string& ttfPath, float pixelHeight) {
    std::string key = ttfPath + "@" + std::to_string(pixelHeight);

    auto it = s_Fonts.find(key);
    if (it != s_Fonts.end())
        return it->second;

    try {
        Ref<Font> font = Font::Load(ttfPath, pixelHeight);
        s_Fonts[key] = font;
        return font;
    } catch (const std::exception& e) {
        WK_CORE_ERROR("AssetManager: failed to load font '{0}': {1}", ttfPath, e.what());
        return nullptr;
    }
}

void AssetManager::Clear() {
    s_Meshes.clear();
    s_Shaders.clear();
    s_Fonts.clear();
}

} // namespace Wankel
