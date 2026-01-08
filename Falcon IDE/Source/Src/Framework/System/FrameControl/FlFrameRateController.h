#pragma once

class FlFrameRateController {
public:
    /// <summary>
    /// 指定されたフレームレートでフレームレート制御を行うコントローラーのコンストラクタです。
    /// </summary>
    /// <param name="desiredFPS">目標とするフレーム毎秒（FPS）値。</param>
    explicit FlFrameRateController(const float desiredFPS, const float backgroundFPS) :
		m_windowHandle(nullptr),
        m_minFrameTime(std::chrono::duration<double>(Def::DoubleOne / desiredFPS)),
        m_backgroundMinFrameTime(std::chrono::duration<double>(Def::DoubleOne / backgroundFPS)),
        m_previousStartTime(std::chrono::steady_clock::now()),
        m_lastFPSTime(m_previousStartTime),
        m_frameTimeAccumulator(Def::DoubleZero),
        m_desiredFPS(desiredFPS),
		m_backgroundFPS(backgroundFPS),
        m_deltaTime(Def::FloatZero),
        m_currentFPS(Def::FloatZero),
        m_frameCount(Def::UIntZero),
        m_firstFrame(true),
		m_isLimitless(true),
		m_isVsync(false)
    {}

    /// <summary>
    /// フレームの処理を開始します。
    /// </summary>
    void BeginFrame() noexcept;

    /// <summary>
    /// フレームの処理を終了します。
    /// </summary>
    void EndFrame() const noexcept;

    /// <summary>
    /// 経過時間（デルタタイム）を取得します。
    /// </summary>
    /// <returns>m_deltaTime の現在の値を返します。</returns>
    const auto GetDeltaTime() const noexcept { return m_deltaTime; }

    /// <summary>
    /// 現在のFPS（フレーム毎秒）を取得します。
    /// </summary>
    /// <returns>現在のFPS値。</returns>
    const auto GetCurrentFPS() const noexcept { return m_currentFPS; }

	auto& WorkIsVsync() noexcept { return m_isVsync; }
	const auto& GetIsVsync() const noexcept { return m_isVsync; }

    auto& WorkIsLimitless() noexcept { return m_isLimitless; }
    const auto& GetIsLimitless() const noexcept { return m_isLimitless; }

    /// <summary>
    /// ウィンドウハンドルを設定します。
    /// </summary>
    /// <param name="windowHandle">設定するウィンドウハンドル。</param>
    auto SetWindowHandle(const HWND& windowHandle) noexcept { m_windowHandle = windowHandle; }

    /// <summary>
    /// 希望するフレームレート（FPS）を設定します。
    /// </summary>
    /// <param name="desiredFPS">設定する希望フレームレート（1秒あたりのフレーム数）。</param>
    void SetDesiredFPS(float desiredFPS) noexcept 
    {
        m_desiredFPS = desiredFPS;
        m_minFrameTime = std::chrono::duration<double>(Def::DoubleOne / desiredFPS);
    }

	/// <summary>
	/// 制限なし状態を設定します。
	/// </summary>
	/// <param name="isLimitless">制限なし状態に設定する場合は true、そうでない場合は false を指定します。</param>
	auto SetIsLimitless(bool isLimitless) noexcept { m_isLimitless = isLimitless; }

    /// <summary>
    /// 垂直同期（Vsync）の有効・無効を設定します。
    /// </summary>
    /// <param name="isVsync">Vsync を有効にする場合は true、無効にする場合は false を指定します。</param>
    void SetIsVsync(bool isVsync) noexcept;

private:
    /// <summary>
    /// FPS（フレーム毎秒）を更新します。
    /// </summary>
    void UpdateFPS() noexcept;

	HWND m_windowHandle; // ウィンドウハンドル

    std::chrono::duration<double> m_minFrameTime;              // 1フレームの最小時間
    std::chrono::duration<double>  m_backgroundMinFrameTime;   // BackGround時1フレームの最小時間
    std::chrono::steady_clock::time_point m_previousStartTime; // 前フレームの開始時間
    std::chrono::steady_clock::time_point m_lastFPSTime;       // 最後のFPS計算時間

    double m_frameTimeAccumulator; // フレーム時間累積

    float m_desiredFPS;            // 希望するFPS
	float m_backgroundFPS;         // バックグラウンド時の希望FPS
    float m_deltaTime;             // 最新のデルタタイム（秒）
    float m_currentFPS;            // 現在のFPS

    unsigned int m_frameCount; // フレーム数カウンタ

    bool m_firstFrame;  // 初回フレームフラグ
	bool m_isLimitless; // フレームレート制限を無効にするフラグ
	bool m_isVsync;     // Vsync（垂直同期）を有効にするフラグ
};