#pragma once


#include "pch.h"
#include <string>
//#include "thirdparty/tiny_obj_loader.h"
#include "Scene.h"


namespace SonarPropagation
{
	namespace Graphics
	{
		namespace Utils
		{
			class ObjectLibrary {
			public: 
				ObjectLibrary(ID3D12Device* device) : m_device(device) {};
				~ObjectLibrary() {};

				
				size_t LoadWavefront(const std::string& filename);
				
				template<typename V>
				size_t LoadPredefined(const std::vector<V> vertices, const std::vector<UINT> indices)
				{
					UINT bufferSize = vertices.size() * sizeof(V);

					size_t modelIndex = m_objects.size();
					m_objects.push_back(Scene::Model());

					{
						DX::ThrowIfFailed(
							m_device->CreateCommittedResource(
								&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD),
								D3D12_HEAP_FLAG_NONE,
								&CD3DX12_RESOURCE_DESC::Buffer(bufferSize),
								D3D12_RESOURCE_STATE_GENERIC_READ,
								nullptr,
								IID_PPV_ARGS(&(m_objects[modelIndex].m_bufferData.vertexBuffer))
							)
						);

						UINT8* pVertexDataBegin;
						CD3DX12_RANGE readRange(0, 0);

						DX::ThrowIfFailed(m_objects[modelIndex].m_bufferData.vertexBuffer->Map(0, &readRange, reinterpret_cast<void**>(&pVertexDataBegin)));
						memcpy(pVertexDataBegin, vertices.data(), bufferSize);
						m_objects[modelIndex].m_bufferData.vertexBuffer->Unmap(0, nullptr);

						m_objects[modelIndex].m_bufferData.vertexBufferView.BufferLocation = m_objects[modelIndex].m_bufferData.vertexBuffer->GetGPUVirtualAddress();
						m_objects[modelIndex].m_bufferData.vertexBufferView.StrideInBytes = sizeof(V);
						m_objects[modelIndex].m_bufferData.vertexBufferView.SizeInBytes = bufferSize;

						//NAME_D3D12_OBJECT(model->m_bufferData.vertexBuffer);
					}

					{
						CD3DX12_HEAP_PROPERTIES heapProperties(D3D12_HEAP_TYPE_UPLOAD);
						CD3DX12_RANGE readRangeUp(0, 0);
						const UINT indexBufferSize = static_cast<UINT>(indices.size()) * sizeof(UINT);

						CD3DX12_RESOURCE_DESC bufferResourceDesc = CD3DX12_RESOURCE_DESC::Buffer(indexBufferSize);
						DX::ThrowIfFailed(m_device->CreateCommittedResource(
							&heapProperties,
							D3D12_HEAP_FLAG_NONE,
							&bufferResourceDesc,
							D3D12_RESOURCE_STATE_GENERIC_READ,
							nullptr,
							IID_PPV_ARGS(&(m_objects[modelIndex].m_bufferData.indexBuffer))
						));

						UINT8* pIndexDataBegin;
						DX::ThrowIfFailed(m_objects[modelIndex].m_bufferData.indexBuffer->Map(0, &readRangeUp, reinterpret_cast<void**>(&pIndexDataBegin)));
						memcpy(pIndexDataBegin, indices.data(), indexBufferSize);
						m_objects[modelIndex].m_bufferData.indexBuffer->Unmap(0, nullptr);

						m_objects[modelIndex].m_bufferData.indexBufferView.BufferLocation = m_objects[modelIndex].m_bufferData.indexBuffer->GetGPUVirtualAddress();
						m_objects[modelIndex].m_bufferData.indexBufferView.Format = DXGI_FORMAT_R32_UINT;
						m_objects[modelIndex].m_bufferData.indexBufferView.SizeInBytes = indexBufferSize;

						//NAME_D3D12_OBJECT(model->m_bufferData.indexBuffer);
					}



					return modelIndex;
				}

				template<typename V>
				size_t LoadPredefined(const std::vector<V> vertices)
				{
					UINT bufferSize = vertices.size() * sizeof(V);
					
					size_t modelIndex = m_objects.size();
					m_objects.push_back(Scene::Model());

					{
						DX::ThrowIfFailed(
							m_device->CreateCommittedResource(
								&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD),
								D3D12_HEAP_FLAG_NONE,
								&CD3DX12_RESOURCE_DESC::Buffer(bufferSize),
								D3D12_RESOURCE_STATE_GENERIC_READ,
								nullptr,
								IID_PPV_ARGS(&(m_objects[modelIndex].m_bufferData.vertexBuffer))
							)
						);

						UINT8* pVertexDataBegin;
						CD3DX12_RANGE readRange(0, 0);

						DX::ThrowIfFailed(m_objects[modelIndex].m_bufferData.vertexBuffer->Map(0, &readRange, reinterpret_cast<void**>(&pVertexDataBegin)));
						memcpy(pVertexDataBegin, vertices.data(), bufferSize);
						m_objects[modelIndex].m_bufferData.vertexBuffer->Unmap(0, nullptr);

						m_objects[modelIndex].m_bufferData.vertexBufferView.BufferLocation = bufferData.vertexBuffer->GetGPUVirtualAddress();
						m_objects[modelIndex].m_bufferData.vertexBufferView.StrideInBytes = sizeof(V);
						m_objects[modelIndex].m_bufferData.vertexBufferView.SizeInBytes = bufferSize;

						//NAME_D3D12_OBJECT(model->m_bufferData.vertexBuffer);
					}

					return modelIndex;
				}


				std::vector<Scene::Model> m_objects;

				ID3D12Device* m_device;
			};

		}
	}

}