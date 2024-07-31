#pragma once

#include <Common/DirectXHelper.h>
#include <DXR/Nvidia/DXSampleHelper.h>
#include "../DXR/Nvidia/nvidia_include.h"
#include <dxcapi.h>

namespace SonarPropagation{
	namespace Graphics {
		namespace Common {
			IDxcBlob* CompileShader(LPCWSTR fileName);

			ID3D12DescriptorHeap* CreateDescriptorHeap(ID3D12Device* device, uint32_t count,
				D3D12_DESCRIPTOR_HEAP_TYPE type, bool shaderVisible);
		}
	}
}

