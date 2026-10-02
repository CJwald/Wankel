#pragma once

#include <glm/glm.hpp>

#include <algorithm>
#include <vector>

namespace Wankel {

// Piecewise-linear scalar keyframes over normalized particle life [0,1]. Keys are kept sorted by Time
// (Sort() after editing); no keys evaluates to 1, a single key is constant.
struct ParticleCurve {
    struct Key {
        float Time = 0.0f;
        float Value = 1.0f;
    };
    std::vector<Key> Keys;

    static ParticleCurve Constant(float value) { return {{{0.0f, value}}}; }
    static ParticleCurve Linear(float from, float to) { return {{{0.0f, from}, {1.0f, to}}}; }

    float Evaluate(float t) const {
        if (Keys.empty())
            return 1.0f;
        if (t <= Keys.front().Time)
            return Keys.front().Value;
        for (size_t i = 1; i < Keys.size(); i++) {
            if (t <= Keys[i].Time) {
                const Key& a = Keys[i - 1];
                const Key& b = Keys[i];
                float span = b.Time - a.Time;
                return span > 1e-6f ? glm::mix(a.Value, b.Value, (t - a.Time) / span) : b.Value;
            }
        }
        return Keys.back().Value;
    }

    void Sort() {
        std::stable_sort(Keys.begin(), Keys.end(), [](const Key& a, const Key& b) { return a.Time < b.Time; });
    }
};

// Same as ParticleCurve for rgba colors; alpha is part of the color. No keys evaluates to opaque white.
struct ParticleGradient {
    struct Key {
        float Time = 0.0f;
        glm::vec4 Color {1.0f};
    };
    std::vector<Key> Keys;

    static ParticleGradient Constant(const glm::vec4& color) { return {{{0.0f, color}}}; }
    static ParticleGradient Linear(const glm::vec4& from, const glm::vec4& to) { return {{{0.0f, from}, {1.0f, to}}}; }

    glm::vec4 Evaluate(float t) const {
        if (Keys.empty())
            return glm::vec4(1.0f);
        if (t <= Keys.front().Time)
            return Keys.front().Color;
        for (size_t i = 1; i < Keys.size(); i++) {
            if (t <= Keys[i].Time) {
                const Key& a = Keys[i - 1];
                const Key& b = Keys[i];
                float span = b.Time - a.Time;
                return span > 1e-6f ? glm::mix(a.Color, b.Color, (t - a.Time) / span) : b.Color;
            }
        }
        return Keys.back().Color;
    }

    void Sort() {
        std::stable_sort(Keys.begin(), Keys.end(), [](const Key& a, const Key& b) { return a.Time < b.Time; });
    }
};

} // namespace Wankel
