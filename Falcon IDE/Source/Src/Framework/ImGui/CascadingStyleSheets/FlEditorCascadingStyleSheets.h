#pragma once

class FlEditorCascadingStyleSheets
{
public:
    struct StyleTheme 
    {
        // === 色設定 ===
        std::unordered_map<ImGuiCol, ImVec4> colors;

        // === 数値パラメータ設定 ===
        float Alpha             = 1.0f;
        ImVec2 WindowPadding    = ImVec2(8, 8);
        float WindowRounding    = 4.0f;
        float FrameRounding     = 4.0f;
        ImVec2 FramePadding     = ImVec2(4, 3);
        ImVec2 ItemSpacing      = ImVec2(8, 4);
        ImVec2 ItemInnerSpacing = ImVec2(4, 4);
        float ScrollbarSize     = 14.0f;
        float GrabMinSize       = 10.0f;
        float TabRounding       = 4.0f;

        void Reset() noexcept
        {
            colors.clear();

            Alpha            = 1.0f;
            WindowPadding    = ImVec2(8, 8);
            WindowRounding   = 4.0f;
            FrameRounding    = 4.0f;
            FramePadding     = ImVec2(4, 3);
            ItemSpacing      = ImVec2(8, 4);
            ItemInnerSpacing = ImVec2(4, 4);
            ScrollbarSize    = 14.0f;
            GrabMinSize      = 10.0f;
            TabRounding      = 4.0f;
        }
    };

    // ================================
    // テーマ登録
    // ================================
    void AddTheme(const std::string& name, const StyleTheme& style) noexcept { m_themes[name] = style; }

    // ================================
    // 即時テーマ適用
    // ================================
    void ApplyTheme(const std::string& name);

    // ================================
    // トランジションでテーマ切替
    // ================================
    void TransitionToTheme(const std::string& name, double durationSec,
        FlAnimator::EaseFunc ease = FlEasing::EaseInOutQuad);

    // ================================
    // 毎フレーム呼び出してアニメ適用
    // ================================
    void Update();

    const std::string& CurrentTheme() const { return m_currentTheme; }

private:
    std::unordered_map<std::string, StyleTheme> m_themes;
    std::string m_currentTheme;
    std::string m_targetTheme;

    FlAnimator m_animator{ Def::DoubleOne };
    ImGuiStyle m_startStyle;

    // ==== lerp helpers ====
    static float lerp(float a, float b, float t) {
        return a + (b - a) * t;
    }
    static ImVec2 lerp(const ImVec2& a, const ImVec2& b, float t) {
        return ImVec2(a.x + (b.x - a.x) * t, a.y + (b.y - a.y) * t);
    }
};
