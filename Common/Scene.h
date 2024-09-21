#pragma once

#include "Model.h"
#include "BufferData.h"
#include "ObjectType.h"

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

					virtual void ProcessObject() = 0;

					Transform m_transform;
				};

				struct Model {
					
					Model();
					Model(BufferData bufferData);
					~Model();

					void AddInstance(Object* object);

					BufferData m_bufferData;
					std::vector<Object*> m_objects;

				};

				class SoundRecevier : public Object
				{
				public:
					SoundRecevier(Transform transform, SonarCollection* sonarCollection);
					~SoundRecevier();


					void ProcessObject() override;

				private:
					SonarCollection* m_sonarCollection;

				};

				class SoundSource : public Object
				{
				public:
					SoundSource(Transform transform, SonarCollection* sonarCollection, XMMATRIX rayProject);
					~SoundSource();

					void ProcessObject() override;
				private:
					SonarCollection* m_sonarCollection;
					XMMATRIX m_rayProjection;

				};

				class SoundReflector : public Object
				{
				public:
					SoundReflector(
						Transform transform,
						Model* model,
						ObjectType type
					);
					~SoundReflector();

					void ProcessObject() override;
					ObjectType GetType() const { return m_type; };

				private:
					Model* m_model;
					ObjectType m_type; 
					
				};

				Scene();
				~Scene();

				void AddObject(SoundReflector* object) {
					m_objects.push_back(object);
					object->m_transform.SetParent(nullptr, nullptr);
				}
				
				void GetInstanceInformation() const;

				std::vector<SoundReflector*> m_objects;

			};


		}
	}
}


