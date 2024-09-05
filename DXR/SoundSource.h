#pragma once
#include "pch.h"

namespace SoundPropagation
{
	namespace Graphics 
	{
		namespace DXR 
		{
			class SoundSource 
			{
			public:
				SoundSource();
				~SoundSource();

			private:
				XMVECTOR m_position;

				
				//TODO: Direction for sound rays
				
			};
		}
	}
}