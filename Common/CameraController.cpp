#include "pch.h"
#include "CameraController.h"

SonarPropagation::Graphics::Utils::CameraController::CameraController()
{

}

SonarPropagation::Graphics::Utils::CameraController::CameraController(Camera* camera)
{
	m_camera = camera;
}


SonarPropagation::Graphics::Utils::CameraController::~CameraController()
{
}

void SonarPropagation::Graphics::Utils::CameraController::ProcessCameraUpdate(DX::StepTimer const& timer)
{
	if (m_forwardSpeed != 0.0f || m_sidewaysSpeed != 0.0f || m_upwardsSpeed != 0.0f)
	{
		XMVECTOR deltaPosition = (m_forwardSpeed * m_camera->m_forward 
			+ m_sidewaysSpeed * m_camera->m_right 
			+ m_upwardsSpeed * m_camera->m_up) * timer.GetElapsedSeconds() * 5.5;

		m_camera->m_eye += deltaPosition;
		m_camera->m_at += deltaPosition;
		m_camera->m_isViewDirty = true;
	}



	if (m_camera->m_isViewDirty)
	{
		m_camera->UpdateViewMatrix();
	}

	if (m_camera->m_isProjectionDirty)
	{
		m_camera->UpdateProjectionMatrix();
	}

	if (m_camera-> m_isProjectionDirty || m_camera->m_isViewDirty)
	{	
		m_camera->UpdateCameraBuffer();
		m_camera->m_isProjectionDirty = false;
		m_camera->m_isViewDirty = false;
	}


}

void SonarPropagation::Graphics::Utils::CameraController::KeyPressed(Windows::UI::Core::KeyEventArgs^ args)
{
	switch (args->VirtualKey)
	{
	case Windows::System::VirtualKey::W:
		m_forwardSpeed = 0.2f;
		break;
	case Windows::System::VirtualKey::S:
		m_forwardSpeed = -0.2f;
		break;
	case Windows::System::VirtualKey::A:
		m_sidewaysSpeed = 0.2f;
		break;
	case Windows::System::VirtualKey::D:
		m_sidewaysSpeed = -0.2f;
		break;
	case Windows::System::VirtualKey::Q:
		m_upwardsSpeed = -0.2f;
		break;
	case Windows::System::VirtualKey::E:
		m_upwardsSpeed = 0.2f;
		break;
	case Windows::System::VirtualKey::Up:
		m_pitchSpeed = 0.5f;
		break;
	case Windows::System::VirtualKey::Down:
		m_pitchSpeed = -0.5f;
		break;
	case Windows::System::VirtualKey::Left:
		m_yawSpeed = -0.5f;
		break;
	case Windows::System::VirtualKey::Right:
		m_yawSpeed = 0.5f;
		break;

	
	}
}

void SonarPropagation::Graphics::Utils::CameraController::KeyReleased(Windows::UI::Core::KeyEventArgs^ args)
{
	switch (args->VirtualKey)
	{
	case Windows::System::VirtualKey::W:
		m_forwardSpeed = 0.0f;
		break;
	case Windows::System::VirtualKey::S:
		m_forwardSpeed = 0.0f;
		break;
	case Windows::System::VirtualKey::A:
		m_sidewaysSpeed = 0.0f;
		break;
	case Windows::System::VirtualKey::D:
		m_sidewaysSpeed = 0.0f;
		break;
	case Windows::System::VirtualKey::Q:
		m_upwardsSpeed = 0.0f;
		break;
	case Windows::System::VirtualKey::E:
		m_upwardsSpeed = 0.0f;
		break;
	case Windows::System::VirtualKey::Up:
		m_pitchSpeed = 0.0f;
		break;
	case Windows::System::VirtualKey::Down:
		m_pitchSpeed = 0.0f;
		break;
	case Windows::System::VirtualKey::Left:
		m_yawSpeed = 0.0f;
		break;
	case Windows::System::VirtualKey::Right:
		m_yawSpeed = 0.0f;
		break;

	case Windows::System::VirtualKey::F1:
		DebugBreak();
		break;
	}
}

void SonarPropagation::Graphics::Utils::CameraController::MouseMoved(Windows::Devices::Input::MouseEventArgs^ args)
{
	float xrel = args->MouseDelta.X / 800.f;
	float yrel = args->MouseDelta.Y / -800.f;

	m_camera->UpdateUV(yrel, xrel);
}

void SonarPropagation::Graphics::Utils::CameraController::MouseWheelMoved(Windows::UI::Core::PointerEventArgs^ args)
{
	float delta = args->CurrentPoint->Properties->MouseWheelDelta;
	m_camera->UpdateDistance(delta / 120.0f);
}