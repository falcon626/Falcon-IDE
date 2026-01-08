#pragma once
#include "../../Math/FlTransform.hpp"

using entityId = uint32_t;

struct TransformComponent
{
    std::shared_ptr<FlTransform> m_transform;

    entityId m_parent{ UINT32_MAX };
    std::vector<entityId> m_children;
};