#include "FlEditorCascadingStyleSheets.h"

void FlEditorCascadingStyleSheets::ApplyTheme(const std::string& name)
{
    if (m_themes.find(name) == m_themes.end()) return;

    const auto& theme{ m_themes[name] };
    auto& style{ ImGui::GetStyle() };

    // 色反映
    for (auto& [col, val] : theme.colors) 
        style.Colors[col] = val;

    // 数値反映
    style.Alpha            = theme.Alpha;
    style.WindowPadding    = theme.WindowPadding;
    style.WindowRounding   = theme.WindowRounding;
    style.FrameRounding    = theme.FrameRounding;
    style.FramePadding     = theme.FramePadding;
    style.ItemSpacing      = theme.ItemSpacing;
    style.ItemInnerSpacing = theme.ItemInnerSpacing;
    style.ScrollbarSize    = theme.ScrollbarSize;
    style.GrabMinSize      = theme.GrabMinSize;
    style.TabRounding      = theme.TabRounding;

    m_currentTheme = name;
}

void FlEditorCascadingStyleSheets::TransitionToTheme(const std::string& name, double durationSec, FlAnimator::EaseFunc ease)
{
    if (m_themes.find(name) == m_themes.end()) return;
    m_targetTheme = name;
    m_animator = FlAnimator(durationSec, ease);
    m_animator.start();

    // 遷移開始時の状態をキャプチャ
    m_startStyle = ImGui::GetStyle();
}

void FlEditorCascadingStyleSheets::Update()
{
    if (!m_animator.isRunning()) return;

    ImGuiStyle& style = ImGui::GetStyle();
    const StyleTheme& target = m_themes[m_targetTheme];
    float t = static_cast<float>(m_animator.value());

    // ---- 色補間 ----
    for (auto& [col, targetCol] : target.colors) {
        ImVec4 s = m_startStyle.Colors[col];
        style.Colors[col] = ImVec4(
            s.x + (targetCol.x - s.x) * t,
            s.y + (targetCol.y - s.y) * t,
            s.z + (targetCol.z - s.z) * t,
            s.w + (targetCol.w - s.w) * t
        );
    }

    // ---- 数値補間 ----
    style.Alpha            = lerp(m_startStyle.Alpha, target.Alpha, t);
    style.WindowPadding    = lerp(m_startStyle.WindowPadding, target.WindowPadding, t);
    style.WindowRounding   = lerp(m_startStyle.WindowRounding, target.WindowRounding, t);
    style.FrameRounding    = lerp(m_startStyle.FrameRounding, target.FrameRounding, t);
    style.FramePadding     = lerp(m_startStyle.FramePadding, target.FramePadding, t);
    style.ItemSpacing      = lerp(m_startStyle.ItemSpacing, target.ItemSpacing, t);
    style.ItemInnerSpacing = lerp(m_startStyle.ItemInnerSpacing, target.ItemInnerSpacing, t);
    style.ScrollbarSize    = lerp(m_startStyle.ScrollbarSize, target.ScrollbarSize, t);
    style.GrabMinSize      = lerp(m_startStyle.GrabMinSize, target.GrabMinSize, t);
    style.TabRounding      = lerp(m_startStyle.TabRounding, target.TabRounding, t);

    if (t >= Def::FloatOne) m_currentTheme = m_targetTheme;
}
