#pragma once

class FlLogEditor
{
public:

	inline void ImGuiShowHelp(const std::string& text, bool showIcon = false)
	{
		if (showIcon) {
			ImGui::SameLine();
			ImGui::TextDisabled("(?)");
		}
		if (ImGui::IsItemHovered())
		{
			ImGui::BeginTooltip();
			ImGui::PushTextWrapPos(450.0f);
			ImGui::TextUnformatted(text.c_str());
			ImGui::PopTextWrapPos();
			ImGui::EndTooltip();
		}
	}


	template<class... Args>
	void AddLog(const std::string& fmt, Args... args)
	{
		Math::Color color{ Def::Half, Def::Half, Def::Half, Def::FloatOne };

		AddLog(color, fmt, args...);
	}

	template<class... Args>
	void AddWarningLog(const std::string& fmt, Args... args)
	{
		Math::Color color{ Def::Half, Def::Half, Def::FloatZero, Def::FloatOne };

		AddLog(color, fmt, args...);
	}

	template<class... Args>
	void AddChangeLog(const std::string& fmt, Args... args)
	{
		Math::Color color{ Def::Half, Def::FloatZero, Def::FloatOne, Def::FloatOne };

		AddLog(color, fmt, args...);
	}

	template<class... Args>
	void AddErrorLog(const std::string& fmt, Args... args)
	{
		Math::Color color{ Def::FloatOne, Def::FloatZero, Def::FloatZero, Def::FloatOne };

		AddLog(color, fmt, args...);
	}

	template<class... Args>
	void AddSuccessLog(const std::string& fmt, Args... args)
	{
		Math::Color color{ Def::FloatZero, Def::FloatOne, Def::Half, Def::FloatOne };

		AddLog(color, fmt, args...);
	}

	template<class... Args>
	void AddLog(const Math::Color& color, const std::string& fmt, Args... args)
	{
		auto buffer{ FlChronus::now_iso8601() + " | " + Str::FormatString(fmt.c_str(),args...)};

		m_logEntries.push_back({ buffer, color });
		m_scrollToBottom = true;
	}

	// std::wstring対応のメソッド群
	template<class... Args>
	void AddLogW(const std::wstring& fmt, Args... args)
	{
		Math::Color color{ Def::Half, Def::Half, Def::Half, Def::FloatOne };
		AddLogW(color, fmt, args...);
	}

	template<class... Args>
	void AddWarningLogW(const std::wstring& fmt, Args... args)
	{
		Math::Color color{ Def::Half, Def::Half, Def::FloatZero, Def::FloatOne };
		AddLogW(color, fmt, args...);
	}

	template<class... Args>
	void AddChangeLogW(const std::wstring& fmt, Args... args)
	{
		Math::Color color{ Def::Half, Def::FloatZero, Def::FloatOne, Def::FloatOne };
		AddLogW(color, fmt, args...);
	}

	template<class... Args>
	void AddErrorLogW(const std::wstring& fmt, Args... args)
	{
		Math::Color color{ Def::FloatOne, Def::FloatZero, Def::FloatZero, Def::FloatOne };
		AddLogW(color, fmt, args...);
	}

	template<class... Args>
	void AddSuccessLogW(const std::wstring& fmt, Args... args)
	{
		Math::Color color{ Def::FloatZero, Def::FloatOne, Def::Half, Def::FloatOne };
		AddLogW(color, fmt, args...);
	}

	template<class... Args>
	void AddLogW(const Math::Color& color, const std::wstring& fmt, Args... args)
	{
		auto wbuffer{ Str::FormatStringW(fmt.c_str(), args...) };
		auto buffer{ FlChronus::now_iso8601() + " | " + wide_to_ansi(wbuffer) };

		m_logEntries.push_back({ buffer, color });
		m_scrollToBottom = true;
	}

	// std::u8string対応のメソッド群
	template<class... Args>
	void AddLogU8(const std::u8string& fmt, Args... args)
	{
		Math::Color color{ Def::Half, Def::Half, Def::Half, Def::FloatOne };
		AddLogU8(color, fmt, args...);
	}

	template<class... Args>
	void AddWarningLogU8(const std::u8string& fmt, Args... args)
	{
		Math::Color color{ Def::Half, Def::Half, Def::FloatZero, Def::FloatOne };
		AddLogU8(color, fmt, args...);
	}

	template<class... Args>
	void AddChangeLogU8(const std::u8string& fmt, Args... args)
	{
		Math::Color color{ Def::Half, Def::FloatZero, Def::FloatOne, Def::FloatOne };
		AddLogU8(color, fmt, args...);
	}

	template<class... Args>
	void AddErrorLogU8(const std::u8string& fmt, Args... args)
	{
		Math::Color color{ Def::FloatOne, Def::FloatZero, Def::FloatZero, Def::FloatOne };
		AddLogU8(color, fmt, args...);
	}

	template<class... Args>
	void AddSuccessLogU8(const std::u8string& fmt, Args... args)
	{
		Math::Color color{ Def::FloatZero, Def::FloatOne, Def::Half, Def::FloatOne };
		AddLogU8(color, fmt, args...);
	}

	template<class... Args>
	void AddLogU8(const Math::Color& color, const std::u8string& fmt, Args... args)
	{
		auto u8buffer{ Str::FormatStringU8(fmt.c_str(), args...) };
		auto buffer{ FlChronus::now_iso8601() + " | " + Str::U8StringToStringSafe(u8buffer) };

		m_logEntries.push_back({ buffer, color });
		m_scrollToBottom = true;
	}

	void RenderLog(const std::string& title, bool* p_open = NULL, ImGuiWindowFlags flags = ImGuiWindowFlags_None);

	void SetFontScale(float scale) { m_fontScale = std::clamp(scale, m_minFontScale, m_maxFontScale); }
	void ResetFontScale() { m_fontScale = Def::FloatOne; }

	const auto GetFontScale() const { return m_fontScale; }

private:

	struct LogEntry
	{
		std::string text;
		Math::Color color;
	};

	void Clear() { m_logEntries.clear(); }
	void Copy();
	void ExportLog();

	std::list<LogEntry> m_logEntries;
	bool                m_scrollToBottom{ false };

	std::string			m_exportPath{"Assets/Data/Log/Framework"};

	float               m_fontScale{ Def::FloatOne };
	float               m_minFontScale{ Def::Half };
	float               m_maxFontScale{ Def::FloatOne + Def::FloatOne + Def::FloatOne };
};