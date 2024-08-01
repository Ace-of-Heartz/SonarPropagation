#pragma once
#include "pch.h"


namespace SonarPropagation {
	namespace Graphics {
		namespace Utils {
			class CameraController {
			public: 

				CameraController();
				CameraController(Camera *camera);
			
				~CameraController();
			
				inline void AddCamera(Camera *camera) 
				{
					m_camera = camera;
				}

				void ProcessCameraUpdate(DX::StepTimer const& timer);
				
				void KeyPressed(Windows::UI::Core::KeyEventArgs^ args);
				void KeyReleased(Windows::UI::Core::KeyEventArgs^ args);

				void MouseMoved(Windows::Devices::Input::MouseEventArgs^ args);
				void MouseWheelMoved(Windows::UI::Core::PointerEventArgs^ args);

			private:

				Camera* m_camera;

				size_t m_cameraIndex = 0;

				float m_forwardSpeed  = 0.0f;
				float m_sidewaysSpeed = 0.0f;
				float m_upwardsSpeed  = 0.0f;

				float m_yawSpeed = 0.0f;
				float m_pitchSpeed = 0.0f;
				float m_rollSpeed = 0.0f;
			};
		}
	}
}

