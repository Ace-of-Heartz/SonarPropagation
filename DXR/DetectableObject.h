#pragma once
#include "pch.h"

namespace SoundPropagation
{
	namespace Graphics 
	{
		namespace DXR 
		{
			class DetectableObject
			{
			public:
				DetectableObject();
				~DetectableObject();

			private:
				Model m_model;
				XMMATRIX m_transform;	
			};
		}
	}
}