#pragma once


#include <cmath>
#include "./Content/ShaderStructures.h"
#include "./DXR/Nvidia/DXSampleHelper.h"
using namespace SonarPropagation;
using namespace DirectX;
using namespace Microsoft::WRL;


namespace SonarPropagation {
	namespace Graphics {
		namespace Utils {
			void CreateTriangleMesh(
				ComPtr<ID3D12Device> device,
				ComPtr<ID3D12Resource> vertexBuffer,
				D3D12_VERTEX_BUFFER_VIEW& vertexBufferView,
				ComPtr<ID3D12Resource> indexBuffer,
				D3D12_INDEX_BUFFER_VIEW indexBufferView);
		}
	}
}

