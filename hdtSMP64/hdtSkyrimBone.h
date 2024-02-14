#pragma once

#include "hdtConvertNi.h"
#include "hdtSkinnedMesh/hdtSkinnedMeshBone.h"

namespace hdt
{
	class SkyrimBone : public SkinnedMeshBone
	{
	public:

		SkyrimBone(IDStr name, ConsoleRE::NiNode* node, ConsoleRE::NiNode* skeleton, btRigidBody::btRigidBodyConstructionInfo& ci);

		void resetTransformToOriginal() override;
		void readTransform(float timeStep) override;
		void writeTransform() override;

		int m_depth;
		ConsoleRE::NiNode* m_node;
		ConsoleRE::NiNode* m_skeleton;

	private:
		int m_forceUpdateType;
	};
}
