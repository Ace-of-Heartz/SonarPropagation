
#include "pch.h"
#include "RayTracingRenderer.h"
#include ".\Nvidia\nvidia_include.h"
#include "..\Content\ShaderStructures.h"
//-----------------------------------------------------------------------------
//
// Create a bottom-level acceleration structure based on a list of vertex
// buffers in GPU memory along with their vertex count. The build is then done
// in 3 steps: gathering the geometry, computing the sizes of the required
// buffers, and building the actual AS
//

using namespace nv_helpers_dx12;
using namespace DirectX;
using namespace Microsoft::WRL;


void SonarPropagation::RayTracingRenderer::CheckRayTracingSupport() {
    D3D12_FEATURE_DATA_D3D12_OPTIONS5 options5 = {};
    ThrowIfFailed(m_deviceResources->GetD3DDevice()->CheckFeatureSupport(D3D12_FEATURE_D3D12_OPTIONS5,
        &options5, sizeof(options5)));
    if (options5.RaytracingTier < D3D12_RAYTRACING_TIER_1_0)
        throw std::runtime_error("Raytracing not supported on device");
}


SonarPropagation::AccelerationStructureBuffers
SonarPropagation::RayTracingRenderer::CreateBottomLevelAS(
    std::vector<std::pair<ComPtr<ID3D12Resource>, uint32_t>> vVertexBuffers) {
    nv_helpers_dx12::BottomLevelASGenerator bottomLevelAS;

    // Adding all vertex buffers and not transforming their position.
    for (const auto& buffer : vVertexBuffers) {
        bottomLevelAS.AddVertexBuffer(buffer.first.Get(), 0, buffer.second,
            sizeof(VertexPositionColor), 0, 0);
    }

    // The AS build requires some scratch space to store temporary information.
    // The amount of scratch memory is dependent on the scene complexity.
    UINT64 scratchSizeInBytes = 0;
    // The final AS also needs to be stored in addition to the existing vertex
    // buffers. It size is also dependent on the scene complexity.
    UINT64 resultSizeInBytes = 0;

    bottomLevelAS.ComputeASBufferSizes(m_deviceResources->GetD3DDevice(), false, &scratchSizeInBytes,
        &resultSizeInBytes);

    // Once the sizes are obtained, the application is responsible for allocating
    // the necessary buffers. Since the entire generation will be done on the GPU,
    // we can directly allocate those on the default heap
    AccelerationStructureBuffers buffers;
    buffers.pScratch = nv_helpers_dx12::CreateBuffer(
        m_deviceResources->GetD3DDevice(), scratchSizeInBytes,
        D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_COMMON,
        nv_helpers_dx12::kDefaultHeapProps);
    buffers.pResult = nv_helpers_dx12::CreateBuffer(
        m_deviceResources->GetD3DDevice(), resultSizeInBytes,
        D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS,
        D3D12_RESOURCE_STATE_RAYTRACING_ACCELERATION_STRUCTURE,
        nv_helpers_dx12::kDefaultHeapProps);

    // Build the acceleration structure. Note that this call integrates a barrier
    // on the generated AS, so that it can be used to compute a top-level AS right
    // after this method.
    bottomLevelAS.Generate(m_commandList.Get(), buffers.pScratch.Get(),
        buffers.pResult.Get(), false, nullptr);

    return buffers;
}

//-----------------------------------------------------------------------------
// Create the main acceleration structure that holds all instances of the scene.
// Similarly to the bottom-level AS generation, it is done in 3 steps: gathering
// the instances, computing the memory requirements for the AS, and building the
// AS itself
//
void SonarPropagation::RayTracingRenderer::CreateTopLevelAS(const std::vector<std::pair<ComPtr<ID3D12Resource>,DirectX::XMMATRIX>>& instances 
    // pair of bottom level AS and matrix of the instance
) {
    // Gather all the instances into the builder helper
    for (size_t i = 0; i < instances.size(); i++) {
        m_topLevelASGenerator.AddInstance(instances[i].first.Get(),
            instances[i].second, static_cast<uint32_t>(i),
            static_cast<uint32_t>(0));
    }

    // As for the bottom-level AS, the building the AS requires some scratch space
    // to store temporary data in addition to the actual AS. In the case of the
    // top-level AS, the instance descriptors also need to be stored in GPU
    // memory. This call outputs the memory requirements for each (scratch,
    // results, instance descriptors) so that the application can allocate the
    // corresponding memory
    UINT64 scratchSize, resultSize, instanceDescsSize;

    m_topLevelASGenerator.ComputeASBufferSizes(reinterpret_cast<ID3D12Device5*>(m_deviceResources->GetD3DDevice()), true, &scratchSize,
        &resultSize, &instanceDescsSize);

    // Create the scratch and result buffers. Since the build is all done on GPU,
    // those can be allocated on the default heap
    m_topLevelASBuffers.pScratch = nv_helpers_dx12::CreateBuffer(
        m_deviceResources->GetD3DDevice(), scratchSize, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS,
        D3D12_RESOURCE_STATE_UNORDERED_ACCESS,
        nv_helpers_dx12::kDefaultHeapProps);
    m_topLevelASBuffers.pResult = nv_helpers_dx12::CreateBuffer(
        m_deviceResources->GetD3DDevice(), resultSize, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS,
        D3D12_RESOURCE_STATE_RAYTRACING_ACCELERATION_STRUCTURE,
        nv_helpers_dx12::kDefaultHeapProps);

    // The buffer describing the instances: ID, shader binding information,
    // matrices ... Those will be copied into the buffer by the helper through
    // mapping, so the buffer has to be allocated on the upload heap.
    m_topLevelASBuffers.pInstanceDesc = nv_helpers_dx12::CreateBuffer(
        m_deviceResources->GetD3DDevice(), instanceDescsSize, D3D12_RESOURCE_FLAG_NONE,
        D3D12_RESOURCE_STATE_GENERIC_READ, nv_helpers_dx12::kUploadHeapProps);

    // After all the buffers are allocated, or if only an update is required, we
    // can build the acceleration structure. Note that in the case of the update
    // we also pass the existing AS as the 'previous' AS, so that it can be
    // refitted in place.
    m_topLevelASGenerator.Generate(m_commandList.Get(),
        m_topLevelASBuffers.pScratch.Get(),
        m_topLevelASBuffers.pResult.Get(),
        m_topLevelASBuffers.pInstanceDesc.Get());
}

//-----------------------------------------------------------------------------
//
// Combine the BLAS and TLAS builds to construct the entire acceleration
// structure required to raytrace the scene
//
void SonarPropagation::RayTracingRenderer::CreateAccelerationStructures() {
    // Build the bottom AS from the Triangle vertex buffer
    AccelerationStructureBuffers bottomLevelBuffers =
        CreateBottomLevelAS({ {m_vertexBuffer.Get(), 3} });

    // Just one instance for now
    m_instances = { {bottomLevelBuffers.pResult, XMMatrixIdentity()} };
    CreateTopLevelAS(m_instances);

    // Flush the command list and wait for it to finish
    m_commandList->Close();
    ID3D12CommandList* ppCommandLists[] = { m_commandList.Get() };
    m_deviceResources->GetCommandQueue()->ExecuteCommandLists(1, ppCommandLists);
    m_deviceResources->WaitForGpu();

 
    // Once the command list is finished executing, reset it to be reused for
    // rendering
    ThrowIfFailed(
        m_commandList->Reset(m_deviceResources->GetCommandAllocator(), m_pipelineState.Get()));

    // Store the AS buffers. The rest of the buffers will be released once we exit
    // the function
    m_bottomLevelAS = bottomLevelBuffers.pResult;
}

//-----------------------------------------------------------------------------
// The ray generation shader needs to access 2 resources: the raytracing output
// and the top-level acceleration structure
//
ComPtr<ID3D12RootSignature> SonarPropagation::RayTracingRenderer::CreateRayGenSignature() {
    nv_helpers_dx12::RootSignatureGenerator rsc;
    rsc.AddHeapRangesParameter(
        { {0 /*u0*/, 1 /*1 descriptor */, 0 /*use the implicit register space 0*/,
          D3D12_DESCRIPTOR_RANGE_TYPE_UAV /* UAV representing the output buffer*/,
          0 /*heap slot where the UAV is defined*/},
         {0 /*t0*/, 1, 0,
          D3D12_DESCRIPTOR_RANGE_TYPE_SRV /*Top-level acceleration structure*/,
          1} });

    return rsc.Generate(m_deviceResources->GetD3DDevice(), true);
}

//-----------------------------------------------------------------------------
// The hit shader communicates only through the ray payload, and therefore does
// not require any resources
//
ComPtr<ID3D12RootSignature> SonarPropagation::RayTracingRenderer::CreateHitSignature() {
    nv_helpers_dx12::RootSignatureGenerator rsc;
    return rsc.Generate(m_deviceResources->GetD3DDevice(), true);
}

//-----------------------------------------------------------------------------
// The miss shader communicates only through the ray payload, and therefore
// does not require any resources
//
ComPtr<ID3D12RootSignature> SonarPropagation::RayTracingRenderer::CreateMissSignature() {
    nv_helpers_dx12::RootSignatureGenerator rsc;
    return rsc.Generate(m_deviceResources->GetD3DDevice(), true);
}

//-----------------------------------------------------------------------------
//
// The raytracing pipeline binds the shader code, root signatures and pipeline
// characteristics in a single structure used by DXR to invoke the shaders and
// manage temporary memory during raytracing
//
//
void SonarPropagation::RayTracingRenderer::CreateRaytracingPipeline()
{
    nv_helpers_dx12::RayTracingPipelineGenerator pipeline(m_deviceResources->GetD3DDevice());

    // The pipeline contains the DXIL code of all the shaders potentially executed
    // during the raytracing process. This section compiles the HLSL code into a
    // set of DXIL libraries. We chose to separate the code in several libraries
    // by semantic (ray generation, hit, miss) for clarity. Any code layout can be
    // used.
    m_rayGenLibrary = nv_helpers_dx12::CompileShaderLibrary(L"RayGen.hlsl");
    m_missLibrary = nv_helpers_dx12::CompileShaderLibrary(L"Miss.hlsl");
    m_hitLibrary = nv_helpers_dx12::CompileShaderLibrary(L"Hit.hlsl");
    // In a way similar to DLLs, each library is associated with a number of
    // exported symbols. This
    // has to be done explicitly in the lines below. Note that a single library
    // can contain an arbitrary number of symbols, whose semantic is given in HLSL
    // using the [shader("xxx")] syntax
    pipeline.AddLibrary(m_rayGenLibrary.Get(), { L"RayGen" });
    pipeline.AddLibrary(m_missLibrary.Get(), { L"Miss" });
    pipeline.AddLibrary(m_hitLibrary.Get(), { L"ClosestHit" });

    // To be used, each DX12 shader needs a root signature defining which
    // parameters and buffers will be accessed.
    m_rayGenSignature = CreateRayGenSignature();
    m_missSignature = CreateMissSignature();
    m_hitSignature = CreateHitSignature();

    // 3 different shaders can be invoked to obtain an intersection: an
    // intersection shader is called
    // when hitting the bounding box of non-triangular geometry. This is beyond
    // the scope of this tutorial. An any-hit shader is called on potential
    // intersections. This shader can, for example, perform alpha-testing and
    // discard some intersections. Finally, the closest-hit program is invoked on
    // the intersection point closest to the ray origin. Those 3 shaders are bound
    // together into a hit group.
    
    // Note that for triangular geometry the intersection shader is built-in. An
    // empty any-hit shader is also defined by default, so in our simple case each
    // hit group contains only the closest hit shader. Note that since the
    // exported symbols are defined above the shaders can be simply referred to by
    // name.
    
    // Hit group for the triangles, with a shader simply interpolating vertex
    // colors
    pipeline.AddHitGroup(L"HitGroup", L"ClosestHit");

    // The following section associates the root signature to each shader. Note
    // that we can explicitly show that some shaders share the same root signature
    // (eg. Miss and ShadowMiss). Note that the hit shaders are now only referred
    // to as hit groups, meaning that the underlying intersection, any-hit and
    // closest-hit shaders share the same root signature.
    pipeline.AddRootSignatureAssociation(m_rayGenSignature.Get(), { L"RayGen" });
    pipeline.AddRootSignatureAssociation(m_missSignature.Get(), { L"Miss" });
    pipeline.AddRootSignatureAssociation(m_hitSignature.Get(), { L"HitGroup" });

    // The payload size defines the maximum size of the data carried by the rays,
    // ie. the the data
    // exchanged between shaders, such as the HitInfo structure in the HLSL code.
    // It is important to keep this value as low as possible as a too high value
    // would result in unnecessary memory consumption and cache trashing.
    pipeline.SetMaxPayloadSize(4 * sizeof(float)); // RGB + distance

    // Upon hitting a surface, DXR can provide several attributes to the hit. In
    // our sample we just use the barycentric coordinates defined by the weights
    // u,v of the last two vertices of the triangle. The actual barycentrics can
    // be obtained using float3 barycentrics = float3(1.f-u-v, u, v);
    pipeline.SetMaxAttributeSize(2 * sizeof(float)); // barycentric coordinates

    // The raytracing process can shoot rays from existing hit points, resulting
    // in nested TraceRay calls. Our sample code traces only primary rays, which
    // then requires a trace depth of 1. Note that this recursion depth should be
    // kept to a minimum for best performance. Path tracing algorithms can be
    // easily flattened into a simple loop in the ray generation.
    pipeline.SetMaxRecursionDepth(1);

    // Compile the pipeline for execution on the GPU
    m_rtStateObject = pipeline.Generate();

    // Cast the state object into a properties object, allowing to later access
    // the shader pointers by name
    ThrowIfFailed(
        m_rtStateObject->QueryInterface(IID_PPV_ARGS(&m_rtStateObjectProps)));
}

void SonarPropagation::RayTracingRenderer::Init() {
	// Initialize the acceleration structures
	CreateAccelerationStructures();

    ThrowIfFailed(m_commandList->Close());
}