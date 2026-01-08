#include "FlListingEditor.h"

void FlListingEditor::RenderListingViewer(const std::string& title, bool* p_open, ImGuiWindowFlags flags)
{
    ImGui::Begin(title.c_str(), p_open, flags);

    m_filter.Draw("Filter (type to search)", 200.0f);
    ImGui::SameLine();
    if (ImGui::Button("Option")) m_isOption = !m_isOption;
    ImGui::Separator();

    if (m_isOption)
    {
        bool checked1{ (m_listingTypes & GuidAndAssets) == GuidAndAssets }; 
        if (ImGui::Checkbox("GuidAndAssets", &checked1))
        {
            if (checked1) m_listingTypes |= GuidAndAssets;
            else m_listingTypes &= ~GuidAndAssets;
        }
        bool checked2{ (m_listingTypes & Guid) != None };
        if (ImGui::Checkbox("Guid Only", &checked2))
        {
            if (checked2) m_listingTypes |= Guid;
            else m_listingTypes &= ~Guid;
        }
        bool checked3{ (m_listingTypes & Assets) != None };
        if (ImGui::Checkbox("Assets Only", &checked3))
        {
            if (checked3) m_listingTypes |= Assets;
            else m_listingTypes &= ~Assets;
        }
        if (checked1 || (!checked2 && !checked3)) m_listingTypes = GuidAndAssets;
        if (checked1 && checked2) m_listingTypes = Guid;
        if (checked1 && checked3) m_listingTypes = Assets;
    }
    else
    {
        ListingGuidAssets((m_listingTypes & GuidAndAssets), (m_listingTypes & Guid), (m_listingTypes & Assets));
    }

    ImGui::End();
}

void FlListingEditor::ListingGuidAssets(bool isAssetsAndGuid, bool isGuidOnly, bool isAssetsOnly)
{
    auto guidMap{ FlResourceAdministrator::Instance().GetMetaFileManager()->GetGuidMap() };

    // テーブルとして表示
    if (isAssetsAndGuid)
    {
        if (ImGui::BeginTable("MapTable", 2, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg | ImGuiTableFlags_Resizable))
        {
            ImGui::TableSetupColumn("Key");
            ImGui::TableSetupColumn("Value");
            ImGui::TableHeadersRow();

            for (const auto& [key, value] : guidMap) {
                // フィルタに一致しない場合はスキップ
                std::string combined = key + " " + value;
                if (!m_filter.PassFilter(combined.c_str())) continue;

                ImGui::TableNextRow();
                ImGui::TableSetColumnIndex(0);
                ImGui::TextUnformatted(key.c_str());

                ImGui::TableSetColumnIndex(1);
                ImGui::TextUnformatted(value.c_str());
            }

            ImGui::EndTable();
        }
    }

    else if (isGuidOnly)
    {
        if (ImGui::BeginTable("MapTable", 1, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg | ImGuiTableFlags_Resizable))
        {

            ImGui::TableSetupColumn("Key");
            ImGui::TableHeadersRow();

            for (const auto& pair : guidMap) {
                if (!m_filter.PassFilter(pair.first.c_str())) continue;

                ImGui::TableNextRow();
                ImGui::TableSetColumnIndex(0);
                ImGui::TextUnformatted(pair.first.c_str());
            }

            ImGui::EndTable();
        }
    }

    else if (isAssetsOnly)
    {
        if (ImGui::BeginTable("MapTable", 1, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg | ImGuiTableFlags_Resizable))
        {
            ImGui::TableSetupColumn("Value");
            ImGui::TableHeadersRow();

            for (const auto& pair : guidMap) {
                if (!m_filter.PassFilter(pair.second.c_str())) continue;

                ImGui::TableNextRow();
                ImGui::TableSetColumnIndex(0);
                ImGui::TextUnformatted(pair.second.c_str());
            }

            ImGui::EndTable();
        }
    }
}
