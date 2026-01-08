#pragma once
#include "../BaseBasicResource/BaseBasicResourceManager.hpp"

class FlTextureManager : public BaseBasicResourceManager<Texture>
{
public:
	const bool Load(const std::string& path) override;
};