#pragma once
#include "../System/Timer/FlChronus.hpp"
#include "../Math/FlEasing.hpp"

class FlAnimator
{
public:
    using EaseFunc = std::function<double(double)>;

    FlAnimator(double durationSec, EaseFunc ease = FlEasing::Linear)
        : m_duration(durationSec), m_ease(ease)
    {}

    void start() {
        m_timer.start();
        m_running = true;
    }

    void reset() {
        m_timer.reset();
        m_running = false;
    }

    bool isRunning() const {
        return m_running && progress() < 1.0;
    }

    // 進行度 (0.0〜1.0, イージング適用済み)
    double value() const {
        return m_ease(progress());
    }

    // 生の進行度 (イージング未適用)
    double progress() const {
        if (!m_running) return 0.0;
        double t = duration_cast<std::chrono::duration<double>>(m_timer.elapsed()).count() / m_duration;
        return std::clamp(t, 0.0, 1.0);
    }

private:
    FlChronus m_timer;
    double m_duration;
    EaseFunc m_ease;
    bool m_running = false;
};