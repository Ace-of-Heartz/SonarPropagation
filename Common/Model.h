#include "pch.h"
#include "BufferData.h"

struct Model {
	BufferData bufferData;
	std::vector<std::tuple<XMMATRIX, UINT>> transforms;

	void AddInstance(XMMATRIX transform, UINT instanceID) {
		transforms.push_back(std::make_tuple(transform, instanceID));
	}
};

struct SonarCollection {
	std::vector<std::tuple<XMMATRIX,XMMATRIX>> m_soundSources;
	std::vector<XMMATRIX> m_soundReceivers;
	
	void AddSoundSource(XMMATRIX transform,XMMATRIX rayProjection) {
		m_soundSources.push_back(std::make_tuple(transform,rayProjection));
	}

	void AddSoundReceiver(XMMATRIX transform) {
		m_soundReceivers.push_back(transform);
	}

};