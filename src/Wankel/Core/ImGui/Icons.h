#pragma once

// UTF-8 byte sequences for the icon glyphs ImGuiLayer::OnAttach merges into the default ImGui font
// (see docs/IMGUIRefactor.md Phase 8, and ImGuiLayer.cpp's kIconGlyphRanges). Plain #define macros
// (not constexpr) specifically so they compose via ordinary adjacent-string-literal concatenation,
// e.g. ImGui::Text(ICON_LIGHT_BULB " Lighting"). Extend this list and kIconGlyphRanges together
// whenever a new icon is adopted - loading full symbol/emoji block ranges instead would needlessly
// bloat the font atlas.
#define ICON_PLAY "\xE2\x96\xB6"           // U+25B6 BLACK RIGHT-POINTING TRIANGLE
#define ICON_PAUSE "\xE2\x8F\xB8"          // U+23F8 DOUBLE VERTICAL BAR
#define ICON_STEP "\xE2\x8F\xAD"           // U+23ED BLACK RIGHT-POINTING DOUBLE TRIANGLE WITH VERTICAL BAR
#define ICON_LIGHT_BULB "\xF0\x9F\x92\xA1" // U+1F4A1 ELECTRIC LIGHT BULB
