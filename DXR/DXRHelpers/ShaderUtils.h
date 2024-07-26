#pragma once

#include <Common/DirectXHelper.h>
#include <DXR/Nvidia/DXSampleHelper.h>
#include "../DXR/Nvidia/nvidia_include.h"
#include <dxcapi.h>
//#include "Utils.h"

namespace AceShaderUtils{
	IDxcBlob* CompileShader(LPCWSTR fileName);
}