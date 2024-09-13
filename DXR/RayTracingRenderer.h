#pragma once

#include <memory.h>
#include <map>


#include "pch.h"
#include "pix3.h"

#include "Common/ImGuiManager.h"
#include "DXR/RayTracingConfig.h"


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
				
				/// <summary>
				/// Parameterized constructor for the RayTracingRenderer.
				/// </summary>
				/// <param name="deviceResources"></param>
				RayTracingRenderer(const std::shared_ptr<DX::DeviceResources>& deviceResources);
				
				/// <summary>
				/// Destructor for the RayTracingRenderer.
				/// </summary>
				~RayTracingRenderer();

			// Graphics Initialization:

				/// <summary>
				/// Creates the necessary resources.
				/// </summary>
				void CreateDeviceDependentResources();

				/// <summary>
				/// Creates the window size dependent resources.
				/// </summary>
				void CreateWindowSizeDependentResources();

			// Raytracing Render Loop:

				/// <summary>
				/// Main update function for updating different components used in the main loop.
				/// </summary>
				/// <param name="timer"></param>
				void Update(DX::StepTimer const& timer);

				/// <summary>
				/// Main render function for rendering the scene used in the main loop.
				/// </summary>
				/// <returns></returns>
				bool Render();

				/// <summary>
				/// Saves the current state of the renderer.
				/// </summary>
				void SaveState();
				
				/// <summary>
				/// Handles the press of different keys.
				/// </summary>
				/// <param name="args"></param>
				void KeyPressed(Windows::UI::Core::KeyEventArgs^ args);

				/// <summary>
				/// Handles the release of different keys.
				/// </summary>
				/// <param name="args"></param>
				void KeyReleased(Windows::UI::Core::KeyEventArgs^ args);

				/// <summary>
				/// Handles the movement of the mouse.
				/// </summary>
				/// <param name="args"></param>
				void MouseMoved(Windows::UI::Core::PointerEventArgs^ args);

				/// <summary>
				/// Handles the movement of the mouse wheel.
				/// </summary>
				/// <param name="args"></param>
				void MouseWheelMoved(Windows::UI::Core::PointerEventArgs^ args);

			// Raytracing Utils.:

				/// <summary>
				/// Checks if the current device supports raytracing.
				/// </summary>
				void CheckRayTracingSupport();

			private:

			// Raytracing Initialization:

				/// <summary>
				/// Creates the signature for the ray generation shader.
				/// </summary>
				/// <returns></returns>
				ComPtr<ID3D12RootSignature> CreateRayGenSignature();

				/// <summary>
				/// Creates the signature for the hit shader.
				/// </summary>
				/// <returns></returns>
				ComPtr<ID3D12RootSignature> CreateHitSignature();

				/// <summary>
				/// Creates the signature for the miss shader.
				/// </summary>
				/// <returns></returns>
				ComPtr<ID3D12RootSignature> CreateMissSignature();

				/// <summary>
				/// Creates the necessary resources and interfaces for raytracing.
				/// At the moment it only creates the DXR compatible device.
				/// </summary>
				void CreateRaytracingInterfaces();

				/// <summary>
				/// Creates the acceleration structures for raytracing.
				/// </summary>
				/// <typeparam name="V">
				///	Vertex type
				/// </typeparam>
				template <typename V>
				void CreateAccelerationStructures();

				/// <summary>
				/// Creates the raytracing pipeline.
				/// </summary>
				void CreateRaytracingPipeline();

				/// <summary>
				/// Creates the raytracing output buffer.
				/// </summary>
				void CreateRaytracingOutputBuffer();

				/// <summary>
				/// Creates the shader resource heap.
				/// </summary>
				void CreateShaderResourceHeap();

				/// <summary>
				/// Creates the shader binding table.
				/// </summary>
				void CreateShaderBindingTable();

				/// <summary>
				/// Creates the per instance constant buffers.
				/// </summary>
				//void CreatePerInstanceConstantBuffers();

				/// <summary>
				///   Creates the rendered instances.
				/// </summary>
				/// <param name="asBuffers"></param>
				//void CreateInstances(std::vector<std::pair<AccelerationStructureBuffers, uint32_t>> asBuffers);

				/// <summary>
				/// Initializes the vertex buffers.
				/// </summary>
				/// <typeparam name="V">Vertex type</typeparam>
				//template <typename V>
				//void CreateVertexBuffers();

				/// <summary>
				/// Initializes the index buffers.
				/// </summary>
				//void CreateIndexBuffers();

				void InitializeObjects();
				
				void CreateScene();

				


			// Raytracing Render Loop:
				
				/// <summary>
				/// Populates the command list. Utilizes PIX for additional debugging.
				/// </summary>
				void PopulateCommandListWithPIX();

				/// <summary>
				/// Updates the transforms found in the instance constant buffers.
				/// </summary>
				void UpdateInstanceTransforms();
				
				/// <summary>
				/// Renders the ImGui windows.
				/// </summary>
				void RenderImGui();
				
			// Raytracing Initialization:

				/// <summary>
				/// Loads the state of the renderer.
				/// </summary>
				void LoadState();
				
				/// <summary>
				/// Creates the top level acceleration structure.
				/// </summary>
				/// <param name="instances"></param>
				/// <param name="updateOnly"></param>
				void CreateTopLevelAS(const std::vector < std::pair < ComPtr<ID3D12Resource>, DirectX::XMMATRIX>>& instances, bool updateOnly);
				
				/// <summary>
				/// Creates the bottom level acceleration structure.
				/// </summary>
				/// <typeparam name="V">Vertex type</typeparam>
				/// <param name="vVertexBuffers"></param>
				/// <param name="vIndexBuffers"></param>
				/// <returns></returns>
				template <typename V>
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

				//// Generic Direct3D Buffers:
				//Microsoft::WRL::ComPtr<ID3D12Resource>				m_tetrahedronVertexBuffer;
				//Microsoft::WRL::ComPtr<ID3D12Resource>				m_tetrahedronIndexBuffer;
				//UINT 												m_tetrahedronInstanceCount;

				//Microsoft::WRL::ComPtr<ID3D12Resource>				m_quadVertexBuffer;
				//Microsoft::WRL::ComPtr<ID3D12Resource>				m_quadIndexBuffer;
				//UINT												m_quadInstanceCount;

				Microsoft::WRL::ComPtr<ID3D12Resource>				m_constantBuffer;

				SonarPropagation::Graphics::Utils::Scene			m_scene;
				SonarPropagation::Graphics::Utils::ObjectLibrary    m_objectLibrary;

				//D3D12_VERTEX_BUFFER_VIEW							m_tetrahedronVertexBufferView;
				//D3D12_INDEX_BUFFER_VIEW								m_tetrahedronIndexBufferView;

				//D3D12_VERTEX_BUFFER_VIEW 							m_quadVertexBufferView;
				//D3D12_INDEX_BUFFER_VIEW 							m_quadIndexBufferView;

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

				RayTracingConfig									m_dxrConfig;

				// Shaders Bytes: 
				std::vector<byte>									m_vertexShader;
				std::vector<byte>									m_pixelShader;

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
				//std::vector<std::pair<ComPtr<ID3D12Resource>, XMMATRIX>>	m_instances;
				//std::vector<ComPtr<ID3D12Resource>> 					m_perInstanceConstantBuffers;

				// Camera: 

				SonarPropagation::Graphics::Utils::Camera			m_camera;
				SonarPropagation::Graphics::Utils::CameraController m_cameraController;

				uint32_t											m_aspectRatio;

				// ImGUI:

				ComPtr<ID3D12DescriptorHeap>						m_imguiHeap;
				SonarPropagation::Graphics::Utils::ImGuiManager		m_imguiManager;


				// Variables used with the rendering loop:
				bool												m_loadingComplete;
				bool												m_showDemoWindow = false;
				bool												m_showRaytracingWindow = true;
				bool												m_cameraWindow = false;

				bool												m_useReflections = true;

				bool												m_pipelineDirty;
				bool												m_sbtDirty;
				bool												m_ASDirty;

				bool												m_animate = true;


				uint32_t											m_time = 0;

			};
		}
	}
}