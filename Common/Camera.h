#pragma once
#include <array>

using namespace DirectX;
using namespace Microsoft::WRL;



namespace SonarPropagation {
	namespace Graphics {
		namespace Utils {

			class Camera {
			public:
				Camera();
				~Camera();

				inline void SetPosition(float x, float y, float z);
				inline void SetLookAt(float x, float y, float z);

				void CreateCameraBuffer(ComPtr<ID3D12Device> device);
				void UpdateCameraBuffer();

				inline ComPtr<ID3D12Resource> GetCameraBuffer() {
					return m_cameraBuffer;
				}
				inline ComPtr<ID3D12DescriptorHeap> GetCameraHeap() {
					return m_cameraHeap;
				}

				inline uint32_t GetCameraBufferSize() {
					return m_cameraBufferSize;
				}

			private:

				XMVECTOR m_eye;
				XMVECTOR m_at;
				XMVECTOR m_up;

				float m_fovAngleY;
				float m_aspectRatio;

				float m_nearZ;
				float m_farZ;

				DirectX::XMFLOAT4X4 m_viewMatrix;

				std::array<XMMATRIX, 4> m_camMatrices;

				ComPtr<ID3D12Resource> m_cameraBuffer;
				ComPtr<ID3D12DescriptorHeap> m_cameraHeap;

				uint32_t m_cameraBufferSize = 0;
			};

		}
	}
}
