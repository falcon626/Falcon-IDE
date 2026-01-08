#pragma once

class FlEditorCamera
{
public:
	
	FlEditorCamera(const float width, const float height);

	void RenderCameraParameter(const std::string& title, bool* p_open = NULL, ImGuiWindowFlags flags = ImGuiWindowFlags_None);

	void Update();

	const auto IsEnable() const noexcept { return m_isEnable; }
private:

	CBufferData::Camera m_cameraData;

	std::unique_ptr<FlTransform> m_upTransform;

	Math::Vector2 m_clips;
	Math::Vector2 m_aspect;

	Math::Vector3 m_pos;
	Math::Vector3 m_eulerDegrees;

	Math::Matrix m_mView;
	Math::Matrix m_mProj;

	float m_moveSpeed;
	float m_rotSpeed;

	float m_fov;
	bool m_isDirty;
	bool m_isEnable;
};