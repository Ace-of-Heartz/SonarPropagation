#pragma once 
#include "thirdparty/imgui_impl_uwp.h"
#include "imgui_impl_dx12.h"
#include "DeviceResources.h"

namespace SonarPropagation {
	namespace Graphics {
		namespace Utils {
			class ImGuiManager {
			public:
				ImGuiManager();
				~ImGuiManager();

				void InitImGui(int frameCount, DXGI_FORMAT backBufferFormat, ID3D12Device5* dxrDevice, ID3D12DescriptorHeap* srvHeap);
				void RenderImGui(ID3D12GraphicsCommandList4* commandList);
				


			private: 
			};
		}
	}
}