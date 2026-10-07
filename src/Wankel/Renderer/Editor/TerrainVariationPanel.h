#pragma once

namespace Wankel {

// ImGui controls for TerrainMaterials::GetVariationSettings() (texture repetition mitigation) - call inside any
// window. Edits apply live: the settings are uploaded with every terrain draw.
class TerrainVariationPanel {
public:
    static void Draw();
};

} // namespace Wankel
