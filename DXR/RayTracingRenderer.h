#pragma once

#include <memory.h>
#include <pch.h>
#include "pix3.h"
#include "Common/ImGuiManager.h"
#include <map>

using namespace Microsoft::WRL;
using namespace DirectX;



namespace SonarPropagation{
	namespace Graphics {
		namespace DXR {
			/// <summary>
			/// Struct to hold the Acceleration Structure Buffers.
			/// </summary>
			struct AccelerationStructureBuffers {
				ComPtr<ID3D12Resource> pScratch;
				ComPtr<ID3D12Resource> pResult;
				ComPtr<ID3D12Resource> pInstanceDesc; // Used only for top-level AS
			};




			/// <summary>
			/// Main class for the RayTracingRenderer. Responsible for creating the RayTracing Pipeline.
			/// </summary>
			class RayTracingRenderer {
			public:

				// Constructor and Destructor:
				RayTracingRenderer(const std::shared_ptr<DX::DeviceResources>& deviceResources);
				~RayTracingRenderer();

				// Graphics Initialization:
				void CreateDeviceDependentResources();
				void CreateWindowSizeDependentResources();

				// Raytracing Render Loop:
				void Update(DX::StepTimer const& timer);
				bool Render();
				void SaveState();
				
				void KeyPressed(Windows::UI::Core::KeyEventArgs^ args);
				void KeyReleased(Windows::UI::Core::KeyEventArgs^ args);
				void MouseMoved(Windows::UI::Core::PointerEventArgs^ args);
				void MouseWheelMoved(Windows::UI::Core::PointerEventArgs^ args);

				// Raytracing Utils.:
				void CheckRayTracingSupport();

			private:

				// Raytracing Initialization:
				ComPtr<ID3D12RootSignature> CreateRayGenSignature();
				ComPtr<ID3D12RootSignature> CreateHitSignature();
				ComPtr<ID3D12RootSignature> CreateMissSignature();

				void CreateRaytracingInterfaces();
				void CreateAccelerationStructures();
				void CreateRaytracingPipeline();
				void CreateRaytracingOutputBuffer();
				void CreateShaderResourceHeap();
				void CreateShaderBindingTable();
				void CreatePerInstanceConstantBuffers();
				void CreateInstances(std::vector<std::pair<AccelerationStructureBuffers, uint32_t>> asBuffers);


				// Raytracing Render Loop:
				void PopulateCommandListWithPIX();
				void UpdateInstanceTransforms();
				
				void RenderImGui();
				
				// Raytracing Initialization:
				void LoadState();
				void CreateTopLevelAS(const std::vector < std::pair < ComPtr<ID3D12Resource>, DirectX::XMMATRIX>>& instances, bool updateOnly);
				AccelerationStructureBuffers CreateBottomLevelAS(std::vector<std::pair<ComPtr<ID3D12Resource>, uint32_t>> vVertexBuffers,
					std::vector<std::pair<ComPtr<ID3D12Resource>, uint32_t>> vIndexBuffers
				);

			private:
				// Raytracing pipeline objects:
				ComPtr<ID3D12StateObject> m_rtStateObject;

				ComPtr<ID3D12StateObjectProperties> m_rtStateObjectProps;

				// Constant buffers must be 256-byte aligned:
				static const UINT c_alignedConstantBufferSize = (sizeof(ModelViewProjectionConstantBuffer) + 255) & ~255;

				// Cached pointer to device resources:
				std::shared_ptr<DX::DeviceResources>				m_deviceResources;

				// Generic 3D Objects:
				Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList4>	m_commandList;
				Microsoft::WRL::ComPtr<ID3D12RootSignature>			m_rootSignature;
				Microsoft::WRL::ComPtr<ID3D12PipelineState>			m_pipelineState;
				Microsoft::WRL::ComPtr<ID3D12DescriptorHeap>		m_cbvHeap;

				UINT												m_cbvDescriptorSize;
				D3D12_RECT											m_scissorRect;

				// Generic Direct3D Buffers:
				Microsoft::WRL::ComPtr<ID3D12Resource>				m_tetrahedronVertexBuffer;
				Microsoft::WRL::ComPtr<ID3D12Resource>				m_tetrahedronIndexBuffer;
				UINT 												m_tetrahedronInstanceCount;

				Microsoft::WRL::ComPtr<ID3D12Resource>				m_quadVertexBuffer;
				Microsoft::WRL::ComPtr<ID3D12Resource>				m_quadIndexBuffer;
				UINT												m_quadInstanceCount;


				Microsoft::WRL::ComPtr<ID3D12Resource>				m_constantBuffer;

				D3D12_VERTEX_BUFFER_VIEW							m_tetrahedronVertexBufferView;
				D3D12_INDEX_BUFFER_VIEW								m_tetrahedronIndexBufferView;

				D3D12_VERTEX_BUFFER_VIEW 							m_quadVertexBufferView;
				D3D12_INDEX_BUFFER_VIEW 							m_quadIndexBufferView;

				//ModelViewProjectionConstantBuffer					m_constantBufferData;
				//UINT8*												m_mappedConstantBuffer;

				// DXR Specific Attributes:
				ComPtr<ID3D12Device5>								m_dxrDevice;

				ComPtr<ID3D12Resource>								m_outputResource;

				ComPtr<ID3D12DescriptorHeap>						m_srvUavHeap;
				nv_helpers_dx12::ShaderBindingTableGenerator		m_sbtHelper;
				ComPtr<ID3D12Resource>								m_sbtStorage;
				ComPtr<ID3D12Resource>								m_bottomLevelAS;
				AccelerationStructureBuffers						m_topLevelASBuffers;
				nv_helpers_dx12::TopLevelASGenerator				m_topLevelASGenerator;

				// Shaders Bytes: 
				std::vector<byte>									m_vertexShader;
				std::vector<byte>									m_pixelShader;
				std::vector<byte>									m_rayGenShader;
				std::vector<byte>									m_missShader;
				std::vector<byte>									m_hitShader;

				// Shader Libraries:
				ComPtr<IDxcBlob>									m_rayGenLibrary;
				ComPtr<IDxcBlob>									m_hitLibrary;
				ComPtr<IDxcBlob>									m_missLibrary;
				ComPtr<IDxcBlob>									m_shadowLibrary;
				ComPtr<IDxcBlob> 								    m_reflectionLibrary;

				// Root Signatures for Shader:
				ComPtr<ID3D12RootSignature>							m_rayGenSignature;
				ComPtr<ID3D12RootSignature>							m_hitSignature;
				ComPtr<ID3D12RootSignature>							m_missSignature;
				ComPtr<ID3D12RootSignature>							m_shadowSignature;
				ComPtr<ID3D12RootSignature>							m_reflectionSignature;	

				// Instances: 
				std::vector<std::pair<ComPtr<ID3D12Resource>, XMMATRIX>>	m_instances;
				std::vector<ComPtr<ID3D12Resource>> 					m_perInstanceConstantBuffers;

				// Camera: 

				SonarPropagation::Graphics::Utils::Camera			m_camera;
				SonarPropagation::Graphics::Utils::CameraController m_cameraController;

				uint32_t											m_aspectRatio;

				// ImGUI:

				ComPtr<ID3D12DescriptorHeap>						m_imguiHeap;
				SonarPropagation::Graphics::Utils::ImGuiManager		m_imguiManager;


				// Variables used with the rendering loop:
				bool												m_loadingComplete;
				bool m_showDemoWindow = false;
				bool m_showRaytracingWindow = true;
				bool m_cameraWindow = false;

				bool m_useReflections = true;

				uint32_t m_time = 0;

			};
		}
	}
}