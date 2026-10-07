#include "wkpch.h"
#include "TerrainVariationPanel.h"

#include "Wankel/Renderer/TerrainMaterial.h"

#include <imgui.h>

#include <string>

namespace Wankel {

namespace {

enum class Compare { Off, Transform, Variants, Macro, Detail, Material, All, Custom };
constexpr const char* kCompareLabels[] = {"Off", "Transform", "Variants", "Macro", "Detail", "Material", "All", "Custom"};

Compare CurrentCompare(const TextureVariationSettings& s) {
    if (!s.Enabled)
        return Compare::Off;
    bool t = s.Transform.Enabled, v = s.Variants.Enabled, m = s.Macro.Enabled, d = s.Detail.Enabled,
         mat = s.Material.Enabled;
    if (t && v && m && d && mat)
        return Compare::All;
    int on = (int)t + (int)v + (int)m + (int)d + (int)mat;
    if (on != 1)
        return Compare::Custom;
    return t ? Compare::Transform : v ? Compare::Variants : m ? Compare::Macro : d ? Compare::Detail : Compare::Material;
}

// Sets the master and per-technique toggles for one comparison mode, leaving every tuning value alone.
void ApplyCompare(TextureVariationSettings& s, Compare mode) {
    s.Enabled = mode != Compare::Off;
    bool all = mode == Compare::All;
    s.Transform.Enabled = all || mode == Compare::Transform;
    s.Variants.Enabled = all || mode == Compare::Variants;
    s.Macro.Enabled = all || mode == Compare::Macro;
    s.Detail.Enabled = all || mode == Compare::Detail;
    s.Material.Enabled = all || mode == Compare::Material;
}

void SeedField(const char* label, uint32_t& seed) {
    int value = (int)seed;
    if (ImGui::InputInt(label, &value))
        seed = (uint32_t)value;
}

} // namespace

void TerrainVariationPanel::Draw() {
    TextureVariationSettings& s = TerrainMaterials::GetVariationSettings();

    ImGui::Checkbox("Texture Repetition Mitigation", &s.Enabled);
    int compare = (int)CurrentCompare(s);
    if (ImGui::Combo("Compare", &compare, kCompareLabels, IM_ARRAYSIZE(kCompareLabels)) && compare != (int)Compare::Custom)
        ApplyCompare(s, (Compare)compare);
    ImGui::TextDisabled("Off renders exactly as without mitigation. Patterns follow world position and wrap at the\n"
                        "world period (%.0f, %.0f, %.0f), so chunk borders never show.",
                        TerrainMaterials::GetWorldPeriod().x, TerrainMaterials::GetWorldPeriod().y,
                        TerrainMaterials::GetWorldPeriod().z);

    ImGui::BeginDisabled(!s.Enabled);
    if (ImGui::CollapsingHeader("Transform Variation", ImGuiTreeNodeFlags_DefaultOpen)) {
        ImGui::PushID("Transform");
        ImGui::Checkbox("Enabled", &s.Transform.Enabled);
        ImGui::DragFloat("Variation Scale", &s.Transform.Scale, 0.05f, 0.25f, 256.0f, "%.2f m");
        ImGui::SliderFloat("Edge Blend", &s.Transform.EdgeBlend, 0.0f, 0.5f, "%.2f");
        ImGui::Checkbox("Rotation", &s.Transform.Rotation);
        ImGui::SameLine();
        ImGui::Checkbox("Mirroring", &s.Transform.Mirror);
        SeedField("Seed", s.Transform.Seed);
        ImGui::TextDisabled("Each cell's texture is rotated/mirrored; Edge Blend softens the seams between cells.");
        ImGui::PopID();
    }

    if (ImGui::CollapsingHeader("Texture Variants")) {
        ImGui::PushID("Variants");
        ImGui::Checkbox("Enabled", &s.Variants.Enabled);
        ImGui::SliderInt("Variant Count", &s.Variants.Count, 1, (int)kMaxTerrainVariants);
        ImGui::DragFloat("Variation Scale", &s.Variants.Scale, 0.05f, 0.25f, 256.0f, "%.2f m");
        SeedField("Seed", s.Variants.Seed);
        std::string loaded;
        for (uint32_t slot = 0; slot < kMaxTerrainMaterialSlots; slot++)
            if (TerrainMaterials::IsSlotActive(slot) && TerrainMaterials::GetSlotVariantCount(slot) > 1)
                loaded += (loaded.empty() ? "" : ", ") + std::to_string(slot) + ": " +
                          std::to_string(TerrainMaterials::GetSlotVariantCount(slot));
        ImGui::TextDisabled("Slots with variants (slot: count): %s", loaded.empty() ? "none" : loaded.c_str());
        ImGui::TextDisabled("Add <name>_v1_<channel>.png, _v2_... next to a material's maps; no effect without them.");
        ImGui::PopID();
    }

    if (ImGui::CollapsingHeader("Macro Variation")) {
        ImGui::PushID("Macro");
        ImGui::Checkbox("Enabled", &s.Macro.Enabled);
        ImGui::DragFloat("Scale", &s.Macro.Scale, 0.25f, 1.0f, 1000.0f, "%.1f m");
        ImGui::SliderFloat("Strength", &s.Macro.Strength, 0.0f, 0.5f, "%.3f");
        ImGui::SliderFloat("Contrast", &s.Macro.Contrast, 0.0f, 4.0f, "%.2f");
        SeedField("Seed", s.Macro.Seed);
        ImGui::PopID();
    }

    if (ImGui::CollapsingHeader("Multi-Scale Detail")) {
        ImGui::PushID("Detail");
        ImGui::Checkbox("Enabled", &s.Detail.Enabled);
        ImGui::DragFloat("Detail Scale", &s.Detail.Scale, 0.01f, 0.01f, 16.0f, "%.2f repeats/m");
        ImGui::SliderFloat("Detail Strength", &s.Detail.Strength, 0.0f, 1.0f, "%.2f");
        ImGui::Checkbox("Base Color", &s.Detail.BaseColor);
        ImGui::SameLine();
        ImGui::Checkbox("Normal", &s.Detail.Normal);
        ImGui::SameLine();
        ImGui::Checkbox("Roughness", &s.Detail.Roughness);
        ImGui::TextDisabled("Samples each material's own maps again at this scale (primary scale is per slot).");
        ImGui::PopID();
    }

    if (ImGui::CollapsingHeader("Material Variation")) {
        ImGui::PushID("Material");
        ImGui::Checkbox("Enabled", &s.Material.Enabled);
        ImGui::DragFloat("Scale", &s.Material.Scale, 0.25f, 1.0f, 1000.0f, "%.1f m");
        ImGui::SliderFloat("Albedo", &s.Material.Albedo, 0.0f, 0.5f, "%.3f");
        ImGui::SliderFloat("Roughness", &s.Material.Roughness, 0.0f, 0.5f, "%.3f");
        ImGui::SliderFloat("Normal", &s.Material.Normal, 0.0f, 1.0f, "%.3f");
        SeedField("Seed", s.Material.Seed);
        ImGui::PopID();
    }

    if (ImGui::CollapsingHeader("Debug View")) {
        const char* views[] = {"None", "Show Macro Pattern", "Show Variation Cells", "Show Texture Variant",
                               "Show Transform ID"};
        int view = (int)s.View;
        if (ImGui::Combo("View", &view, views, IM_ARRAYSIZE(views)))
            s.View = (TextureVariationSettings::DebugView)view;
        ImGui::TextDisabled("Flat colors in place of the material; cells/variants/IDs need Transform or Variants on.");
    }
    ImGui::EndDisabled();
}

} // namespace Wankel
