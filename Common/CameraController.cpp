#include "pch.h"
#include "CameraController.h"

SonarPropagation::Graphics::Utils::CameraController::CameraController()
{

}

SonarPropagation::Graphics::Utils::CameraController::CameraController(Camera* camera)
{
	m_camera = camera;
	m_prevPointer = Windows::Foundation::Point(0, 0);
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
	case Windows::System::VirtualKey::I:
		m_pitchSpeed = 2.5f;
		break;
	case Windows::System::VirtualKey::K:
		m_pitchSpeed = -2.5f;
		break;
	case Windows::System::VirtualKey::J:
		m_yawSpeed = -2.5f;
		break;
	case Windows::System::VirtualKey::L:
		m_yawSpeed = 2.5f;
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
	case Windows::System::VirtualKey::I:
		m_pitchSpeed = 0.0f;
		break;
	case Windows::System::VirtualKey::K:
		m_pitchSpeed = 0.0f;
		break;
	case Windows::System::VirtualKey::J:
		m_yawSpeed = 0.0f;
		break;
	case Windows::System::VirtualKey::L:
		m_yawSpeed = 0.0f;
		break;

	case Windows::System::VirtualKey::F1:
		DebugBreak();
		break;
	}
}

void SonarPropagation::Graphics::Utils::CameraController::MouseMoved(Windows::UI::Core::PointerEventArgs^ args)
{
	
	if (m_prevPointer.X == 0 && m_prevPointer.Y == 0)
	{
		m_prevPointer = args->CurrentPoint->Position;
		return;
	}

	auto pointer = args->CurrentPoint->Position;
	if(args->CurrentPoint->Properties->IsLeftButtonPressed)
	{
		float xrel = (pointer.X - m_prevPointer.X) / 6000.f;
		float yrel = (pointer.Y - m_prevPointer.Y) / 6000.f;

		m_camera->UpdateUV(yrel, xrel);
		m_prevPointer = pointer;

	}



}

void SonarPropagation::Graphics::Utils::CameraController::MouseWheelMoved(Windows::UI::Core::PointerEventArgs^ args)
{
	float delta = args->CurrentPoint->Properties->MouseWheelDelta;
	m_camera->UpdateDistance(delta / 120.0f);
}



