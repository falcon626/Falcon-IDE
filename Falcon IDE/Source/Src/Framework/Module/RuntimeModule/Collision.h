#pragma once
#include "../../Math/FlCollisionShape.h"
struct CollisionComponent
{
	FlCollisionShape m_collision;
	FlCollisionShape::Type m_collType{ FlCollisionShape::Type::None };

	Math::Vector3 m_boxSize;
	Math::Vector3 m_rayDir;
	float m_radius{ Def::FloatZero };

	bool m_isHit{ false };
};