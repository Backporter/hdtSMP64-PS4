#pragma once

#include "hdtSkinnedMesh/hdtBulletHelper.h"
#include "../hdtSSEUtils/NetImmerseUtils.h"

namespace hdt
{
	btQuaternion convertNi(const ConsoleRE::NiMatrix3& rhs);

	inline btVector3 convertNi(const ConsoleRE::NiPoint3& rhs)
	{
		return btVector3(rhs.x, rhs.y, rhs.z);
	}

	inline btQsTransform convertNi(const ConsoleRE::NiTransform& rhs)
	{
		btQsTransform ret;
		ret.setBasis(convertNi(rhs.rotate));
		ret.setOrigin(convertNi(rhs.translate));
		ret.setScale(rhs.scale);
		return ret;
	}

	ConsoleRE::NiPoint3 convertBt(const btVector3& rhs);
	ConsoleRE::NiMatrix3 convertBt(const btMatrix3x3& rhs);
	ConsoleRE::NiMatrix3 convertBt(const btQuaternion& rhs);
	ConsoleRE::NiTransform convertBt(const btQsTransform& rhs);

	static const float scaleRealWorld = 0.01425;
	static const float scaleSkyrim = 1 / scaleRealWorld;
}
