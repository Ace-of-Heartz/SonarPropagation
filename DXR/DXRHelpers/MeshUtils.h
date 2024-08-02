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

			std::vector<VertexPosition> GetTetrahedronVertices(
			);

			std::vector<UINT> GetTetrahedronIndices(
			);


			std::vector<VertexPosition> GetQuadVertices(
				float width, float height
			);

			std::vector<UINT> GetQuadIndices(
			);
		}
	}
}

