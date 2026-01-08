#pragma once

class Buffer
{
public:
	Buffer() {}
	virtual ~Buffer() {}

	auto* GetResource() const noexcept { return m_pBuffer.Get(); }

protected:
	GraphicsDevice* m_pGraphicsDevice = nullptr;
	ComPtr<ID3D12Resource> m_pBuffer = nullptr;
};