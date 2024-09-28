#include "pch.h"
#include "CameraController.h"

SonarPropagation::Graphics::Utils::CameraController::CameraController(ComPtr<ID3D12Device> device)
	: m_cameras(std::vector<Camera*>()),
	m_cameraIndex(0),
	m_device(device)
{
	AddCamera(new Camera());
	m_prevPointer = Windows::Foundation::Point(0, 0);
}

SonarPropagation::Graphics::Utils::CameraController::CameraController(ComPtr<ID3D12Device> device,Camera* camera)
	: m_cameras(std::vector<Camera*>()),
	m_cameraIndex(0),
	m_device(device)
{
	AddCamera(camera);
	m_prevPointer = Windows::Foundation::Point(0, 0);
}


SonarPropagation::Graphics::Utils::CameraController::~CameraController()
{
}

void SonarPropagation::Graphics::Utils::CameraController::ProcessCameraUpdate(DX::StepTimer const& timer)
{
	if (m_forwardSpeed != 0.0f || m_sidewaysSpeed != 0.0f || m_upwardsSpeed != 0.0f)
	{
		XMVECTOR deltaPosition = (m_forwardSpeed * m_cameras[m_cameraIndex]->m_forward
			+ m_sidewaysSpeed * m_cameras[m_cameraIndex]->m_right
			+ m_upwardsSpeed * m_cameras[m_cameraIndex]->m_up) * timer.GetElapsedSeconds() * m_cameras[m_cameraIndex]->GetSpeed();

		m_cameras[m_cameraIndex]->m_eye += deltaPosition;
		m_cameras[m_cameraIndex]->m_at += deltaPosition;
		m_cameras[m_cameraIndex]->m_isViewDirty = true;
	}

	if (m_yawSpeed != 0.0f || m_pitchSpeed != 0.0f) {
		float deltaYaw = m_yawSpeed * timer.GetElapsedSeconds();
		float deltaPitch = m_pitchSpeed * timer.GetElapsedSeconds();

		m_cameras[m_cameraIndex]->UpdateUV(deltaPitch, deltaYaw);
		m_cameras[m_cameraIndex]->m_isViewDirty = true;
	}

	if (m_cameras[m_cameraIndex]->m_isViewDirty)
	{
		m_cameras[m_cameraIndex]->UpdateViewMatrix();
	}

	if (m_cameras[m_cameraIndex]->m_isProjectionDirty)
	{
		m_cameras[m_cameraIndex]->UpdateProjectionMatrix();
	}

	if (m_cameras[m_cameraIndex]->m_isProjectionDirty || m_cameras[m_cameraIndex]->m_isViewDirty)
	{
		m_cameras[m_cameraIndex]->UpdateCameraBuffer();
		m_cameras[m_cameraIndex]->m_isProjectionDirty = false;
		m_cameras[m_cameraIndex]->m_isViewDirty = false;
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
		m_sidewaysSpeed = -0.2f;
		break;
	case Windows::System::VirtualKey::D:
		m_sidewaysSpeed = 0.2f;
		break;
	case Windows::System::VirtualKey::Q:
		m_upwardsSpeed = -0.2f;
		break;
	case Windows::System::VirtualKey::E:
		m_upwardsSpeed = 0.2f;
		break;
	case Windows::System::VirtualKey::I:
		m_yawSpeed = -1.5f;
		break;
	case Windows::System::VirtualKey::K:
		m_yawSpeed = 1.5f;
		break;
	case Windows::System::VirtualKey::J:
		m_pitchSpeed = 1.5f;
		break;
	case Windows::System::VirtualKey::L:
		m_pitchSpeed = -1.5f;
		break;
	case Windows::System::VirtualKey::F:
		CycleToPreviousCamera();
		break;
	case Windows::System::VirtualKey::G:
		CycleToNextCamera();
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
		m_yawSpeed = 0.0f;
		break;
	case Windows::System::VirtualKey::K:
		m_yawSpeed = 0.0f;
		break;
	case Windows::System::VirtualKey::J:
		m_pitchSpeed = 0.0f;
		break;
	case Windows::System::VirtualKey::L:
		m_pitchSpeed = 0.0f;
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
	if (args->CurrentPoint->Properties->IsLeftButtonPressed)
	{
		float xrel = (pointer.X - m_prevPointer.X) / 600.f;
		float yrel = (pointer.Y - m_prevPointer.Y) / 600.f;

		m_cameras[m_cameraIndex]->UpdateUV(xrel, yrel);
		m_prevPointer = pointer;
	}
}

void SonarPropagation::Graphics::Utils::CameraController::MouseWheelMoved(Windows::UI::Core::PointerEventArgs^ args)
{
	float delta = args->CurrentPoint->Properties->MouseWheelDelta;
	m_cameras[m_cameraIndex]->UpdateDistance(delta / 120.0f);
}

void SonarPropagation::Graphics::Utils::CameraController::RenderImGui() {
	m_cameras[m_cameraIndex]->RenderCameraImGui();
}

Camera* SonarPropagation::Graphics::Utils::CameraController::GetCurrentCamera() const {
	return m_cameras[m_cameraIndex];
}