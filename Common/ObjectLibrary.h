//#include "pch.h"
#include <string>
#include "thirdparty/tiny_obj_loader.h"

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

				
				ComPtr<Scene::Model> LoadObject(const std::string& filename) {};

			private:
				std::vector<Scene::Model*> m_objects;

				ID3D12Device* m_device;
			};
		}
	}

}