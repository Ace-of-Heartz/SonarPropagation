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
				/// <summary>
				/// Default constructor.
				/// </summary>
				Camera(
					XMVECTOR eye = XMVectorSet(0.0f, 0.0f, 0.0f, 0.0f),
					XMVECTOR at = XMVectorSet(0.0f, 0.0f, 1.0f, 0.0f),
					float yaw = 0.f,
					float pitch = 0.f,
					float fovAngleY = 75.0f,
					float aspectRatio = 1.0,
					float zNear = 0.1f,
					float zFar = 10000.f,
					float speed = 5.0f
					);

				/// <summary>
				/// Default destructor.
				/// </summary>
				~Camera();

				/// <summary>
				/// Sets the camera position.
				/// </summary>
				/// <param name="x"></param>
				/// <param name="y"></param>
				/// <param name="z"></param>
				inline void SetPosition(float x, float y, float z) {
					m_eye = XMVectorSet(x, y, z, 0.0f);
					m_isViewDirty = true;
				}

				/// <summary>
				/// Sets the camera look at position.
				/// </summary>
				/// <param name="x"></param>
				/// <param name="y"></param>
				/// <param name="z"></param>
				inline void SetLookAt(float x, float y, float z) {
					m_at = XMVectorSet(x, y, z, 0.0f);
					m_isViewDirty = true;
				}

				/// <summary>
				/// Sets the camera up vector.
				/// </summary>
				/// <param name="aspectRatio"></param>
				inline void SetAspectRatio(float aspectRatio) {
					m_aspectRatio = aspectRatio;
					m_isProjectionDirty = true;
				}

				/// <summary>
				/// Sets the camera field of view.
				/// </summary>
				/// <param name="fovAngleY"></param>
				inline void SetFOV(float fovAngleY) {
					m_fovAngleY = fovAngleY;
					m_isProjectionDirty = true;
				}

				/// <summary>
				/// Creates the camera buffer.
				/// </summary>
				/// <param name="device"></param>
				void CreateCameraBuffer(ComPtr<ID3D12Device> device);

				/// <summary>
				/// Updates the camera buffer.
				/// </summary>
				void UpdateCameraBuffer();

				/// <summary>
				/// Renders the ImGui components for the camera.
				/// </summary>
				void RenderCameraImGui();

				/// <summary>
				/// Getter for the camera buffer.
				/// </summary>
				/// <returns></returns>
				inline ComPtr<ID3D12Resource> GetCameraBuffer() const {
					return m_cameraBuffer;
				}

				/// <summary>
				/// Getter for the camera heap.
				/// </summary>
				/// <returns></returns>
				inline ComPtr<ID3D12DescriptorHeap> GetCameraHeap() const {
					return m_cameraHeap;
				}

				/// <summary>
				/// Getter for the camera buffer size.
				/// </summary>
				/// <returns></returns>
				inline uint32_t GetCameraBufferSize() const {
					return m_cameraBufferSize;
				}

				/// <summary>
				/// Get for the current camera speed.
				/// </summary>
				/// <returns></returns>
				inline float GetSpeed() const {
					return m_speed;
				}

				inline void ToggleSoundSource() {
					m_isSoundSource = !m_isSoundSource;
				}

				inline bool IsSoundSource() {
					return m_isSoundSource;
				}
				
			private:

				/// <summary>
				/// Updates the camera parameters.
				/// </summary>
				void UpdateParameters();

				/// <summary>
				/// Updates the camera UV values
				/// </summary>
				/// <param name="du"></param>
				/// <param name="dv"></param>
				void UpdateUV(float du, float dv);

				/// <summary>
				/// Updates the camera distance.
				/// </summary>
				/// <param name="dDistance"></param>
				void UpdateDistance(float dDistance);

				/// <summary>
				/// Updates the view matrix.
				/// </summary>
				void UpdateViewMatrix();

				/// <summary>
				/// Updates the projection matrix.
				/// </summary>
				void UpdateProjectionMatrix();

				bool m_isViewDirty = true;
				bool m_isProjectionDirty = true;
				
				bool m_isSoundSource = false;
				

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
