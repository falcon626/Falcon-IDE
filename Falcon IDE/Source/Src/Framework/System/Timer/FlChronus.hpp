#pragma once

class FlChronus 
{
public:
    using Steady = std::chrono::steady_clock;          // 単調時計（経過時間向け）
    using System = std::chrono::system_clock;          // 壁時計（日時向け）
    using TP     = Steady::time_point;
    using Dur    = Steady::duration;

    // ===== 便利な型 =====
    using ns  = std::chrono::nanoseconds;
    using us  = std::chrono::microseconds;
    using ms  = std::chrono::milliseconds;
    using sec = std::chrono::seconds;
    using min = std::chrono::minutes;

    // ====== ストップウォッチ ======
    FlChronus(bool startNow = true) { if (startNow) start(); }
    void start() noexcept {
        if (!_running) { _running = true; _t0 = Steady::now(); }
    }
    void stop() noexcept {
        if (_running) { _acc += Steady::now() - _t0; _running = false; }
    }
    void reset() noexcept { 
        _acc = Dur::zero();
        _running = false;
        _t0 = Steady::now();     
        _lapMark = _t0;          
    }
    void restart() noexcept { reset(); start(); }
    bool running() const noexcept { return _running; }

    // 経過時間を任意のdurationで
    template<class D = ms>
    D elapsed() const noexcept {
        Dur d{ _acc };
        if (_running) d += (Steady::now() - _t0);
        return std::chrono::duration_cast<D>(d);
    }

    // ラップ（差分取得して内部クリアはしない）
    template<class D = ms>
    D lap() noexcept {
        auto now{ Steady::now() };
        Dur d{ now - _lapMark };
        _lapMark = now;
        return std::chrono::duration_cast<D>(d);
    }

    // ====== カウントダウン（締切 or 残り時間） ======
    // deadline までの残りを返す。過ぎていれば0。
    template<class D = ms>
    static D remaining_until(Steady::time_point deadline) noexcept {
        auto now{ Steady::now() };
        if (deadline <= now) return D::zero();
        return std::chrono::duration_cast<D>(deadline - now);
    }
    // duration のカウントダウン（開始時間 + span）
    template<class D = ms, class Rep, class Per>
    static D remaining_span(std::chrono::duration<Rep, Per> span,
        Steady::time_point start) noexcept {
        auto deadline{ start + std::chrono::duration_cast<Dur>(span) };
        return remaining_until<D>(deadline);
    }
    static bool expired(Steady::time_point deadline) noexcept {
        return Steady::now() >= deadline;
    }

    // ====== ティッカー（一定間隔で true を返す） ======
    class Ticker {
    public:
        explicit Ticker(Dur interval) : _interval(interval), _next(Steady::now() + interval) {}
        // 規定間隔が経てば true を返し、次回しきい値を進める（遅延を詰める）
        bool tick() noexcept {
            auto now{ Steady::now() };
            if (now < _next) return false;
            // 遅れを複数間隔分まとめて前進（スリップ防止）
            do { _next += _interval; } while (_next <= now);
            return true;
        }
        void reset(Dur interval) noexcept { _interval = interval; _next = Steady::now() + interval; }
        Dur interval() const noexcept { return _interval; }
    private:
        Dur _interval{};
        TP  _next{};
    };

    // ====== FPS 計測（移動平均） ======
    class FpsAverager {
    public:
        explicit FpsAverager(size_t window = 100) : _window(window) { _prev = Steady::now(); }
        // 毎フレーム呼ぶ。現在のFPSと平均FPS
        std::pair<double, double> on_frame() {
            auto now{ Steady::now() };
            auto dt{ std::chrono::duration<double>(now - _prev).count() };
            _prev = now;
            auto inst{ (dt > Def::DoubleZero) ? (Def::DoubleOne / dt) : Def::DoubleZero };

            if (_samples.size() == _window) _samples.erase(_samples.begin());
            _samples.push_back(inst);

            auto avg{ Def::DoubleZero };
            if (!_samples.empty()) {
                avg = std::accumulate(_samples.begin(), _samples.end(), Def::DoubleZero) / _samples.size();
            }
            return { inst, avg };
        }
        void reset() { _samples.clear(); _prev = Steady::now(); }
    private:
        size_t _window{};
        std::vector<double> _samples;
        TP _prev{};
    };

    /// <summary> Resource Acquisition Is Initialization </summary>

    // ====== スコープ計測（終了時にコールバック/ログ） ======
    class Scoped {
    public:
        using Callback = std::function<void(Dur)>;
        explicit Scoped(Callback cb) : _cb(std::move(cb)), _t0(Steady::now()) {}
        ~Scoped() { if (_cb) _cb(Steady::now() - _t0); }
    private:
        Callback _cb;
        TP _t0{};
    };

    // ====== 変換・整形ユーティリティ ======
    template<class Rep, class Per>
    static double to_seconds(std::chrono::duration<Rep, Per> d) {
        return std::chrono::duration<double>(d).count();
    }
    template<class Rep, class Per>
    static std::string format_hms(std::chrono::duration<Rep, Per> d) {
        using namespace std::chrono;
        auto secs = duration_cast<sec>(d);
        auto h = secs.count() / 3600;
        auto m = (secs.count() % 3600) / 60;
        auto s = (secs.count() % 60);
        std::ostringstream oss;
        oss << std::setfill('0') << std::setw(2) << h << ":"
            << std::setw(2) << m << ":" << std::setw(2) << s;
        return oss.str();
    }

    // 壁時計：ISOっぽい文字列
    static std::string now_iso8601() {
        auto now = System::now();
        auto t = System::to_time_t(now);
        std::tm tm{};
#if defined(_WIN32)
        localtime_s(&tm, &t);
#else
        localtime_r(&t, &tm);
#endif
        std::ostringstream oss;
        oss << std::put_time(&tm, "%Y-%m-%d %H:%M:%S");
        return oss.str();
    }

    // スリープヘルパ
    template<class Rep, class Per>
    static void sleep_for(std::chrono::duration<Rep, Per> d) {
        std::this_thread::sleep_for(d);
    }

private:
    bool _running{ false };
    TP   _t0{};
    TP   _lapMark{ Steady::now() };
    Dur  _acc{};
};