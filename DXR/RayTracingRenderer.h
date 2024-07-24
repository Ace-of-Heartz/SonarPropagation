#pragma once

#include <memory.h>
#include <pch.h>
#include <dxcapi.h>

#include "..\Common\DeviceResources.h"
#include "..\Content\ShaderStructures.h"
#include "..\Common\StepTimer.h"
#include "..\DXR\Nvidia\nvidia_include.h"

using namespace Microsoft::WRL;
using namespace DirectX;
//using namespace nv_helpers_dx12;

namespace SonarPropagation {

	struct AccelerationStructureBuffers {
		ComPtr<ID3D12Resource> pScratch;
		ComPtr<ID3D12Resource> pResult;
		ComPtr<ID3D12Resource> pInstanceDesc; // Used only for top-level AS
	};

	class RayTracingRenderer {
	public:
		RayTracingRenderer(const std::shared_ptr<DX::DeviceResources>& deviceResources);
		~RayTracingRenderer();

		// Generic Render Initialization
		void CreateDeviceDependentResources();
		void CreateWindowSizeDependentResources();

		// Raytracing Initialization
		void CreateAccelerationStructures();
		void CreateRaytracingPipeline();
		void CreateRaytracingOutputBuffer(); 
		void CreateShaderResourceHeap(); 
		void CreateShaderBindingTable(); 

		void CheckRayTracingSupport();

		void Update(DX::StepTimer const& timer);
		bool Render();
		void SaveState();

		//void StartTracking();
		//void TrackingUpdate(float positionX);
		//void StopTracking();
		//bool IsTracking() { return m_tracking; }

	private:
		void LoadState();
		void CreateTopLevelAS(const std::vector < std::pair < ComPtr<ID3D12Resource>, DirectX::XMMATRIX>>& instances);
		SonarPropagation::AccelerationStructureBuffers CreateBottomLevelAS(std::vector<std::pair<ComPtr<ID3D12Resource>, uint32_t>> vVertexBuffers,
			std::vector<std::pair<ComPtr<ID3D12Resource>, uint32_t>> vIndexBuffers
		);

		ComPtr<ID3D12RootSignature> CreateRayGenSignature();
		ComPtr<ID3D12RootSignature> CreateHitSignature();
		ComPtr<ID3D12RootSignature> CreateMissSignature();

		void PopulateCommandList();
		

	private: 
		// Raytracing pipeline objects

		ComPtr<IDxcBlob> m_rayGenLibrary;
		ComPtr<IDxcBlob> m_hitLibrary;
		ComPtr<IDxcBlob> m_missLibrary;

		ComPtr<ID3D12RootSignature> m_rayGenSignature;
		ComPtr<ID3D12RootSignature> m_hitSignature;
		ComPtr<ID3D12RootSignature> m_missSignature;

		ComPtr<ID3D12StateObject> m_rtStateObject;

		ComPtr<ID3D12StateObjectProperties> m_rtStateObjectProps;

		// Constant buffers must be 256-byte aligned.
		static const UINT c_alignedConstantBufferSize = (sizeof(ModelViewProjectionConstantBuffer) + 255) & ~255;

		// Cached pointer to device resources.
		std::shared_ptr<DX::DeviceResources> m_deviceResources;

		// Direct3D resources
		Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList4>	m_commandList;
		Microsoft::WRL::ComPtr<ID3D12RootSignature>			m_rootSignature;
		Microsoft::WRL::ComPtr<ID3D12PipelineState>			m_pipelineState;
		Microsoft::WRL::ComPtr<ID3D12DescriptorHeap>		m_cbvHeap;
		Microsoft::WRL::ComPtr<ID3D12Resource>				m_vertexBuffer;
		Microsoft::WRL::ComPtr<ID3D12Resource>				m_indexBuffer;
		Microsoft::WRL::ComPtr<ID3D12Resource>				m_constantBuffer;
		ModelViewProjectionConstantBuffer					m_constantBufferData;
		UINT8*												m_mappedConstantBuffer;
		UINT												m_cbvDescriptorSize;
		D3D12_RECT											m_scissorRect;

		ComPtr<ID3D12Resource> m_outputResource;
		ComPtr<ID3D12DescriptorHeap> m_srvUavHeap;
		nv_helpers_dx12::ShaderBindingTableGenerator m_sbtHelper;
		ComPtr<ID3D12Resource> m_sbtStorage;


		std::vector<byte>									m_vertexShader;
		std::vector<byte>									m_pixelShader;
		std::vector<byte>									m_rayGenShader;
		std::vector<byte>									m_closestHitShader;
		std::vector<byte>									m_missShader;
		std::vector<byte>									m_anyHitShader;


		D3D12_VERTEX_BUFFER_VIEW							m_vertexBufferView;
		D3D12_INDEX_BUFFER_VIEW								m_indexBufferView;

		ComPtr<ID3D12Resource>								m_bottomLevelAS;
		AccelerationStructureBuffers						m_topLevelASBuffers;
		nv_helpers_dx12::TopLevelASGenerator				m_topLevelASGenerator;
		std::vector<std::pair<ComPtr<ID3D12Resource>, XMMATRIX>>	m_instances;
		
		// Variables used with the rendering loop.
		bool	m_loadingComplete;
		//float	m_radiansPerSecond;
		//float	m_angle;
		//bool	m_tracking;
	};

}