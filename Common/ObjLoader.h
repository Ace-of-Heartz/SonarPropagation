#include "./thirdparty/tiny_obj_loader.h"
#include "pch.h"

#define TINYOBJLOADER_IMPLEMENTATION

namespace SonarPropagation 
{
	namespace Graphics 
	{
		namespace Utils 
		{
			class ObjLoader
			{
				ObjLoader();

				static std::vector<VertexPositionNormalUV> LoadObj(const std::string& filename);
			};
		}
	}

}