
#include "pch.h"
#include "RayTracingRenderer.h"
#include <ResourceUploadBatch.h>



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
	m_dxrConfig({ 1, 4 * sizeof(float), 2 * sizeof(float) }),
	m_objectLibrary({ deviceResources->GetD3DDevice() }),
	m_scene({ m_deviceResources->GetD3DDevice() }),
	m_cameraController({m_deviceResources->GetD3DDevice()})
{
	LoadState();

	m_cameraController.AddCamera(new Camera());
	m_cameraController.CycleToNextCamera();
	m_cameraController.GetCurrentCamera()->ToggleSoundSource();
	m_cameraController.CycleToPreviousCamera();

	CreateDeviceDependentResources();
	CreateWindowSizeDependentResources();
}

#pragma endregion

#pragma region Destructor

SonarPropagation::Graphics::DXR::RayTracingRenderer::~RayTracingRenderer() {
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
		DX::ThrowIfFailed(D3D12SerializeRootSignature(&descRootSignature, D3D_ROOT_SIGNATURE_VERSION_1, pSignature.GetAddressOf(), pError.GetAddressOf()));
		DX::ThrowIfFailed(d3dDevice->CreateRootSignature(0, pSignature->GetBufferPointer(), pSignature->GetBufferSize(), IID_PPV_ARGS(&m_rootSignature)));
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
			{ "COLOR",    0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 12, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
			{ "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 24, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0}
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
		DX::ThrowIfFailed(d3dDevice->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, m_deviceResources->GetCommandAllocator(), m_pipelineState.Get(), IID_PPV_ARGS(&m_commandList)));
		NAME_D3D12_OBJECT(m_commandList);

		CD3DX12_HEAP_PROPERTIES uploadHeapProperties(D3D12_HEAP_TYPE_UPLOAD);

		auto aspectRatio = m_deviceResources->GetOutputSize().Width / m_deviceResources->GetOutputSize().Height;

		CD3DX12_HEAP_PROPERTIES heapProperty =
			CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);

		InitializeObjects();
		CreateScene();

		{
			D3D12_DESCRIPTOR_HEAP_DESC heapDesc = {};
			heapDesc.NumDescriptors = 1;
			heapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
			// This flag indicates that this descriptor heap can be bound to the pipeline and that descriptors contained in it can be referenced by a root table.
			heapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
			DX::ThrowIfFailed(d3dDevice->CreateDescriptorHeap(&heapDesc, IID_PPV_ARGS(&m_imguiHeap)));

			NAME_D3D12_OBJECT(m_imguiHeap);
		}

		// Wait for the command list to finish executing; the vertex/index buffers need to be uploaded to the GPU before the upload resources go out of scope.
		m_deviceResources->WaitForGpu();

		});


	auto createDXRResources = createAssetsTask.then([this]() {

		// Create the acceleration structures
		CreateAccelerationStructures<VertexPositionNormalUV>();

		DX::ThrowIfFailed(m_commandList->Close());

		CreateRaytracingPipeline();

		CreateRaytracingOutputBuffer();

		CreateShaderResourceHeap();

		if (m_textures.size() != m_textureDatas.size()) {
			throw std::logic_error("Texture resources and texture data mismatch");
		}

		CreateSamplerResources();

		CreateShaderBindingTable();

		m_imguiManager.InitImGui(DX::c_frameCount, m_deviceResources->GetBackBufferFormat(), m_dxrDevice.Get(), m_imguiHeap.Get());

		});

	createDXRResources.then([this]() {
		m_loadingComplete = true;
		});

}

void SonarPropagation::Graphics::DXR::RayTracingRenderer::CreateScene() {
	

}

void SonarPropagation::Graphics::DXR::RayTracingRenderer::InitializeObjects() {

	//size_t cubeData = m_objectLibrary.LoadPredefined<VertexPositionNormalUV>(GetCubeVertices<VertexPositionNormalUV>(1, 1, 1), GetCubeIndices());

	size_t quadData = m_objectLibrary.LoadPredefined<VertexPositionNormalUV>(GetQuadVertices<VertexPositionNormalUV>(1, 1), GetQuadIndices());
	
	{
		XMFLOAT3 position = { 0.f, 0.f, 0.f };
		XMFLOAT4 scale = { 1000000.f, 1.f, 1000000.f , 1.f };
		XMFLOAT4 rotation = { 0.f, 0.f, 0.f, 1.f };

		Scene::Transform transform = { position,rotation,scale };
		transform.SetParent(nullptr, nullptr);
		m_scene.AddObject(Scene::SoundReflector(transform, quadData, ObjectType::Boundary));
	}

	{
		XMFLOAT3 position = { 0.f, 50000.f, 0.f };
		XMFLOAT4 scale = { 1000000.f, 1.f, 1000000.f , 1.f };
		XMFLOAT4 rotation = { 0.f, 0.f, 0.f, 1.f };

		Scene::Transform transform = { position,rotation,scale };
		transform.SetParent(nullptr, nullptr);
		m_scene.AddObject(Scene::SoundReflector(transform, quadData, ObjectType::Boundary));
	}


	{
		XMFLOAT3 position = { 0.f,-7500.f,0.f };
		XMFLOAT4 scale = { 1.f, 1000000.f, 1000000.f , 1.f };
		XMFLOAT4 rotation = { g_XMHalfPi[0],g_XMHalfPi[0],0.f, 1.f };
		Scene::Transform transform = { position,rotation,scale };
		transform.SetParent(nullptr, nullptr);
		m_scene.AddObject(Scene::SoundReflector(transform, quadData, ObjectType::Object));
	}
}

void SonarPropagation::Graphics::DXR::RayTracingRenderer::CreateWindowSizeDependentResources() {
	Size outputSize = m_deviceResources->GetOutputSize();
	m_aspectRatio = outputSize.Width / outputSize.Height;
	float fovAngleY = 70.0f * XM_PI / 180.0f;

	m_cameraController.GetCurrentCamera()->SetAspectRatio(m_aspectRatio);
	m_cameraController.GetCurrentCamera()->SetFOV(fovAngleY);

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
	for (auto& object : m_scene.m_objects) {
		
		auto& model = m_objectLibrary.m_objects[object.GetModelIndex()];
		
		if (!model.IsASInstanciated() )
		{
			model.m_asBuffers = CreateBottomLevelAS<V>(
				{ {
					model.m_bufferData.vertexBuffer.Get(),
					model.m_bufferData.vertexBufferView.SizeInBytes / sizeof(V),
				} },
				{ {
					model.m_bufferData.indexBuffer.Get(),
					model.m_bufferData.indexBufferView.SizeInBytes / sizeof(UINT)
				} }
			);
		}

		
		m_instances.push_back({ model.m_asBuffers.pResult, object.m_transform.LocalToWorld() });
	}

	CreateTopLevelAS(m_instances, false);

	m_commandList->Close();
	ID3D12CommandList* ppCommandLists[] = { m_commandList.Get() };
	m_deviceResources->GetCommandQueue()->ExecuteCommandLists(1, ppCommandLists);
	m_deviceResources->WaitForGpu();

	DX::ThrowIfFailed(
		m_commandList->Reset(m_deviceResources->GetCommandAllocator(), m_pipelineState.Get())
	);


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
			bottomLevelAS.AddVertexBuffer(
				vVertexBuffers[i].first.Get(), 0,
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
				static_cast<UINT>(i));
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
		 {0 /*b0*/, 1 /*1 descriptor*/, 0, D3D12_DESCRIPTOR_RANGE_TYPE_CBV, 2} // Camera Buffer
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
	rsc.AddRootParameter(D3D12_ROOT_PARAMETER_TYPE_SRV, 2);
	rsc.AddHeapRangesParameter(
		{ {0 /*s0*/, 1 /*1 descriptor*/, 0, D3D12_DESCRIPTOR_RANGE_TYPE_SAMPLER, 0} // Sampler
		});
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

ComPtr<ID3D12RootSignature> SonarPropagation::Graphics::DXR::RayTracingRenderer::CreateSonarRayGenSignature() {
	nv_helpers_dx12::RootSignatureGenerator rsc;
	rsc.AddHeapRangesParameter({
		 {0 /*t0*/, 1, 0,
		  D3D12_DESCRIPTOR_RANGE_TYPE_SRV /*Top-level acceleration structure*/,
		  1},
		 {0 /*b0*/, 1 /*1 descriptor*/, 0, D3D12_DESCRIPTOR_RANGE_TYPE_CBV, 2} // Camera Buffer
	
		});

	return rsc.Generate(m_dxrDevice.Get(), true);
}

ComPtr<ID3D12RootSignature> SonarPropagation::Graphics::DXR::RayTracingRenderer::CreateSonarHitSignature() {
	nv_helpers_dx12::RootSignatureGenerator rsc;
	rsc.AddRootParameter(D3D12_ROOT_PARAMETER_TYPE_SRV, 0);
	rsc.AddRootParameter(D3D12_ROOT_PARAMETER_TYPE_SRV, 1);
	//rsc.AddRootParameter(D3D12_ROOT_PARAMETER_TYPE_UAV, 0);
	//rsc.AddHeapRangesParameter({ {0 /*s0*/, 1 /*1 descriptor*/, 0, D3D12_DESCRIPTOR_RANGE_TYPE_SAMPLER, 0} // Sampler});
	rsc.AddHeapRangesParameter({ {2 /*t0*/, 1, 0,D3D12_DESCRIPTOR_RANGE_TYPE_SRV /*Top-level acceleration structure*/,1} });

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
	m_sonarRayGenLibrary = CompileShader(L"SonarShaders/SonarRayGen.hlsl");
	m_sonarMissLibrary = CompileShader(L"SonarShaders/SonarMiss.hlsl");
	m_sonarHitLibrary = CompileShader(L"SonarShaders/SonarHit.hlsl");

	pipeline.AddLibrary(m_rayGenLibrary.Get(), { L"CameraRayGen" });
	pipeline.AddLibrary(m_missLibrary.Get(), { L"MeshMiss" });
	pipeline.AddLibrary(m_hitLibrary.Get(), { L"MeshClosestHit"});

	pipeline.AddLibrary(m_sonarRayGenLibrary.Get(), { L"SonarRayGen" });
	pipeline.AddLibrary(m_sonarHitLibrary.Get(), { L"BoundaryClosestHit", L"ObjectClosestHit"});
	pipeline.AddLibrary(m_sonarMissLibrary.Get(), { L"BoundaryMiss", L"ObjectMiss" });

	m_rayGenSignature = CreateRayGenSignature();
	m_missSignature = CreateMissSignature();
	m_hitSignature = CreateHitSignature();

	m_sonarRayGenSignature = CreateSonarRayGenSignature();
	m_sonarHitSignature = CreateSonarHitSignature();

	pipeline.AddHitGroup(L"MeshHitGroup", L"MeshClosestHit");
	pipeline.AddHitGroup(L"ObjectHitGroup", L"ObjectClosestHit");
	pipeline.AddHitGroup(L"BoundaryHitGroup", L"BoundaryClosestHit");

	pipeline.AddRootSignatureAssociation(m_rayGenSignature.Get(), { L"CameraRayGen"});
	pipeline.AddRootSignatureAssociation(m_missSignature.Get(), { L"MeshMiss"});
	pipeline.AddRootSignatureAssociation(m_hitSignature.Get(), { L"MeshHitGroup" });

	pipeline.AddRootSignatureAssociation(m_sonarRayGenSignature.Get(), {L"SonarRayGen"});
	pipeline.AddRootSignatureAssociation(m_sonarHitSignature.Get(), { L"BoundaryHitGroup", L"ObjectHitGroup" });

	pipeline.SetMaxPayloadSize(m_dxrConfig.m_maxPayloadSize); // RGB + distance

	pipeline.SetMaxAttributeSize(m_dxrConfig.m_maxAttributeSize); // barycentric coordinates

	pipeline.SetMaxRecursionDepth(m_dxrConfig.m_recursionDepth);

	// Compile the pipeline for execution on the GPU
	m_rtStateObject = pipeline.Generate();

	DX::ThrowIfFailed(
		m_rtStateObject->QueryInterface(IID_PPV_ARGS(&m_rtStateObjectProps))
	);
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
	DX::ThrowIfFailed(m_dxrDevice.Get()->CreateCommittedResource(
		&nv_helpers_dx12::kDefaultHeapProps, D3D12_HEAP_FLAG_NONE, &resDesc,
		D3D12_RESOURCE_STATE_COPY_SOURCE, nullptr,
		IID_PPV_ARGS(&m_outputResource)));
}

void SonarPropagation::Graphics::DXR::RayTracingRenderer::CreateSamplerResources() {

	// Create sampler heap
	m_samplerHeap = CreateDescriptorHeap(
		m_dxrDevice.Get(), 1, D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER, true);

	NAME_D3D12_OBJECT(m_samplerHeap);

	D3D12_CPU_DESCRIPTOR_HANDLE samplerHandle =
		m_samplerHeap->GetCPUDescriptorHandleForHeapStart();

	// Create a sampler on the heap
	D3D12_SAMPLER_DESC samplerDesc = {};
	samplerDesc.Filter = D3D12_FILTER_MIN_MAG_MIP_LINEAR;
	samplerDesc.AddressU = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
	samplerDesc.AddressV = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
	samplerDesc.AddressW = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
	samplerDesc.MinLOD = 0;
	samplerDesc.MaxLOD = 1;

	m_deviceResources->GetD3DDevice()->CreateSampler(&samplerDesc, m_samplerHeap->GetCPUDescriptorHandleForHeapStart());
}



void SonarPropagation::Graphics::DXR::RayTracingRenderer::CreateShaderResourceHeap() {

	std::vector<Camera*> soundSources = m_cameraController.GetSoundSources(); //size of 1
	// For now, we assume that there is only one sound source

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

	D3D12_CONSTANT_BUFFER_VIEW_DESC cameraCbvDesc = {};
	cameraCbvDesc.BufferLocation = m_cameraController.GetCurrentCamera()->GetCameraBuffer()->GetGPUVirtualAddress();
	cameraCbvDesc.SizeInBytes = m_cameraController.GetCurrentCamera()->GetCameraBufferSize();
	m_dxrDevice->CreateConstantBufferView(&cameraCbvDesc, srvHandle);

	srvHandle.ptr += m_dxrDevice.Get()->GetDescriptorHandleIncrementSize(
		D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

	D3D12_CONSTANT_BUFFER_VIEW_DESC soundSourceCbvDesc = {};
	soundSourceCbvDesc.BufferLocation = soundSources[0]->GetCameraBuffer()->GetGPUVirtualAddress();
	soundSourceCbvDesc.SizeInBytes = soundSources[0]->GetCameraBufferSize();
	m_dxrDevice->CreateConstantBufferView(&soundSourceCbvDesc, srvHandle);

}


/// <summary>
/// Create the shader binding table for the raytracing pipeline by initializing the RayGen, Miss shaders
/// and the Hit shaders per instances.
/// </summary>
void SonarPropagation::Graphics::DXR::RayTracingRenderer::CreateShaderBindingTable() {

	m_sbtHelper.Reset();

	D3D12_GPU_DESCRIPTOR_HANDLE srvUavHeapHandle =
		m_srvUavHeap->GetGPUDescriptorHandleForHeapStart();
	D3D12_GPU_DESCRIPTOR_HANDLE samplerHeapHandle =
		m_samplerHeap->GetGPUDescriptorHandleForHeapStart();

	auto srvHeapPointer = reinterpret_cast<UINT64*>(srvUavHeapHandle.ptr);
	auto samplerHeapPointer = reinterpret_cast<UINT64*>(samplerHeapHandle.ptr);

	m_sbtHelper.AddRayGenerationProgram(L"CameraRayGen", { srvHeapPointer });
	m_sbtHelper.AddMissProgram(L"MeshMiss", {});

	for (auto& object : m_scene.m_objects) {
		auto model = m_objectLibrary.m_objects[object.GetModelIndex()];

		if (model.m_bufferData.indexBuffer)
		{
			m_sbtHelper.AddHitGroup(
				L"MeshHitGroup",
				{
					(void*)(model.m_bufferData.vertexBuffer->GetGPUVirtualAddress()),
					(void*)(model.m_bufferData.indexBuffer->GetGPUVirtualAddress()),
					(void*)(object.m_texture->GetGPUVirtualAddress()),
					(void*)(samplerHeapPointer)
				});
		}
		else {
			m_sbtHelper.AddHitGroup(
				L"MeshHitGroup",
				{
					(void*)(model.m_bufferData.vertexBuffer->GetGPUVirtualAddress()),
					(void*)(object.m_texture->GetGPUVirtualAddress()),
					(void*)(samplerHeapPointer)
				});
		}

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

void SonarPropagation::Graphics::DXR::RayTracingRenderer::CreateShaderBindingTableForPhotonMapping() {

	m_sbtHelper.Reset();

	D3D12_GPU_DESCRIPTOR_HANDLE srvUavHeapHandle =
		m_srvUavHeap->GetGPUDescriptorHandleForHeapStart();
	D3D12_GPU_DESCRIPTOR_HANDLE samplerHeapHandle =
		m_samplerHeap->GetGPUDescriptorHandleForHeapStart();

	auto srvHeapPointer = reinterpret_cast<UINT64*>(srvUavHeapHandle.ptr);
	auto samplerHeapPointer = reinterpret_cast<UINT64*>(samplerHeapHandle.ptr);

	m_sbtHelper.AddRayGenerationProgram(L"SonarRayGen", { srvHeapPointer });

	m_sbtHelper.AddMissProgram(L"ObjectMiss", {});
	m_sbtHelper.AddMissProgram(L"BoundaryMiss", {});

	for (auto& object : m_scene.m_objects) {
		auto model = m_objectLibrary.m_objects[object.GetModelIndex()];
		
		if (model.m_bufferData.indexBuffer)
		{
			if (object.GetType() == ObjectType::Boundary)
			m_sbtHelper.AddHitGroup(
				L"BoundaryHitGroup",
				{
					(void*)(model.m_bufferData.vertexBuffer->GetGPUVirtualAddress()),
					(void*)(model.m_bufferData.indexBuffer->GetGPUVirtualAddress()),
					(void*)(object.m_texture->GetGPUVirtualAddress()),
					(void*)(samplerHeapPointer)
				});

			else
				m_sbtHelper.AddHitGroup(
				L"ObjectHitGroup",
				{
					(void*)(model.m_bufferData.vertexBuffer->GetGPUVirtualAddress()),
					(void*)(model.m_bufferData.indexBuffer->GetGPUVirtualAddress()),
					(void*)(object.m_texture->GetGPUVirtualAddress()),
					(void*)(samplerHeapPointer)
				});
		}
		else {
			if (object.GetType() == ObjectType::Boundary)
			m_sbtHelper.AddHitGroup(
				L"BoundaryHitGroup",
				{
					(void*)(model.m_bufferData.vertexBuffer->GetGPUVirtualAddress()),
					(void*)(object.m_texture->GetGPUVirtualAddress()),
					(void*)(samplerHeapPointer)
				});
			else 
				m_sbtHelper.AddHitGroup(
				L"ObjectHitGroup",
				{
					(void*)(model.m_bufferData.vertexBuffer->GetGPUVirtualAddress()),
					(void*)(object.m_texture->GetGPUVirtualAddress()),
					(void*)(samplerHeapPointer)
				});
		}

	}

	uint32_t sbtSize = m_sbtHelper.ComputeSBTSize();

	m_sbtStorage = nv_helpers_dx12::CreateBuffer(
		m_dxrDevice.Get(), sbtSize, D3D12_RESOURCE_FLAG_NONE,
		D3D12_RESOURCE_STATE_GENERIC_READ, nv_helpers_dx12::kUploadHeapProps);
	if (!m_sbtStorage) {
		throw std::logic_error("Could not allocate the shader binding table");
	}

	m_sbtHelper.Generate(m_sbtStorage.Get(), m_rtStateObjectProps.Get());
}

#pragma endregion

#pragma region Raytracing Utils.:

void SonarPropagation::Graphics::DXR::RayTracingRenderer::CheckRayTracingSupport() {
	D3D12_FEATURE_DATA_D3D12_OPTIONS5 options5 = {};
	DX::ThrowIfFailed(m_dxrDevice.Get()->CheckFeatureSupport(D3D12_FEATURE_D3D12_OPTIONS5,
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

	if (m_cameraController.IsBufferDirty())
	{
		CreateShaderResourceHeap();
		CreateShaderBindingTable();
		m_cameraController.SetBufferToClean();
		
	}
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

	//PhotonMappingPreprocess();
	PopulateCommandListForRendering();

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

void SonarPropagation::Graphics::DXR::RayTracingRenderer::PopulateCommandListForRendering() {

	DX::ThrowIfFailed(m_deviceResources->GetCommandAllocator()->Reset());

	// The command list can be reset anytime after ExecuteCommandList() is called.
	DX::ThrowIfFailed(m_commandList->Reset(m_deviceResources->GetCommandAllocator(), m_pipelineState.Get()));

	PIXBeginEvent(m_commandList.Get(), 0, L"Draw Scene");
	{
		// Set the graphics root signature and descriptor heaps to be used by this frame.
		m_commandList->SetGraphicsRootSignature(m_rootSignature.Get());

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

		std::vector<ID3D12DescriptorHeap*> heaps = { m_srvUavHeap.Get(), m_samplerHeap.Get()};


		m_commandList->SetDescriptorHeaps(static_cast<UINT>(heaps.size()),
			heaps.data());

		CD3DX12_RESOURCE_BARRIER transition = CD3DX12_RESOURCE_BARRIER::Transition(
			m_outputResource.Get(), D3D12_RESOURCE_STATE_COPY_SOURCE,
			D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
		m_commandList->ResourceBarrier(1, &transition);

		// Setup the raytracing task
		D3D12_DISPATCH_RAYS_DESC desc = {};

		uint32_t rayGenerationSectionSizeInBytes = m_sbtHelper.GetRayGenSectionSize();
		desc.RayGenerationShaderRecord.StartAddress = m_sbtStorage->GetGPUVirtualAddress();
		desc.RayGenerationShaderRecord.SizeInBytes = rayGenerationSectionSizeInBytes;

		uint32_t missSectionSizeInBytes = m_sbtHelper.GetMissSectionSize();
		desc.MissShaderTable.StartAddress = m_sbtStorage->GetGPUVirtualAddress() + rayGenerationSectionSizeInBytes;
		desc.MissShaderTable.SizeInBytes = missSectionSizeInBytes;
		desc.MissShaderTable.StrideInBytes = m_sbtHelper.GetMissEntrySize();

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

void SonarPropagation::Graphics::DXR::RayTracingRenderer::PhotonMappingPreprocess() {
	CreateShaderBindingTableForPhotonMapping();

	DX::ThrowIfFailed(m_deviceResources->GetCommandAllocator()->Reset());

	DX::ThrowIfFailed(m_commandList->Reset(m_deviceResources->GetCommandAllocator(), m_pipelineState.Get()));

	PIXBeginEvent(m_commandList.Get(), 0, L"Photon map");
	{
		m_commandList->SetGraphicsRootSignature(m_rootSignature.Get());

		std::vector<ID3D12DescriptorHeap*> heaps = {};
		m_commandList->SetDescriptorHeaps(static_cast<UINT>(heaps.size()), heaps.data());

		D3D12_DISPATCH_RAYS_DESC desc = {};

		uint32_t rayGenerationSectionSizeInBytes = m_sbtHelper.GetRayGenSectionSize();
		desc.RayGenerationShaderRecord.StartAddress = m_sbtStorage->GetGPUVirtualAddress();
		desc.RayGenerationShaderRecord.SizeInBytes = rayGenerationSectionSizeInBytes;

		uint32_t missSectionSizeInBytes = m_sbtHelper.GetMissSectionSize();
		desc.MissShaderTable.StartAddress = m_sbtStorage->GetGPUVirtualAddress() + rayGenerationSectionSizeInBytes;
		desc.MissShaderTable.SizeInBytes = missSectionSizeInBytes;
		desc.MissShaderTable.StrideInBytes = m_sbtHelper.GetMissEntrySize();

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
	}
	PIXEndEvent(m_commandList.Get());
	DX::ThrowIfFailed(m_commandList->Close());

	CreateShaderBindingTable();
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
			m_cameraController.GetCurrentCamera()->RenderCameraImGui();
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
