#include "ShaderManager.h"

ComPtr<ID3DBlob> LoadCompiledShaderObject(const std::string& filePath)
{
    auto outBlob{ ComPtr<ID3DBlob>{} };
    auto wpath{ ansi_to_wide(filePath) };
    auto hr{ D3DReadFileToBlob(wpath.c_str(), outBlob.GetAddressOf()) };
    if (FAILED(hr)) return nullptr;
    return outBlob;
}

const std::string LatestShaderPath(const std::string& path) noexcept
{
    auto basePath{ std::filesystem::path{ path }.replace_extension("") };
    auto hlslPath{ basePath };
    hlslPath.replace_extension("hlsl");

    auto csoPath{ basePath };
    csoPath.replace_extension("cso");

    bool hlslExists{ std::filesystem::exists(hlslPath) };
    bool csoExists{ std::filesystem::exists(csoPath) };

    auto resultPath{ std::string{path} };

    if (hlslExists && csoExists)
    {
        auto hlslTime{ std::filesystem::last_write_time(hlslPath) };
        auto csoTime{ std::filesystem::last_write_time(csoPath) };

        // ÅV‚Ì•û‚ð‘I‘ð
        resultPath = (hlslTime > csoTime) ? hlslPath.string() : csoPath.string();
    }

    return resultPath;
}

const bool ShaderManager::Load(const std::string& path)
{
    auto shadPath{ LatestShaderPath(path) };

    if (Str::FileExtensionSearcher(shadPath) == "cso")
    {
        auto blob{ LoadCompiledShaderObject(shadPath) };
        if (!blob) return false;

        auto guid{ FlResourceAdministrator::Instance().GetMetaFileManager()->FindGuidByAsset(path).value() };
        m_resources[guid]  = std::make_shared<ComPtr<ID3DBlob>>();
        *m_resources[guid] = blob;

        FlEditorAdministrator::Instance().GetLogger()->AddSuccessLog("Success: Shader Loaded %s", shadPath.c_str());
        std::make_unique<FlMetaFileManager>()->IncrementLoadFlag(shadPath.c_str());

        return true;
    }

    auto include   { static_cast<ID3DInclude*>(D3D_COMPILE_STANDARD_FILE_INCLUDE) };
    auto flag      { UINT{D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION} };
    auto pErrorBlob{ static_cast<ID3DBlob*>(nullptr) };

    // <lambda Function>
    auto compile { [&](const char* profile, ComPtr<ID3DBlob>& outBlob)
        {
            auto fullPath { ansi_to_wide(shadPath) };
            auto hr { D3DCompileFromFile(fullPath.c_str(), nullptr, include, "main",
                profile, flag, NULL, &outBlob, &pErrorBlob) };

            // Optional Shader Stages (HS, DS, GS)
            if (FAILED(hr)) outBlob = nullptr;
            else
            {
                auto csoPath { std::filesystem::path{shadPath}.replace_extension("cso") };
                auto ofs     { std::ofstream(csoPath, std::ios::binary) };

                if (!ofs) return;

                ofs.write(static_cast<const char*>(outBlob->GetBufferPointer()),
                    outBlob->GetBufferSize());
            }
        }
    };
    // </lambda Function>

    auto guid{ FlResourceAdministrator::Instance().GetMetaFileManager()->FindGuidByAsset(shadPath).value() };

    m_resources[guid] = std::make_shared<ComPtr<ID3DBlob>>();

    if (Str::Contains(path, "VS"))
    {
        compile("vs_5_0", *m_resources[guid]);
        if (!*m_resources[guid])
        {
            assert(false && "Faild: VS Compiled");
            return false;
        }
    }

    else if (Str::Contains(path, "HS")) compile("hs_5_0", *m_resources[guid]);
    else if (Str::Contains(path, "DS")) compile("ds_5_0", *m_resources[guid]);
    else if (Str::Contains(path, "GS")) compile("gs_5_0", *m_resources[guid]);

    else if (Str::Contains(path, "PS"))
    {
        compile("ps_5_0", *m_resources[guid]);
        if (!*m_resources[guid])
        {
            assert(false && "Faild: PS Compiled");
            return false;
        }
    }
    else
    {
        assert(false && "Shader Stage Not Found");
        return false;
	}

    include = nullptr;
    if (pErrorBlob) pErrorBlob->Release();

    FlEditorAdministrator::Instance().GetLogger()->AddSuccessLog("Success: Shader Loaded %s", shadPath.c_str());
    std::make_unique<FlMetaFileManager>()->IncrementLoadFlag(shadPath.c_str());

    return true;
}