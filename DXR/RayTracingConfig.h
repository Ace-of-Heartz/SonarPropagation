#pragma once
#include "pch.h"

struct RayTracingConfig {
	UINT m_recursionDepth = 1;
	UINT m_maxPayloadSize;
	UINT m_maxAttributeSize;

	RayTracingConfig(UINT recursionDepth, UINT maxPayloadSize, UINT maxAttributeSize ) :
		m_recursionDepth(recursionDepth),
		m_maxPayloadSize(maxPayloadSize),
		m_maxAttributeSize(maxAttributeSize)
	{
	}
};