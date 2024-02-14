#pragma once

//
#include "Ref.h"

namespace hdt
{
	inline void setBSFixedString(ConsoleRE::BSFixedString& ref, const char* value)
	{
		ref = value;
	}

	inline void setNiNodeName(ConsoleRE::NiNode* node, const char* name)
	{
		setBSFixedString(node->name, name);
	}

	inline ConsoleRE::NiNode* castNiNode(ConsoleRE::NiAVObject* obj) { return obj ? obj->AsNode() : nullptr; }
	inline ConsoleRE::BSTriShape* castBSTriShape(ConsoleRE::NiAVObject* obj) { return obj ? obj->AsTriShape() : nullptr; }
	inline ConsoleRE::BSDynamicTriShape* castBSDynamicTriShape(ConsoleRE::NiAVObject* obj) { return obj ? obj->AsDynamicTriShape() : nullptr; }

	ConsoleRE::NiNode* addParentToNode(ConsoleRE::NiNode* node, const char* name);

	ConsoleRE::NiAVObject* findObject(ConsoleRE::NiAVObject* obj, const ConsoleRE::BSFixedString& name);
	ConsoleRE::NiNode* findNode(ConsoleRE::NiNode* obj, const ConsoleRE::BSFixedString& name);

	inline float length(const ConsoleRE::NiPoint3& a) { return sqrt(a.x*a.x + a.y*a.y + a.z*a.z); }
	inline float distance(const ConsoleRE::NiPoint3& a, const ConsoleRE::NiPoint3& b) { return length(a - b); }

	namespace ref
	{
		inline void retain(ConsoleRE::NiRefObject* object) { object->IncRefCount(); }
		inline void release(ConsoleRE::NiRefObject* object) { object->DecRefCount(); }
	}

	std::string readAllFile(const char* path);
	std::string readAllFile2(const char* path);

	void updateTransformUpDown(ConsoleRE::NiAVObject* obj, bool dirty);
}

