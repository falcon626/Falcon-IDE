#pragma once
typedef struct PlayerComponent
{
    bool m_isEnable{ true };
    float m_speed{ 0.01f };
    Math::Vector3 m_repopPos;
}PlayerComponent;
