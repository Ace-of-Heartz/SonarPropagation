
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
	m_mappedConstantBuffer(nullptr)
{
	LoadState();
	ZeroMemory(&m_constantBufferData, sizeof(m_constantBufferData));

	CreateDeviceDependentResources();
	CreateWindowSizeDependentResources();
}

#pragma endregion

#pragma region Destructor

SonarPropagation::Graphics::DXR::RayTracingRenderer::~RayTracingRenderer() {
	m_constantBuffer->Unmap(0, nullptr);
	m_mappedConstantBuffer = nullptr;
}

#pragma endregion

#pragma region Graphics Init.: 



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

		VertexPositionColor vertices[] = {
			{{std::sqrtf(8.f / 9.f), 0.f, -1.f / 3.f}, {1.f, 0.f, 0.f}},
			{{-std::sqrtf(2.f / 9.f), std::sqrtf(2.f / 3.f), -1.f / 3.f}, {0.f, 1.f, 0.f}},
			{{-std::sqrtf(2.f / 9.f), -std::sqrtf(2.f / 3.f), -1.f / 3.f}, {0.f, 0.f, 1.f}},
			{{0.f, 0.f, 1.f}, {1, 0, 1}}
		};

		const UINT vertexBufferSize = sizeof(vertices);

		ThrowIfFailed(m_dxrDevice->CreateCommittedResource(
			&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD), D3D12_HEAP_FLAG_NONE,
			&CD3DX12_RESOURCE_DESC::Buffer(vertexBufferSize),
			D3D12_RESOURCE_STATE_GENERIC_READ, nullptr,
			IID_PPV_ARGS(&m_vertexBuffer)));

		UINT8* pVertexDataBegin;
		CD3DX12_RANGE readRange(
			0, 0); // We do not intend to read from this resource on the CPU.
		ThrowIfFailed(m_vertexBuffer->Map(
			0, &readRange, reinterpret_cast<void**>(&pVertexDataBegin)));
		memcpy(pVertexDataBegin, vertices, sizeof(vertices));
		m_vertexBuffer->Unmap(0, nullptr);

		m_vertexBufferView.BufferLocation = m_vertexBuffer->GetGPUVirtualAddress();
		m_vertexBufferView.StrideInBytes = sizeof(VertexPositionColor);
		m_vertexBufferView.SizeInBytes = vertexBufferSize;

		NAME_D3D12_OBJECT(m_vertexBuffer);

		std::vector<UINT> indices = { 0, 1, 2, 0, 3, 1, 0, 2, 3, 1, 3, 2 };
		const UINT indexBufferSize =
			static_cast<UINT>(indices.size()) * sizeof(UINT);

		CD3DX12_HEAP_PROPERTIES heapProperty =
			CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);
		CD3DX12_RESOURCE_DESC bufferResource =
			CD3DX12_RESOURCE_DESC::Buffer(indexBufferSize);
		ThrowIfFailed(m_dxrDevice->CreateCommittedResource(
			&heapProperty, D3D12_HEAP_FLAG_NONE, &bufferResource, //
			D3D12_RESOURCE_STATE_GENERIC_READ, nullptr,
			IID_PPV_ARGS(&m_indexBuffer)));

		// Copy the triangle data to the index buffer.
		UINT8* pIndexDataBegin;
		ThrowIfFailed(m_indexBuffer->Map(
			0, &readRange, reinterpret_cast<void**>(&pIndexDataBegin)));
		memcpy(pIndexDataBegin, indices.data(), indexBufferSize);
		m_indexBuffer->Unmap(0, nullptr);

		// Initialize the index buffer view.
		m_indexBufferView.BufferLocation = m_indexBuffer->GetGPUVirtualAddress();
		m_indexBufferView.Format = DXGI_FORMAT_R32_UINT;
		m_indexBufferView.SizeInBytes = indexBufferSize;

		NAME_D3D12_OBJECT(m_indexBuffer);

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

		CD3DX12_RESOURCE_DESC constantBufferDesc = CD3DX12_RESOURCE_DESC::Buffer(DX::c_frameCount * c_alignedConstantBufferSize);
		ThrowIfFailed(d3dDevice->CreateCommittedResource(
			&uploadHeapProperties,
			D3D12_HEAP_FLAG_NONE,
			&constantBufferDesc,
			D3D12_RESOURCE_STATE_GENERIC_READ,
			nullptr,
			IID_PPV_ARGS(&m_constantBuffer)));

		NAME_D3D12_OBJECT(m_constantBuffer);

		// Create constant buffer views to access the upload buffer.
		D3D12_GPU_VIRTUAL_ADDRESS cbvGpuAddress = m_constantBuffer->GetGPUVirtualAddress();
		CD3DX12_CPU_DESCRIPTOR_HANDLE cbvCpuHandle(m_cbvHeap->GetCPUDescriptorHandleForHeapStart());
		m_cbvDescriptorSize = d3dDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

		for (int n = 0; n < DX::c_frameCount; n++)
		{
			D3D12_CONSTANT_BUFFER_VIEW_DESC desc;
			desc.BufferLocation = cbvGpuAddress;
			desc.SizeInBytes = c_alignedConstantBufferSize;
			d3dDevice->CreateConstantBufferView(&desc, cbvCpuHandle);

			cbvGpuAddress += desc.SizeInBytes;
			cbvCpuHandle.Offset(m_cbvDescriptorSize);
		}

		// Map the constant buffers.
		CD3DX12_RANGE readRangeConstant(0, 0);		// We do not intend to read from this resource on the CPU.
		ThrowIfFailed(m_constantBuffer->Map(0, &readRangeConstant, reinterpret_cast<void**>(&m_mappedConstantBuffer)));
		ZeroMemory(m_mappedConstantBuffer, DX::c_frameCount * c_alignedConstantBufferSize);
		// We don't unmap this until the app closes. Keeping things mapped for the lifetime of the resource is okay.

		// Wait for the command list to finish executing; the vertex/index buffers need to be uploaded to the GPU before the upload resources go out of scope.
		m_deviceResources->WaitForGpu();

		});


	auto createDXRResources = createAssetsTask.then([this]() {

		// Create the acceleration structures
		CreateAccelerationStructures();

		ThrowIfFailed(m_commandList->Close());

		CreateRaytracingPipeline();

		CreateRaytracingOutputBuffer();

		m_camera.CreateCameraBuffer(m_dxrDevice);

		m_cameraController.AddCamera(&m_camera);

		CreateShaderResourceHeap();

		CreateShaderBindingTable();

		});

	createDXRResources.then([this]() {
		m_loadingComplete = true;
		});

}


void SonarPropagation::Graphics::DXR::RayTracingRenderer::CreateWindowSizeDependentResources() {
	Size outputSize = m_deviceResources->GetOutputSize();
	m_aspectRatio = outputSize.Width / outputSize.Height;
	float fovAngleY = 70.0f * XM_PI / 180.0f;

	D3D12_VIEWPORT viewport = m_deviceResources->GetScreenViewport();
	m_scissorRect = { 0, 0, static_cast<LONG>(viewport.Width), static_cast<LONG>(viewport.Height) };

	// This is a simple example of change that can be made when the app is in
	// portrait or snapped view.
	if (m_aspectRatio < 1.0f)
	{
		fovAngleY *= 2.0f;
	}


	// Note that the OrientationTransform3D matrix is post-multiplied here
	// in order to correctly orient the scene to match the display orientation.
	// This post-multiplication step is required for any draw calls that are
	// made to the swap chain render target. For draw calls to other targets,
	// this transform should not be applied.



	// This sample makes use of a right-handed coordinate system using row-major matrices.
	XMMATRIX perspectiveMatrix = XMMatrixPerspectiveFovRH(
		fovAngleY,
		m_aspectRatio,
		0.01f,
		100.0f
	);

	XMFLOAT4X4 orientation = m_deviceResources->GetOrientationTransform3D();
	XMMATRIX orientationMatrix = XMLoadFloat4x4(&orientation);

	XMStoreFloat4x4(
		&m_constantBufferData.projection,
		XMMatrixTranspose(perspectiveMatrix * orientationMatrix)
	);

	// Eye is at (0,0.7,1.5), looking at point (0,-0.1,0) with the up-vector along the y-axis.
	static const XMVECTORF32 eye = { 0.0f, 0.7f, 1.5f, 0.0f };
	static const XMVECTORF32 at = { 0.0f, -0.1f, 0.0f, 0.0f };
	static const XMVECTORF32 up = { 0.0f, 1.0f, 0.0f, 0.0f };

	XMStoreFloat4x4(&m_constantBufferData.view, XMMatrixTranspose(XMMatrixLookAtRH(eye, at, up)));
}

#pragma endregion

#pragma region Raytracing Init.:

/// <summary>
/// Create the acceleration structures from the pre-initialized vertex and index buffers.
/// </summary>
void SonarPropagation::Graphics::DXR::RayTracingRenderer::CreateAccelerationStructures() {
	AccelerationStructureBuffers bottomLevelBuffers =
		CreateBottomLevelAS({ {m_vertexBuffer.Get(), 4} },
			{ {m_indexBuffer.Get(), 12} }
		);

	m_instances = { {bottomLevelBuffers.pResult, XMMatrixIdentity()} };
	CreateTopLevelAS(m_instances, false);

	m_commandList->Close();
	ID3D12CommandList* ppCommandLists[] = { m_commandList.Get() };
	m_deviceResources->GetCommandQueue()->ExecuteCommandLists(1, ppCommandLists);
	m_deviceResources->WaitForGpu();

	ThrowIfFailed(
		m_commandList->Reset(m_deviceResources->GetCommandAllocator(), m_pipelineState.Get()));

	m_bottomLevelAS = bottomLevelBuffers.pResult;
}

/// <summary>
/// Create the raytracing device for raytracing.
/// </summary>
void SonarPropagation::Graphics::DXR::RayTracingRenderer::CreateRaytracingInterfaces() {
	auto device = m_deviceResources->GetD3DDevice();
	ThrowIfFailed(device->QueryInterface(IID_PPV_ARGS(&m_dxrDevice)), L"Couldn't get DirectX Raytracing interface for the device.\n");
}

SonarPropagation::Graphics::DXR::AccelerationStructureBuffers
SonarPropagation::Graphics::DXR::RayTracingRenderer::CreateBottomLevelAS(
	std::vector<std::pair<ComPtr<ID3D12Resource>, uint32_t>> vVertexBuffers,
	std::vector<std::pair<ComPtr<ID3D12Resource>, uint32_t>> vIndexBuffers
) {
	nv_helpers_dx12::BottomLevelASGenerator bottomLevelAS;

	for (size_t i = 0; i < vVertexBuffers.size(); i++) {
		if (i < vIndexBuffers.size() && vIndexBuffers[i].second > 0)
			bottomLevelAS.AddVertexBuffer(vVertexBuffers[i].first.Get(), 0,
				vVertexBuffers[i].second, sizeof(VertexPositionColor),
				vIndexBuffers[i].first.Get(), 0,
				vIndexBuffers[i].second, nullptr, 0, true);

		else
			bottomLevelAS.AddVertexBuffer(vVertexBuffers[i].first.Get(), 0,
				vVertexBuffers[i].second, sizeof(VertexPositionColor), 0,
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
				static_cast<UINT>(2 * i));
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

	pipeline.AddLibrary(m_rayGenLibrary.Get(), { L"RayGen" });
	pipeline.AddLibrary(m_missLibrary.Get(), { L"Miss" });
	pipeline.AddLibrary(m_hitLibrary.Get(), { L"ClosestHit" });

	m_rayGenSignature = CreateRayGenSignature();
	m_missSignature = CreateMissSignature();
	m_hitSignature = CreateHitSignature();

	pipeline.AddHitGroup(L"HitGroup", L"ClosestHit");

	pipeline.AddRootSignatureAssociation(m_rayGenSignature.Get(), { L"RayGen" });
	pipeline.AddRootSignatureAssociation(m_missSignature.Get(), { L"Miss" });
	pipeline.AddRootSignatureAssociation(m_hitSignature.Get(), { L"HitGroup" });


	pipeline.SetMaxPayloadSize(4 * sizeof(float)); // RGB + distance

	pipeline.SetMaxAttributeSize(2 * sizeof(float)); // barycentric coordinates

	pipeline.SetMaxRecursionDepth(1);

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
		m_dxrDevice.Get(), 3, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, true);


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

void SonarPropagation::Graphics::DXR::RayTracingRenderer::CreateShaderBindingTable() {
	// The SBT helper class collects calls to Add*Program.  If called several
	// times, the helper must be emptied before re-adding shaders.
	m_sbtHelper.Reset();

	// The pointer to the beginning of the heap is the only parameter required by
	// shaders without root parameters
	D3D12_GPU_DESCRIPTOR_HANDLE srvUavHeapHandle =
		m_srvUavHeap->GetGPUDescriptorHandleForHeapStart();
	// The helper treats both root parameter pointers and heap pointers as void*,
	// while DX12 uses the
	// D3D12_GPU_DESCRIPTOR_HANDLE to define heap pointers. The pointer in this
	// struct is a UINT64, which then has to be reinterpreted as a pointer.
	auto heapPointer = reinterpret_cast<UINT64*>(srvUavHeapHandle.ptr);
	// The ray generation only uses heap data
	m_sbtHelper.AddRayGenerationProgram(L"RayGen", { heapPointer });

	// The miss and hit shaders do not access any external resources: instead they
	// communicate their results through the ray payload
	m_sbtHelper.AddMissProgram(L"Miss", {});

	// Adding the triangle hit shader
	m_sbtHelper.AddHitGroup(L"HitGroup", { (void*)(m_vertexBuffer->GetGPUVirtualAddress()),
		(void*)(m_indexBuffer->GetGPUVirtualAddress()) });

	// Compute the size of the SBT given the number of shaders and their
	// parameters
	uint32_t sbtSize = m_sbtHelper.ComputeSBTSize();

	// Create the SBT on the upload heap. This is required as the helper will use
	// mapping to write the SBT contents. After the SBT compilation it could be
	// copied to the default heap for performance.
	m_sbtStorage = nv_helpers_dx12::CreateBuffer(
		m_dxrDevice.Get(), sbtSize, D3D12_RESOURCE_FLAG_NONE,
		D3D12_RESOURCE_STATE_GENERIC_READ, nv_helpers_dx12::kUploadHeapProps);
	if (!m_sbtStorage) {
		throw std::logic_error("Could not allocate the shader binding table");
	}

	// Compile the SBT from the shader and parameters info
	m_sbtHelper.Generate(m_sbtStorage.Get(), m_rtStateObjectProps.Get());
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

	// Update camera here ...

	m_cameraController.ProcessCameraUpdate(timer);

}

bool SonarPropagation::Graphics::DXR::RayTracingRenderer::Render() {

	if (!m_loadingComplete)
	{
		return false;
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

void SonarPropagation::Graphics::DXR::RayTracingRenderer::PopulateCommandList() {
	// Command list allocators can only be reset when the associated
  // command lists have finished execution on the GPU; apps should use
  // fences to determine GPU execution progress.
	ThrowIfFailed(m_deviceResources->GetCommandAllocator()->Reset());

	// However, when ExecuteCommandList() is called on a particular command
	// list, that command list can then be reset at any time and must be before
	// re-recording.
	ThrowIfFailed(
		m_commandList->Reset(m_deviceResources->GetCommandAllocator(), m_pipelineState.Get()));

	// Set necessary state.
	m_commandList->SetGraphicsRootSignature(m_rootSignature.Get());
	m_commandList->RSSetViewports(1, &m_deviceResources->GetScreenViewport());
	m_commandList->RSSetScissorRects(1, &m_scissorRect);

	// Indicate that the back buffer will be used as a render target.
	m_commandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(
		m_deviceResources->GetRenderTarget(),
		D3D12_RESOURCE_STATE_PRESENT,
		D3D12_RESOURCE_STATE_RENDER_TARGET));

	CD3DX12_CPU_DESCRIPTOR_HANDLE rtvHandle = m_deviceResources->GetRenderTargetView();
	CD3DX12_CPU_DESCRIPTOR_HANDLE dsvHandle = m_deviceResources->GetDepthStencilView();

	m_commandList->OMSetRenderTargets(1, &rtvHandle, FALSE, &dsvHandle);


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
	transition = CD3DX12_RESOURCE_BARRIER::Transition(
		m_outputResource.Get(), D3D12_RESOURCE_STATE_UNORDERED_ACCESS,
		D3D12_RESOURCE_STATE_COPY_SOURCE);
	m_commandList->ResourceBarrier(1, &transition);
	transition = CD3DX12_RESOURCE_BARRIER::Transition(
		m_deviceResources->GetRenderTarget(), D3D12_RESOURCE_STATE_RENDER_TARGET,
		D3D12_RESOURCE_STATE_COPY_DEST);
	m_commandList->ResourceBarrier(1, &transition);

	m_commandList->CopyResource(m_deviceResources->GetRenderTarget(),
		m_outputResource.Get());

	transition = CD3DX12_RESOURCE_BARRIER::Transition(
		m_deviceResources->GetRenderTarget(), D3D12_RESOURCE_STATE_COPY_DEST,
		D3D12_RESOURCE_STATE_RENDER_TARGET);
	m_commandList->ResourceBarrier(1, &transition);

	m_commandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(
		m_deviceResources->GetRenderTarget(),
		D3D12_RESOURCE_STATE_RENDER_TARGET,
		D3D12_RESOURCE_STATE_PRESENT));

	ThrowIfFailed(m_commandList->Close());
}

void SonarPropagation::Graphics::DXR::RayTracingRenderer::PopulateCommandListWithPIX() {

	ThrowIfFailed(m_deviceResources->GetCommandAllocator()->Reset());

	// The command list can be reset anytime after ExecuteCommandList() is called.
	ThrowIfFailed(m_commandList->Reset(m_deviceResources->GetCommandAllocator(), m_pipelineState.Get()));

	PIXBeginEvent(m_commandList.Get(), 0, L"Draw Triangle");
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
		transition = CD3DX12_RESOURCE_BARRIER::Transition(
			m_outputResource.Get(), D3D12_RESOURCE_STATE_UNORDERED_ACCESS,
			D3D12_RESOURCE_STATE_COPY_SOURCE);
		m_commandList->ResourceBarrier(1, &transition);
		transition = CD3DX12_RESOURCE_BARRIER::Transition(
			m_deviceResources->GetRenderTarget(), D3D12_RESOURCE_STATE_RENDER_TARGET,
			D3D12_RESOURCE_STATE_COPY_DEST);
		m_commandList->ResourceBarrier(1, &transition);

		m_commandList->CopyResource(m_deviceResources->GetRenderTarget(),
			m_outputResource.Get());

		transition = CD3DX12_RESOURCE_BARRIER::Transition(
			m_deviceResources->GetRenderTarget(), D3D12_RESOURCE_STATE_COPY_DEST,
			D3D12_RESOURCE_STATE_RENDER_TARGET);

		m_commandList->ResourceBarrier(1, &transition);

		m_commandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(
			m_deviceResources->GetRenderTarget(),
			D3D12_RESOURCE_STATE_RENDER_TARGET,
			D3D12_RESOURCE_STATE_PRESENT));
	}
	PIXEndEvent(m_commandList.Get());

	DX::ThrowIfFailed(m_commandList->Close());

}

void SonarPropagation::Graphics::DXR::RayTracingRenderer::KeyPressed(Windows::UI::Core::KeyEventArgs^ args) {
	m_cameraController.KeyPressed( args);
}

void SonarPropagation::Graphics::DXR::RayTracingRenderer::KeyReleased(Windows::UI::Core::KeyEventArgs^ args) {
	m_cameraController.KeyReleased(args);
}

void SonarPropagation::Graphics::DXR::RayTracingRenderer::MouseMoved(Windows::UI::Core::PointerEventArgs^ args) {
	m_cameraController.MouseMoved(args);
}

void SonarPropagation::Graphics::DXR::RayTracingRenderer::MouseWheelMoved(Windows::UI::Core::PointerEventArgs^ args) {
	m_cameraController.MouseWheelMoved(args);
}


#pragma endregion
