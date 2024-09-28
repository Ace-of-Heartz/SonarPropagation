#pragma once

#include <memory.h>
#include <map>


#include "pch.h"
#include "pix3.h"

#include "Common/ImGuiManager.h"
#include "DXR/RayTracingConfig.h"
#include "Common/ObjectLibrary.h"
#include "DescriptorHeap.h"


using namespace Microsoft::WRL;
using namespace DirectX;
using namespace SonarPropagation::Graphics::Utils;


namespace SonarPropagation{
	namespace Graphics {
		namespace DXR {


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

				void CreateSamplerResources();



				/// <summary>
				/// Creates the shader resource heap.
				/// </summary>
				void CreateShaderResourceHeap();

				/// <summary>
				/// Creates the shader binding table.
				/// </summary>
				void CreateShaderBindingTable();

				void CreateShaderBindingTableForPhotonMapping();

				void InitializeObjects();
				
				void CreateScene();

			// Raytracing Render Loop:
				
				/// <summary>
				/// Populates the command list. Utilizes PIX for additional debugging.
				/// </summary>
				void PopulateCommandListForRendering();

				void PhotonMappingPreprocess();

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

				D3D12_RECT											m_scissorRect;

				Microsoft::WRL::ComPtr<ID3D12Resource>				m_constantBuffer;

				Scene			m_scene;
				ObjectLibrary    m_objectLibrary;

				std::vector<std::pair<ComPtr<ID3D12Resource>, XMMATRIX>> m_instances;

				// DXR Specific Attributes:
				ComPtr<ID3D12Device5>								m_dxrDevice;

				ComPtr<ID3D12Resource>								m_outputResource;

				ComPtr<ID3D12DescriptorHeap>						m_samplerHeap;
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
				ComPtr<IDxcBlob>								m_sonarRayGenLibrary;
				ComPtr<IDxcBlob>								m_sonarHitLibrary;
				ComPtr<IDxcBlob>								m_sonarMissLibrary;

				// Root Signatures for Shader:
				ComPtr<ID3D12RootSignature>							m_rayGenSignature;
				ComPtr<ID3D12RootSignature>							m_hitSignature;
				ComPtr<ID3D12RootSignature>							m_missSignature;


				// Photon Mapping Resources: 
				std::vector<ComPtr<ID3D12Resource>>					m_textures;
				std::vector<D3D12_SUBRESOURCE_DATA>			        m_textureDatas;

				std::unique_ptr<DescriptorHeap>						 m_resourceDescriptors;

				// Camera: 

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