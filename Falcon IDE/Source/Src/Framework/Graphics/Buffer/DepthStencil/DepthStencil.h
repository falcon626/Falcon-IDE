#pragma once

enum class DepthStencilFormat
{
	DepthLowQuality = DXGI_FORMAT_R16_TYPELESS,
	DepthHighQuality = DXGI_FORMAT_R32_TYPELESS,
	DepthHighQualityAndStencil = DXGI_FORMAT_R24G8_TYPELESS,
};

class DepthStencil :public Buffer
{
public:

	// <Create:深度テクスチャ作成>　<Arg:GraphicsDeviceのポインタ,解像度,深度テクスチャのフォーマット>
	bool Create(GraphicsDevice* pGraphicsDevice, const Math::Vector2& resolution,
		DepthStencilFormat format = DepthStencilFormat::DepthHighQuality);

	// <Clear:深度バッファを初期化>
	void ClearBuffer();

	// <Getter:DSV番号>
	inline const auto GetDSVNumber() const noexcept { return m_dsvNumber;   }
	inline const auto GetSRVNumber() const noexcept { return m_dsSrvNumber; }

private:
	uint32_t m_dsvNumber{ Def::UIntZero };
	int32_t m_dsSrvNumber{ Def::IntZero };
};