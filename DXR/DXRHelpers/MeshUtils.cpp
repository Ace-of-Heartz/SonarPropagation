#include "pch.h"
#include "MeshUtils.h"



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

std::vector<UINT> SonarPropagation::Graphics::Utils::GetTetrahedronIndices() {
	std::vector<UINT> indices = { 0, 1, 2, 0, 3, 1, 0, 2, 3, 1, 3, 2 };
	return indices;
};


 std::vector<VertexPosition> SonarPropagation::Graphics::Utils::GetQuadVertices(
	float w, float h
) {

	std::vector<VertexPosition> vertices =
	{
		{{1.f * w,0.f,1.f * h}},
		{{-1.f * w,0.f,1.f * h}},
		{{-1.f * w,0.f,-1.f * h}},
		{{1.f * w,0.f,-1.f * h}},
	};

	return vertices;
}

 std::vector<UINT> SonarPropagation::Graphics::Utils::GetQuadIndices() {
 	std::vector<UINT> indices = { 0, 1, 2, 0, 2, 3 };
	return indices;
 }

