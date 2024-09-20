#include "pch.h"
#include "Scene.h"

//--------------------------------------------------------------------------------------
// Scene::Object implementation

SonarPropagation::Graphics::Utils::Scene::Object::Object(Transform transform)
	: m_transform(transform) {}

SonarPropagation::Graphics::Utils::Scene::Object::~Object() {}

//--------------------------------------------------------------------------------------
// Scene::SoundRecevier implementation

SonarPropagation::Graphics::Utils::Scene::SoundRecevier::SoundRecevier(Transform transform, SonarCollection* sonarCollection) 
	: Object(transform), m_sonarCollection(sonarCollection) {}

SonarPropagation::Graphics::Utils::Scene::SoundRecevier::~SoundRecevier() {}

void SonarPropagation::Graphics::Utils::Scene::SoundRecevier::ProcessObject() {
	m_sonarCollection->AddSoundReceiver(m_transform.LocalToWorld());
}


//--------------------------------------------------------------------------------------
// Scene::SoundSource implementation

SonarPropagation::Graphics::Utils::Scene::SoundSource::SoundSource(Transform transform, SonarCollection* sonarCollection, XMMATRIX rayProject)
	: Object(transform), m_sonarCollection(sonarCollection), m_rayProjection(rayProject) {}

SonarPropagation::Graphics::Utils::Scene::SoundSource::~SoundSource() {}

void SonarPropagation::Graphics::Utils::Scene::SoundSource::ProcessObject() {
	m_sonarCollection->AddSoundSource(m_transform.LocalToWorld(), m_rayProjection);
};


//--------------------------------------------------------------------------------------
// Scene::SoundReflector implementation
SonarPropagation::Graphics::Utils::Scene::SoundReflector::SoundReflector(
	Transform transform,
	Model* model,
	ObjectType type
) : Object(transform), m_model(model), m_type(type) {}

SonarPropagation::Graphics::Utils::Scene::SoundReflector::~SoundReflector()
{
}

void SonarPropagation::Graphics::Utils::Scene::SoundReflector::ProcessObject() {
	m_model->AddInstance(this);
}

//--------------------------------------------------------------------------------------
// Scene implementation

SonarPropagation::Graphics::Utils::Scene::Scene()
{
}

SonarPropagation::Graphics::Utils::Scene::~Scene()
{
}

//--------------------------------------------------------------------------------------
// Scene::Model implementation

SonarPropagation::Graphics::Utils::Scene::Model::Model()
	: m_bufferData(BufferData()) {}

SonarPropagation::Graphics::Utils::Scene::Model::Model(BufferData bufferData)
	: m_bufferData(bufferData) {}

SonarPropagation::Graphics::Utils::Scene::Model::~Model() {
	for (auto& object : m_objects) {
		delete object;
	}
	m_objects.clear();
}

void SonarPropagation::Graphics::Utils::Scene::Model::AddInstance(Object* object) {
	m_objects.push_back(object);
}

//--------------------------------------------------------------------------------------
// Scene::Transform implementation

SonarPropagation::Graphics::Utils::Scene::Transform::Transform(
	const XMFLOAT3& position,
	const XMFLOAT4& rotation,
	const XMFLOAT4& scale
) : m_position(position), m_rotation(rotation), m_scale(scale) {
}


SonarPropagation::Graphics::Utils::Scene::Transform::~Transform() {
	while (m_lastChild) {
		m_lastChild->SetParent(nullptr,nullptr);
	} 
	if (m_parent) {
		SetParent(nullptr,nullptr);
	}

}

XMMATRIX SonarPropagation::Graphics::Utils::Scene::Transform::LocalToWorld() const {
	if (m_parent) {
		return m_parent->LocalToWorld() * LocalToParent();
	}
	else {
		return LocalToParent();
	}
}

XMMATRIX SonarPropagation::Graphics::Utils::Scene::Transform::LocalToParent() const {
	return	XMMatrixTranslation(m_position.x, m_position.y, m_position.z) *
			XMMatrixRotationRollPitchYawFromVector(XMLoadFloat4(&m_rotation)) *
			XMMatrixScaling(m_scale.x, m_scale.y, m_scale.z);
}

XMMATRIX SonarPropagation::Graphics::Utils::Scene::Transform::WorldToLocal() const {
	if (m_parent)
	{
		return m_parent->LocalToWorld() * m_parent->WorldToLocal();
	}
	else {
		return ParentToLocal();
	}
}

XMMATRIX SonarPropagation::Graphics::Utils::Scene::Transform::ParentToLocal() const {
	XMFLOAT3 invScale;
	invScale.x = (m_scale.x == 0.0) ? 0.0 : 1.0f / m_scale.x;
	invScale.y = (m_scale.y == 0.0) ? 0.0 : 1.0f / m_scale.y;
	invScale.z = (m_scale.z == 0.0) ? 0.0 : 1.0f / m_scale.z;

	return XMMatrixScaling(invScale.x, invScale.y, invScale.z) *
		XMMatrixRotationRollPitchYawFromVector( -1. * XMLoadFloat4(&m_rotation)) *
		XMMatrixTranslation(-m_position.x, -m_position.y, -m_position.z);

}

void SonarPropagation::Graphics::Utils::Scene::Transform::SetParent(Transform* newParent, Transform* before) {
	if (m_parent) {
		if (m_prevSibling) {
			m_prevSibling->m_nextSibling = m_nextSibling;
		}
		if (m_nextSibling) {
			m_nextSibling->m_prevSibling = m_prevSibling;
		}
		else m_parent->m_lastChild = m_prevSibling;
		m_nextSibling = m_prevSibling = nullptr;
	}
	m_parent = newParent;
	if (m_parent)
	{
		if (before) {
			m_prevSibling = before->m_prevSibling;
			m_nextSibling = before;
			if (m_prevSibling) {
				m_prevSibling->m_nextSibling = this;
			}
			else {
				m_parent->m_lastChild = this;
			}
			before->m_prevSibling = this;
		}
		else {
			m_prevSibling = m_parent->m_lastChild;
			if (m_prevSibling) {
				m_prevSibling->m_nextSibling = this;
			}
			else {
				m_parent->m_lastChild = this;
			}
			if (m_prevSibling) {
				m_prevSibling->m_nextSibling = this;
			}
		}

	}
}

void SonarPropagation::Graphics::Utils::Scene::Transform::DEBUGAssertValidPointers() const {
	if (m_parent == nullptr) {
		assert(m_prevSibling == nullptr);
		assert(m_nextSibling == nullptr);
	}
	else {
		assert((m_nextSibling == nullptr) == (this == m_parent->m_lastChild));
	}
	assert(m_prevSibling == nullptr || m_prevSibling->m_nextSibling == this);
	assert(m_nextSibling == nullptr || m_nextSibling->m_prevSibling == this);
	assert(m_lastChild == nullptr || m_lastChild->m_parent == this);
}

void SonarPropagation::Graphics::Utils::Scene::GetInstanceInformation() const {
	CD3DX12_HEAP_PROPERTIES heapProperties(D3D12_HEAP_TYPE_UPLOAD);
	
	for (auto& object : m_objects) {
		if (object )

		if ( object->m_transform.isChanged) {
			object->m_transform.isChanged = false;
			
			object->ProcessObject();
		}
	}
}