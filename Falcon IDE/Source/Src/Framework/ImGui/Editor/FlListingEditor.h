#pragma once

class FlListingEditor
{
public:
    void RenderListingViewer(const std::string& title, bool* p_open = NULL, ImGuiWindowFlags flags = ImGuiWindowFlags_None);
private:
    void ListingGuidAssets(bool isAssetsAndGuid, bool isGuidOnly, bool isAssetsOnly);

    ImGuiTextFilter m_filter;

    bool m_isOption{ false };

    enum ListingType : uint32_t
    {
        None = Def::BitMaskPos0,
        AssetsListing = Def::BitMaskPos1,
            GuidAndAssets = Def::BitMaskPos2,
            Guid = Def::BitMaskPos3,
            Assets = Def::BitMaskPos4,

        Max
    };

    uint32_t m_listingTypes{ GuidAndAssets };
};