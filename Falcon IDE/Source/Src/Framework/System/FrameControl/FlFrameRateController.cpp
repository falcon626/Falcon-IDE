#include "FlFrameRateController.h"

void FlFrameRateController::BeginFrame() noexcept
{
    auto currentTime{ std::chrono::steady_clock::now() };
    if (m_firstFrame) {
        m_deltaTime = Def::FloatZero;
        m_firstFrame = false;
    }
    else {
        auto duration{ currentTime - m_previousStartTime };
        m_deltaTime = std::chrono::duration_cast<std::chrono::duration<float>>(duration).count();
        UpdateFPS();
    }
    m_previousStartTime = currentTime;
}

void FlFrameRateController::EndFrame() const noexcept
{
    // <Quick Return:Vsync‚ª—LŒø‚Ìê‡‚ÍƒtƒŒ[ƒ€ŽžŠÔ‚ð’²®‚µ‚È‚¢>
    if (m_isVsync) return; 

    auto endTime{ std::chrono::steady_clock::now() };
    auto frameDuration{ endTime - m_previousStartTime };

    auto isForegroundWindow{ (GetForegroundWindow() == m_windowHandle) };

    auto targetMinFrameTime = isForegroundWindow ? m_minFrameTime : m_backgroundMinFrameTime;

    if (isForegroundWindow && m_isLimitless) return;

    if (frameDuration < targetMinFrameTime) 
    {
        auto sleepTime{ targetMinFrameTime - frameDuration };
        std::this_thread::sleep_for(sleepTime);
    }
}

void FlFrameRateController::SetIsVsync(bool isVsync) noexcept
{
    m_isVsync = isVsync;
	GraphicsDevice::Instance().SetVsync(isVsync);
}

void FlFrameRateController::UpdateFPS() noexcept
{
    ++m_frameCount;
    m_frameTimeAccumulator += m_deltaTime;

    auto currentTime{ std::chrono::steady_clock::now() };
    auto elapsedSinceLastFPS{ std::chrono::duration_cast<std::chrono::duration<double>>(currentTime - m_lastFPSTime).count() };

    if (elapsedSinceLastFPS >= Def::DoubleOne) 
    { // 1•b‚²‚Æ‚ÉFPS‚ðŒvŽZ
        m_currentFPS           = static_cast<float>(m_frameCount) / static_cast<float>(m_frameTimeAccumulator);
        m_frameCount           = Def::UIntZero;
        m_frameTimeAccumulator = Def::DoubleZero;
        m_lastFPSTime          = currentTime;
    }
}
