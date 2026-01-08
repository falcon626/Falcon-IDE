#include "FlThumbnailGenerator.h"

#include <wrl.h>

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"

using namespace FlThumbnailGenerator;
using namespace Microsoft::WRL;
using namespace DirectX;

class com_exception : public std::exception
{
public:
    com_exception(const HRESULT hr) : result(hr) {}
    const char* what() const override
    {
        static char s_str[64]{ 0 };
        sprintf_s(s_str, "Failure with HRESULT of %08X", static_cast<unsigned int>(result));
        return s_str;
    }
private:
    HRESULT result;
};

static inline void ThrowIfFailed(const HRESULT hr)
{
    if (FAILED(hr)) throw com_exception(hr);
}

static const char* vertexShaderSource = R"(
    cbuffer Camera : register(b0) {
        float4x4 viewProj;
    };
    struct VSInput {
        float3 position : POSITION;
        float3 normal : NORMAL;
    };
    struct VSOutput {
        float4 position : SV_POSITION;
        float3 normal : NORMAL;
    };
    VSOutput main(VSInput input) {
        VSOutput output;
        output.position = mul(float4(input.position, 1.0f), viewProj);
        output.normal = input.normal;
        return output;
    }
)";

static const char* pixelShaderSource = R"(
    struct PSInput {
        float4 position : SV_POSITION;
        float3 normal : NORMAL;
    };
    float4 main(PSInput input) : SV_TARGET {
        float3 lightDir = normalize(float3(1.0f, 1.0f, -1.0f));
        float diffuse = max(dot(input.normal, lightDir), 0.2f);
        return float4(diffuse, diffuse, diffuse, 1.0f);
    }
)";

struct Vertex {
    XMFLOAT3 position;
    XMFLOAT3 normal;
};

ThumbnailResult FlThumbnailGenerator::GenerateThumbnail(const ThumbnailConfig& config)
{
    ThumbnailResult result = { false, "" };
    try {
        // DX12デバイスの初期化
        ComPtr<ID3D12Device> device;
        ComPtr<IDXGIFactory4> factory;
        ComPtr<ID3D12CommandQueue> commandQueue;
        ComPtr<ID3D12CommandAllocator> commandAllocator;
        ComPtr<ID3D12GraphicsCommandList> commandList;
        ComPtr<ID3D12Fence> fence;
        UINT64 fenceValue = 0;
        HANDLE fenceEvent = CreateEvent(nullptr, FALSE, FALSE, nullptr);

        ThrowIfFailed(CreateDXGIFactory1(IID_PPV_ARGS(&factory)));
        ThrowIfFailed(D3D12CreateDevice(nullptr, D3D_FEATURE_LEVEL_11_0, IID_PPV_ARGS(&device)));

        D3D12_COMMAND_QUEUE_DESC queueDesc = {};
        queueDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;
        ThrowIfFailed(device->CreateCommandQueue(&queueDesc, IID_PPV_ARGS(&commandQueue)));
        ThrowIfFailed(device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&commandAllocator)));
        ThrowIfFailed(device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, commandAllocator.Get(), nullptr, IID_PPV_ARGS(&commandList)));
        commandList->Close();
        ThrowIfFailed(device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&fence)));
        fenceValue = 0;

        // Assimpでモデルをロード
        Assimp::Importer importer;
        const aiScene* scene = importer.ReadFile(config.modelPath, aiProcess_Triangulate | aiProcess_GenNormals);
        if (!scene || !scene->HasMeshes()) {
            throw std::runtime_error("Failed to load model: " + config.modelPath);
        }

        // 頂点データの抽出
        std::vector<Vertex> vertices;
        std::vector<uint32_t> indices;
        for (unsigned int i = 0; i < scene->mNumMeshes; ++i) {
            const aiMesh* mesh = scene->mMeshes[i];
            for (unsigned int j = 0; j < mesh->mNumVertices; ++j) {
                Vertex vertex{};
                vertex.position = { mesh->mVertices[j].x, mesh->mVertices[j].y, mesh->mVertices[j].z };
                vertex.normal = { mesh->mNormals[j].x, mesh->mNormals[j].y, mesh->mNormals[j].z };
                vertices.push_back(vertex);
            }
            for (unsigned int j = 0; j < mesh->mNumFaces; ++j) {
                const aiFace& face = mesh->mFaces[j];
                for (unsigned int k = 0; k < face.mNumIndices; ++k) {
                    indices.push_back(face.mIndices[k]);
                }
            }
        }

        // チェック: 頂点とインデックスが存在するか
        if (vertices.empty()) {
            throw std::runtime_error("No vertices in the model");
        }
        if (indices.empty()) {
            throw std::runtime_error("No indices in the model");
        }

        // 頂点バッファとインデックスバッファの作成
        ComPtr<ID3D12Resource> vertexBuffer;
        ComPtr<ID3D12Resource> indexBuffer;
        {
            CD3DX12_HEAP_PROPERTIES uploadHeapProps(D3D12_HEAP_TYPE_UPLOAD);

            const UINT64 vertexBufferSize = vertices.size() * sizeof(Vertex);
            CD3DX12_RESOURCE_DESC vertexBufferDesc = CD3DX12_RESOURCE_DESC::Buffer(vertexBufferSize);

            ThrowIfFailed(device->CreateCommittedResource(
                &uploadHeapProps,
                D3D12_HEAP_FLAG_NONE,
                &vertexBufferDesc,
                D3D12_RESOURCE_STATE_GENERIC_READ,
                nullptr,
                IID_PPV_ARGS(&vertexBuffer)
            ));

            // マッピング処理は変更なし
            void* vData; // Vertex* から void* へ変更するとより汎用的
            ThrowIfFailed(vertexBuffer->Map(0, nullptr, &vData));
            memcpy(vData, vertices.data(), vertexBufferSize);
            vertexBuffer->Unmap(0, nullptr);

            // --- インデックスバッファも同様に修正 ---
            const UINT64 indexBufferSize = indices.size() * sizeof(uint32_t);
            CD3DX12_RESOURCE_DESC indexBufferDesc = CD3DX12_RESOURCE_DESC::Buffer(indexBufferSize);

            ThrowIfFailed(device->CreateCommittedResource(
                &uploadHeapProps, // 同じものを再利用
                D3D12_HEAP_FLAG_NONE,
                &indexBufferDesc,
                D3D12_RESOURCE_STATE_GENERIC_READ,
                nullptr,
                IID_PPV_ARGS(&indexBuffer)
            ));

            // マッピング処理は変更なし
            void* iData;
            ThrowIfFailed(indexBuffer->Map(0, nullptr, &iData));
            memcpy(iData, indices.data(), indexBufferSize);
            indexBuffer->Unmap(0, nullptr);
        }

        // レンダーターゲットの作成
        ComPtr<ID3D12Resource> renderTarget;
        {
            CD3DX12_HEAP_PROPERTIES heapProps(D3D12_HEAP_TYPE_DEFAULT);

            CD3DX12_RESOURCE_DESC rtDesc = CD3DX12_RESOURCE_DESC::Tex2D(
                DXGI_FORMAT_R8G8B8A8_UNORM,
                config.width,
                config.height,
                1, // ArraySize
                1, // MipLevels
                1, // SampleCount
                0, // SampleQuality
                D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET
            );

            // 最適化クリア値の設定
            D3D12_CLEAR_VALUE clearValue = {};
            clearValue.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
            clearValue.Color[0] = 0.0f;
            clearValue.Color[1] = 0.0f;
            clearValue.Color[2] = 0.0f;
            clearValue.Color[3] = 1.0f;

            ThrowIfFailed(device->CreateCommittedResource(
                &heapProps,
                D3D12_HEAP_FLAG_NONE,
                &rtDesc,
                D3D12_RESOURCE_STATE_RENDER_TARGET, // 初期状態はレンダーターゲット
                &clearValue,
                IID_PPV_ARGS(&renderTarget)
            ));
        }

        // シェーダーのコンパイル
        ComPtr<ID3DBlob> vertexShader, pixelShader;
        ComPtr<ID3DBlob> errorBlob;
        {
            UINT compileFlags = D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION;
            ThrowIfFailed(D3DCompile(vertexShaderSource, strlen(vertexShaderSource), nullptr, nullptr, nullptr, "main", "vs_5_0", compileFlags, 0, &vertexShader, &errorBlob));
            ThrowIfFailed(D3DCompile(pixelShaderSource, strlen(pixelShaderSource), nullptr, nullptr, nullptr, "main", "ps_5_0", compileFlags, 0, &pixelShader, &errorBlob));
        }

        // ルートシグネチャの作成
        ComPtr<ID3D12RootSignature> rootSignature;
        {
            D3D12_DESCRIPTOR_RANGE descRange = {};
            descRange.RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_CBV;
            descRange.NumDescriptors = 1;
            descRange.BaseShaderRegister = 0;
            descRange.RegisterSpace = 0;

            D3D12_ROOT_PARAMETER rootParam = {};
            rootParam.ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
            rootParam.DescriptorTable.pDescriptorRanges = &descRange;
            rootParam.DescriptorTable.NumDescriptorRanges = 1;

            D3D12_ROOT_SIGNATURE_DESC rootSigDesc = {};
            rootSigDesc.NumParameters = 1;
            rootSigDesc.pParameters = &rootParam;
            rootSigDesc.Flags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;

            ComPtr<ID3DBlob> signatureBlob;
            ThrowIfFailed(D3D12SerializeRootSignature(&rootSigDesc, D3D_ROOT_SIGNATURE_VERSION_1, &signatureBlob, &errorBlob));
            ThrowIfFailed(device->CreateRootSignature(0, signatureBlob->GetBufferPointer(), signatureBlob->GetBufferSize(), IID_PPV_ARGS(&rootSignature)));
        }

        // PSOの作成
        ComPtr<ID3D12PipelineState> pipelineState;
        {
            D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc = {};
            psoDesc.VS = { vertexShader->GetBufferPointer(), vertexShader->GetBufferSize() };
            psoDesc.PS = { pixelShader->GetBufferPointer(), pixelShader->GetBufferSize() };
            psoDesc.RasterizerState = { D3D12_FILL_MODE_SOLID, D3D12_CULL_MODE_BACK };
            psoDesc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
            psoDesc.DepthStencilState = CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);
            psoDesc.SampleMask = UINT_MAX;
            psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
            psoDesc.NumRenderTargets = 1;
            psoDesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;
            psoDesc.DSVFormat = DXGI_FORMAT_D32_FLOAT;
            psoDesc.SampleDesc.Count = 1;
            psoDesc.pRootSignature = rootSignature.Get();

            D3D12_INPUT_ELEMENT_DESC inputElements[] = {
                { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
                { "NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 12, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 }
            };
            psoDesc.InputLayout = { inputElements, _countof(inputElements) };
            ThrowIfFailed(device->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&pipelineState)));
        }

        // カメラとビューポート設定
        XMMATRIX view = XMMatrixLookAtLH(
            XMVectorSet(0.0f, 5.0f, config.cameraDistance, 0.0f),
            XMVectorSet(0.0f, 0.0f, 0.0f, 0.0f),
            XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f)
        );
        XMMATRIX proj = XMMatrixPerspectiveFovLH(XM_PIDIV4, (float)config.width / config.height, 0.1f, 100.0f);
        XMMATRIX viewProj = XMMatrixMultiply(view, proj);

        // コンスタントバッファの作成
        ComPtr<ID3D12Resource> constantBuffer;
        {
            CD3DX12_HEAP_PROPERTIES heapProps(D3D12_HEAP_TYPE_UPLOAD);

            const UINT constantBufferSize = (sizeof(XMMATRIX) + 255) & ~255;
            CD3DX12_RESOURCE_DESC cbDesc = CD3DX12_RESOURCE_DESC::Buffer(constantBufferSize);

            ThrowIfFailed(device->CreateCommittedResource(&heapProps, D3D12_HEAP_FLAG_NONE, &cbDesc,
                D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(&constantBuffer)));

            XMMATRIX* cbData{};
            ThrowIfFailed(constantBuffer->Map(0, nullptr, reinterpret_cast<void**>(&cbData)));
            *cbData = viewProj;
            constantBuffer->Unmap(0, nullptr);
        }

        // デプスバッファの作成
        ComPtr<ID3D12Resource> depthBuffer;
        {
            CD3DX12_HEAP_PROPERTIES heapProps(D3D12_HEAP_TYPE_DEFAULT);

            CD3DX12_RESOURCE_DESC depthDesc = CD3DX12_RESOURCE_DESC::Tex2D(
                DXGI_FORMAT_D32_FLOAT,
                config.width,
                config.height,
                1, 0, 1, 0,
                D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL
            );

            D3D12_CLEAR_VALUE depthClear = { DXGI_FORMAT_D32_FLOAT, { 1.0f, 0 } };
            ThrowIfFailed(device->CreateCommittedResource(&heapProps, D3D12_HEAP_FLAG_NONE, &depthDesc,
                D3D12_RESOURCE_STATE_DEPTH_WRITE, &depthClear, IID_PPV_ARGS(&depthBuffer)));
        }

        // ディスクリプタヒープの作成
        ComPtr<ID3D12DescriptorHeap> rtvHeap, dsvHeap, cbvHeap;
        {
            D3D12_DESCRIPTOR_HEAP_DESC rtvHeapDesc = { D3D12_DESCRIPTOR_HEAP_TYPE_RTV, 1 };
            ThrowIfFailed(device->CreateDescriptorHeap(&rtvHeapDesc, IID_PPV_ARGS(&rtvHeap)));
            D3D12_DESCRIPTOR_HEAP_DESC dsvHeapDesc = { D3D12_DESCRIPTOR_HEAP_TYPE_DSV, 1 };
            ThrowIfFailed(device->CreateDescriptorHeap(&dsvHeapDesc, IID_PPV_ARGS(&dsvHeap)));

            D3D12_RENDER_TARGET_VIEW_DESC rtvDesc = {};
            rtvDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
            rtvDesc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D;
            device->CreateRenderTargetView(renderTarget.Get(), &rtvDesc, rtvHeap->GetCPUDescriptorHandleForHeapStart());

            D3D12_DEPTH_STENCIL_VIEW_DESC dsvDesc = {};
            dsvDesc.Format = DXGI_FORMAT_D32_FLOAT;
            dsvDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;
            device->CreateDepthStencilView(depthBuffer.Get(), &dsvDesc, dsvHeap->GetCPUDescriptorHandleForHeapStart());

            D3D12_DESCRIPTOR_HEAP_DESC cbvHeapDesc = { D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, 1, D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE };
            ThrowIfFailed(device->CreateDescriptorHeap(&cbvHeapDesc, IID_PPV_ARGS(&cbvHeap)));

            D3D12_CONSTANT_BUFFER_VIEW_DESC cbvDesc = {};
            cbvDesc.BufferLocation = constantBuffer->GetGPUVirtualAddress();
            cbvDesc.SizeInBytes = (sizeof(XMMATRIX) + 255) & ~255;
            device->CreateConstantBufferView(&cbvDesc, cbvHeap->GetCPUDescriptorHandleForHeapStart());
        }

        // レンダリングコマンドの記録
        // コマンドリストをリセットして記録開始
        ThrowIfFailed(commandAllocator->Reset());
        ThrowIfFailed(commandList->Reset(commandAllocator.Get(), pipelineState.Get()));

        // レンダリングコマンドの記録
        D3D12_VIEWPORT viewport = { 0.0f, 0.0f, (float)config.width, (float)config.height, 0.0f, 1.0f };
        D3D12_RECT scissorRect = { 0, 0, (LONG)config.width, (LONG)config.height };
        commandList->SetGraphicsRootSignature(rootSignature.Get());
        commandList->RSSetViewports(1, &viewport);
        commandList->RSSetScissorRects(1, &scissorRect);

        ID3D12DescriptorHeap* ppHeaps[] = { cbvHeap.Get() };
        commandList->SetDescriptorHeaps(_countof(ppHeaps), ppHeaps);

        D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle = rtvHeap->GetCPUDescriptorHandleForHeapStart();
        D3D12_CPU_DESCRIPTOR_HANDLE dsvHandle = dsvHeap->GetCPUDescriptorHandleForHeapStart();
        commandList->OMSetRenderTargets(1, &rtvHandle, FALSE, &dsvHandle);

        const float clearColor[] = { 0.1f, 0.1f, 0.1f, 1.0f };
        commandList->ClearRenderTargetView(rtvHandle, clearColor, 0, nullptr);
        commandList->ClearDepthStencilView(dsvHandle, D3D12_CLEAR_FLAG_DEPTH, 1.0f, 0, 0, nullptr);

        commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

        D3D12_VERTEX_BUFFER_VIEW vbv = {};
        vbv.BufferLocation = vertexBuffer->GetGPUVirtualAddress();
        vbv.SizeInBytes = static_cast<UINT>(vertices.size() * sizeof(Vertex));
        vbv.StrideInBytes = sizeof(Vertex);
        D3D12_INDEX_BUFFER_VIEW ibv = {};
        ibv.BufferLocation = indexBuffer->GetGPUVirtualAddress();
        ibv.SizeInBytes = static_cast<UINT>(indices.size() * sizeof(uint32_t));
        ibv.Format = DXGI_FORMAT_R32_UINT;

        commandList->IASetVertexBuffers(0, 1, &vbv);
        commandList->IASetIndexBuffer(&ibv);
        commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

        commandList->SetGraphicsRootDescriptorTable(0, cbvHeap->GetGPUDescriptorHandleForHeapStart());
        commandList->DrawIndexedInstanced((UINT)indices.size(), 1, 0, 0, 0);

        // レンダーターゲットをコピーソースに遷移
        CD3DX12_RESOURCE_BARRIER barrier = CD3DX12_RESOURCE_BARRIER::Transition(renderTarget.Get(), D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_COPY_SOURCE);
        commandList->ResourceBarrier(1, &barrier);

        ThrowIfFailed(commandList->Close());

        // コマンドの実行
        ID3D12CommandList* cmdLists[] = { commandList.Get() };
        commandQueue->ExecuteCommandLists(_countof(cmdLists), cmdLists);

        // 描画完了まで待機
        UINT64 currentFenceValue = ++fenceValue;
        ThrowIfFailed(commandQueue->Signal(fence.Get(), currentFenceValue));
        if (fence->GetCompletedValue() < currentFenceValue)
        {
            ThrowIfFailed(fence->SetEventOnCompletion(currentFenceValue, fenceEvent));
            WaitForSingleObject(fenceEvent, INFINITE);
        }

        // レンダーターゲットからデータを取得
        ComPtr<ID3D12Resource> readbackBuffer;
        {
            // リードバック用バッファはUPLOAD/DEFAULTヒープとは異なるREADBACKヒープに作成
            CD3DX12_HEAP_PROPERTIES heapProps(D3D12_HEAP_TYPE_READBACK);

            // コピー先バッファのサイズとアライメントを計算
            D3D12_PLACED_SUBRESOURCE_FOOTPRINT footprint = {};
            UINT64 rowPitch, totalBytes;
            D3D12_RESOURCE_DESC desc = { renderTarget->GetDesc() };
            device->GetCopyableFootprints(&desc, 0, 1, 0, &footprint, nullptr, &rowPitch, &totalBytes);

            CD3DX12_RESOURCE_DESC bufferDesc = CD3DX12_RESOURCE_DESC::Buffer(totalBytes);

            ThrowIfFailed(device->CreateCommittedResource(
                &heapProps,
                D3D12_HEAP_FLAG_NONE,
                &bufferDesc,
                D3D12_RESOURCE_STATE_COPY_DEST, // リードバックバッファは常にこの状態で作成
                nullptr,
                IID_PPV_ARGS(&readbackBuffer)
            ));

            // コマンドリストをリセットしてコピーコマンドを記録
            ThrowIfFailed(commandAllocator->Reset());
            ThrowIfFailed(commandList->Reset(commandAllocator.Get(), nullptr));

            CD3DX12_TEXTURE_COPY_LOCATION srcLoc(renderTarget.Get(), 0);
            CD3DX12_TEXTURE_COPY_LOCATION dstLoc(readbackBuffer.Get(), footprint);

            commandList->CopyTextureRegion(&dstLoc, 0, 0, 0, &srcLoc, nullptr);

            // レンダーターゲットを元の状態に戻す（お作法）
            barrier = CD3DX12_RESOURCE_BARRIER::Transition(renderTarget.Get(), D3D12_RESOURCE_STATE_COPY_SOURCE, D3D12_RESOURCE_STATE_RENDER_TARGET);
            commandList->ResourceBarrier(1, &barrier);

            ThrowIfFailed(commandList->Close());

            // コピーコマンドを実行
            ID3D12CommandList* copyCmdLists[] = { commandList.Get() };
            commandQueue->ExecuteCommandLists(_countof(copyCmdLists), copyCmdLists);

            // コピー完了まで待機
            currentFenceValue = ++fenceValue;
            ThrowIfFailed(commandQueue->Signal(fence.Get(), currentFenceValue));
            if (fence->GetCompletedValue() < currentFenceValue)
            {
                ThrowIfFailed(fence->SetEventOnCompletion(currentFenceValue, fenceEvent));
                WaitForSingleObject(fenceEvent, INFINITE);
            }
        }

        // 画像データの保存
        {
            uint8_t* pixelData{};
            D3D12_RANGE readRange = { 0, (SIZE_T)(config.width * config.height * 4) }; // マップする範囲は画像のサイズだけで十分
            ThrowIfFailed(readbackBuffer->Map(0, &readRange, reinterpret_cast<void**>(&pixelData)));

            stbi_write_png(config.outputPath.c_str(), config.width, config.height, 4, pixelData, config.width * 4);

            // マップ解除時は範囲をnullptrにするのが一般的
            readbackBuffer->Unmap(0, nullptr);
        }

        CloseHandle(fenceEvent);
        result.success = true;
    }
    catch (const std::exception& e) {
        result.errorMessage = e.what();
    }
    return result;
}