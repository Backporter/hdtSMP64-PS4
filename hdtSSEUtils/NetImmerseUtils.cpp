#include "NetImmerseUtils.h"

#include <fstream>

namespace hdt
{
	ConsoleRE::NiNode* addParentToNode(ConsoleRE::NiNode * node, const char * name)
	{
		auto parent = node->parent;
		auto newParent = ConsoleRE::NiNode::Create(1);
		
		node->IncRefCount();

		if (parent)
		{
			parent->DetachChild2(node);
			parent->AttachChild(newParent, false);
		}

		newParent->AttachChild(node, false);
		setNiNodeName(newParent, name);
		node->DecRefCount();
		return newParent;
	}

	ConsoleRE::NiAVObject* findObject(ConsoleRE::NiAVObject* obj, const ConsoleRE::BSFixedString& name)
	{
		return obj->GetObjectByName(name);
	}

	ConsoleRE::NiNode* findNode(ConsoleRE::NiNode * obj, const ConsoleRE::BSFixedString & name)
	{
		auto ret = obj->GetObjectByName(name);
		return ret ? ret->AsNode() : nullptr;
	}

	std::string readAllFile(const char* path)
	{
		ConsoleRE::BSResourceNiBinaryStream fin(path);
		if (!fin.good())
		{
			return "";
		}

		size_t readed;
		char buffer[4096];
		std::string ret;
		do 
		{
			readed = fin.read(buffer, sizeof(buffer));
			ret.append(buffer, readed);
		} 
		while (readed == sizeof(buffer));

		return ret;
	}

	std::string readAllFile2(const char* path)
	{
		std::ifstream fin(path, std::ios::binary);
		if (!fin.is_open())
		{
			return "";
		}

		fin.seekg(0, std::ios::end);
		auto size = fin.tellg();
		fin.seekg(0, std::ios::beg);
		std::string ret;
		ret.resize(size);
		fin.read(&ret[0], size);
		return ret;
	}

	void updateTransformUpDown(ConsoleRE::NiAVObject * obj, bool dirty)
	{
		if (!obj)
		{
			return;
		}

		ConsoleRE::NiUpdateData ctx =
		{ 
			0.f,
			dirty ? static_cast<uint32_t>(ConsoleRE::NiAVObject::Flag::kHidden) : static_cast<uint32_t>(ConsoleRE::NiAVObject::Flag::kNone)
		};
		
		obj->UpdateWorldData(&ctx);

		auto node = castNiNode(obj);

		if (node)
		{
			for (int i = 0; i < node->children.capacity(); ++i)
			{
				auto child = node->children._data[i].get();
				if (child)
				{
					updateTransformUpDown(child, dirty);
				}
			}
		}
	}
}
