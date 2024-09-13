#include "pch.h"
#include <tuple>


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