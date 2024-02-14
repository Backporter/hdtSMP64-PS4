#pragma once

#include "DynamicHDT.h"

namespace hdt 
{
	namespace papyrus 
	{
		bool RegisterAllFunctions(Interface::PapyrusInterface* a_papy_intfc);

		bool ReloadPhysicsFile(ConsoleRE::StaticFunctionTag* base, ConsoleRE::Actor* on_actor, ConsoleRE::TESObjectARMA* on_item, ConsoleRE::BSFixedString physics_file_path, bool persist, bool verbose_log);

		bool SwapPhysicsFile(ConsoleRE::StaticFunctionTag* base, ConsoleRE::Actor* on_actor, ConsoleRE::BSFixedString old_physics_file_path, ConsoleRE::BSFixedString new_physics_file_path, bool persist, bool verbose_log);

		ConsoleRE::BSFixedString QueryCurrentPhysicsFile(ConsoleRE::StaticFunctionTag* base, ConsoleRE::Actor* on_actor, ConsoleRE::TESObjectARMA* on_item, bool verbose_log);

		namespace impl 
		{
			bool ReloadPhysicsFileImpl(uint32_t on_actor_formID, uint32_t on_item_formID, std::string physics_file_path, bool persist, bool verbose_log);

			bool SwapPhysicsFileImpl(uint32_t on_actor_formID, std::string old_physics_file_path, std::string new_physics_file_path, bool persist, bool verbose_log);

			std::string QueryCurrentPhysicsFileImpl(uint32_t on_actor_formID, uint32_t on_item_formID, bool verbose_log);
		}


		//uint32_t FindOrCreateAnonymousSystem(StaticFunctionTag* base, TESObjectARMA* system_model, bool verbose_log);

		//uint32_t AttachAnonymousSystem(StaticFunctionTag* base, Actor* on_actor, uint32_t system_handle, bool verbose_log);

		//uint32_t DetachAnonymousSystem(StaticFunctionTag* base, Actor* on_actor, uint32_t system_handle, bool verbose_log);
	}
}
