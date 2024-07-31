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

void SonarPropagation::Graphics::Utils::CameraController::ProcessCameraUpdate()
{
	if (m_forwardSpeed != 0.0f || m_sidewaysSpeed != 0.0f || m_upwardsSpeed != 0.0f)
	{
		XMVECTOR deltaPosition = m_forwardSpeed * m_camera->m_forward 
			+ m_sidewaysSpeed * m_camera->m_right 
			+ m_upwardsSpeed * m_camera->m_up;

		m_camera->m_eye += deltaPosition;
		m_camera->m_at += deltaPosition;
		m_camera->m_isDirty = true;
	}

	if (m_camera->m_isDirty)
	{
		m_camera->UpdateParameters();
		m_camera->UpdateCameraBuffer();
		m_camera->m_isDirty = false;
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
	}
}

void SonarPropagation::Graphics::Utils::CameraController::MouseMoved(Windows::UI::Core::PointerEventArgs^ args)
{
	//float xrel = args->CurrentPoint.Position.X;
	//float yrel = args->CurrentPoint.Position.Y;

}

void SonarPropagation::Graphics::Utils::CameraController::MouseWheelMoved(Windows::UI::Core::PointerEventArgs^ args)
{
	
}