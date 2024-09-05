#pragma once
#include "pch.h"


namespace SonarPropagation {
	namespace Graphics {
		namespace Utils {
			static Windows::Foundation::Point m_prevPointer;

			class CameraController {
			public: 
				/// <summary>
				/// Default constructor.
				/// </summary>
				CameraController();

				/// <summary>
				/// Constructor with a default camera.
				/// </summary>
				/// <param name="camera"></param>
				CameraController(Camera *camera);
			
				/// <summary>
				/// Default destructor.
				/// </summary>
				~CameraController();
			
				/// <summary>
				/// Adds a camera to the controller.
				/// </summary>
				/// <param name="camera"></param>
				inline void AddCamera(Camera *camera) 
				{
					m_camera = camera;
				}

				/// <summary>
				/// Proccesses camera updates.
				/// </summary>
				/// <param name="timer"></param>
				void ProcessCameraUpdate(DX::StepTimer const& timer);
				
				/// <summary>
				/// Handles key presses.
				/// </summary>
				/// <param name="args"></param>
				void KeyPressed(Windows::UI::Core::KeyEventArgs^ args);

				/// <summary>
				/// Handles key releases.
				/// </summary>
				/// <param name="args"></param>
				void KeyReleased(Windows::UI::Core::KeyEventArgs^ args);

				/// <summary>
				/// Handles mouse movement.
				/// </summary>
				/// <param name="args"></param>
				void MouseMoved(Windows::UI::Core::PointerEventArgs^ args);
				
				/// <summary>
				/// Handles mouse wheel movement.
				/// </summary>
				/// <param name="args"></param>
				void MouseWheelMoved(Windows::UI::Core::PointerEventArgs^ args);

				/// <summary>
				/// Renders the ImGui components for the camera.
				/// </summary>
				void RenderImGui();

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

