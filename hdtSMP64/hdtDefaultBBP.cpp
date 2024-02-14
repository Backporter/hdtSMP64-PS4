#include "hdtDefaultBBP.h"

#include "XmlReader.h"

#include "../hdtSSEUtils/NetImmerseUtils.h"


#include <clocale>
#include <algorithm>

namespace hdt
{
	DefaultBBP* DefaultBBP::instance()
	{
		static DefaultBBP s;
		return &s;
	}

	DefaultBBP::PhysicsFile DefaultBBP::scanBBP(ConsoleRE::NiNode* scan)
	{
		for (int i = 0; i < scan->extraDataSize; ++i)
		{
			auto stringData = netimmerse_cast<ConsoleRE::NiStringExtraData*>(scan->extra[i]);
			if (stringData && !strcasecmp(stringData->value, "HDT Skinned Mesh Physics Object") && stringData->value)
			{
				return { stringData->value, defaultNameMap(scan) };
			}
		}

		return scanDefaultBBP(scan);
	}

	DefaultBBP::DefaultBBP()
	{
		loadDefaultBBPs();
	}

	void DefaultBBP::loadDefaultBBPs()
	{
		auto path = "/app0/SKSE/Plugins/hdtSkinnedMeshConfigs/defaultBBPs.xml";

		auto loaded = readAllFile(path);
		if (loaded.empty()) 
		{
			return;
		}

		// Store original locale
		char saved_locale[32];
		strcpy_s(saved_locale, std::setlocale(LC_NUMERIC, nullptr));

		// Set locale to en_US
		std::setlocale(LC_NUMERIC, "en_US");

		XMLReader reader((uint8_t*)loaded.data(), loaded.size());

		reader.nextStartElement();
		if (reader.GetName() != "default-bbps")
		{
			return;
		}

		while (reader.Inspect())
		{
			if (reader.GetInspected() == Xml::Inspected::StartTag)
			{
				if (reader.GetName() == "map")
				{
					try
					{
						auto shape = reader.getAttribute("shape");
						auto file = reader.getAttribute("file");
						bbpFileList.insert(std::make_pair(shape, file));
					}
					catch (...)
					{
						xUtilty::Log::GetSingleton()->Write(xUtilty::Log::logLevel::kNone, "defaultBBP(%d,%d) : invalid map", reader.GetRow(), reader.GetColumn());
					}

					reader.skipCurrentElement();
				}
				else if (reader.GetName() == "remap")
				{
					auto target = reader.getAttribute("target");
					Remap remap = { target, { }, { } };
					while (reader.Inspect())
					{
						if (reader.GetInspected() == Xml::Inspected::StartTag)
						{
							if (reader.GetName() == "source")
							{
								int priority = 0;
								try
								{
									priority = reader.getAttributeAsInt("priority");
								}
								catch (...)
								{
								}
								auto source = reader.readText();
								remap.entries.insert({ priority, source });
							}
							else if (reader.GetName() == "requires")
							{
								auto req = reader.readText();
								remap.required.insert(req);
							}
							else
							{
								xUtilty::Log::GetSingleton()->Write(xUtilty::Log::logLevel::kNone, "defaultBBP(%d,%d) : unknown element", reader.GetRow(), reader.GetColumn());
								reader.skipCurrentElement();
							}
						}
						else if (reader.GetInspected() == Xml::Inspected::EndTag)
						{
							break;
						}
					}

					remaps.push_back(remap);
				}
				else
				{
					xUtilty::Log::GetSingleton()->Write(xUtilty::Log::logLevel::kNone, "defaultBBP(%d,%d) : unknown element", reader.GetRow(), reader.GetColumn());
					reader.skipCurrentElement();
				}
			}
			else if (reader.GetInspected() == Xml::Inspected::EndTag)
			{
				break;
			}
		}

		// Restore original locale
		std::setlocale(LC_NUMERIC, saved_locale);
	}

	DefaultBBP::PhysicsFile DefaultBBP::scanDefaultBBP(ConsoleRE::NiNode* armor)
	{
		static std::mutex s_lock;
		std::lock_guard<std::mutex> l(s_lock);

		if (bbpFileList.empty()) return { "", {} };

		auto remappedNames = DefaultBBP::instance()->getNameMap(armor);

		auto it = std::find_if(bbpFileList.begin(), bbpFileList.end(), [&](const std::pair<std::string, std::string>& e) { return remappedNames.find(e.first) != remappedNames.end(); });
		return { it == bbpFileList.end() ? "" : it->second, remappedNames };
	}

	DefaultBBP::NameMap DefaultBBP::getNameMap(ConsoleRE::NiNode* armor)
	{
		auto nameMap = defaultNameMap(armor);

		for (auto remap : remaps)
		{
			bool doRemap = true;
			for (auto req : remap.required)
			{
				if (nameMap.find(req) == nameMap.end())
				{
					doRemap = false;
				}
			}

			if (doRemap)
			{
				auto start = std::find_if(remap.entries.rbegin(), remap.entries.rend(), [&](const RemapEntry& e) { return nameMap.find(e.second) != nameMap.end(); });
				auto end = std::find_if(start, remap.entries.rend(), [&](const RemapEntry& e) { return e.first != start->first; });
				if (start != remap.entries.rend())
				{
					const auto& s = nameMap.insert({ remap.name, { } }).first;
					std::for_each(start, end, [&](const RemapEntry& e)
					{
						auto it = nameMap.find(e.second);
						if (it != nameMap.end())
						{
							std::for_each(it->second.begin(), it->second.end(), [&](const std::string& name) { s->second.insert(name); }); 
						}
					});
				}
			}
		}

		return nameMap;
	}

	DefaultBBP::NameMap DefaultBBP::defaultNameMap(ConsoleRE::NiNode* armor)
	{
		std::unordered_map<std::string, std::unordered_set<std::string> > nameMap;
		// This case never happens to a lurker skeleton, thus we don't need to test.
		auto skinned = findNode(armor, "BSFaceGenNiNodeSkinned");
		if (skinned)
		{
			for (int i = 0; i < skinned->children._capacity; ++i)
			{
				if (!skinned->children._data[i])
				{
					continue;
				}

				auto tri = skinned->children._data[i]->AsTriShape();
				if (!tri || !tri->name) 
				{
					continue;
				}

				nameMap.insert({ tri->name, {tri->name } });
			}
		}

		for (int i = 0; i < armor->children._capacity; ++i)
		{
			if (!armor->children._data[i]) 
			{
				continue;
			}

			auto tri = armor->children._data[i]->AsTriShape();
			if (!tri || !tri->name) 
			{
				continue;
			}

			nameMap.insert({ tri->name, { tri->name } });
		}
		return nameMap;
	}
}
