#pragma once

class Application
{
public:

	void Execute();

	inline auto End() noexcept { m_isEnd = true; }

	inline const auto GetDeltaTime() const noexcept { return m_spFrameRateController->GetDeltaTime(); }
	inline const auto GetWindow() const noexcept { return m_window; }

	static auto& Instance() noexcept
	{
		static auto instance{ Application{} };
		return instance;
	}
private:

	Application();
	~Application();

	Application(const Application&) = delete;
	Application& operator=(const Application&) = delete;

	void Update();

	void Release();

	Window m_window{};

	std::shared_ptr<FlFrameRateController> m_spFrameRateController;

	bool m_isEnd{ false };

	int32_t m_windowW;
	int32_t m_windowH;

	Math::Vector3 m_cPos;
	Math::Vector3 m_cRot;
};