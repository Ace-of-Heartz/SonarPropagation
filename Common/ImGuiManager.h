#pragma once 
#include "thirdparty/imgui_impl_uwp.h"
#include "imgui_impl_dx12.h"
#include "DeviceResources.h"

namespace SonarPropagation {
	namespace Graphics {
		namespace Utils {
			class ImGuiManager {
			public:
				
				/// <summary>
				/// Constructor for the ImGuiManager.
				/// </summary>
				ImGuiManager();

				/// <summary>
				/// Destructor for the ImGuiManager.
				/// </summary>
				~ImGuiManager();
				
				/// <summary>
				/// Initializes Dear ImGui.
				/// </summary>
				/// <param name="frameCount"></param>
				/// <param name="backBufferFormat"></param>
				/// <param name="dxrDevice"></param>
				/// <param name="srvHeap"></param>
				void InitImGui(int frameCount, DXGI_FORMAT backBufferFormat, ID3D12Device5* dxrDevice, ID3D12DescriptorHeap* srvHeap);
				
				/// <summary>
				/// Begins the Dear ImGui render call.
				/// </summary>
				/// <param name="commandList"></param>
				void BeginImGui(ID3D12GraphicsCommandList4* commandList);
				
				/// <summary>
				/// Ends the Dear ImGui render call.
				/// </summary>
				/// <param name="commandList"></param>
				void EndImGui(ID3D12GraphicsCommandList4* commandList);


			private: 
			};
		}
	}
}