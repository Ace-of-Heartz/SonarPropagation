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
				CameraController(ComPtr<ID3D12Device> device);
				/// <summary>
				/// Constructor with a default camera.
				/// </summary>
				/// <param name="camera"></param>
				CameraController(ComPtr<ID3D12Device> device,Camera *camera);
			
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
					camera->CreateCameraBuffer(m_device);
					m_cameras.push_back(camera);

				}

				/// <summary>
				/// Proccesses camera updates.
				/// </summary>
				/// <param name="timer"></param>
				void ProcessCameraUpdate(DX::StepTimer const& timer);
				
				void CycleToNextCamera(){
					m_isBufferDirty = true;
					m_cameraIndex = (m_cameraIndex + 1) % m_cameras.size();
				}

				void CycleToPreviousCamera(){
					m_isBufferDirty = true;
					m_cameraIndex = (m_cameraIndex - 1) % m_cameras.size();
				}

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

				Camera* GetCurrentCamera() const;

				bool IsBufferDirty() const { return m_isBufferDirty; }

				void SetBufferToClean() { m_isBufferDirty = false; }

				std::vector<Camera*> GetSoundSources() const {
					std::vector<Camera*> soundSources;
					for (auto camera : m_cameras) {
						if (camera->IsSoundSource()) {
							soundSources.push_back(camera);
						}
					}
					return soundSources;
				} 

			private:
				ComPtr<ID3D12Device> m_device;

				std::vector<Camera*> m_cameras;
				size_t m_cameraIndex = 0;
				bool m_isBufferDirty = true;


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

