#pragma once

#include "BufferData.h"
#include "ObjectType.h"
#include "..\DXR\AccelerationStructures.h"
#include "DirectXHelper.h"
using namespace DX;

namespace SonarPropagation {
	namespace Graphics {
		namespace Utils {
			struct Scene {
				struct Transform {
					Transform() = default;
					Transform(const XMFLOAT3& position, const XMFLOAT4& rotation, const XMFLOAT4& scale);
					//Transform(const Transform&);
					~Transform();

					bool isChanged = true;
					XMMATRIX LocalToWorld() const;
					XMMATRIX LocalToParent() const;
					XMMATRIX WorldToLocal() const;
					XMMATRIX ParentToLocal() const;
					void SetParent(Transform* newParent, Transform* before);
					
					void DEBUGAssertValidPointers() const;

					XMFLOAT3 m_position = { 0., 0., 0. };
					XMFLOAT4 m_rotation = { 0., 0., 0., 1. };
					XMFLOAT4 m_scale = { 1, 1, 1, 0 };

					Transform* m_parent = nullptr;
					Transform* m_lastChild = nullptr;
					Transform* m_prevSibling = nullptr;
					Transform* m_nextSibling = nullptr;
				};

				class Object
				{
				public:
					
					Object(Transform transform);
					~Object();

					Transform m_transform;
				};

				struct Model {
					
					Model();
					Model(BufferData bufferData);
					~Model();

					bool IsASInstanciated() const { 
						return m_asBuffers.pInstanceDesc !=  nullptr 
							&& m_asBuffers.pResult != nullptr 
							&& m_asBuffers.pScratch != nullptr;
						
						; }
					void AddInstance(Object* object);

					BufferData m_bufferData;
					std::vector<Object*> m_objects;
					SonarPropagation::Graphics::DXR::AccelerationStructureBuffers m_asBuffers;

				};

				class SoundRecevier : public Object
				{
				public:
					SoundRecevier(Transform transform);
					~SoundRecevier();



				private:

				};

				class SoundSource : public Object
				{
				public:
					SoundSource(Transform transform, XMMATRIX rayProject);
					~SoundSource();

				private:
					XMMATRIX m_rayProjection;

				};

				class SoundReflector : public Object
				{
				public:
					SoundReflector(
						Transform transform,
						size_t modelIndex,
						ObjectType type
					);
					~SoundReflector();


					ObjectType GetType() const { return m_type; };
					size_t GetModelIndex() const { return m_modelIndex; };
					ComPtr<ID3D12Resource> GetTexture() const { return m_texture; 
					};
					ComPtr<ID3D12Resource> m_texture;

				private:
					size_t m_modelIndex;
					ObjectType m_type; 
					
				};

				Scene(ComPtr<ID3D12Device> device);
				~Scene();

				void AddObject(SoundReflector object) {

					D3D12_RESOURCE_DESC txtDesc = {};
					txtDesc.DepthOrArraySize = 1;
					txtDesc.MipLevels = txtDesc.DepthOrArraySize = 1;
					txtDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
					txtDesc.Width = 1024;
					txtDesc.Height = 1024;
					txtDesc.SampleDesc.Count = 1;
					txtDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
					txtDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;

					CD3DX12_HEAP_PROPERTIES heapProps(D3D12_HEAP_TYPE_DEFAULT);


					DX::ThrowIfFailed(
						m_device->CreateCommittedResource(
							&heapProps,
							D3D12_HEAP_FLAG_NONE,
							&txtDesc,
							D3D12_RESOURCE_STATE_COPY_SOURCE,
							nullptr,
							IID_PPV_ARGS(object.m_texture.ReleaseAndGetAddressOf()))
					);

					m_objects.push_back(object);

				}
				
				void AddSoundSource(SoundSource source) {
					m_SoundSources.push_back(source);
				}

				void AddSoundRecevier(SoundRecevier recevier) {
					m_SoundReceivers.push_back(recevier);
				}


				std::vector<SoundReflector> m_objects;
				std::vector<SoundSource> m_SoundSources;
				std::vector<SoundRecevier> m_SoundReceivers;

				private: 
					ComPtr<ID3D12Device> m_device;

			};


		}
	}
}


