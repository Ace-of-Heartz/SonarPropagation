#include "pch.h"

namespace SonarPropagation {
	namespace Graphics {
		namespace Utils {
			struct Scene {
				struct Transform {
					Transform() = default;
					Transform(const Transform&) = delete;

					bool isChanged = true;
					XMMATRIX LocalToWorld() const;
					XMMATRIX LocalToParent() const;
					XMMATRIX WorldToLocal() const;
					XMMATRIX ParentToLocal() const;
					void SetParent(Transform* newParent, Transform* before = nullptr);
					
					void DEBUGAssertValidPointers() const;

					XMFLOAT3 position = { 0., 0., 0. };
					XMFLOAT4 rotation = { 0., 0., 0., 1. };
					XMFLOAT4 scale = { 1, 1, 1, 0 };

					Transform* parent = nullptr;
					Transform* lastChild = nullptr;
					Transform* prevSibling = nullptr;
					Transform* nextSibling = nullptr;
				};

				class Object
				{
				public:
					Object();
					~Object();

					Transform m_transform;
				};

				class SoundRecevier : public Object
				{
				public:
					SoundRecevier();
					~SoundRecevier();
				private:
				};

				class SoundSource : public Object
				{
				public:
					SoundSource();
					~SoundSource();
				private:
					XMMATRIX m_rayProjection;

				};

				struct Model {
					std::vector<VertexPositionNormalUV> vertices;
					std::vector<tinyobj::index_t> indices;
				};

				class SoundReflector : public Object
				{
				public:
					SoundReflector();
					~SoundReflector();
				private:
					Model* m_model;
				};

				std::vector<BufferData> GetBufferData() const;

				std::vector<Object*> m_objects;

			};

			struct BufferData {
				Microsoft::WRL::ComPtr<ID3D12Resource> vertexBuffer;
				Microsoft::WRL::ComPtr<ID3D12Resource> indexBuffer;

				UINT vertexCount;
				UINT indexCount;

				D3D12_VERTEX_BUFFER_VIEW vertexBufferView;
				D3D12_INDEX_BUFFER_VIEW indexBufferView;
			};
		}
	}
}


