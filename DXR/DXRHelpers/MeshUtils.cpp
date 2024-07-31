#include "pch.h"
#include "MeshUtils.h"



void SonarPropagation::Graphics::Utils::CreateTriangleMesh(
	ComPtr<ID3D12Device> device,
	ComPtr<ID3D12Resource> vertexBuffer,
	D3D12_VERTEX_BUFFER_VIEW& vertexBufferView,
	ComPtr<ID3D12Resource> indexBuffer,
	D3D12_INDEX_BUFFER_VIEW indexBufferView
) {
	VertexPositionColor vertices[] =
	{
		{{std::sqrtf(8.f / 9.f), 0.f, -1.f / 3.f}, {1.f, 0.f, 0.f}},
		{{-std::sqrtf(2.f / 9.f), std::sqrtf(2.f / 3.f), -1.f / 3.f},{0.f, 1.f, 0.f}},
		{{-std::sqrtf(2.f / 9.f), -std::sqrtf(2.f / 3.f), -1.f / 3.f},{0.f, 0.f, 1.f}},
		{{0.f, 0.f, 1.f}, {1, 0, 1}}
	};

	const UINT vertexBufferSize = sizeof(vertices);

	ThrowIfFailed(device->CreateCommittedResource(
		&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD), D3D12_HEAP_FLAG_NONE,
		&CD3DX12_RESOURCE_DESC::Buffer(vertexBufferSize),
		D3D12_RESOURCE_STATE_GENERIC_READ, nullptr,
		IID_PPV_ARGS(&vertexBuffer)));

	UINT8* pVertexDataBegin;
	CD3DX12_RANGE readRange(
		0, 0); // We do not intend to read from this resource on the CPU.
	ThrowIfFailed(vertexBuffer->Map(
		0, &readRange, reinterpret_cast<void**>(&pVertexDataBegin)));
	memcpy(pVertexDataBegin, vertices, sizeof(vertices));
	vertexBuffer->Unmap(0, nullptr);

	vertexBufferView.BufferLocation = vertexBuffer->GetGPUVirtualAddress();
	vertexBufferView.StrideInBytes = sizeof(VertexPositionColor);
	vertexBufferView.SizeInBytes = vertexBufferSize;

	std::vector<UINT> indices = { 0, 1, 2, 0, 3, 1, 0, 2, 3, 1, 3, 2 };
	const UINT indexBufferSize =
		static_cast<UINT>(indices.size()) * sizeof(UINT);

	CD3DX12_HEAP_PROPERTIES heapProperty =
		CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);
	CD3DX12_RESOURCE_DESC bufferResource =
		CD3DX12_RESOURCE_DESC::Buffer(indexBufferSize);
	ThrowIfFailed(device->CreateCommittedResource(
		&heapProperty, D3D12_HEAP_FLAG_NONE, &bufferResource, //
		D3D12_RESOURCE_STATE_GENERIC_READ, nullptr,
		IID_PPV_ARGS(&indexBuffer)));

	// Copy the triangle data to the index buffer.
	UINT8* pIndexDataBegin;
	ThrowIfFailed(indexBuffer->Map(
		0, &readRange, reinterpret_cast<void**>(&pIndexDataBegin)));
	memcpy(pIndexDataBegin, indices.data(), indexBufferSize);
	indexBuffer->Unmap(0, nullptr);

	// Initialize the index buffer view.
	indexBufferView.BufferLocation = indexBuffer->GetGPUVirtualAddress();
	indexBufferView.Format = DXGI_FORMAT_R32_UINT;
	indexBufferView.SizeInBytes = indexBufferSize;

}




