#pragma once

#include <d3d12.h>
#include <wrl.h>
#include <memory>
#include "KdTexture.h"
#include "Math/Color.h"

using Microsoft::WRL::ComPtr;

//===========================================
//
// レンダーターゲット変更
//
//===========================================
struct KdRenderTargetPack
{
	KdRenderTargetPack() {}

	void CreateRenderTarget(ID3D12Device* device, int width, int height, bool needDSV = false, DXGI_FORMAT format = DXGI_FORMAT_R8G8B8A8_UNORM, D3D12_VIEWPORT* pVP = nullptr);
	void SetRenderTarget(ID3D12GraphicsCommandList* commandList, std::shared_ptr<KdTexture> RTT, std::shared_ptr<KdTexture> DST = nullptr, D3D12_VIEWPORT* pVP = nullptr);

	void SetViewPort(ID3D12GraphicsCommandList* commandList, D3D12_VIEWPORT* pVP);

	void ClearTexture(ID3D12GraphicsCommandList* commandList, const Math::Color& fillColor = kBlueColor);

	std::shared_ptr<KdTexture> m_RTTexture = nullptr;
	std::shared_ptr<KdTexture> m_ZBuffer = nullptr;
	D3D12_VIEWPORT m_viewPort = { 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f };
};

struct KdRenderTargetChanger
{
	~KdRenderTargetChanger() { Release(); }

	ComPtr<ID3D12Resource> m_saveRTV;
	ComPtr<ID3D12Resource> m_saveDSV;
	D3D12_VIEWPORT			m_saveVP = {};
	UINT					m_numVP = 1;
	bool					m_changeVP = false;

	bool Validate(ID3D12Resource* pRTV);

	bool ChangeRenderTarget(ID3D12GraphicsCommandList* commandList, ID3D12Resource* pRTV, ID3D12Resource* pDSV = nullptr, D3D12_VIEWPORT* pVP = nullptr);
	bool ChangeRenderTarget(ID3D12GraphicsCommandList* commandList, std::shared_ptr<KdTexture> RTT, std::shared_ptr<KdTexture> DST = nullptr, D3D12_VIEWPORT* pVP = nullptr);
	bool ChangeRenderTarget(ID3D12GraphicsCommandList* commandList, KdRenderTargetPack& RTPack);

	void UndoRenderTarget(ID3D12GraphicsCommandList* commandList);

	void Release();
};

// KdRenderTargetPack メソッドの実装
void KdRenderTargetPack::CreateRenderTarget(ID3D12Device* device, int width, int height, bool needDSV, DXGI_FORMAT format, D3D12_VIEWPORT* pVP)
{
	// RenderTargetテクスチャの作成
	m_RTTexture = std::make_shared<KdTexture>();
	m_RTTexture->CreateRenderTarget(device, width, height, format);

	if (needDSV)
	{
		// DepthStencilテクスチャの作成
		m_ZBuffer = std::make_shared<KdTexture>();
		m_ZBuffer->CreateDepthStencil(device, width, height);
	}

	if (pVP)
	{
		m_viewPort = *pVP;
	}
	else
	{
		m_viewPort.TopLeftX = 0;
		m_viewPort.TopLeftY = 0;
		m_viewPort.Width = static_cast<float>(width);
		m_viewPort.Height = static_cast<float>(height);
		m_viewPort.MinDepth = 0.0f;
		m_viewPort.MaxDepth = 1.0f;
	}
}

void KdRenderTargetPack::SetRenderTarget(ID3D12GraphicsCommandList* commandList, std::shared_ptr<KdTexture> RTT, std::shared_ptr<KdTexture> DST, D3D12_VIEWPORT* pVP)
{
	// RenderTargetViewの設定
	D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle = RTT->GetRTVHandle();
	commandList->OMSetRenderTargets(1, &rtvHandle, FALSE, DST ? &DST->GetDSVHandle() : nullptr);

	// Viewportの設定
	if (pVP)
	{
		SetViewPort(commandList, pVP);
	}
	else
	{
		SetViewPort(commandList, &m_viewPort);
	}
}

void KdRenderTargetPack::SetViewPort(ID3D12GraphicsCommandList* commandList, D3D12_VIEWPORT* pVP)
{
	commandList->RSSetViewports(1, pVP);
}

void KdRenderTargetPack::ClearTexture(ID3D12GraphicsCommandList* commandList, const Math::Color& fillColor)
{
	// RenderTargetのクリア
	D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle = m_RTTexture->GetRTVHandle();
	commandList->ClearRenderTargetView(rtvHandle, fillColor, 0, nullptr);

	// DepthStencilのクリア
	if (m_ZBuffer)
	{
		D3D12_CPU_DESCRIPTOR_HANDLE dsvHandle = m_ZBuffer->GetDSVHandle();
		commandList->ClearDepthStencilView(dsvHandle, D3D12_CLEAR_FLAG_DEPTH, 1.0f, 0, 0, nullptr);
	}
}

// KdRenderTargetChanger メソッドの実装
bool KdRenderTargetChanger::Validate(ID3D12Resource* pRTV)
{
	return pRTV != nullptr;
}

bool KdRenderTargetChanger::ChangeRenderTarget(ID3D12GraphicsCommandList* commandList, ID3D12Resource* pRTV, ID3D12Resource* pDSV, D3D12_VIEWPORT* pVP)
{
	if (!Validate(pRTV)) return false;

	// 現在のRenderTargetとDepthStencilを保存
	m_saveRTV = pRTV;
	m_saveDSV = pDSV;

	// RenderTargetの変更
	D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle = pRTV->GetCPUDescriptorHandleForHeapStart();
	commandList->OMSetRenderTargets(1, &rtvHandle, FALSE, pDSV ? &pDSV->GetCPUDescriptorHandleForHeapStart() : nullptr);

	// Viewportの変更
	if (pVP)
	{
		m_saveVP = *pVP;
		m_changeVP = true;
		commandList->RSSetViewports(1, pVP);
	}
	else
	{
		m_changeVP = false;
	}

	return true;
}

bool KdRenderTargetChanger::ChangeRenderTarget(ID3D12GraphicsCommandList* commandList, std::shared_ptr<KdTexture> RTT, std::shared_ptr<KdTexture> DST, D3D12_VIEWPORT* pVP)
{
	return ChangeRenderTarget(commandList, RTT->GetResource(), DST ? DST->GetResource() : nullptr, pVP);
}

bool KdRenderTargetChanger::ChangeRenderTarget(ID3D12GraphicsCommandList* commandList, KdRenderTargetPack& RTPack)
{
	return ChangeRenderTarget(commandList, RTPack.m_RTTexture->GetResource(), RTPack.m_ZBuffer ? RTPack.m_ZBuffer->GetResource() : nullptr, &RTPack.m_viewPort);
}

void KdRenderTargetChanger::UndoRenderTarget(ID3D12GraphicsCommandList* commandList)
{
	if (m_saveRTV)
	{
		D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle = m_saveRTV->GetCPUDescriptorHandleForHeapStart();
		commandList->OMSetRenderTargets(1, &rtvHandle, FALSE, m_saveDSV ? &m_saveDSV->GetCPUDescriptorHandleForHeapStart() : nullptr);

		if (m_changeVP)
		{
			commandList->RSSetViewports(1, &m_saveVP);
		}
	}
}

void KdRenderTargetChanger::Release()
{
	m_saveRTV.Reset();
	m_saveDSV.Reset();
}
