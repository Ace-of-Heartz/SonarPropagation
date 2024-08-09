
#include "pch.h"
#include "RayTracingRenderer.h"



//-----------------------------------------------------------------------------
//
// Create a bottom-level acceleration structure based on a list of vertex
// buffers in GPU memory along with their vertex count. The build is then done
// in 3 steps: gathering the geometry, computing the sizes of the required
// buffers, and building the actual AS
//

//-----------------------------------------------------------------------------
// Namespace for helper functions



// ---------------------------------------------------------------------------
// Aliases
using Size = Windows::Foundation::Size;

#pragma region Constructors

SonarPropagation::Graphics::DXR::RayTracingRenderer::RayTracingRenderer(
	const std::shared_ptr<DX::DeviceResources>& deviceResources) :
	m_loadingComplete(false),
	m_deviceResources(deviceResources),
	m_dxrConfig({ 1, 4 * sizeof(float), 2 * sizeof(float) })
{
	LoadState();

	CreateDeviceDependentResources();
	CreateWindowSizeDependentResources();
}

#pragma endregion

#pragma region Destructor

SonarPropagation::Graphics::DXR::RayTracingRenderer::~RayTracingRenderer() {
}

#pragma endregion

#pragma region Graphics Init.: 

void SonarPropagation::Graphics::DXR::RayTracingRenderer::CreatePerInstanceConstantBuffers() {

	// Due to HLSL packing rules, we create the CB with 9 float4 (each needs to start on a 16-byte
// boundary)
	XMVECTOR bufferData[] = {
		// A
		XMVECTOR{1.0f, 0.0f, 0.0f, 1.0f},
		XMVECTOR{1.0f, 0.4f, 0.0f, 1.0f},
		XMVECTOR{1.f, 0.7f, 0.0f, 1.0f},
		XMVECTOR{0.0f, 0.0f, 1.0, 1.0f},

		// B
		XMVECTOR{0.0f, 1.0f, 0.0f, 1.0f},
		XMVECTOR{0.0f, 1.0f, 0.4f, 1.0f},
		XMVECTOR{0.0f, 1.0f, 0.7f, 1.0f},
		XMVECTOR{0.0f, 0.0f, 1.0, 1.0f},

		// C
		XMVECTOR{0.0f, 0.0f, 1.0f, 1.0f},
		XMVECTOR{0.4f, 0.0f, 1.0f, 1.0f},
		XMVECTOR{0.7f, 0.0f, 1.0f, 1.0f},
		XMVECTOR{0.0f, 0.0f, 1.0, 1.0f},

		XMVECTOR{0.0f, 0.0f, 1.0f, 1.0f},
		XMVECTOR{0.0f, 0.0f, 0.2f, 1.0f},
		XMVECTOR{0.0f, 0.0f, 0.5f, 1.0f},
		XMVECTOR{0.0f, 0.0f, 1.0, 1.0f},

		XMVECTOR{0.0f, 0.2f, 0.0f, 1.0f},
		XMVECTOR{0.4f, 0.3f, 0.0f, 1.0f},
		XMVECTOR{0.7f, 0.5f, 0.0f, 1.0f},
		XMVECTOR{0.0f, 1.0f, 0.0, 1.0f},

		XMVECTOR{0.0f, 0.8f, 0.5f, 1.0f},
		XMVECTOR{0.7f, 0.9f, 0.0f, 1.0f},
		XMVECTOR{0.7f, 0.8f, 0.6f, 1.0f},
		XMVECTOR{0.2f, 0.9f, 1.0, 1.0f},

		XMVECTOR{0.949f, 0.843f, 0.561, 1.0f},
		XMVECTOR{0.949f, 0.843f, 0.561, 1.0f},
		XMVECTOR{0.949f, 0.843f, 0.561, 1.0f},
		XMVECTOR{0.949f, 0.843f, 0.561, 1.0f},
	};

	m_perInstanceConstantBuffers.resize(7);

	auto vertexCountPerInstance = 4;

	int i(0);
	for (auto& cb : m_perInstanceConstantBuffers)
	{
		const uint32_t bufferSize = sizeof(XMVECTOR) * vertexCountPerInstance;
		cb = nv_helpers_dx12::CreateBuffer(m_dxrDevice.Get(), bufferSize, D3D12_RESOURCE_FLAG_NONE,
			D3D12_RESOURCE_STATE_GENERIC_READ,
			nv_helpers_dx12::kUploadHeapProps);
		uint8_t* pData;
		ThrowIfFailed(cb->Map(0, nullptr, (void**)&pData));
		memcpy(pData, &bufferData[i * 4], bufferSize);
		cb->Unmap(0, nullptr);
		++i;
	}
}

void SonarPropagation::Graphics::DXR::RayTracingRenderer::CreateDeviceDependentResources() {

	CreateRaytracingInterfaces();

	CheckRayTracingSupport();

	auto d3dDevice = m_dxrDevice;

	// Create a root signature with a single constant buffer slot.
	{
		CD3DX12_DESCRIPTOR_RANGE range;
		CD3DX12_ROOT_PARAMETER parameter;

		range.Init(D3D12_DESCRIPTOR_RANGE_TYPE_CBV, 1, 0);
		parameter.InitAsDescriptorTable(1, &range, D3D12_SHADER_VISIBILITY_ALL);

		D3D12_ROOT_SIGNATURE_FLAGS rootSignatureFlags =
			D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT | // Only the input assembler stage needs access to the constant buffer.
			D3D12_ROOT_SIGNATURE_FLAG_DENY_DOMAIN_SHADER_ROOT_ACCESS |
			D3D12_ROOT_SIGNATURE_FLAG_DENY_GEOMETRY_SHADER_ROOT_ACCESS |
			D3D12_ROOT_SIGNATURE_FLAG_DENY_HULL_SHADER_ROOT_ACCESS |
			D3D12_ROOT_SIGNATURE_FLAG_DENY_PIXEL_SHADER_ROOT_ACCESS;

		CD3DX12_ROOT_SIGNATURE_DESC descRootSignature;
		descRootSignature.Init(1, &parameter, 0, nullptr, rootSignatureFlags);

		ComPtr<ID3DBlob> pSignature;
		ComPtr<ID3DBlob> pError;
		ThrowIfFailed(D3D12SerializeRootSignature(&descRootSignature, D3D_ROOT_SIGNATURE_VERSION_1, pSignature.GetAddressOf(), pError.GetAddressOf()));
		ThrowIfFailed(d3dDevice->CreateRootSignature(0, pSignature->GetBufferPointer(), pSignature->GetBufferSize(), IID_PPV_ARGS(&m_rootSignature)));
		NAME_D3D12_OBJECT(m_rootSignature);
	}

	// Load shaders asynchronously.
	auto createVSTask = DX::ReadDataAsync(L"SampleVertexShader.cso").then([this](std::vector<byte>& fileData) {
		m_vertexShader = fileData;
		});

	auto createPSTask = DX::ReadDataAsync(L"SamplePixelShader.cso").then([this](std::vector<byte>& fileData) {
		m_pixelShader = fileData;
		});

	// Create the pipeline state once the shaders are loaded.
	auto createPipelineStateTask = (createPSTask && createVSTask).then([this]() {

		static const D3D12_INPUT_ELEMENT_DESC inputLayout[] =
		{
			{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
			{ "COLOR", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 12, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		};

		D3D12_GRAPHICS_PIPELINE_STATE_DESC state = {};
		state.InputLayout = { inputLayout, _countof(inputLayout) };
		state.pRootSignature = m_rootSignature.Get();
		state.VS = CD3DX12_SHADER_BYTECODE(&m_vertexShader[0], m_vertexShader.size());
		state.PS = CD3DX12_SHADER_BYTECODE(&m_pixelShader[0], m_pixelShader.size());
		state.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
		state.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
		state.DepthStencilState = CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);
		state.SampleMask = UINT_MAX;
		state.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
		state.NumRenderTargets = 1;
		state.RTVFormats[0] = m_deviceResources->GetBackBufferFormat();
		state.DSVFormat = m_deviceResources->GetDepthBufferFormat();
		state.SampleDesc.Count = 1;
		state.RasterizerState.CullMode = D3D12_CULL_MODE_BACK;

		DX::ThrowIfFailed(m_dxrDevice.Get()->CreateGraphicsPipelineState(&state, IID_PPV_ARGS(&m_pipelineState)));

		// Shader data can be deleted once the pipeline state is created.
		m_vertexShader.clear();
		m_pixelShader.clear();
		});

	// Create and upload cube geometry resources to the GPU.
	auto createAssetsTask = createPipelineStateTask.then([this]() {
		auto d3dDevice = m_dxrDevice.Get();

		// Create a command list.
		ThrowIfFailed(d3dDevice->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, m_deviceResources->GetCommandAllocator(), m_pipelineState.Get(), IID_PPV_ARGS(&m_commandList)));
		NAME_D3D12_OBJECT(m_commandList);

		CD3DX12_HEAP_PROPERTIES uploadHeapProperties(D3D12_HEAP_TYPE_UPLOAD);

		auto aspectRatio = m_deviceResources->GetOutputSize().Width / m_deviceResources->GetOutputSize().Height;

		CD3DX12_HEAP_PROPERTIES heapProperty =
			CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);

		CreateVertexBuffers<VertexPositionColor>();

		CreateIndexBuffers();

		// Create a descriptor heap for the constant buffers.
		{
			D3D12_DESCRIPTOR_HEAP_DESC heapDesc = {};
			heapDesc.NumDescriptors = DX::c_frameCount;
			heapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
			// This flag indicates that this descriptor heap can be bound to the pipeline and that descriptors contained in it can be referenced by a root table.
			heapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
			ThrowIfFailed(d3dDevice->CreateDescriptorHeap(&heapDesc, IID_PPV_ARGS(&m_cbvHeap)));

			NAME_D3D12_OBJECT(m_cbvHeap);
		}

		{
			D3D12_DESCRIPTOR_HEAP_DESC heapDesc = {};
			heapDesc.NumDescriptors = 1;
			heapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
			// This flag indicates that this descriptor heap can be bound to the pipeline and that descriptors contained in it can be referenced by a root table.
			heapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
			ThrowIfFailed(d3dDevice->CreateDescriptorHeap(&heapDesc, IID_PPV_ARGS(&m_imguiHeap)));

			NAME_D3D12_OBJECT(m_imguiHeap);
		}

		// Wait for the command list to finish executing; the vertex/index buffers need to be uploaded to the GPU before the upload resources go out of scope.
		m_deviceResources->WaitForGpu();

		});


	auto createDXRResources = createAssetsTask.then([this]() {

		// Create the acceleration structures
		CreateAccelerationStructures<VertexPositionColor>();

		ThrowIfFailed(m_commandList->Close());

		CreateRaytracingPipeline();

		CreatePerInstanceConstantBuffers();

		CreateRaytracingOutputBuffer();

		m_camera.CreateCameraBuffer(m_dxrDevice);

		m_cameraController.AddCamera(&m_camera);

		CreateShaderResourceHeap();

		CreateShaderBindingTable();

		m_imguiManager.InitImGui(DX::c_frameCount, m_deviceResources->GetBackBufferFormat(), m_dxrDevice.Get(), m_imguiHeap.Get());

		});

	createDXRResources.then([this]() {
		m_loadingComplete = true;
		});

}

template <typename V>
void SonarPropagation::Graphics::DXR::RayTracingRenderer::CreateVertexBuffers() {

	{
		std::vector<V> tetrahedronVertices = GetTetrahedronVertices<V>();

		const UINT tetrahedronVertexBufferSize = sizeof(tetrahedronVertices) * sizeof(V);
		ThrowIfFailed(m_dxrDevice->CreateCommittedResource(
			&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD), D3D12_HEAP_FLAG_NONE,
			&CD3DX12_RESOURCE_DESC::Buffer(tetrahedronVertexBufferSize),
			D3D12_RESOURCE_STATE_GENERIC_READ, nullptr,
			IID_PPV_ARGS(&m_tetrahedronVertexBuffer)));

		UINT8* pVertexDataBegin;
		CD3DX12_RANGE readRange(
			0, 0); // We do not intend to read from this resource on the CPU.
		ThrowIfFailed(m_tetrahedronVertexBuffer->Map(
			0, &readRange, reinterpret_cast<void**>(&pVertexDataBegin)));
		memcpy(pVertexDataBegin, tetrahedronVertices.data(), tetrahedronVertexBufferSize);
		m_tetrahedronVertexBuffer->Unmap(0, nullptr);

		m_tetrahedronVertexBufferView.BufferLocation = m_tetrahedronVertexBuffer->GetGPUVirtualAddress();
		m_tetrahedronVertexBufferView.StrideInBytes = sizeof(V);
		m_tetrahedronVertexBufferView.SizeInBytes = tetrahedronVertexBufferSize;

		NAME_D3D12_OBJECT(m_tetrahedronVertexBuffer);


	}

	{
		std::vector<V> quadVertices = GetQuadVertices<V>(6, 6);
		const UINT quadVertexBufferSize = sizeof(quadVertices) * sizeof(V);

		ThrowIfFailed(m_dxrDevice->CreateCommittedResource(
			&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD), D3D12_HEAP_FLAG_NONE,
			&CD3DX12_RESOURCE_DESC::Buffer(quadVertexBufferSize),
			D3D12_RESOURCE_STATE_GENERIC_READ, nullptr,
			IID_PPV_ARGS(&m_quadVertexBuffer)));

		UINT8* pVertexDataBegin;
		CD3DX12_RANGE readRange(
			0, 0); // We do not intend to read from this resource on the CPU.
		ThrowIfFailed(m_quadVertexBuffer->Map(
			0, &readRange, reinterpret_cast<void**>(&pVertexDataBegin)));
		memcpy(pVertexDataBegin, quadVertices.data(), quadVertexBufferSize);
		m_quadVertexBuffer->Unmap(0, nullptr);

		m_quadVertexBufferView.BufferLocation = m_quadVertexBuffer->GetGPUVirtualAddress();
		m_quadVertexBufferView.StrideInBytes = sizeof(V);
		m_quadVertexBufferView.SizeInBytes = quadVertexBufferSize;

		NAME_D3D12_OBJECT(m_quadVertexBuffer);

	}
}

void SonarPropagation::Graphics::DXR::RayTracingRenderer::CreateIndexBuffers()
{
	CD3DX12_HEAP_PROPERTIES heapProperty =
		CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);
	CD3DX12_RANGE readRange(
		0, 0); // We do not intend to read from this resource on the CPU.
	{
		std::vector<UINT> indices = GetTetrahedronIndices();
		const UINT indexBufferSize =
			static_cast<UINT>(indices.size()) * sizeof(UINT);

		CD3DX12_RESOURCE_DESC bufferResource =
			CD3DX12_RESOURCE_DESC::Buffer(indexBufferSize);
		ThrowIfFailed(m_dxrDevice->CreateCommittedResource(
			&heapProperty, D3D12_HEAP_FLAG_NONE, &bufferResource, //
			D3D12_RESOURCE_STATE_GENERIC_READ, nullptr,
			IID_PPV_ARGS(&m_tetrahedronIndexBuffer)));

		// Copy the triangle data to the index buffer.
		UINT8* pIndexDataBegin;
		ThrowIfFailed(m_tetrahedronIndexBuffer->Map(
			0, &readRange, reinterpret_cast<void**>(&pIndexDataBegin)));
		memcpy(pIndexDataBegin, indices.data(), indexBufferSize);
		m_tetrahedronIndexBuffer->Unmap(0, nullptr);

		// Initialize the index buffer view.
		m_tetrahedronIndexBufferView.BufferLocation = m_tetrahedronIndexBuffer->GetGPUVirtualAddress();
		m_tetrahedronIndexBufferView.Format = DXGI_FORMAT_R32_UINT;
		m_tetrahedronIndexBufferView.SizeInBytes = indexBufferSize;

		NAME_D3D12_OBJECT(m_tetrahedronIndexBuffer);
	}

	{
		std::vector<UINT> indices = GetQuadIndices();
		const UINT quadIndexBufferSize =
			static_cast<UINT>(indices.size()) * sizeof(UINT);

		CD3DX12_RESOURCE_DESC bufferResource =
			CD3DX12_RESOURCE_DESC::Buffer(quadIndexBufferSize);
		ThrowIfFailed(m_dxrDevice->CreateCommittedResource(
			&heapProperty, D3D12_HEAP_FLAG_NONE, &bufferResource, //
			D3D12_RESOURCE_STATE_GENERIC_READ, nullptr,
			IID_PPV_ARGS(&m_quadIndexBuffer)));

		// Copy the triangle data to the index buffer.
		UINT8* pIndexDataBegin;
		ThrowIfFailed(m_quadIndexBuffer->Map(
			0, &readRange, reinterpret_cast<void**>(&pIndexDataBegin)));
		memcpy(pIndexDataBegin, indices.data(), quadIndexBufferSize);
		m_quadIndexBuffer->Unmap(0, nullptr);

		// Initialize the index buffer view.
		m_quadIndexBufferView.BufferLocation = m_tetrahedronIndexBuffer->GetGPUVirtualAddress();
		m_quadIndexBufferView.Format = DXGI_FORMAT_R32_UINT;
		m_quadIndexBufferView.SizeInBytes = quadIndexBufferSize;


		NAME_D3D12_OBJECT(m_quadIndexBuffer);
	}
}

void SonarPropagation::Graphics::DXR::RayTracingRenderer::CreateWindowSizeDependentResources() {
	Size outputSize = m_deviceResources->GetOutputSize();
	m_aspectRatio = outputSize.Width / outputSize.Height;
	float fovAngleY = 70.0f * XM_PI / 180.0f;

	m_camera.SetAspectRatio(m_aspectRatio);
	m_camera.SetFOV(fovAngleY);

	D3D12_VIEWPORT viewport = m_deviceResources->GetScreenViewport();
	m_scissorRect = { 0, 0, static_cast<LONG>(viewport.Width), static_cast<LONG>(viewport.Height) };

	// This is a simple example of change that can be made when the app is in
	// portrait or snapped view.
	if (m_aspectRatio < 1.0f)
	{
		fovAngleY *= 2.0f;
	}


	//Initialization dependent resources
	if (!m_loadingComplete) {
		return;
	}

	CreateRaytracingOutputBuffer();
	CreateShaderResourceHeap();
	CreateShaderBindingTable();

}

#pragma endregion

#pragma region Raytracing Init.:

/// <summary>
/// Create the acceleration structures from the pre-initialized vertex and index buffers.
/// </summary>

template <typename V>
void SonarPropagation::Graphics::DXR::RayTracingRenderer::CreateAccelerationStructures() {
	AccelerationStructureBuffers bottomLevelBuffers =
		CreateBottomLevelAS<V>(
			{
				{m_tetrahedronVertexBuffer.Get(), m_tetrahedronVertexBufferView.SizeInBytes / sizeof(V)}
			},
			{
				{m_tetrahedronIndexBuffer.Get(), m_tetrahedronIndexBufferView.SizeInBytes / sizeof(UINT)}
			}
		);
	AccelerationStructureBuffers planeBottomLevelBuffers =
		CreateBottomLevelAS<V>(
			{
				{m_quadVertexBuffer.Get(), m_tetrahedronVertexBufferView.SizeInBytes / sizeof(V)}
			},
			{
				{m_quadIndexBuffer.Get(), m_quadIndexBufferView.SizeInBytes / sizeof(UINT)}
			}
		);

	CreateInstances(
		{
			{bottomLevelBuffers,6},
		}
		);

	m_instances.push_back({ planeBottomLevelBuffers.pResult, XMMatrixTranslation(0.f,-2.5f,0.f) });

	CreateTopLevelAS(m_instances, false);

	m_commandList->Close();
	ID3D12CommandList* ppCommandLists[] = { m_commandList.Get() };
	m_deviceResources->GetCommandQueue()->ExecuteCommandLists(1, ppCommandLists);
	m_deviceResources->WaitForGpu();

	ThrowIfFailed(
		m_commandList->Reset(m_deviceResources->GetCommandAllocator(), m_pipelineState.Get()));

	m_bottomLevelAS = bottomLevelBuffers.pResult;

	NAME_D3D12_OBJECT(m_bottomLevelAS);
}

/// <summary>
/// Create the raytracing device for raytracing.
/// </summary>
void SonarPropagation::Graphics::DXR::RayTracingRenderer::CreateRaytracingInterfaces() {
	auto device = m_deviceResources->GetD3DDevice();
	ThrowIfFailed(device->QueryInterface(IID_PPV_ARGS(&m_dxrDevice)), L"Couldn't get DirectX Raytracing interface for the device.\n");
}

template <typename V>
SonarPropagation::Graphics::DXR::AccelerationStructureBuffers
SonarPropagation::Graphics::DXR::RayTracingRenderer::CreateBottomLevelAS(
	std::vector<std::pair<ComPtr<ID3D12Resource>, uint32_t>> vVertexBuffers,
	std::vector<std::pair<ComPtr<ID3D12Resource>, uint32_t>> vIndexBuffers
) {
	nv_helpers_dx12::BottomLevelASGenerator bottomLevelAS;

	for (size_t i = 0; i < vVertexBuffers.size(); i++) {
		if (i < vIndexBuffers.size() && vIndexBuffers[i].second > 0)
			bottomLevelAS.AddVertexBuffer(vVertexBuffers[i].first.Get(), 0,
				vVertexBuffers[i].second, sizeof(V),
				vIndexBuffers[i].first.Get(), 0,
				vIndexBuffers[i].second, nullptr, 0, true);

		else
			bottomLevelAS.AddVertexBuffer(vVertexBuffers[i].first.Get(), 0,
				vVertexBuffers[i].second, sizeof(V), 0,
				0);
	}

	UINT64 scratchSizeInBytes = 0;

	UINT64 resultSizeInBytes = 0;

	bottomLevelAS.ComputeASBufferSizes(m_dxrDevice.Get(), false, &scratchSizeInBytes,
		&resultSizeInBytes);

	AccelerationStructureBuffers buffers;
	buffers.pScratch = nv_helpers_dx12::CreateBuffer(
		m_dxrDevice.Get(), scratchSizeInBytes,
		D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_COMMON,
		nv_helpers_dx12::kDefaultHeapProps);
	buffers.pResult = nv_helpers_dx12::CreateBuffer(
		m_dxrDevice.Get(), resultSizeInBytes,
		D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS,
		D3D12_RESOURCE_STATE_RAYTRACING_ACCELERATION_STRUCTURE,
		nv_helpers_dx12::kDefaultHeapProps);

	bottomLevelAS.Generate(m_commandList.Get(), buffers.pScratch.Get(),
		buffers.pResult.Get(), false, nullptr);

	return buffers;
}

void SonarPropagation::Graphics::DXR::RayTracingRenderer::CreateTopLevelAS(const std::vector<std::pair<ComPtr<ID3D12Resource>, DirectX::XMMATRIX>>& instances, bool updateOnly = false
) {
	if (!updateOnly) {
		for (size_t i = 0; i < instances.size(); i++) {
			m_topLevelASGenerator.AddInstance(
				instances[i].first.Get(), instances[i].second, static_cast<UINT>(i),
				static_cast<UINT>(i * 2));
		}

		UINT64 scratchSize, resultSize, instanceDescsSize;

		m_topLevelASGenerator.ComputeASBufferSizes(
			m_dxrDevice.Get(), true, &scratchSize, &resultSize, &instanceDescsSize);

		m_topLevelASBuffers.pScratch = nv_helpers_dx12::CreateBuffer(
			m_dxrDevice.Get(), scratchSize, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS,
			D3D12_RESOURCE_STATE_UNORDERED_ACCESS,
			nv_helpers_dx12::kDefaultHeapProps);
		m_topLevelASBuffers.pResult = nv_helpers_dx12::CreateBuffer(
			m_dxrDevice.Get(), resultSize, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS,
			D3D12_RESOURCE_STATE_RAYTRACING_ACCELERATION_STRUCTURE,
			nv_helpers_dx12::kDefaultHeapProps);

		m_topLevelASBuffers.pInstanceDesc = nv_helpers_dx12::CreateBuffer(
			m_dxrDevice.Get(), instanceDescsSize, D3D12_RESOURCE_FLAG_NONE,
			D3D12_RESOURCE_STATE_GENERIC_READ, nv_helpers_dx12::kUploadHeapProps);
	}

	m_topLevelASGenerator.Generate(m_commandList.Get(),
		m_topLevelASBuffers.pScratch.Get(),
		m_topLevelASBuffers.pResult.Get(),
		m_topLevelASBuffers.pInstanceDesc.Get(),
		updateOnly, m_topLevelASBuffers.pResult.Get());
}

/// <summary>
/// Create the signature of the ray generation shader.
/// </summary>
/// <returns></returns>
ComPtr<ID3D12RootSignature> SonarPropagation::Graphics::DXR::RayTracingRenderer::CreateRayGenSignature() {
	nv_helpers_dx12::RootSignatureGenerator rsc;
	rsc.AddHeapRangesParameter(
		{ {0 /*u0*/, 1 /*1 descriptor */, 0 /*use the implicit register space 0*/,
		  D3D12_DESCRIPTOR_RANGE_TYPE_UAV /* UAV representing the output buffer*/,
		  0 /*heap slot where the UAV is defined*/},
		 {0 /*t0*/, 1, 0,
		  D3D12_DESCRIPTOR_RANGE_TYPE_SRV /*Top-level acceleration structure*/,
		  1},
		 {0, 1, 0, D3D12_DESCRIPTOR_RANGE_TYPE_CBV, 2}
		});

	return rsc.Generate(m_dxrDevice.Get(), true);
}

/// <summary>
/// Create the signature of the hit shader.
/// </summary>
/// <returns></returns>
ComPtr<ID3D12RootSignature> SonarPropagation::Graphics::DXR::RayTracingRenderer::CreateHitSignature() {
	nv_helpers_dx12::RootSignatureGenerator rsc;

	rsc.AddRootParameter(D3D12_ROOT_PARAMETER_TYPE_SRV, 0);
	rsc.AddRootParameter(D3D12_ROOT_PARAMETER_TYPE_SRV, 1);
	rsc.AddRootParameter(D3D12_ROOT_PARAMETER_TYPE_CBV, 0);

	rsc.AddHeapRangesParameter(
		{
			{2,1,0,D3D12_DESCRIPTOR_RANGE_TYPE_SRV,1},
		}
		);

	return rsc.Generate(m_dxrDevice.Get(), true);
}

/// <summary>
/// Create the signature of the miss shader.
/// </summary>
/// <returns></returns>
ComPtr<ID3D12RootSignature> SonarPropagation::Graphics::DXR::RayTracingRenderer::CreateMissSignature() {
	nv_helpers_dx12::RootSignatureGenerator rsc;
	return rsc.Generate(m_dxrDevice.Get(), true);
}

/// <summary>
/// Create the raytracing pipeline from HLSL shaders.
/// Initializes the libraries, shaders, hit groups, root signatures, etc.
/// </summary>
void SonarPropagation::Graphics::DXR::RayTracingRenderer::CreateRaytracingPipeline()
{
	nv_helpers_dx12::RayTracingPipelineGenerator pipeline(m_dxrDevice.Get());

	m_rayGenLibrary = CompileShader(L"RayGen.hlsl");
	m_missLibrary = CompileShader(L"Miss.hlsl");
	m_hitLibrary = CompileShader(L"Hit.hlsl");
	m_shadowLibrary = CompileShader(L"Shadow.hlsl");
	m_reflectionLibrary = CompileShader(L"Reflection.hlsl");

	pipeline.AddLibrary(m_rayGenLibrary.Get(), { L"RayGen" });
	pipeline.AddLibrary(m_missLibrary.Get(), { L"Miss" });
	pipeline.AddLibrary(m_hitLibrary.Get(), { L"ClosestHit",L"QuadClosestHit",L"QuadReflectionClosestHit" });
	pipeline.AddLibrary(m_shadowLibrary.Get(), { L"ShadowClosestHit",L"ShadowMiss" });
	pipeline.AddLibrary(m_reflectionLibrary.Get(), { L"ReflectionClosestHit", L"ReflectionMiss" });

	m_rayGenSignature = CreateRayGenSignature();
	m_missSignature = CreateMissSignature();
	m_hitSignature = CreateHitSignature();
	//m_shadowSignature = CreateHitSignature();
	//m_reflectionSignature = CreateHitSignature();

	pipeline.AddHitGroup(L"HitGroup", L"ClosestHit");
	pipeline.AddHitGroup(L"QuadHitGroup", L"QuadClosestHit");
	pipeline.AddHitGroup(L"QuadReflectionHitGroup", L"QuadReflectionClosestHit");
	pipeline.AddHitGroup(L"ShadowHitGroup", L"ShadowClosestHit");
	pipeline.AddHitGroup(L"ReflectionHitGroup", L"ReflectionClosestHit");

	pipeline.AddRootSignatureAssociation(m_rayGenSignature.Get(), { L"RayGen" });
	pipeline.AddRootSignatureAssociation(m_missSignature.Get(), { L"Miss",L"ShadowMiss", L"ReflectionMiss" });
	pipeline.AddRootSignatureAssociation(m_hitSignature.Get(), { L"HitGroup",	L"QuadHitGroup", L"QuadReflectionHitGroup" });
	pipeline.AddRootSignatureAssociation(m_shadowSignature.Get(), { L"ShadowHitGroup" });
	pipeline.AddRootSignatureAssociation(m_reflectionSignature.Get(), { L"ReflectionHitGroup" });

	pipeline.SetMaxPayloadSize(m_dxrConfig.m_maxPayloadSize); // RGB + distance

	pipeline.SetMaxAttributeSize(m_dxrConfig.m_maxAttributeSize); // barycentric coordinates

	pipeline.SetMaxRecursionDepth(m_dxrConfig.m_recursionDepth);

	// Compile the pipeline for execution on the GPU
	m_rtStateObject = pipeline.Generate();

	ThrowIfFailed(
		m_rtStateObject->QueryInterface(IID_PPV_ARGS(&m_rtStateObjectProps)));
}

void SonarPropagation::Graphics::DXR::RayTracingRenderer::CreateRaytracingOutputBuffer() {
	D3D12_RESOURCE_DESC resDesc = {};
	resDesc.DepthOrArraySize = 1;
	resDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
	resDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;

	resDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;
	resDesc.Width = m_deviceResources->GetOutputSize().Width;
	resDesc.Height = m_deviceResources->GetOutputSize().Height;
	resDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
	resDesc.MipLevels = 1;
	resDesc.SampleDesc.Count = 1;
	ThrowIfFailed(m_dxrDevice.Get()->CreateCommittedResource(
		&nv_helpers_dx12::kDefaultHeapProps, D3D12_HEAP_FLAG_NONE, &resDesc,
		D3D12_RESOURCE_STATE_COPY_SOURCE, nullptr,
		IID_PPV_ARGS(&m_outputResource)));
}

void SonarPropagation::Graphics::DXR::RayTracingRenderer::CreateShaderResourceHeap() {

	m_srvUavHeap = CreateDescriptorHeap(
		m_dxrDevice.Get(), 4, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, true);

	NAME_D3D12_OBJECT(m_srvUavHeap);

	D3D12_CPU_DESCRIPTOR_HANDLE srvHandle =
		m_srvUavHeap->GetCPUDescriptorHandleForHeapStart();

	D3D12_UNORDERED_ACCESS_VIEW_DESC uavDesc = {};
	uavDesc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE2D;
	m_dxrDevice.Get()->CreateUnorderedAccessView(m_outputResource.Get(), nullptr, &uavDesc,
		srvHandle);

	srvHandle.ptr += m_dxrDevice.Get()->GetDescriptorHandleIncrementSize(
		D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

	D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc;
	srvDesc.Format = DXGI_FORMAT_UNKNOWN;
	srvDesc.ViewDimension = D3D12_SRV_DIMENSION_RAYTRACING_ACCELERATION_STRUCTURE;
	srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	srvDesc.RaytracingAccelerationStructure.Location =
		m_topLevelASBuffers.pResult->GetGPUVirtualAddress();
	m_dxrDevice.Get()->CreateShaderResourceView(nullptr, &srvDesc, srvHandle);

	srvHandle.ptr += m_dxrDevice.Get()->GetDescriptorHandleIncrementSize(
		D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

	D3D12_CONSTANT_BUFFER_VIEW_DESC cbvDesc = {};
	cbvDesc.BufferLocation = m_camera.GetCameraBuffer()->GetGPUVirtualAddress();
	cbvDesc.SizeInBytes = m_camera.GetCameraBufferSize();
	m_dxrDevice->CreateConstantBufferView(&cbvDesc, srvHandle);

}


/// <summary>
/// Create the shader binding table for the raytracing pipeline by initializing the RayGen, Miss shaders
/// and the Hit shaders per instances.
/// </summary>
void SonarPropagation::Graphics::DXR::RayTracingRenderer::CreateShaderBindingTable() {

	m_sbtHelper.Reset();

	D3D12_GPU_DESCRIPTOR_HANDLE srvUavHeapHandle =
		m_srvUavHeap->GetGPUDescriptorHandleForHeapStart();

	auto heapPointer = reinterpret_cast<UINT64*>(srvUavHeapHandle.ptr);


	m_sbtHelper.AddRayGenerationProgram(L"RayGen", { heapPointer });
	m_sbtHelper.AddMissProgram(L"Miss", {});


	if (m_useReflections) {
		m_sbtHelper.AddMissProgram(L"ReflectionMiss", {});
	}
	else {
		m_sbtHelper.AddMissProgram(L"ShadowMiss", {});
	}

	auto constNum = m_perInstanceConstantBuffers.size();

	for (int i = 0; i < constNum - 1; ++i) {
		m_sbtHelper.AddHitGroup(
			L"HitGroup",
			{
				(void*)(m_tetrahedronVertexBuffer->GetGPUVirtualAddress()),
				(void*)(m_tetrahedronIndexBuffer->GetGPUVirtualAddress()),
				(void*)(m_perInstanceConstantBuffers[i]->GetGPUVirtualAddress()),
				heapPointer,
			});
		if (m_useReflections) {
			m_sbtHelper.AddHitGroup(L"ReflectionHitGroup", {});
		}
		else {
			m_sbtHelper.AddHitGroup(L"ShadowHitGroup", {});
		}
	}

	if (m_useReflections) {
		m_sbtHelper.AddHitGroup(
			L"QuadReflectionHitGroup",
			{
				(void*)(m_quadVertexBuffer->GetGPUVirtualAddress()),
				(void*)(m_quadIndexBuffer->GetGPUVirtualAddress()),
				(void*)(m_perInstanceConstantBuffers[constNum - 1]->GetGPUVirtualAddress()),
				heapPointer,
			});
	}
	else {
		m_sbtHelper.AddHitGroup(
			L"QuadHitGroup",
			{
				(void*)(m_quadVertexBuffer->GetGPUVirtualAddress()),
				(void*)(m_quadIndexBuffer->GetGPUVirtualAddress()),
				(void*)(m_perInstanceConstantBuffers[constNum - 1]->GetGPUVirtualAddress()),
				heapPointer,
			});
	}
	// Shadow hit group is added after each addition of the original hitgroup, 
	// so that all geometry can be hit!
	uint32_t sbtSize = m_sbtHelper.ComputeSBTSize();

	m_sbtStorage = nv_helpers_dx12::CreateBuffer(
		m_dxrDevice.Get(), sbtSize, D3D12_RESOURCE_FLAG_NONE,
		D3D12_RESOURCE_STATE_GENERIC_READ, nv_helpers_dx12::kUploadHeapProps);
	if (!m_sbtStorage) {
		throw std::logic_error("Could not allocate the shader binding table");
	}

	m_sbtHelper.Generate(m_sbtStorage.Get(), m_rtStateObjectProps.Get());
}

/// <summary>
/// Creates the transform matrices for the instances and populates the associated metadata.
/// </summary>
void SonarPropagation::Graphics::DXR::RayTracingRenderer::CreateInstances(
	std::vector<std::pair<AccelerationStructureBuffers, uint32_t>> asBuffers
) {


	for (int i = 0; i < asBuffers.size(); ++i) {

		float amount = asBuffers[i].second;


		for (int j = 0; j < amount; ++j) {
			m_instances.push_back({ asBuffers[i].first.pResult, (
				XMMatrixTranslation(1.75f,.0f,.0f) *
				XMMatrixRotationAxis(XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f), (360.f / amount) * j)
				) });
		}

	}
}


#pragma endregion

#pragma region Raytracing Utils.:

void SonarPropagation::Graphics::DXR::RayTracingRenderer::CheckRayTracingSupport() {
	D3D12_FEATURE_DATA_D3D12_OPTIONS5 options5 = {};
	ThrowIfFailed(m_dxrDevice.Get()->CheckFeatureSupport(D3D12_FEATURE_D3D12_OPTIONS5,
		&options5, sizeof(options5)));
	if (options5.RaytracingTier < D3D12_RAYTRACING_TIER_1_0)
		throw std::runtime_error("Raytracing not supported on device");
}

#pragma endregion

#pragma region Rendering Loop:



void SonarPropagation::Graphics::DXR::RayTracingRenderer::Update(DX::StepTimer const& timer) {

	if (!m_loadingComplete)
	{
		return;
	}

	if (m_animate) {
		UpdateInstanceTransforms();
	}

	m_cameraController.ProcessCameraUpdate(timer);
}

bool SonarPropagation::Graphics::DXR::RayTracingRenderer::Render() {

	if (!m_loadingComplete)
	{
		return false;
	}

	if (m_pipelineDirty) {
		CreateRaytracingPipeline();
		m_sbtDirty = true;
		m_pipelineDirty = false;
	}

	if (m_sbtDirty) {
		CreateShaderBindingTable();
		m_sbtDirty = false;
	}

	PopulateCommandListWithPIX();

	ID3D12CommandList* ppCommandLists[] = { m_commandList.Get() };
	m_deviceResources->GetCommandQueue()->ExecuteCommandLists(_countof(ppCommandLists), ppCommandLists);

	m_deviceResources->Present();

	m_deviceResources->WaitForGpu();

	return true;
}

void SonarPropagation::Graphics::DXR::RayTracingRenderer::SaveState() {
	return;
}

void SonarPropagation::Graphics::DXR::RayTracingRenderer::LoadState() {
	return;
}

void SonarPropagation::Graphics::DXR::RayTracingRenderer::PopulateCommandListWithPIX() {

	ThrowIfFailed(m_deviceResources->GetCommandAllocator()->Reset());

	// The command list can be reset anytime after ExecuteCommandList() is called.
	ThrowIfFailed(m_commandList->Reset(m_deviceResources->GetCommandAllocator(), m_pipelineState.Get()));

	PIXBeginEvent(m_commandList.Get(), 0, L"Draw Scene");
	{
		// Set the graphics root signature and descriptor heaps to be used by this frame.
		m_commandList->SetGraphicsRootSignature(m_rootSignature.Get());
		ID3D12DescriptorHeap* ppHeaps[] = { m_cbvHeap.Get() };
		m_commandList->SetDescriptorHeaps(_countof(ppHeaps), ppHeaps);

		// Bind the current frame's constant buffer to the pipeline.
		CD3DX12_GPU_DESCRIPTOR_HANDLE gpuHandle(m_cbvHeap->GetGPUDescriptorHandleForHeapStart(), m_deviceResources->GetCurrentFrameIndex(), m_cbvDescriptorSize);
		m_commandList->SetGraphicsRootDescriptorTable(0, gpuHandle);

		// Set the viewport and scissor rectangle.
		D3D12_VIEWPORT viewport = m_deviceResources->GetScreenViewport();
		m_commandList->RSSetViewports(1, &viewport);
		m_commandList->RSSetScissorRects(1, &m_scissorRect);

		// Indicate this resource will be in use as a render target.
		CD3DX12_RESOURCE_BARRIER renderTargetResourceBarrier =
			CD3DX12_RESOURCE_BARRIER::Transition(m_deviceResources->GetRenderTarget(), D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET);
		m_commandList->ResourceBarrier(1, &renderTargetResourceBarrier);

		// Record drawing commands.
		D3D12_CPU_DESCRIPTOR_HANDLE renderTargetView = m_deviceResources->GetRenderTargetView();
		D3D12_CPU_DESCRIPTOR_HANDLE depthStencilView = m_deviceResources->GetDepthStencilView();
		m_commandList->ClearRenderTargetView(renderTargetView, DirectX::Colors::Orange, 0, nullptr);
		m_commandList->ClearDepthStencilView(depthStencilView, D3D12_CLEAR_FLAG_DEPTH, 1.0f, 0, 0, nullptr);

		m_commandList->OMSetRenderTargets(1, &renderTargetView, false, &depthStencilView);

		// #DXR Extra - Refitting
		// Refit the top-level acceleration structure to account for the new
		// transform matrix of the triangle. Note that the build contains a barrier,
		// hence we can do the rendering in the same command list
		CreateTopLevelAS(m_instances, true);
		// #DXR
		// Bind the descriptor heap giving access to the top-level acceleration
		// structure, as well as the raytracing output
		std::vector<ID3D12DescriptorHeap*> heaps = { m_srvUavHeap.Get() };
		m_commandList->SetDescriptorHeaps(static_cast<UINT>(heaps.size()),
			heaps.data());

		// On the last frame, the raytracing output was used as a copy source, to
		// copy its contents into the render target. Now we need to transition it to
		// a UAV so that the shaders can write in it.
		CD3DX12_RESOURCE_BARRIER transition = CD3DX12_RESOURCE_BARRIER::Transition(
			m_outputResource.Get(), D3D12_RESOURCE_STATE_COPY_SOURCE,
			D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
		m_commandList->ResourceBarrier(1, &transition);

		// Setup the raytracing task
		D3D12_DISPATCH_RAYS_DESC desc = {};
		// The layout of the SBT is as follows: ray generation shader, miss
		// shaders, hit groups. As described in the CreateShaderBindingTable method,
		// all SBT entries of a given type have the same size to allow a fixed
		// stride.

		// The ray generation shaders are always at the beginning of the SBT.
		uint32_t rayGenerationSectionSizeInBytes = m_sbtHelper.GetRayGenSectionSize();
		desc.RayGenerationShaderRecord.StartAddress = m_sbtStorage->GetGPUVirtualAddress();
		desc.RayGenerationShaderRecord.SizeInBytes = rayGenerationSectionSizeInBytes;

		// The miss shaders are in the second SBT section, right after the ray
		// generation shader. We have one miss shader for the camera rays and one
		// for the shadow rays, so this section has a size of 2*m_sbtEntrySize. We
		// also indicate the stride between the two miss shaders, which is the size
		// of a SBT entry
		uint32_t missSectionSizeInBytes = m_sbtHelper.GetMissSectionSize();
		desc.MissShaderTable.StartAddress = m_sbtStorage->GetGPUVirtualAddress() + rayGenerationSectionSizeInBytes;
		desc.MissShaderTable.SizeInBytes = missSectionSizeInBytes;
		desc.MissShaderTable.StrideInBytes = m_sbtHelper.GetMissEntrySize();

		// The hit groups section start after the miss shaders. In this sample we
		// have one 1 hit group for the triangle
		uint32_t hitGroupsSectionSize = m_sbtHelper.GetHitGroupSectionSize();
		desc.HitGroupTable.StartAddress = m_sbtStorage->GetGPUVirtualAddress() + rayGenerationSectionSizeInBytes + missSectionSizeInBytes;
		desc.HitGroupTable.SizeInBytes = hitGroupsSectionSize;
		desc.HitGroupTable.StrideInBytes = m_sbtHelper.GetHitGroupEntrySize();

		// Dimensions of the image to render, identical to a kernel launch dimension
		desc.Width = m_deviceResources->GetScreenViewport().Width;
		desc.Height = m_deviceResources->GetScreenViewport().Height;
		desc.Depth = 1;

		// Bind the raytracing pipeline
		m_commandList->SetPipelineState1(m_rtStateObject.Get());
		// Dispatch the rays and write to the raytracing output
		m_commandList->DispatchRays(&desc);

		// The raytracing output needs to be copied to the actual render target used
		// for display. For this, we need to transition the raytracing output from a
		// UAV to a copy source, and the render target buffer to a copy destination.
		// We can then do the actual copy, before transitioning the render target
		// buffer into a render target, that will be then used to display the image
		{
			auto transitionOutputResource = CD3DX12_RESOURCE_BARRIER::Transition(
				m_outputResource.Get(), D3D12_RESOURCE_STATE_UNORDERED_ACCESS,
				D3D12_RESOURCE_STATE_COPY_SOURCE);
			//m_commandList->ResourceBarrier(1, &transition);
			auto transitionDeviceResources = CD3DX12_RESOURCE_BARRIER::Transition(
				m_deviceResources->GetRenderTarget(), D3D12_RESOURCE_STATE_RENDER_TARGET,
				D3D12_RESOURCE_STATE_COPY_DEST);
			
			std::vector<CD3DX12_RESOURCE_BARRIER> transitions = { transitionOutputResource, transitionDeviceResources };

			m_commandList->ResourceBarrier(2, transitions.data());
		}

		m_commandList->CopyResource(m_deviceResources->GetRenderTarget(),
			m_outputResource.Get());

		transition = CD3DX12_RESOURCE_BARRIER::Transition(
			m_deviceResources->GetRenderTarget(), D3D12_RESOURCE_STATE_COPY_DEST,
			D3D12_RESOURCE_STATE_RENDER_TARGET);

		m_commandList->ResourceBarrier(1, &transition);

		// Render ImGui
		m_commandList->SetDescriptorHeaps(1, m_imguiHeap.GetAddressOf());
		RenderImGui();

		m_commandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(
			m_deviceResources->GetRenderTarget(),
			D3D12_RESOURCE_STATE_RENDER_TARGET,
			D3D12_RESOURCE_STATE_PRESENT));
	}
	PIXEndEvent(m_commandList.Get());

	DX::ThrowIfFailed(m_commandList->Close());

}

void SonarPropagation::Graphics::DXR::RayTracingRenderer::RenderImGui() {
	m_imguiManager.BeginImGui(m_commandList.Get());

	{
		if (ImGui::Begin("Main Window"))
		{
			ImGuiTabBarFlags tabBarFlags = ImGuiTabBarFlags_None;
			if (ImGui::BeginTabBar("Windows", tabBarFlags))
			{
				if (ImGui::BeginTabItem("Controls"))
				{
					ImGui::Checkbox("Show Demo Window", &m_showDemoWindow);
					ImGui::Checkbox("Show Raytracing Controls", &m_showRaytracingWindow);
					ImGui::Checkbox("Show Camera Controls", &m_cameraWindow);
					ImGui::EndTabItem();
				}

				if (ImGui::BeginTabItem("Animation"))
				{
					ImGui::Checkbox("Animate", &m_animate);
					ImGui::EndTabItem();
				}
				ImGui::EndTabBar();
			}
			ImGui::End();
		}
	}

	{
		if (m_showDemoWindow)
		{
			ImGui::ShowDemoWindow(&m_showDemoWindow);
		}
	}

	{
		if (m_cameraWindow)
		{
			m_camera.RenderCameraImGui();
		}
	}

	{
		if (m_showRaytracingWindow)
		{
			if (ImGui::Begin("Raytracing"))
			{
				static bool showDXRInfo = false;
				static bool showDXRControls = false;
				ImGui::Checkbox("Show raytracing information", &showDXRInfo);
				ImGui::Checkbox("Show raytracing controls", &showDXRControls);

				if (showDXRInfo) {
					if (ImGui::BeginChild("Information", ImVec2(400, 200)))
					{
						int maxPayloadSize = m_dxrConfig.m_maxPayloadSize;
						int maxAttributeSize = m_dxrConfig.m_maxAttributeSize;
						int recursionDepth = m_dxrConfig.m_recursionDepth;

						ImGui::InputInt("Max Payload Size", &maxPayloadSize, 0, 0, ImGuiInputTextFlags_ReadOnly);
						ImGui::InputInt("Max Attribute Size", &maxAttributeSize, 0, 0, ImGuiInputTextFlags_ReadOnly);
						ImGui::InputInt("Max Recursion Depth", &recursionDepth, 0, 0, ImGuiInputTextFlags_ReadOnly);

						ImGui::EndChild();
					}
				}

				if (showDXRControls)
				{
					if (ImGui::BeginChild("Controls"))
					{
						if (ImGui::Checkbox("Use reflective materials?", &m_useReflections))
						{
							m_sbtDirty = true;
						}

						int recursionDepth = m_dxrConfig.m_recursionDepth;

						if (ImGui::InputInt("Ray Recursion Depth", &recursionDepth, 1, 1))
						{
							m_dxrConfig.m_recursionDepth = recursionDepth;
							m_pipelineDirty = true;
						}
						ImGui::EndChild();
					}
				}

				ImGui::End();

			}


		}
	}
	m_imguiManager.EndImGui(m_commandList.Get());

}

void SonarPropagation::Graphics::DXR::RayTracingRenderer::UpdateInstanceTransforms() {
	m_time++;

	auto tetrahedronAmount = m_instances.size() - 1;

	auto sinT = sinf(m_time);

	for (size_t i = 0; i < tetrahedronAmount; i++) {
		m_instances[i].second =

			XMMatrixTranslation(1.75f, .0f, .0f) *
			XMMatrixRotationAxis(XMVector4Normalize(XMVectorSet(0.0f, 0.8f, 0.2f, 0.0f)), (360.f / tetrahedronAmount) * i + (m_time) / 600.f);

	}
}


void SonarPropagation::Graphics::DXR::RayTracingRenderer::KeyPressed(Windows::UI::Core::KeyEventArgs^ args) {
	m_cameraController.KeyPressed(args);
}

void SonarPropagation::Graphics::DXR::RayTracingRenderer::KeyReleased(Windows::UI::Core::KeyEventArgs^ args) {
	m_cameraController.KeyReleased(args);
}

void SonarPropagation::Graphics::DXR::RayTracingRenderer::MouseMoved(Windows::UI::Core::PointerEventArgs^ args) {
	//m_cameraController.MouseMoved(args);
}

void SonarPropagation::Graphics::DXR::RayTracingRenderer::MouseWheelMoved(Windows::UI::Core::PointerEventArgs^ args) {
	m_cameraController.MouseWheelMoved(args);
}

#pragma endregion
