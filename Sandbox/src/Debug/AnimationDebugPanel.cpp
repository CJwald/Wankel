#include "AnimationDebugPanel.h"
#include "SecondOrderPreview.h"

#include <Wankel/ECS/Scene.h>
#include <Wankel/ECS/Components.h>

#include <imgui.h>

#include <vector>
#include <string>
#include <algorithm>

using namespace Wankel;

namespace {

const char* MotionAxisName(MotionAxis axis) {
    switch (axis) {
        case MotionAxis::X:
            return "X";
        case MotionAxis::Y:
            return "Y";
        case MotionAxis::Z:
            return "Z";
        case MotionAxis::Pitch:
            return "Pitch";
        case MotionAxis::Yaw:
            return "Yaw";
        case MotionAxis::Roll:
            return "Roll";
        case MotionAxis::Count:
            break;
    }
    return "Unknown";
}

const char* MotionAxisLabels[] = {"X", "Y", "Z", "Pitch", "Yaw", "Roll"};

} // namespace

void AnimationDebugPanel::Draw(Scene& scene, entt::entity& selectedEntity) {
    ImGui::PushStyleVar(ImGuiStyleVar_IndentSpacing, 20.0f);
    ImGui::Indent();

    auto& registry = scene.Registry();

    // BUILD ENTITY LIST
    std::vector<entt::entity> entities;
    auto& storage = registry.storage<entt::entity>();

    for (auto entity : storage)
        entities.push_back(entity);

    std::reverse(entities.begin(), entities.end());

    // default selection
    if (!entities.empty()) {
        bool valid = selectedEntity != entt::null && registry.valid(selectedEntity);

        if (!valid)
            selectedEntity = entities[0];
    }

    // ENTITY DROPDOWN
    std::vector<std::string> storageNames;
    std::vector<const char*> labels;
    for (auto entity : entities) {
        std::string name = "Unknown";
        if (registry.all_of<Tag>(entity))
            name = registry.get<Tag>(entity).Name;
        storageNames.push_back("[" + std::to_string((uint32_t)entity) + "]  |  " + name);
    }

    for (auto& s : storageNames)
        labels.push_back(s.c_str());

    static int selectedIndex = 0;

    if (!labels.empty()) {
        if (ImGui::Combo("Selected", &selectedIndex, labels.data(), (int)labels.size())) {
            selectedEntity = entities[selectedIndex];
        }
    }

    ImGui::Separator();

    // ANIMATION SECTION (ALWAYS VISIBLE)
    if (ImGui::CollapsingHeader("Animation")) {
        bool validEntity = selectedEntity != entt::null && registry.valid(selectedEntity);
        if (!validEntity) {
            ImGui::TextDisabled("No entity selected.");
        } else if (!registry.all_of<MeshAnimation>(selectedEntity)) {
            // NO COMPONENT → ADD BUTTON
            if (ImGui::Button("Add MeshAnimation")) {
                registry.emplace<MeshAnimation>(selectedEntity);
            }
        } else {
            // EDIT EXISTING COMPONENT
            auto& anim = registry.get<MeshAnimation>(selectedEntity);
            static int selectedOutputAxis[MeshAnimation::AxisCount] = {};
            for (int input = 0; input < MeshAnimation::AxisCount; input++) {
                MotionAxis inAxis = (MotionAxis)input;

                if (!ImGui::TreeNode(MotionAxisName(inAxis)))
                    continue;

                bool anyShown = false;
                for (int output = 0; output < MeshAnimation::AxisCount; output++) {
                    auto& link = anim.Links[input][output];

                    if (!link.Enabled)
                        continue;

                    anyShown = true;
                    MotionAxis outAxis = (MotionAxis)output;
                    std::string label = std::string(MotionAxisName(inAxis)) + " -> " + MotionAxisName(outAxis);
                    bool removeMapping = false;
                    if (ImGui::TreeNode(label.c_str())) {
                        ImGui::Checkbox(("Enabled##" + label).c_str(), &link.Enabled);
                        ImGui::DragFloat(("Magnitude##" + label).c_str(), &link.Magnitude, 0.01f);
                        ImGui::DragFloat(("Frequency##" + label).c_str(), &link.Frequency, 0.01f, 0.01f, 20.0f);
                        ImGui::DragFloat(("Damping##" + label).c_str(), &link.Damping, 0.01f, 0.0f, 10.0f);
                        ImGui::DragFloat(("Response##" + label).c_str(), &link.Response, 0.01f, -10.0f, 10.0f);
                        ImGui::DragFloat(("Clamp##" + label).c_str(), &link.Clamp, 0.01f, 0.0f, 1000.0f);
                        ImGui::Text("Output: %.3f", link.Output);
                        ImGui::Separator();
                        auto values = SecondOrderPreview::GetStepResponse(link.Frequency, link.Damping, link.Response);
                        ImGui::PlotLines("Step Response", values.data(), (int)values.size(), 0, nullptr, -0.5f, 2.0f,
                                         ImVec2(0, 100));

                        if (ImGui::Button(("Remove##" + label).c_str()))
                            removeMapping = true;

                        ImGui::TreePop();
                    }

                    if (removeMapping)
                        link = {};
                }

                if (!anyShown)
                    ImGui::TextDisabled("No active mappings");

                ImGui::Separator();

                // ADD NEW LINK
                std::string buttonId = "+ Add Mapping##" + std::to_string(input);

                if (ImGui::Button(buttonId.c_str())) {
                    int output = selectedOutputAxis[input];
                    auto& link = anim.Links[input][output];

                    if (!link.Enabled) {
                        anim.SetLink((MotionAxis)input, (MotionAxis)output, 1.0f, 2.0f, 0.5f, 1.0f, 10.0f);
                    }
                }

                ImGui::SameLine();
                std::string comboId = "##AddMappingCombo" + std::to_string(input);
                ImGui::Combo(comboId.c_str(), &selectedOutputAxis[input], MotionAxisLabels, MeshAnimation::AxisCount);

                ImGui::TreePop();
            }
        }
    }

    ImGui::Unindent();
    ImGui::PopStyleVar();
}
