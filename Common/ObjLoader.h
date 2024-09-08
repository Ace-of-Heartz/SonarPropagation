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
			public:
				static std::tuple<std::vector<VertexPositionNormalUV>,std::vector<tinyobj::index_t>> LoadObj(const std::string& filename);
			};
		}
	}

}