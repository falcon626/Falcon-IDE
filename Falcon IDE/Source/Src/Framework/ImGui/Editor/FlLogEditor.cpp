#include "FlLogEditor.h"

void FlLogEditor::RenderLog(const std::string& title, bool* p_opened, ImGuiWindowFlags flags)
{
	ImGui::Begin(title.c_str(), p_opened, flags);

	if (ImGui::Button("Clear")) Clear();
	ImGui::SameLine();
	if (ImGui::Button("Copy")) Copy();
	ImGui::SameLine();
	if (ImGui::Button("Export")) ExportLog();
	ImGui::SameLine();
	ImGui::InputText(".log  Export File Path", &m_exportPath);

    ImGui::Separator();

    ImGui::Text("Font Size:");
    ImGui::SameLine();
    if (ImGui::Button("-##FontDown") && m_fontScale > m_minFontScale) m_fontScale = std::max(m_fontScale - 0.1f, m_minFontScale);
    ImGui::SameLine();
    if (ImGui::Button("+##FontUp") && m_fontScale < m_maxFontScale) m_fontScale = std::min(m_fontScale + 0.1f, m_maxFontScale);
    ImGui::SameLine();
    ImGui::SliderFloat("##FontScale", &m_fontScale, m_minFontScale, m_maxFontScale, "%.1fx");
    ImGui::SameLine();
    if (ImGui::Button("Reset##FontReset")) ResetFontScale();

    ImGui::Separator();

    decltype(auto) io   { ImGui::GetIO() };
    decltype(auto) style{ ImGui::GetStyle() };

    auto originalFontScale{ io.FontGlobalScale };
    auto originalStyle    { style };

    // スケーリング適用
    io.FontGlobalScale = m_fontScale;
    if (m_fontScale != Def::FloatOne) style.ScaleAllSizes(m_fontScale);

    ImGui::BeginChild("LogScroll", ImVec2(Def::Vec2.x, Def::Vec2.y), false, ImGuiWindowFlags_HorizontalScrollbar);

    for (const auto& entry : m_logEntries)
    {
        const auto imCol{ ImVec4{entry.color.R(), entry.color.G(), entry.color.B(), entry.color.A()} };

        ImGui::PushStyleColor(ImGuiCol_Text, imCol);
        ImGui::TextUnformatted(entry.text.c_str());
        ImGui::PopStyleColor();
    }

    if (m_scrollToBottom) ImGui::SetScrollHereY(Def::FloatOne);
    m_scrollToBottom   = false;

    ImGui::EndChild();

    io.FontGlobalScale = originalFontScale;
    style              = originalStyle;

    ImGui::End();
}

void FlLogEditor::Copy()
{
	ImGui::LogToClipboard();

	AddLog("Copy: Log to Clipboard");
}

void FlLogEditor::ExportLog()
{
	if (m_logEntries.empty()) 
	{
		AddWarningLog("Warning: Log is empty: Nothing to export");
		return;
	}

	if (m_exportPath.empty())
	{
		AddErrorLog("Error: Export path cannot be empty");
		return;
	}
	auto path{ m_exportPath + ".log" };

	auto upDebLogger{ std::make_unique<DebugLogger>(path) };

	for (const auto& logText : m_logEntries) {
		DEBUG_LOG(upDebLogger, logText.text.c_str());
	}

	DEBUG_LOG(upDebLogger, "-------------------------------------------");

	if (!upDebLogger->IsOpen()) 
		AddErrorLog("Error: Could not open log file %s", path.c_str());
	else 
		AddSuccessLog("Export successful: Path %s", path.c_str());
}