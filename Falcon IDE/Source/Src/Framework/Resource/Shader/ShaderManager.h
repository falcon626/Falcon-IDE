#pragma once
#include "../BaseBasicResource/BaseBasicResourceManager.hpp"

class ShaderManager : public BaseBasicResourceManager<ComPtr<ID3DBlob>>
{
public:
	const bool Load(const std::string& path) override;
};