#include "pch.h"
#include "Model.h"

namespace SonarPropagation {
	namespace Graphics {
		namespace Utils {
			struct Scene {
				struct Transform {
					Transform() = default;
					Transform(XMFLOAT3 position, XMFLOAT4 rotation, XMFLOAT4 scale) : position(position), rotation(rotation), scale(scale) {}
					Transform(const Transform&);

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
					
					Object(Transform transform) : m_transform(transform) {};
					~Object();

					virtual void ProcessObject() = 0;

					Transform m_transform;
				};

				struct Model {
					BufferData bufferData;
					std::vector<Object*> m_objects;

					void AddInstance(Object* object) {
						m_objects.push_back(object);
					}



					Model(BufferData bufferData) : bufferData(bufferData) {}
				};

				class SoundRecevier : public Object
				{
				public:
					SoundRecevier();
					~SoundRecevier();


					void ProcessObject() override {
						m_sonarCollection->AddSoundReceiver(m_transform.LocalToWorld());
					};

				private:
					SonarCollection* m_sonarCollection;

				};

				class SoundSource : public Object
				{
				public:
					SoundSource(Transform transform,SonarCollection* sonarCollection, XMMATRIX rayProject) 
						: Object(transform), m_sonarCollection(sonarCollection) {};
					~SoundSource();

					void ProcessObject() override {
						m_sonarCollection->AddSoundSource(m_transform.LocalToWorld(), m_rayProjection);
					};
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
					) : Object(transform), m_model(model), m_type(type) {};
					~SoundReflector();

					void ProcessObject() override {
						m_model->AddInstance(this);
					};

				private:
					Model* m_model;
					ObjectType m_type; 
				};

				void AddObject(Object* object) {
					m_objects.push_back(object);
				}
				
				void GetInstanceInformation() const;

				std::vector<Object*> m_objects;

			};


		}
	}
}


