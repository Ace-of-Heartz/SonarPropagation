#include "pch.h"
#include "Camera.h"

SonarPropagation::Graphics::Utils::Camera::Camera(
	XMVECTOR eye,
	XMVECTOR at,
	float yaw,
	float pitch,
	float fovAngleY,
	float aspectRatio,
	float zNear,
	float zFar,
	float speed
) : m_eye(eye),
	m_at(at),
	m_up(XMVectorSet(0.0f,1.0f,0.0f,0.0f)),
	m_worldUp(XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f)),
	m_forward(at - eye),
	m_right(XMVector3Cross(m_forward, m_worldUp)),
	m_distance(1.0f),
	m_fovAngleY(fovAngleY),
	m_aspectRatio(aspectRatio),
	m_zNear(zNear),
	m_zFar(zFar),
	m_yaw(yaw),
	m_pitch(pitch),
	m_rotationQ(XMQuaternionRotationRollPitchYaw(m_pitch, m_yaw, 0.0f)),
	m_speed(speed),
	m_isProjectionDirty(true),
	m_isViewDirty(true)
{
	UpdateParameters();
}

SonarPropagation::Graphics::Utils::Camera::~Camera()
{

}

void SonarPropagation::Graphics::Utils::Camera::CreateCameraBuffer(ComPtr<ID3D12Device> device)
{
	uint32_t nbMatrix = 4;
	m_cameraBufferSize = nbMatrix * sizeof(XMMATRIX);

	m_cameraBuffer = nv_helpers_dx12::CreateBuffer(
		device.Get(), m_cameraBufferSize, D3D12_RESOURCE_FLAG_NONE,
		D3D12_RESOURCE_STATE_GENERIC_READ, nv_helpers_dx12::kUploadHeapProps);

	m_cameraHeap = CreateDescriptorHeap(
		device.Get(), 1, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, true);

	D3D12_CONSTANT_BUFFER_VIEW_DESC cbvDesc = {};
	cbvDesc.BufferLocation = m_cameraBuffer->GetGPUVirtualAddress();
	cbvDesc.SizeInBytes = m_cameraBufferSize;

	D3D12_CPU_DESCRIPTOR_HANDLE srvHandle =
		m_cameraHeap->GetCPUDescriptorHandleForHeapStart();
	device->CreateConstantBufferView(&cbvDesc, srvHandle);
}

void SonarPropagation::Graphics::Utils::Camera::UpdateViewMatrix()
{
	XMVECTOR det;

	m_viewMatrix = XMMatrixLookAtRH(m_eye, m_at, m_up);
	m_viewMatrixInv = XMMatrixInverse(&det, m_viewMatrix);
}

void SonarPropagation::Graphics::Utils::Camera::UpdateProjectionMatrix()
{
	XMVECTOR det;
	m_viewMatrix = XMMatrixLookAtRH(m_eye, m_at, m_up);
	m_viewMatrixInv = XMMatrixInverse(&det, m_viewMatrix);
	m_projectionMatrix = XMMatrixPerspectiveFovRH(m_fovAngleY, m_aspectRatio, m_zNear, m_zFar);
	m_projectionMatrixInv = XMMatrixInverse(&det, m_projectionMatrix);
}

void SonarPropagation::Graphics::Utils::Camera::UpdateCameraBuffer()
{
	allMatrices = { m_viewMatrix,m_projectionMatrix, m_viewMatrixInv, m_projectionMatrixInv };

	uint8_t* pData;
	DX::ThrowIfFailed(m_cameraBuffer->Map(0, nullptr, (void**)&pData));
	memcpy(pData, allMatrices.data(), m_cameraBufferSize);
	m_cameraBuffer->Unmap(0, nullptr);
}

void SonarPropagation::Graphics::Utils::Camera::UpdateParameters()
{
	XMVECTOR lookDirection;

	lookDirection = XMVector3Rotate(XMVectorSet(0.f, 0.f, 1.0f, 0.0f), m_rotationQ);
	m_at = m_eye + m_distance * lookDirection;

	m_forward = lookDirection;

	m_right = XMVector3Normalize(XMVector3Cross(m_forward, m_worldUp));
	m_up = XMVector3Normalize(XMVector3Cross(m_right, m_forward));

	m_isViewDirty = true;
	m_isProjectionDirty = true;
}

void SonarPropagation::Graphics::Utils::Camera::UpdateUV(float yaw, float pitch)
{
	m_yaw += yaw;
	m_pitch += pitch;

	m_rotationQ = XMQuaternionRotationRollPitchYaw(m_pitch, m_yaw, 0.0f);


	UpdateParameters();

}

void SonarPropagation::Graphics::Utils::Camera::UpdateDistance(float dDistance) {

	if (dDistance + m_distance < 0.1f)
		return;

	m_distance += dDistance;

	UpdateParameters();
}

void SonarPropagation::Graphics::Utils::Camera::RenderCameraImGui() {

	ImGui::Begin("Camera Window");

	static bool showCameraInfo = false;
	static bool showCameraControls = false;
	ImGui::Checkbox("Show Info", &showCameraInfo);
	ImGui::Checkbox("Show Controls", &showCameraControls);

	if (showCameraInfo)
	{
		ImGui::BeginChild("Camera Info", ImVec2(400, 400));


		std::vector<float> camEyeFloat = { XMVectorGetX(m_eye) , XMVectorGetY(m_eye), XMVectorGetZ(m_eye) };
		std::vector<float> camTargetFloat = { XMVectorGetX(m_at) , XMVectorGetY(m_at), XMVectorGetZ(m_at) };
		std::vector<float> camForwardFloat = { XMVectorGetX(m_forward) , XMVectorGetY(m_forward), XMVectorGetZ(m_forward) };
		std::vector<float> camRightFloat = { XMVectorGetX(m_right) , XMVectorGetY(m_right), XMVectorGetZ(m_right) };
		std::vector<float> camUpFloat = { XMVectorGetX(m_up) , XMVectorGetY(m_up), XMVectorGetZ(m_up) };

		int camBufferSize = m_cameraBufferSize;

		ImGui::InputFloat3("Camera Eye", camEyeFloat.data(), "%.3f", ImGuiInputTextFlags_ReadOnly);
		ImGui::InputFloat3("Camera Target", camTargetFloat.data(), "%.3f", ImGuiInputTextFlags_ReadOnly);

		ImGui::Separator();

		ImGui::InputFloat3("Camera Forward", camForwardFloat.data(), "%.3f", ImGuiInputTextFlags_ReadOnly);
		ImGui::InputFloat3("Camera Right", camRightFloat.data(), "%.3f", ImGuiInputTextFlags_ReadOnly);
		ImGui::InputFloat3("Camera Up", camUpFloat.data(), "%.3f", ImGuiInputTextFlags_ReadOnly);

		ImGui::Separator();

		ImGui::InputFloat3("Camera Yaw", &m_yaw, "%.3f", ImGuiInputTextFlags_ReadOnly);
		ImGui::InputFloat3("Camera Pitch", &m_pitch, "%.3f", ImGuiInputTextFlags_ReadOnly);

		ImGui::Separator();

		ImGui::InputFloat("Aspect Ratio", &m_aspectRatio, 0.f, 0.f, "%.3f", ImGuiInputTextFlags_ReadOnly);
		ImGui::InputFloat("FOV Y", &m_fovAngleY, 0.f, 0.f,"%.3f", ImGuiInputTextFlags_ReadOnly);

		ImGui::Separator();

		ImGui::InputInt("Camera Buffer Size", &camBufferSize, 0, 0, ImGuiInputTextFlags_ReadOnly);

		ImGui::EndChild();
	}

	if (showCameraControls)
	{


		ImGui::BeginChild("Camera Controls", ImVec2(400, 200));


		ImGui::Separator();
		{	//Yaw, pitch controls - update when values are changed
			bool yawChanged = false;
			bool pitchChanged = false;

			yawChanged = ImGui::SliderFloat("Yaw", &m_yaw, -XM_PI, XM_PI);
			pitchChanged = ImGui::SliderFloat("Pitch", &m_pitch, -XM_PI / 2.f, XM_PI / 2.f);

			if (yawChanged || pitchChanged) {
				UpdateUV(0.f, 0.f);
			}
		}
		ImGui::Separator();

		ImGui::SliderFloat("Speed", &m_speed, 1.f, 500.0f);

		ImGui::Separator();

		//TODO: Refine this
		if (ImGui::SliderFloat("FOV Y", &m_fovAngleY, 1.0f, 179.0f))
		{
			m_isProjectionDirty = true;
		}

		ImGui::Separator();
		if (ImGui::Checkbox("Is sound source?",&m_isSoundSource)) {
			
		}

		ImGui::Separator();
		if (ImGui::Button("Reset Camera")) {
			m_rotationQ = XMQuaternionIdentity();
			UpdateParameters();

		}

		ImGui::EndChild();

	}


	ImGui::End();

}