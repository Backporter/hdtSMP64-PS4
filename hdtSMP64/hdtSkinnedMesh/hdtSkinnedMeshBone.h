#pragma once

#include "hdtBulletHelper.h"
#include"hdtAABB.h"
#include <memory>

namespace hdt
{
	class SkinnedMeshBody;
	_CRT_ALIGN(16) struct SkinnedMeshBone : public RefObject
	{
		BT_DECLARE_ALIGNED_ALLOCATOR();

		SkinnedMeshBone(const IDStr& name, btRigidBody::btRigidBodyConstructionInfo& ci);
		~SkinnedMeshBone();

		IDStr m_name;
		float m_marginMultipler;
		float m_boudingSphereMultipler = 1.0f;
		float m_gravityFactor = 1.0f;
		float m_windFactor = 1.0f; // wind factor for each skinnedmeshbody; currently not changed, this may have been intended to be m_windEffect e.g., (wind-effect in xml)

		btRigidBody m_rig;
		btTransform m_localToRig;
		btTransform m_rigToLocal;
		btQsTransform m_currentTransform;
		btQsTransform m_origTransform;
		btQsTransform m_origToSkeletonTransform;

		std::vector<IDStr> m_canCollideWithBone;
		std::vector<IDStr> m_noCollideWithBone;

		virtual void resetTransformToOriginal() = 0;
		virtual void readTransform(float timeStep) = 0;
		virtual void writeTransform() = 0;

		void internalUpdate();

		bool canCollideWith(SkinnedMeshBone* rhs);
	};
}
