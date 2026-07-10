#include "wkpch.h"
#include "GltfLoader.h"
#include "MeshUtils.h"

#include <stdexcept>

#include <glm/gtc/type_ptr.hpp>
#include <glm/gtc/matrix_inverse.hpp>

#define CGLTF_IMPLEMENTATION
#include <cgltf.h>

namespace Wankel {

namespace {

// Blender's glTF exporter performs the textbook Z-up -> Y-up conversion
// (gltf = (x, z, -y)), but that's a different rotation than the one
// PLYLoader has always used to bring Blender's raw Z-up PLY export into
// this engine's world space (engine = (-y, z, -x)). The two conventions
// differ by a fixed 90-degree yaw. Since every existing PLY asset (ships,
// terrain, enemy body/legs, gun - all already visually tuned) relies on
// the PLY convention, glTF assets are remapped to match it here rather
// than changing PLYLoader's long-established remap or the loaded assets.
glm::vec3 ToEngineSpace(const glm::vec3& gltf) {
    return {gltf.z, gltf.y, -gltf.x};
}

void ReadNodeMesh(const cgltf_node* node, std::vector<Vertex>& outVertices, std::vector<uint32_t>& outIndices,
                   bool& anyMissingNormals) {
    if (node->mesh) {
        float worldMatArr[16];
        cgltf_node_transform_world(node, worldMatArr);
        glm::mat4 worldMat = glm::make_mat4(worldMatArr);
        glm::mat3 normalMat = glm::inverseTranspose(glm::mat3(worldMat));

        for (cgltf_size p = 0; p < node->mesh->primitives_count; p++) {
            const cgltf_primitive& prim = node->mesh->primitives[p];

            if (prim.type != cgltf_primitive_type_triangles) {
                WK_CORE_WARNING("GltfLoader: skipping non-triangle primitive in mesh '{0}'",
                                 node->mesh->name ? node->mesh->name : "(unnamed)");
                continue;
            }

            const cgltf_accessor* posAccessor = nullptr;
            const cgltf_accessor* normAccessor = nullptr;
            const cgltf_accessor* colorAccessor = nullptr;

            for (cgltf_size a = 0; a < prim.attributes_count; a++) {
                const cgltf_attribute& attr = prim.attributes[a];

                if (attr.type == cgltf_attribute_type_position)
                    posAccessor = attr.data;
                else if (attr.type == cgltf_attribute_type_normal)
                    normAccessor = attr.data;
                else if (attr.type == cgltf_attribute_type_color)
                    colorAccessor = attr.data;
            }

            if (!posAccessor) {
                WK_CORE_WARNING("GltfLoader: primitive with no POSITION attribute, skipping");
                continue;
            }

            // No texture/material system exists yet, so a primitive's flat
            // material base color factor is the closest equivalent to a
            // "look" this engine's vertex-colored renderer can use - same
            // fallback a real engine takes before it has textures wired up.
            glm::vec4 fallbackColor(1.0f);
            if (prim.material && prim.material->has_pbr_metallic_roughness) {
                const float* c = prim.material->pbr_metallic_roughness.base_color_factor;
                fallbackColor = glm::vec4(c[0], c[1], c[2], c[3]);
            }

            uint32_t baseVertex = (uint32_t)outVertices.size();
            size_t vertexCount = posAccessor->count;

            for (size_t i = 0; i < vertexCount; i++) {
                Vertex v;

                float pos[3] = {0.0f, 0.0f, 0.0f};
                cgltf_accessor_read_float(posAccessor, i, pos, 3);
                glm::vec3 worldPos = glm::vec3(worldMat * glm::vec4(pos[0], pos[1], pos[2], 1.0f));
                v.Position = ToEngineSpace(worldPos);

                if (normAccessor) {
                    float nrm[3] = {0.0f, 1.0f, 0.0f};
                    cgltf_accessor_read_float(normAccessor, i, nrm, 3);
                    glm::vec3 worldNormal = normalMat * glm::vec3(nrm[0], nrm[1], nrm[2]);
                    v.Normal = glm::normalize(ToEngineSpace(worldNormal));
                } else {
                    anyMissingNormals = true;
                }

                if (colorAccessor) {
                    bool isVec3 = colorAccessor->type == cgltf_type_vec3;
                    float col[4] = {1.0f, 1.0f, 1.0f, 1.0f};
                    cgltf_accessor_read_float(colorAccessor, i, col, isVec3 ? 3 : 4);
                    v.Color = glm::vec4(col[0], col[1], col[2], isVec3 ? 1.0f : col[3]);
                } else {
                    v.Color = fallbackColor;
                }

                outVertices.push_back(v);
            }

            if (prim.indices) {
                size_t indexCount = prim.indices->count;
                for (size_t i = 0; i < indexCount; i++)
                    outIndices.push_back(baseVertex + (uint32_t)cgltf_accessor_read_index(prim.indices, i));
            } else {
                // Non-indexed primitive - emit sequential indices.
                for (size_t i = 0; i < vertexCount; i++)
                    outIndices.push_back(baseVertex + (uint32_t)i);
            }
        }
    }

    for (cgltf_size c = 0; c < node->children_count; c++)
        ReadNodeMesh(node->children[c], outVertices, outIndices, anyMissingNormals);
}

} // namespace

void GltfLoader::Load(const std::string& path, std::vector<Vertex>& outVertices, std::vector<uint32_t>& outIndices) {
    cgltf_options options = {};
    cgltf_data* data = nullptr;

    cgltf_result result = cgltf_parse_file(&options, path.c_str(), &data);
    if (result != cgltf_result_success)
        throw std::runtime_error("GltfLoader: failed to parse '" + path + "' (cgltf error " +
                                  std::to_string((int)result) + ")");

    result = cgltf_load_buffers(&options, data, path.c_str());
    if (result != cgltf_result_success) {
        cgltf_free(data);
        throw std::runtime_error("GltfLoader: failed to load buffers for '" + path + "'");
    }

    outVertices.clear();
    outIndices.clear();

    bool anyMissingNormals = false;

    if (data->scenes_count > 0) {
        const cgltf_scene* scene = data->scene ? data->scene : &data->scenes[0];
        for (cgltf_size i = 0; i < scene->nodes_count; i++)
            ReadNodeMesh(scene->nodes[i], outVertices, outIndices, anyMissingNormals);
    } else {
        // No scene defined - fall back to every root node (ReadNodeMesh
        // already recurses into children, so only start from roots here or
        // child nodes would be visited twice).
        for (cgltf_size i = 0; i < data->nodes_count; i++) {
            if (!data->nodes[i].parent)
                ReadNodeMesh(&data->nodes[i], outVertices, outIndices, anyMissingNormals);
        }
    }

    cgltf_free(data);

    if (outVertices.empty())
        throw std::runtime_error("GltfLoader: '" + path + "' contained no triangle mesh data");

    if (anyMissingNormals)
        ComputeSmoothNormals(outVertices, outIndices);

    WK_CORE_INFO("GltfLoader: loaded '{0}' ({1} vertices, {2} indices)", path, outVertices.size(), outIndices.size());
}

} // namespace Wankel
