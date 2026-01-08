#pragma once

class FlEasing 
{
public:
    // 基本線形
    static double Linear(double t) noexcept {
        return t;
    }

    // 二次（Quad）
    static double EaseInQuad(double t) noexcept { return t * t; }
    static double EaseOutQuad(double t) noexcept { return t * (2 - t); }
    static double EaseInOutQuad(double t) noexcept {
        return (t < 0.5) ? 2 * t * t : -1 + (4 - 2 * t) * t;
    }

    // 三次（Cubic）
    static double EaseInCubic(double t) noexcept { return t * t * t; }
    static double EaseOutCubic(double t) noexcept { return 1 + (--t) * t * t; }
    static double EaseInOutCubic(double t) noexcept {
        return (t < 0.5) ? 4 * t * t * t : (t - 1) * (2 * t - 2) * (2 * t - 2) + 1;
    }

    // 四次（Quart）
    static double EaseInQuart(double t) noexcept { return t * t * t * t; }
    static double EaseOutQuart(double t) noexcept { return 1 - (--t) * t * t * t; }
    static double EaseInOutQuart(double t) noexcept {
        return (t < 0.5) ? 8 * t * t * t * t : 1 - 8 * (--t) * t * t * t;
    }

    // 五次（Quint）
    static double EaseInQuint(double t) noexcept { return t * t * t * t * t; }
    static double EaseOutQuint(double t) noexcept { return 1 + (--t) * t * t * t * t; }
    static double EaseInOutQuint(double t) noexcept {
        return (t < 0.5) ? 16 * t * t * t * t * t : 1 + 16 * (--t) * t * t * t * t;
    }

    // 正弦
    static double EaseInSine(double t) noexcept {
        return 1 - std::cos((t * M_PI) / 2.0);
    }
    static double EaseOutSine(double t) noexcept {
        return std::sin((t * M_PI) / 2.0);
    }
    static double EaseInOutSine(double t) noexcept {
        return -(std::cos(M_PI * t) - 1) / 2.0;
    }

    // 指数
    static double EaseInExpo(double t) noexcept {
        return (t == 0.0) ? 0.0 : std::pow(2.0, 10.0 * (t - 1));
    }
    static double EaseOutExpo(double t) noexcept {
        return (t == 1.0) ? 1.0 : 1 - std::pow(2.0, -10.0 * t);
    }
    static double EaseInOutExpo(double t) noexcept {
        if (t == 0.0) return 0.0;
        if (t == 1.0) return 1.0;
        return (t < 0.5) ?
            std::pow(2.0, 20.0 * t - 10.0) / 2.0 :
            (2 - std::pow(2.0, -20.0 * t + 10.0)) / 2.0;
    }

    // 円形（Circular）
    static double EaseInCirc(double t) noexcept {
        return 1 - std::sqrt(1 - t * t);
    }
    static double EaseOutCirc(double t) noexcept {
        return std::sqrt(1 - (--t) * t);
    }
    static double EaseInOutCirc(double t) noexcept {
        return (t < 0.5)
            ? (1 - std::sqrt(1 - 4 * t * t)) / 2
            : (std::sqrt(1 - (2 * t - 2) * (2 * t - 2)) + 1) / 2;
    }

    // バック（オーバーシュート）
    static double EaseInBack(double t) noexcept {
        constexpr double c1 = 1.70158;
        constexpr double c3 = c1 + 1;
        return c3 * t * t * t - c1 * t * t;
    }
    static double EaseOutBack(double t) noexcept {
        constexpr double c1 = 1.70158;
        constexpr double c3 = c1 + 1;
        return 1 + c3 * std::pow(t - 1, 3) + c1 * std::pow(t - 1, 2);
    }
    static double EaseInOutBack(double t) noexcept {
        constexpr double c1 = 1.70158;
        constexpr double c2 = c1 * 1.525;
        return (t < 0.5)
            ? (std::pow(2 * t, 2) * ((c2 + 1) * 2 * t - c2)) / 2
            : (std::pow(2 * t - 2, 2) * ((c2 + 1) * (t * 2 - 2) + c2) + 2) / 2;
    }

    // 弾性（Elastic）
    static double EaseInElastic(double t) noexcept {
        if (t == 0) return 0;
        if (t == 1) return 1;
        return -std::pow(2, 10 * t - 10) *
            std::sin((t * 10 - 10.75) * ((2 * M_PI) / 3));
    }
    static double EaseOutElastic(double t) noexcept {
        if (t == 0) return 0;
        if (t == 1) return 1;
        return std::pow(2, -10 * t) *
            std::sin((t * 10 - 0.75) * ((2 * M_PI) / 3))
            + 1;
    }
    static double EaseInOutElastic(double t) noexcept {
        if (t == 0) return 0;
        if (t == 1) return 1;
        return (t < 0.5)
            ? -(std::pow(2, 20 * t - 10) *
                std::sin((20 * t - 11.125) * ((2 * M_PI) / 4.5))) / 2
            : (std::pow(2, -20 * t + 10) *
                std::sin((20 * t - 11.125) * ((2 * M_PI) / 4.5))) / 2
            + 1;
    }

    // バウンス（跳ね返り）
    static double EaseOutBounce(double t) noexcept {
        constexpr double n1 = 7.5625;
        constexpr double d1 = 2.75;
        if (t < 1 / d1) {
            return n1 * t * t;
        }
        else if (t < 2 / d1) {
            return n1 * (t -= 1.5 / d1) * t + 0.75;
        }
        else if (t < 2.5 / d1) {
            return n1 * (t -= 2.25 / d1) * t + 0.9375;
        }
        else {
            return n1 * (t -= 2.625 / d1) * t + 0.984375;
        }
    }
    static double EaseInBounce(double t) noexcept {
        return 1 - EaseOutBounce(1 - t);
    }
    static double EaseInOutBounce(double t) noexcept {
        return (t < 0.5)
            ? (1 - EaseOutBounce(1 - 2 * t)) / 2
            : (1 + EaseOutBounce(2 * t - 1)) / 2;
    }
};