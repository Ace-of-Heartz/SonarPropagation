#pragma once



#include <string>
//#include "thirdparty/tiny_obj_loader.h"
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

				
				Scene::Model* LoadWavefront(const std::string& filename);
				
				template<typename V>
				Scene::Model* LoadPredefined(const std::vector<V> vertices, const std::vector<UINT> indices);

				std::vector<Scene::Model*> m_objects;

				ID3D12Device* m_device;
			};
		}
	}

}