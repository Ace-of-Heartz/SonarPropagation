#include "pch.h"
#include "Camera.h"


SonarPropagation::Graphics::Utils::Camera::Camera()
{
	m_eye = XMVectorSet(1.0f, 1.0f, -1.0f, 0.0f);
	m_at = XMVectorSet(0.0f, 0.0f, 0.0f, 0.0f);
	m_up = XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f);

	m_fovAngleY = 45.0f;
	m_aspectRatio = 1.0f;

	m_nearZ = 0.1f;
	m_farZ = 1000.0f;

	UpdateParameters();
}


SonarPropagation::Graphics::Utils::Camera::~Camera()
{

}

void SonarPropagation::Graphics::Utils::Camera::SetPosition(float x, float y, float z)
{
	m_eye = XMVectorSet(x, y, z, 0.0f);
}

void SonarPropagation::Graphics::Utils::Camera::SetLookAt(float x, float y, float z)
{
	m_at = XMVectorSet(x, y, z, 0.0f);
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

void SonarPropagation::Graphics::Utils::Camera::UpdateCameraBuffer()
{
	m_camMatrices[0] = XMMatrixLookAtRH(m_eye, m_at, m_up);

	m_camMatrices[1] = XMMatrixPerspectiveFovRH(m_fovAngleY, m_aspectRatio, 0.1f, 1000.0f);

	XMVECTOR det;
	m_camMatrices[2] = XMMatrixInverse(&det, m_camMatrices[0]);
	m_camMatrices[3] = XMMatrixInverse(&det, m_camMatrices[1]);

	uint8_t* pData;
	ThrowIfFailed(m_cameraBuffer->Map(0, nullptr, (void**)&pData));
	memcpy(pData, m_camMatrices.data(), m_cameraBufferSize);
	m_cameraBuffer->Unmap(0, nullptr);
}

void SonarPropagation::Graphics::Utils::Camera::UpdateParameters()
{
	m_forward = XMVector3Normalize(m_at - m_eye);
	m_right = XMVector3Normalize(XMVector3Cross(m_up, m_forward));
	m_upward = XMVector3Normalize(XMVector3Cross(m_forward, m_right));
}