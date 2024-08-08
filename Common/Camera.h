#pragma once
#include <array>

using namespace DirectX;
using namespace Microsoft::WRL;



namespace SonarPropagation {
	namespace Graphics {
		namespace Utils {

			class Camera {
				friend class CameraController;

			public:
				Camera();
				~Camera();

				inline void SetPosition(float x, float y, float z) {
					m_eye = XMVectorSet(x, y, z, 0.0f);
					m_isViewDirty = true;
				}
				inline void SetLookAt(float x, float y, float z) {
					m_at = XMVectorSet(x, y, z, 0.0f);
					m_isViewDirty = true;
				}
				inline void SetAspectRatio(float aspectRatio) {
					m_aspectRatio = aspectRatio;
					m_isProjectionDirty = true;
				}
				inline void SetFOV(float fovAngleY) {
					m_fovAngleY = fovAngleY;
					m_isProjectionDirty = true;
				}

				void CreateCameraBuffer(ComPtr<ID3D12Device> device);
				void UpdateCameraBuffer();
				void RenderCameraImGui();


				inline ComPtr<ID3D12Resource> GetCameraBuffer() const {
					return m_cameraBuffer;
				}
				inline ComPtr<ID3D12DescriptorHeap> GetCameraHeap() const {
					return m_cameraHeap;
				}

				inline uint32_t GetCameraBufferSize() const {
					return m_cameraBufferSize;
				}


				inline float GetSpeed() const {
					return m_speed;
				}
				

			private:

				void UpdateParameters();
				void UpdateUV(float du, float dv);
				void UpdateDistance(float dDistance);
				void UpdateViewMatrix();
				void UpdateProjectionMatrix();

				bool m_isViewDirty = true;
				bool m_isProjectionDirty = true;

				XMVECTOR m_eye;
				XMVECTOR m_at;

				XMVECTOR m_forward;
				XMVECTOR m_right;
				XMVECTOR m_up;

				XMVECTOR m_worldUp;

				float m_yaw;
				float m_pitch;

				XMVECTOR m_rotationQ;

				float m_distance;

				float m_fovAngleY;
				float m_aspectRatio;

				float m_zNear;
				float m_zFar;
				
				float m_lastYaw;
				float m_lastPitch;

				float m_speed;

				XMMATRIX m_viewMatrix;
				XMMATRIX m_viewMatrixInv;
				
				XMMATRIX m_projectionMatrix;
				XMMATRIX m_projectionMatrixInv;

				std::array<XMMATRIX,4> allMatrices;

				ComPtr<ID3D12Resource> m_cameraBuffer;
				ComPtr<ID3D12DescriptorHeap> m_cameraHeap;

				uint32_t m_cameraBufferSize = 0;
			};

		}
	}
}
