#pragma once
#include "hdtBoneScaleConstraint.h"

namespace hdt
{
	class ConeTwistConstraint
		: public BoneScaleConstraint
		  , public btConeTwistConstraint
	{
	public:

		ConeTwistConstraint(SkinnedMeshBone* a, SkinnedMeshBone* b, const btTransform& frameInA,
		                    const btTransform& frameInB);

		void scaleConstraint() override;
	};
}
