#pragma once
#include "pch.h"

namespace SoundPropagation
{
	namespace Graphics 
	{
		namespace DXR 
		{
			class SoundReceiver 
			{
			public:
				SoundReceiver();
				~SoundReceiver();

			private:
				XMVECTOR m_position;
			};
		}
	}
}
