#include "FlEditorCamera.h"

FlEditorCamera::FlEditorCamera(const float width, const float height)
	: m_upTransform{ std::make_unique<FlTransform>() }
	, m_clips	   { 0.01f,1000.0f }
	, m_aspect	   { width,height }
	, m_moveSpeed  { 0.1f }
	, m_rotSpeed   { 0.2f }
	, m_fov		   { 60.0f }
	, m_isDirty	   { true }
	, m_isEnable   { true }
{
	auto currentEuler{ m_upTransform->GetLocalRotation().ToEuler() };

	m_eulerDegrees.x = DirectX::XMConvertToDegrees(currentEuler.x);
	m_eulerDegrees.y = DirectX::XMConvertToDegrees(currentEuler.y);
	m_eulerDegrees.z = DirectX::XMConvertToDegrees(currentEuler.z);
}

void FlEditorCamera::RenderCameraParameter(const std::string& title, bool* p_open, ImGuiWindowFlags flags)
{
	if(ImGui::Begin(title.c_str(), p_open, flags))
	{
		ImGui::Checkbox("Enable", &m_isEnable);

		if (ImGui::DragFloat("Speed", &m_moveSpeed, 0.001f, Def::FloatZero))
			if (m_moveSpeed < Def::FloatZero) m_moveSpeed = Def::FloatZero;
		if (ImGui::DragFloat("Turning force", &m_rotSpeed, 0.001f, Def::FloatZero))
			if (m_rotSpeed < Def::FloatZero)m_rotSpeed = Def::FloatZero;

		if (ImGui::DragFloat("FOV", &m_fov, 1.0f, 1.0f, 360.0f)) m_isDirty = true;
		if (ImGui::DragFloat2("Clips", &m_clips.x, 0.01f, 0.01f, 1000.0f))
		{
			if (m_clips.x > m_clips.y) std::swap(m_clips.x, m_clips.y);
			m_isDirty = true;
		}
		if (ImGui::DragFloat2("Aspect", &m_aspect.x, 1.0f, 0.0f, 5000.0f)) m_isDirty = true;
		ImGui::Text("Aspect Ratio: %f", (m_aspect.x / m_aspect.y));

		if (ImGui::DragFloat3("Position", &m_pos.x, 0.1f))
		{
			m_upTransform->SetLocalPosition(m_pos);
			m_isDirty = true;
		}
		if (ImGui::DragFloat3("Rotation", &m_eulerDegrees.x, 1.0f))
		{
			{
				auto normalizeAngle{ [](float angle) -> float {
					while (angle > 180.0f) angle -= 360.0f;
					while (angle < -180.0f) angle += 360.0f;
					return angle;
					} };

				m_eulerDegrees.x = normalizeAngle(m_eulerDegrees.x);
				m_eulerDegrees.y = normalizeAngle(m_eulerDegrees.y);
				m_eulerDegrees.z = normalizeAngle(m_eulerDegrees.z);
			}

			auto eulerRadians{ Math::Vector3(
				DirectX::XMConvertToRadians(m_eulerDegrees.x),
				DirectX::XMConvertToRadians(m_eulerDegrees.y),
				DirectX::XMConvertToRadians(m_eulerDegrees.z)
			) };

			auto newQuat{
				Math::Quaternion::CreateFromYawPitchRoll(
					eulerRadians.y, eulerRadians.x, eulerRadians.z) };

			newQuat.Normalize();
			m_upTransform->SetWorldRotation(newQuat);

			m_isDirty = true;
		}

		if (ImGui::IsMouseDown(ImGuiMouseButton_Right))
		{
			auto moveDelta{ Def::Vec3 };

			auto& worldMat = m_upTransform->GetWorldMatrix();

			if (ImGui::IsKeyDown(ImGuiKey_W)) moveDelta += worldMat.Backward();  // 前進
			if (ImGui::IsKeyDown(ImGuiKey_S)) moveDelta += worldMat.Forward();   // 後退
			if (ImGui::IsKeyDown(ImGuiKey_D)) moveDelta += worldMat.Right();     // 右へ
			if (ImGui::IsKeyDown(ImGuiKey_A)) moveDelta += worldMat.Left();      // 左へ
			if (ImGui::IsKeyDown(ImGuiKey_E)) moveDelta += worldMat.Up();        // 上昇
			if (ImGui::IsKeyDown(ImGuiKey_Q)) moveDelta += worldMat.Down();      // 下降

			if (moveDelta.LengthSquared() > Def::FloatZero)
			{
				moveDelta.Normalize();
				m_pos += moveDelta * m_moveSpeed;
				m_upTransform->SetLocalPosition(m_pos);
				m_isDirty = true;
			}

			// --- マウスドラッグで視界回転 ---
			auto dragDelta{ ImGui::GetMouseDragDelta(ImGuiMouseButton_Right) };
			if (dragDelta.x != Def::FloatZero || dragDelta.y != Def::FloatZero)
			{
				m_eulerDegrees.y += dragDelta.x * m_rotSpeed;  // Yaw
				m_eulerDegrees.x += dragDelta.y * m_rotSpeed;  // Pitch

				// clamp pitch
				if (m_eulerDegrees.x > 89.0f) m_eulerDegrees.x = 89.0f;
				if (m_eulerDegrees.x < -89.0f) m_eulerDegrees.x = -89.0f;

				auto pitch{ DirectX::XMConvertToRadians(m_eulerDegrees.x) };
				auto yaw{ DirectX::XMConvertToRadians(m_eulerDegrees.y) };

				auto newQuat{ DirectX::SimpleMath::Quaternion::CreateFromYawPitchRoll(yaw, pitch, 0.0f) };

				m_upTransform->SetWorldRotation(newQuat);

				ImGui::ResetMouseDragDelta(ImGuiMouseButton_Right);
				m_isDirty = true;
			}
		}
	}
	ImGui::End();

	if (m_isDirty)
	{
		m_isDirty = false;

		m_mView = m_upTransform->GetWorldMatrix().Invert();
		m_mProj = DirectX::XMMatrixPerspectiveFovLH(DirectX::XMConvertToRadians(m_fov),
				m_aspect.x / m_aspect.y, m_clips.x, m_clips.y);
	}
}

void FlEditorCamera::Update()
{
	if (!m_isEnable)return;

	m_cameraData.mView = m_mView;
	m_cameraData.mProj = m_mProj;

	GraphicsDevice::Instance().GetCBufferAllocater()->BindAndAttachData(0, m_cameraData);
}
