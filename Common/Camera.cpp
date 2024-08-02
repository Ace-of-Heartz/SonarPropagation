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

	m_u = XM_PI;
	m_v = XM_PI/2.f;

	//m_rotationQ = XMQuaternionRotationRollPitchYaw(XM_PI,0.f,0.f);


	m_distance = 0.5f;

	m_fovAngleY = 75.0f;
	m_aspectRatio = 1.0;

	m_nearZ = 0.1f;
	m_farZ = 1000.0f;

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

	m_projectionMatrix = XMMatrixPerspectiveFovRH(45.0 * XM_PI / 180.f, m_aspectRatio, 0.1f, 1000.0f);
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

	XMVECTOR lookDirection{ cosf(m_u) * sinf(m_v),sinf(m_u) * sinf(m_v),cosf(m_u)  };

	//XMVECTOR lookDirection = XMVector3Rotate(m_forward, m_rotationQ);

	m_at = m_eye + m_distance * lookDirection; 

	m_forward = lookDirection;

	m_right = XMVector3Normalize(XMVector3Cross(m_forward,m_worldUp));
	m_up = XMVector3Normalize(XMVector3Cross(m_right,m_forward));

	m_isViewDirty = true;
	m_isProjectionDirty = true;
}

void SonarPropagation::Graphics::Utils::Camera::UpdateUV(float du, float dv)
{
	m_u += m_u;
	m_v = m_v + dv < 0.01f ? 0.01 : m_v + dv > 0.1f ? 0.1f : m_v + dv;

	//ApplyRotation(du, dv, 0.0f);
	UpdateParameters();

}




void SonarPropagation::Graphics::Utils::Camera::UpdateDistance(float dDistance) {

	if (dDistance + m_distance < 0.1f)
		return;

	m_distance += dDistance;

	UpdateParameters();
}


void SonarPropagation::Graphics::Utils::Camera::ApplyRotation(float yaw, float pitch, float roll) {
	XMVECTOR q = XMQuaternionRotationRollPitchYaw(pitch, yaw, roll);
	
	//m_rotationQ = XMQuaternionSlerp(m_rotationQ, q, 0.01f);
	
	UpdateParameters();
}