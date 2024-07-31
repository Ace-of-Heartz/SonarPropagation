#pragma once

#include <wrl.h>
#include <wrl/client.h>
#include <dxgi1_4.h>
#include <d3d12.h>
#include "Common\d3dx12.h"
#include <pix3.h>
#include <DirectXColors.h>
#include <DirectXMath.h>
#include <memory>
#include <vector>
#include <agile.h>
#include <concrt.h>
#include <dxcapi.h>

// 
#include "Common\Camera.h"
#include "Common\CameraController.h"
#include "Common\DeviceResources.h"
#include "Content\ShaderStructures.h"
#include "Common\StepTimer.h"

#include "DXR\Nvidia\nvidia_include.h"
#include "DXR\DXRHelpers\ShaderUtils.h"
#include "DXR\DXRHelpers\MeshUtils.h"

#include "DXR\Nvidia\DXRHelper.h"
#include "DXR\Nvidia\DXSample.h"
#include "DXR\Nvidia\DXSampleHelper.h"
#include "DXR\RaytracingRenderer.h"

using namespace DirectX;
using namespace Microsoft::WRL;
using namespace SonarPropagation::Graphics::Utils;
using namespace SonarPropagation::Graphics::Common;
using namespace SonarPropagation::Graphics::DXR;

//

#if defined(_DEBUG)
#include <dxgidebug.h>
#endif
