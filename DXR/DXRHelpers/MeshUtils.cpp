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