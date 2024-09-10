#include "pch.h"
#include "Scene.h"

namespace SonarPropagation
{
	namespace Graphics
	{
		namespace Utils
		{
			class ObjectLibrary {
			public: 
				ObjectLibrary(ID3D12Device* device) : m_device(device) {};
				~ObjectLibrary() {};

				
				Model* LoadObject(const std::string& filename);

			private:
				std::vector<Model> m_objects;

				ID3D12Device* m_device;
			};
		}
	}

}