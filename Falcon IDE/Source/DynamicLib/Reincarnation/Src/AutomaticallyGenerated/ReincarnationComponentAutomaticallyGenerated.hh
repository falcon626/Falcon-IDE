#pragma once
typedef struct ReincarnationComponent
{
    Math::Vector3 m_move{ Def::Vec3 };
    Math::Vector3 m_start{ Def::Vec3 };
    float m_limit{ Def::FloatZero };
}ReincarnationComponent;
