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
			+ m_upwardsSpeed * m_camera->m_up) * timer.GetElapsedSeconds() * 2.5;

		m_camera->m_eye += deltaPosition;
		m_camera->m_at += deltaPosition;
		m_camera->m_isDirty = true;
	}

	if (m_camera->m_isDirty)
	{
		//m_camera->UpdateParameters();
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
	//if (args->CurrentPoint->Properties->IsLeftButtonPressed)
	//{

	//	//TODO: Fix this, cause it's not working properly
	//	float xrel = args->CurrentPoint->Position.X / 9000.0f;
	//	float yrel = args->CurrentPoint->Position.Y / 9000.0f;

	//	m_camera->UpdateUV(xrel,yrel);
	//}

}

void SonarPropagation::Graphics::Utils::CameraController::MouseWheelMoved(Windows::UI::Core::PointerEventArgs^ args)
{
	
}