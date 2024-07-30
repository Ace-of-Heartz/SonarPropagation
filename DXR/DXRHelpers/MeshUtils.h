#pragma once

#include "..\Content\ShaderStructures.h"
#include ".\DXR\Nvidia\nvidia_include.h"
#include ".\DXR\Nvidia\DXRHelper.h"
#include ".\DXR\Nvidia\DXSample.h"
#include ".\DXR\Nvidia\DXSampleHelper.h"
#include <Common/DirectXHelper.h>
#include <DirectXMath.h>
#include <cmath>

using namespace SonarPropagation;
using namespace DirectX;
using namespace Microsoft::WRL;
using namespace nv_helpers_dx12;

namespace SonarPropgation {
	void CreateTriangleMesh(
		ComPtr<ID3D12Device> device,
		ComPtr<ID3D12Resource> vertexBuffer,
		D3D12_VERTEX_BUFFER_VIEW& vertexBufferView,
		ComPtr<ID3D12Resource> indexBuffer,
		D3D12_INDEX_BUFFER_VIEW indexBufferView);


}

