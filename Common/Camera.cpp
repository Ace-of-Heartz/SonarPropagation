#include "pch.h"
#include "Camera.h"



SonarPropagation::Graphics::Utils::Camera::Camera()
{
	m_eye = XMVectorSet(1.5f, 1.5f, 25.5f, 0.0f);
	m_at = XMVectorSet(0.0f, 0.0f, 0.0f, 0.0f);

	m_forward = XMVectorSet(0.0f, 0.0f, 1.0f, 0.0f);
	m_right = XMVectorSet(1.0f, 0.0f, 0.0f, 0.0f);
	m_up = XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f);

	m_worldUp = XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f);

	m_u = XM_PI;
	m_v = 0.0f;

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
	//std::vector<XMMATRIX> matrices(4);

	//matrices[0] = XMMatrixLookAtRH(m_eye, m_at, m_up);

	//float fovAngleY = 45.0f * XM_PI / 180.0f;
	//matrices[1] =
	//	XMMatrixPerspectiveFovRH(fovAngleY, m_aspectRatio, 0.1f, 1000.0f);

	//// Raytracing has to do the contrary of rasterization: rays are defined in
	//// camera space, and are transformed into world space. To do this, we need to
	//// store the inverse matrices as well.
	//XMVECTOR det;
	//matrices[2] = XMMatrixInverse(&det, matrices[0]);
	//matrices[3] = XMMatrixInverse(&det, matrices[1]);



	allMatrices = { m_viewMatrix,m_projectionMatrix, m_viewMatrixInv, m_projectionMatrixInv };

	uint8_t* pData;
	ThrowIfFailed(m_cameraBuffer->Map(0, nullptr, (void**)&pData));
	memcpy(pData, allMatrices.data(), m_cameraBufferSize);
	m_cameraBuffer->Unmap(0, nullptr);
}

void SonarPropagation::Graphics::Utils::Camera::UpdateParameters()
{

	XMVECTOR lookDirection{ cosf(m_u) * sinf(m_v), sinf(m_u) * sinf(m_v),cosf(m_u), };

	m_at = m_eye + m_distance * lookDirection; 

	m_forward = lookDirection;

	m_right = XMVector3Normalize(XMVector3Cross(m_forward,m_worldUp));
	m_up = XMVector3Normalize(XMVector3Cross(m_right,m_forward));

	m_isViewDirty = true;
}

void SonarPropagation::Graphics::Utils::Camera::UpdateUV(float du, float dv)
{
	m_u += du;
	m_v = m_v + dv < 0.1f ? 0.1 : m_v + dv > 3.1f ? 3.1f : m_v + dv;

	UpdateParameters();
}

void SonarPropagation::Graphics::Utils::Camera::UpdateDistance(float dDistance) {

	if (dDistance + m_distance < 0.1f)
		return;

	m_distance += dDistance;

	UpdateParameters();
}
