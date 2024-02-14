#pragma once

#include "hdtBulletHelper.h"

namespace hdt
{
	_CRT_ALIGN(16) struct Bone
	{
		Bone() { _mm_store_ps(m_reserved, _mm_setzero_ps()); }

		// cache from rigidbody
		btMatrix4x3T m_vertexToWorld;

		float m_reserved[3]; // reserved for float4 aligned
		float m_maginMultipler; // scaled margin
	};
}
