#include "pch.h"
#include "Scene.h"



SonarPropagation::Graphics::Utils::Scene::Object::~Object()
{
}


SonarPropagation::Graphics::Utils::Scene::SoundRecevier::~SoundRecevier()
{
}

SonarPropagation::Graphics::Utils::Scene::SoundSource::~SoundSource()
{
}

SonarPropagation::Graphics::Utils::Scene::SoundReflector::~SoundReflector()
{
}

SonarPropagation::Graphics::Utils::Scene::Scene()
{
}

SonarPropagation::Graphics::Utils::Scene::~Scene()
{
}

SonarPropagation::Graphics::Utils::Scene::Transform::Transform() {

}

SonarPropagation::Graphics::Utils::Scene::Transform::~Transform() {
	while (lastChild) {
		lastChild->SetParent(nullptr);
	} 
	if (parent) {
		SetParent(nullptr);
	}

}

XMMATRIX SonarPropagation::Graphics::Utils::Scene::Transform::LocalToWorld() const {
	if (parent) {
		return parent->LocalToWorld() * LocalToParent();
	}
	else {
		return LocalToParent();
	}
}

XMMATRIX SonarPropagation::Graphics::Utils::Scene::Transform::LocalToParent() const {
	return	XMMatrixTranslation(position.x, position.y, position.z) *
			XMMatrixRotationRollPitchYawFromVector(XMLoadFloat4(&rotation)) *
			XMMatrixScaling(scale.x, scale.y, scale.z);
}

XMMATRIX SonarPropagation::Graphics::Utils::Scene::Transform::WorldToLocal() const {
	if (parent)
	{
		return parent->LocalToWorld() * parent->WorldToLocal();
	}
	else {
		return ParentToLocal();
	}
}

XMMATRIX SonarPropagation::Graphics::Utils::Scene::Transform::ParentToLocal() const {
	XMFLOAT3 invScale;
	invScale.x = (scale.x == 0.0) ? 0.0 : 1.0f / scale.x;
	invScale.y = (scale.y == 0.0) ? 0.0 : 1.0f / scale.y;
	invScale.z = (scale.z == 0.0) ? 0.0 : 1.0f / scale.z;

	return XMMatrixScaling(invScale.x, invScale.y, invScale.z) *
		XMMatrixRotationRollPitchYawFromVector( -1. * XMLoadFloat4(&rotation)) *
		XMMatrixTranslation(-position.x, -position.y, -position.z);

}

void SonarPropagation::Graphics::Utils::Scene::Transform::SetParent(Transform* newParent, Transform* before = nullptr) {
	if (parent) {
		if (prevSibling) {
			prevSibling->nextSibling = nextSibling;
		}
		if (nextSibling) {
			nextSibling->prevSibling = prevSibling;
		}
		else parent->lastChild = prevSibling;
		nextSibling = prevSibling = nullptr;
	}
	parent = newParent;
	if (parent)
	{
		if (before) {
			prevSibling = before->prevSibling;
			nextSibling = before;
			if (prevSibling) {
				prevSibling->nextSibling = this;
			}
			else {
				parent->lastChild = this;
			}
			before->prevSibling = this;
		}
		else {
			prevSibling = parent->lastChild;
			if (prevSibling) {
				prevSibling->nextSibling = this;
			}
			else {
				parent->lastChild = this;
			}
			if (prevSibling) {
				prevSibling->nextSibling = this;
			}
		}

	}
}

void SonarPropagation::Graphics::Utils::Scene::Transform::DEBUGAssertValidPointers() const {
	if (parent == nullptr) {
		assert(prevSibling == nullptr);
		assert(nextSibling == nullptr);
	}
	else {
		assert((nextSibling == nullptr) == (this == parent->lastChild));
	}
	assert(prevSibling == nullptr || prevSibling->nextSibling == this);
	assert(nextSibling == nullptr || nextSibling->prevSibling == this);
	assert(lastChild == nullptr || lastChild->parent == this);
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