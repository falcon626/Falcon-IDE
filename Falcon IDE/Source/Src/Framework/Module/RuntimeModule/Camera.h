#pragma once

struct CameraComponent
{
    bool m_isEnable{ false };

    Math::Matrix m_mProj;
    Math::Matrix m_mView;

    Math::Vector2 m_aspect;
    Math::Vector2 m_clips;
    
    Math::Vector3 m_offset{ Def::Vec3 };

    int32_t m_targetId{ -Def::IntOne };

    float m_fov{ Def::Frame };
};