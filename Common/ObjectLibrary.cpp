#include "pch.h"

#include "ObjectLibrary.h"
#include <iostream>

Scene::Model* SonarPropagation::Graphics::Utils::ObjectLibrary::LoadObject(const std::string& filename) {
	tinyobj::ObjReaderConfig readerConfig;
	readerConfig.mtl_search_path = "./Assets"; // Path to material files

	tinyobj::ObjReader reader;

	std::vector<VertexPositionNormalUV> objVertices;
	std::vector<UINT> objIndices;

	if (!reader.ParseFromFile(filename, readerConfig))
	{
		if (!reader.Error().empty())
		{
			std::cerr << "TinyObjReader: " << reader.Error();
		}
		exit(1);
	}

	if (!reader.Warning().empty())
	{
		std::cout << "TinyObjReader: " << reader.Warning();
	}

	auto& attrib = reader.GetAttrib();
	auto& shapes = reader.GetShapes();
	auto& materials = reader.GetMaterials();

	for (size_t s = 0; s < shapes.size(); ++s)
	{
		size_t index_offset = 0;
		for (size_t f = 0; f < shapes[s].mesh.num_face_vertices.size(); ++f)
		{
			size_t fv = size_t(shapes[s].mesh.num_face_vertices[f]);

			for (size_t v = 0; v < fv; ++v)
			{
				VertexPositionNormalUV vertex;
				tinyobj::index_t idx;

				idx = shapes[s].mesh.indices[index_offset + v];
				vertex.pos.x = attrib.vertices[3 * size_t(idx.vertex_index) + 0];
				vertex.pos.y = attrib.vertices[3 * size_t(idx.vertex_index) + 1];
				vertex.pos.z = attrib.vertices[3 * size_t(idx.vertex_index) + 2];

				// Check if `normal_index` is zero or positive. negative = no normal data
				if (idx.normal_index >= 0) {
					vertex.normal.x = attrib.normals[3 * size_t(idx.normal_index) + 0];
					vertex.normal.y = attrib.normals[3 * size_t(idx.normal_index) + 1];
					vertex.normal.z = attrib.normals[3 * size_t(idx.normal_index) + 2];
				}

				// Check if `texcoord_index` is zero or positive. negative = no texcoord data
				if (idx.texcoord_index >= 0) {
					vertex.uv.x = attrib.texcoords[2 * size_t(idx.texcoord_index) + 0];
					vertex.uv.y = attrib.texcoords[2 * size_t(idx.texcoord_index) + 1];
				}

				objVertices.push_back(vertex);
				objIndices.push_back(idx.vertex_index);
			}
			index_offset += fv;

			// per-face material
			shapes[s].mesh.material_ids[f];
		}
	}

	UINT bufferSize = sizeof(objVertices) * sizeof(VertexPositionNormalUV); 

	BufferData bufferData;
	{
		ThrowIfFailed(
			m_device->CreateCommittedResource(
				&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
				D3D12_HEAP_FLAG_NONE,
				&CD3DX12_RESOURCE_DESC::Buffer(bufferSize),
				D3D12_RESOURCE_STATE_GENERIC_READ,
				nullptr,
				IID_PPV_ARGS(&bufferData.vertexBuffer)
			)
		);

		UINT8* pVertexDataBegin;
		CD3DX12_RANGE readRange(0, 0);
		
		ThrowIfFailed(bufferData.vertexBuffer->Map(0, &readRange, reinterpret_cast<void**>(&pVertexDataBegin)));
		memcpy(pVertexDataBegin, objVertices.data(), bufferSize);
		bufferData.vertexBuffer->Unmap(0, nullptr);

		bufferData.vertexBufferView.BufferLocation = bufferData.vertexBuffer->GetGPUVirtualAddress();
		bufferData.vertexBufferView.StrideInBytes = sizeof(VertexPositionNormalUV);
		bufferData.vertexBufferView.SizeInBytes = bufferSize;

		NAME_D3D12_OBJECT(bufferData.vertexBuffer);
	}

	{
		CD3DX12_HEAP_PROPERTIES heapProperties(D3D12_HEAP_TYPE_UPLOAD);
		CD3DX12_RANGE readRangeUp(0, 0);
		const UINT indexBufferSize = static_cast<UINT>(objIndices.size()) * sizeof(UINT);

		CD3DX12_RESOURCE_DESC bufferResourceDesc = CD3DX12_RESOURCE_DESC::Buffer(indexBufferSize);
		ThrowIfFailed(m_device->CreateCommittedResource(
			&heapProperties,
			D3D12_HEAP_FLAG_NONE,
			&bufferResourceDesc,
			D3D12_RESOURCE_STATE_GENERIC_READ,
			nullptr,
			IID_PPV_ARGS(&bufferData.indexBuffer)
		));

		UINT8* pIndexDataBegin;
		ThrowIfFailed(bufferData.indexBuffer->Map(0, &readRangeUp, reinterpret_cast<void**>(&pIndexDataBegin)));
		memcpy(pIndexDataBegin, objIndices.data(), indexBufferSize);
		bufferData.indexBuffer->Unmap(0, nullptr);

		bufferData.indexBufferView.BufferLocation = bufferData.indexBuffer->GetGPUVirtualAddress();
		bufferData.indexBufferView.Format = DXGI_FORMAT_R32_UINT;
		bufferData.indexBufferView.SizeInBytes = indexBufferSize;

		NAME_D3D12_OBJECT(bufferData.indexBuffer);
	}

	auto model = new Scene::Model( bufferData );
	m_objects.push_back(model);

	return model;
}