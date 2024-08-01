#include "pch.h"
#include "ShaderUtils.h"


IDxcBlob* SonarPropagation::Graphics::Common::CompileShader(LPCWSTR fileName) {
	static IDxcCompiler* pCompiler = nullptr;
	static IDxcLibrary* pLibrary = nullptr;
	static IDxcIncludeHandler* dxcIncludeHandler;

	HRESULT hr;
	UINT32 code(0);
	IDxcBlobEncoding* pShaderText(nullptr);

	// Initialize the DXC compiler and compiler helper
	if (!pCompiler)
	{
		ThrowIfFailed(DxcCreateInstance(CLSID_DxcCompiler, __uuidof(IDxcCompiler), (void**)&pCompiler));
		ThrowIfFailed(DxcCreateInstance(CLSID_DxcLibrary, __uuidof(IDxcLibrary), (void**)&pLibrary));
		ThrowIfFailed(pLibrary->CreateIncludeHandler(&dxcIncludeHandler));
	}

	TCHAR pwd[MAX_PATH];
	GetCurrentDirectory(MAX_PATH, pwd);


	hr = pLibrary->CreateBlobFromFile(fileName, &code, &pShaderText);
	//Utils::Validate(hr, L"Error: failed to create blob from shader file!");


	// Compile
	IDxcOperationResult* pResult;
	std::vector<LPCWSTR> args = { L"-Zi",L"-Qembed_debug", L"-Od" };

	ThrowIfFailed(pCompiler->Compile(pShaderText, fileName, L"", L"lib_6_3", args.data(), args.size(), nullptr, 0,
		dxcIncludeHandler, &pResult));

	// Verify the result
	HRESULT resultCode;
	ThrowIfFailed(pResult->GetStatus(&resultCode));
	if (FAILED(resultCode))
	{
		IDxcBlobEncoding* pError;
		hr = pResult->GetErrorBuffer(&pError);
		if (FAILED(hr))
		{
			throw std::logic_error("Failed to get shader compiler error");
		}

		// Convert error blob to a string
		std::vector<char> infoLog(pError->GetBufferSize() + 1);
		memcpy(infoLog.data(), pError->GetBufferPointer(), pError->GetBufferSize());
		infoLog[pError->GetBufferSize()] = 0;

		std::string errorMsg = "Shader Compiler Error:\n";
		errorMsg.append(infoLog.data());

		throw std::logic_error("Failed compile shader");
	}

	IDxcBlob* pBlob;
	ThrowIfFailed(pResult->GetResult(&pBlob));
	return pBlob;
}

ID3D12DescriptorHeap* SonarPropagation::Graphics::Common::CreateDescriptorHeap(ID3D12Device* device, uint32_t count,
	D3D12_DESCRIPTOR_HEAP_TYPE type, bool shaderVisible) {	D3D12_DESCRIPTOR_HEAP_DESC desc = {};
	desc.NumDescriptors = count;
	desc.Type = type;
	desc.Flags =
		shaderVisible ? D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE : D3D12_DESCRIPTOR_HEAP_FLAG_NONE;

	ID3D12DescriptorHeap* pHeap;
	ThrowIfFailed(device->CreateDescriptorHeap(&desc, IID_PPV_ARGS(&pHeap)));
	return pHeap;
}

