#include "pch.h"
#include "Camera.h"



SonarPropagation::Graphics::Utils::Camera::Camera()
{
	m_eye = XMVectorSet(1.5f, 1.5f, 1.5f, 0.0f);
	m_at = XMVectorSet(0.0f, 0.0f, 0.0f, 0.0f);

	m_forward = XMVectorSet(0.0f, 0.0f, 1.0f, 0.0f);
	m_right = XMVectorSet(1.0f, 0.0f, 0.0f, 0.0f);
	m_up = XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f);

	m_worldUp = XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f);

	m_yaw = 0.0f;
	m_pitch = 0.0f;

	m_rotationQ = XMQuaternionRotationRollPitchYaw(m_yaw,m_pitch,0.0);


	m_distance = 0.5f;

	m_fovAngleY = 75.0f;
	m_aspectRatio = 1.0;

	m_nearZ = 0.1f;
	m_farZ = 1000.0f;

	m_speed = 5.0f;

	m_isProjectionDirty = true;
	m_isViewDirty = true;

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

	m_projectionMatrix = XMMatrixPerspectiveFovRH(m_fovAngleY, m_aspectRatio, 0.1f, 1000.0f);
	m_projectionMatrixInv = XMMatrixInverse(&det, m_projectionMatrix);
}

void SonarPropagation::Graphics::Utils::Camera::UpdateCameraBuffer()
{
	allMatrices = { m_viewMatrix,m_projectionMatrix, m_viewMatrixInv, m_projectionMatrixInv };

	uint8_t* pData;
	ThrowIfFailed(m_cameraBuffer->Map(0, nullptr, (void**)&pData));
	memcpy(pData, allMatrices.data(), m_cameraBufferSize);
	m_cameraBuffer->Unmap(0, nullptr);
}

void SonarPropagation::Graphics::Utils::Camera::UpdateParameters()
{
	XMVECTOR lookDirection;

	if (m_useQuaternion) {
		lookDirection = XMVector3Rotate(XMVectorSet(0.f,0.f,1.0f,0.0f), m_rotationQ);
	}
	else {
		lookDirection = { cosf(m_u) * sinf(m_v),sinf(m_u) * sinf(m_v),cosf(m_u) };
	}

	m_at = m_eye + m_distance * lookDirection; 

	m_forward = lookDirection;

	m_right = XMVector3Normalize(XMVector3Cross(m_forward,m_worldUp));
	m_up = XMVector3Normalize(XMVector3Cross(m_right,m_forward));

	m_isViewDirty = true;
	m_isProjectionDirty = true;
}

void SonarPropagation::Graphics::Utils::Camera::UpdateUV(float du, float dv)
{
	if (m_useQuaternion) {
		m_yaw += du;
		m_pitch += dv;

		m_rotationQ = XMQuaternionRotationRollPitchYaw(m_pitch, m_yaw, 0.0f);
	}
	else {
		m_u += du;
		m_v += dv;
	}

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
	bool static useQuaternion = false;
	ImGui::Checkbox("Show Info", &showCameraInfo);
	ImGui::Checkbox("Show Controls", &showCameraControls);

	if (showCameraInfo)
	{
		ImGui::BeginChild("Camera Info", ImVec2(400, 200));

		auto camEyeVec = GetEye();
		std::vector<float> camEyeFloat = { XMVectorGetX(camEyeVec) , XMVectorGetY(camEyeVec), XMVectorGetZ(camEyeVec) };

		auto camTargetVec = GetAt();
		std::vector<float> camTargetFloat = { XMVectorGetX(camTargetVec) , XMVectorGetY(camTargetVec), XMVectorGetZ(camTargetVec) };

		auto camForwardVec = GetForward();
		std::vector<float> camForwardFloat = { XMVectorGetX(camForwardVec) , XMVectorGetY(camForwardVec), XMVectorGetZ(camForwardVec) };

		auto camRightVec = GetRight();
		std::vector<float> camRightFloat = { XMVectorGetX(camRightVec) , XMVectorGetY(camRightVec), XMVectorGetZ(camRightVec) };

		auto camUpVec = GetUp();
		std::vector<float> camUpFloat = { XMVectorGetX(camUpVec) , XMVectorGetY(camUpVec), XMVectorGetZ(camUpVec) };

		ImGui::InputFloat3("Camera Eye", camEyeFloat.data(), "%.3f", ImGuiInputTextFlags_ReadOnly);
		ImGui::InputFloat3("Camera Target", camTargetFloat.data(), "%.3f", ImGuiInputTextFlags_ReadOnly);
		ImGui::Separator();
		ImGui::InputFloat3("Camera Forward", camForwardFloat.data(), "%.3f", ImGuiInputTextFlags_ReadOnly);
		ImGui::InputFloat3("Camera Right", camRightFloat.data(), "%.3f", ImGuiInputTextFlags_ReadOnly);
		ImGui::InputFloat3("Camera Up", camUpFloat.data(), "%.3f", ImGuiInputTextFlags_ReadOnly);
		ImGui::Separator();

		if (useQuaternion) {

			EnableQuaternionRotation();
			auto camYaw = GetYaw();
			auto camPitch = GetPitch();

			ImGui::InputFloat3("Camera Yaw", &camYaw, "%.3f", ImGuiInputTextFlags_ReadOnly);
			ImGui::InputFloat3("Camera Pitch", &camPitch, "%.3f", ImGuiInputTextFlags_ReadOnly);

		}
		else {
			DisableQuaternionRotation();

			auto camU = GetU();
			auto camV = GetV();

			ImGui::InputFloat3("Camera U", &camU, "%.3f", ImGuiInputTextFlags_ReadOnly);
			ImGui::InputFloat3("Camera V", &camV, "%.3f", ImGuiInputTextFlags_ReadOnly);

		}


		ImGui::EndChild();
	}

	if (showCameraControls)
	{


		ImGui::BeginChild("Camera Controls", ImVec2(400, 200));

		ImGui::Checkbox("Enable rotation with quaternions?", &useQuaternion);

		ImGui::Separator();

		bool yawChanged = false;
		bool pitchChanged = false;

		yawChanged = ImGui::SliderFloat("Yaw", &m_yaw, -XM_PI, XM_PI);
		pitchChanged = ImGui::SliderFloat("Pitch", &m_pitch, -XM_PI / 2.f, XM_PI / 2.f);
		
		if (yawChanged || pitchChanged) {
			UpdateUV(0.f,0.f);
		}

		ImGui::EndChild();

		ImGui::Separator();

		ImGui::SliderFloat("Speed", &m_speed, 0.1f, 10.0f);

		ImGui::Separator();

		if (ImGui::Button("Reset Camera")) {
			m_rotationQ = XMQuaternionIdentity();
			UpdateParameters();

		}


	}


	ImGui::End();

}