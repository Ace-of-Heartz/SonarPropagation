﻿#pragma once

namespace SonarPropagation
{
	// Constant buffer used to send MVP matrices to the vertex shader.
	struct ModelViewProjectionConstantBuffer
	{
		DirectX::XMFLOAT4X4 model;
		DirectX::XMFLOAT4X4 view;
		DirectX::XMFLOAT4X4 projection;
	};

	// Used to send per-vertex data to the vertex shader.
	struct VertexPositionColor
	{
		DirectX::XMFLOAT3 pos;
		DirectX::XMFLOAT3 color;
	};

	struct VertexPositionNormal
	{
		DirectX::XMFLOAT3 pos;
		DirectX::XMFLOAT3 normal;
	};
	
	struct VertexPositionNormalUV
	{
		DirectX::XMFLOAT4 posU;
		DirectX::XMFLOAT4 normalV;

	};

	struct VertexPosition 
	{
		DirectX::XMFLOAT3 pos;
	};
}