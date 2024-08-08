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

			template <typename V>
			std::vector<V> GetTetrahedronVertices(
			);

			std::vector<UINT> GetTetrahedronIndices(
			);

			template <typename V>
			std::vector<V> GetQuadVertices(
				float width, float height
			);

			std::vector<UINT> GetQuadIndices(
			);
		}
	}
}

