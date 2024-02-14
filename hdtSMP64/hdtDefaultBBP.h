#pragma once

#include <string>
#include <unordered_map>
#include <unordered_set>
#include <set>

namespace hdt
{
	class DefaultBBP
	{
	public:
		using RemapEntry = std::pair<int, std::string>;
		using NameSet = std::unordered_set<std::string>;
		using NameMap = std::unordered_map<std::string, NameSet >;
		using PhysicsFile = std::pair<std::string, NameMap>;

		struct Remap
		{
			std::string name;
			std::set<RemapEntry> entries;
			std::unordered_set<std::string> required;
		};

		static DefaultBBP* instance();
		PhysicsFile scanBBP(ConsoleRE::NiNode* scan);

	private:
		DefaultBBP();

		std::unordered_map<std::string, std::string> bbpFileList;
		std::vector<Remap> remaps;

		void		loadDefaultBBPs();
		PhysicsFile scanDefaultBBP(ConsoleRE::NiNode* scan);
		NameMap		getNameMap(ConsoleRE::NiNode* armor);
		NameMap		defaultNameMap(ConsoleRE::NiNode* armor);
	};
}
