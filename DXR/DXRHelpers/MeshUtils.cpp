#include "pch.h"
#include "MeshUtils.h"





template <typename V>
std::vector<V> SonarPropagation::Graphics::Utils::GetTetrahedronVertices<V>() {
	throw std::exception("GetTetrahedronVertices not implemented for this type");
}

template <> 
std::vector<VertexPosition> SonarPropagation::Graphics::Utils::GetTetrahedronVertices(

) {
	std::vector<VertexPosition> vertices =
	{
		{{std::sqrtf(8.f / 9.f), 0.f, 1.f / 3.f}},
		{{-std::sqrtf(2.f / 9.f), std::sqrtf(2.f / 3.f), 1.f / 3.f}},
		{{-std::sqrtf(2.f / 9.f), -std::sqrtf(2.f / 3.f), 1.f / 3.f}},
		{{0.f, 0.f, 1.f}}
	};

	return vertices;
}

template <>
std::vector<VertexPositionNormal> SonarPropagation::Graphics::Utils::GetTetrahedronVertices() 
{
	std::vector<VertexPositionNormal> vertices =
	{
		{{std::sqrtf(8.f / 9.f), 0.f, 1.f / 3.f}, {0.f, 0.f, 1.f}},
		{{-std::sqrtf(2.f / 9.f), std::sqrtf(2.f / 3.f), 1.f / 3.f}, {0.f, 0.f, 1.f}},
		{{-std::sqrtf(2.f / 9.f), -std::sqrtf(2.f / 3.f), 1.f / 3.f}, {0.f, 0.f, 1.f}},
		{{0.f, 0.f, 1.f}, {0.f, 0.f, 1.f}}
	};

	return vertices;
}

template <>
std::vector<VertexPositionColor> SonarPropagation::Graphics::Utils::GetTetrahedronVertices()
{
	std::vector<VertexPositionColor> vertices =
	{
		{{std::sqrtf(8.f / 9.f), 0.f, 1.f / 3.f}, {1.f, 0.f, 0.f}},
		{{-std::sqrtf(2.f / 9.f), std::sqrtf(2.f / 3.f), 1.f / 3.f}, {0.f, 1.f, 0.f}},
		{{-std::sqrtf(2.f / 9.f), -std::sqrtf(2.f / 3.f), 1.f / 3.f}, {0.f, 0.f, 1.f}},
		{{0.f, 0.f, 1.f}, {1.f, 1.f, 0.f}}
	};

	return vertices;
}

std::vector<UINT> SonarPropagation::Graphics::Utils::GetTetrahedronIndices() {
	std::vector<UINT> indices = { 0, 1, 2, 0, 3, 1, 0, 2, 3, 1, 3, 2 };
	return indices;
};


template <typename V> 
std::vector<V> SonarPropagation::Graphics::Utils::GetQuadVertices<V>(float w, float h) {
	throw std::exception("GetQuadVertices not implemented for this type");
}

template <>
std::vector<VertexPosition> SonarPropagation::Graphics::Utils::GetQuadVertices<VertexPosition>(
	float w, float h
)
{
	std::vector<VertexPosition> vertices =
	{
		{{0.5f * w, 0.f, 0.5f * h}},
		{{-0.5f * w, 0.f, 0.5f * h}},
		{{-0.5f * w, 0.f, -0.5f * h}},
		{{0.5f * w, 0.f, -0.5f * h}},
	};

	return vertices;
}

template <>
std::vector<VertexPositionNormal> SonarPropagation::Graphics::Utils::GetQuadVertices<VertexPositionNormal>(
	float w, float h
)
{
	std::vector<VertexPositionNormal> vertices =
	{
		{{0.5f * w, 0.f, 0.5f * h}, {0.f, 1.f, 0.f}},
		{{-0.5f * w, 0.f, 0.5f * h}, {0.f, 1.f, 0.f}},
		{{-0.5f * w, 0.f, -0.5f * h}, {0.f, 1.f, 0.f}},
		{{0.5f * w, 0.f, -0.5f * h}, {0.f, 1.f, 0.f}},
	};

	return vertices;
}

template <>
std::vector<VertexPositionColor> SonarPropagation::Graphics::Utils::GetQuadVertices<VertexPositionColor>(
	float w, float h
)
{
	std::vector<VertexPositionColor> vertices =
	{
		{{0.5f * w, 0.f, 0.5f * h}, {1.f, 0.f, 0.f}},
		{{-0.5f * w, 0.f, 0.5f * h}, {0.f, 1.f, 0.f}},
		{{-0.5f * w, 0.f, -0.5f * h}, {0.f, 0.f, 1.f}},
		{{0.5f * w, 0.f, -0.5f * h}, {1.f, 1.f, 0.f}},
	};

	return vertices;
}



std::vector<UINT> SonarPropagation::Graphics::Utils::GetQuadIndices() {
	std::vector<UINT> indices = { 
		0, 1, 2,
		2, 3, 0 
	};
	return indices;
}

template <>
std::vector<VertexPositionNormalUV>  SonarPropagation::Graphics::Utils::GetCubeVertices<VertexPositionNormalUV>(
	float width, float height, float depth
)
{
	std::vector<VertexPositionNormalUV> vertices = {
		// Front Face
		{ { -0.5f * width, -0.5f * height, -0.5f * depth }, { 0.f, 0.f, -1.f }, { 0.f, 1.f } },
		{ { -0.5f * width,  0.5f * height, -0.5f * depth }, { 0.f, 0.f, -1.f }, { 0.f, 0.f } },
		{ {  0.5f * width,  0.5f * height, -0.5f * depth }, { 0.f, 0.f, -1.f }, { 1.f, 0.f } },
		{ {  0.5f * width, -0.5f * height, -0.5f * depth }, { 0.f, 0.f, -1.f }, { 1.f, 1.f } },
		
		// Back Face
		{ { -0.5f * width, -0.5f * height,  0.5f * depth }, { 0.f, 0.f, 1.f }, { 1.f, 1.f } },
		{ {  0.5f * width, -0.5f * height,  0.5f * depth }, { 0.f, 0.f, 1.f }, { 0.f, 1.f } },
		{ {  0.5f * width,  0.5f * height,  0.5f * depth }, { 0.f, 0.f, 1.f }, { 0.f, 0.f } },
		{ { -0.5f * width,  0.5f * height,  0.5f * depth }, { 0.f, 0.f, 1.f }, { 1.f, 0.f } },
		
		// Top Face
		{ { -0.5f * width,  0.5f * height, -0.5f * depth }, { 0.f, 1.f, 0.f }, { 0.f, 1.f } },
		{ { -0.5f * width,  0.5f * height,  0.5f * depth }, { 0.f, 1.f, 0.f }, { 0.f, 0.f } },
		{ {  0.5f * width,  0.5f * height,  0.5f * depth }, { 0.f, 1.f, 0.f }, { 1.f, 0.f } },
		{ {  0.5f * width,  0.5f * height, -0.5f * depth }, { 0.f, 1.f, 0.f }, { 1.f, 1.f } },
		
		// Bottom Face
		{ { -0.5f * width, -0.5f * height, -0.5f * depth }, { 0.f, -1.f, 0.f }, { 1.f, 1.f } },
		{ {  0.5f * width, -0.5f * height, -0.5f * depth }, { 0.f, -1.f, 0.f }, { 0.f, 1.f } },
		{ {  0.5f * width, -0.5f * height,  0.5f * depth }, { 0.f, -1.f, 0.f }, { 0.f, 0.f } },
		{ { -0.5f * width, -0.5f * height,  0.5f * depth }, { 0.f, -1.f, 0.f }, { 1.f, 0.f } },
		
		// Left Face
		{ { -0.5f * width, -0.5f * height,  0.5f * depth }, { -1.f, 0.f, 0.f }, { 0.f, 1.f } },
		{ { -0.5f * width,  0.5f * height,  0.5f * depth }, { -1.f, 0.f, 0.f }, { 0.f, 0.f } },
		{ { -0.5f * width,  0.5f * height, -0.5f * depth }, { -1.f, 0.f, 0.f }, { 1.f, 0.f } },
		{ { -0.5f * width, -0.5f * height, -0.5f * depth }, { -1.f, 0.f, 0.f }, { 1.f, 1.f } },
		
		// Right Face
		{ {  0.5f * width, -0.5f * height, -0.5f * depth }, { 1.f, 0.f, 0.f }, { 0.f, 1.f } },
		{ {  0.5f * width,  0.5f * height, -0.5f * depth }, { 1.f, 0.f, 0.f }, { 0.f, 0.f } },
		{ {  0.5f * width,  0.5f * height,  0.5f * depth }, { 1.f, 0.f, 0.f }, { 1.f, 0.f } },
		{ {  0.5f * width, -0.5f * height,  0.5f * depth }, { 1.f, 0.f, 0.f }, { 1.f, 1.f } } 
	};

	return vertices;
}

std::vector<UINT> SonarPropagation::Graphics::Utils::GetCubeIndices() {
	std::vector<UINT> indices = {
		// Front Face
		0, 1, 2,
		2, 3, 0,

		// Back Face
		4, 5, 6,
		6, 7, 4,

		// Top Face
		8, 9, 10,
		10, 11, 8,

		// Bottom Face
		12, 13, 14,
		14, 15, 12,

		// Left Face
		16, 17, 18,
		18, 19, 16,

		// Right Face
		20, 21, 22,
		22, 23, 20
	};

	return indices;
}