#pragma once  
#include "../BaseBasicResource/BaseBasicResourceManager.hpp"

class ModelManager : public BaseBasicResourceManager<ModelData>  
{  
public:  

    const bool Load(const std::string& path) override;
};