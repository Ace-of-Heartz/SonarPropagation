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
				ObjectLibrary() {};
				~ObjectLibrary() {};

			private:
				std::vector<Model> m_objects;

			};
		}
	}

}