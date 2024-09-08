#include "pch.h"
#include "Model.h"

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

					virtual void ProcessObject() = 0;

					Transform m_transform;
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
					SoundSource();
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
					SoundReflector();
					~SoundReflector();

					void ProcessObject() override {
						m_model->AddInstance(m_transform.LocalToWorld(),m_hitGroup);
					};

				private:
					Model* m_model;
					UINT m_hitGroup;
				};

				void GetInstanceInformation() const;

				std::vector<Object*> m_objects;

			};
		}
	}
}


