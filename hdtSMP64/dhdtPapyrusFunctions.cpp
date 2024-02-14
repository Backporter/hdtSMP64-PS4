#include "dhdtPapyrusFunctions.h"

#define PAPY_FCN(a) (#a),(PAPYRUS_CLASS_NAME),a

bool RegisterFuncs(ConsoleRE::BSScript::IVirtualMachine* registry)
{
	using namespace hdt::papyrus;

	registry->BindNativeMethod(new ConsoleRE::BSScript::NativeFunction<false, decltype(ReloadPhysicsFile), ConsoleRE::StaticFunctionTag, bool, ConsoleRE::Actor*, ConsoleRE::TESObjectARMA*, ConsoleRE::BSFixedString, bool, bool>(PAPY_FCN(ReloadPhysicsFile)));
	registry->BindNativeMethod(new ConsoleRE::BSScript::NativeFunction<false, decltype(SwapPhysicsFile), ConsoleRE::StaticFunctionTag, bool, ConsoleRE::Actor*, ConsoleRE::BSFixedString, ConsoleRE::BSFixedString, bool, bool>(PAPY_FCN(SwapPhysicsFile)));
	registry->BindNativeMethod(new ConsoleRE::BSScript::NativeFunction<false, decltype(QueryCurrentPhysicsFile), ConsoleRE::StaticFunctionTag, ConsoleRE::BSFixedString, ConsoleRE::Actor*, ConsoleRE::TESObjectARMA*, bool>(PAPY_FCN(QueryCurrentPhysicsFile)));

	return true;
}

bool hdt::papyrus::RegisterAllFunctions(Interface::PapyrusInterface* a_papy_intfc)
{
	return a_papy_intfc->Register(RegisterFuncs);
}

//Some private/protected members are changed to public so that these functions can access them externally.
bool hdt::papyrus::ReloadPhysicsFile(ConsoleRE::StaticFunctionTag* base, ConsoleRE::Actor* on_actor, ConsoleRE::TESObjectARMA* on_item, ConsoleRE::BSFixedString physics_file_path, bool persist, bool verbose_log)
{
	if (!(on_actor && on_item)) 
	{
		if (verbose_log)
			ConsoleRE::ConsoleLog::GetSingleton()->Print("[DynamicHDT] -- Couldn't parse parameters: on_actor(ptr: %016X), on_item(ptr: %016X).", reinterpret_cast<uint64_t>(on_actor), reinterpret_cast<uint64_t>(on_item));

		return false;
	}
	
	return impl::ReloadPhysicsFileImpl(on_actor->formID, on_item->formID, physics_file_path.c_str(), persist, verbose_log);
}

bool hdt::papyrus::SwapPhysicsFile(ConsoleRE::StaticFunctionTag* base, ConsoleRE::Actor* on_actor, ConsoleRE::BSFixedString old_physics_file_path, ConsoleRE::BSFixedString new_physics_file_path, bool persist, bool verbose_log)
{
	if (!on_actor) 
	{
		if (verbose_log) 
			ConsoleRE::ConsoleLog::GetSingleton()->Print("[DynamicHDT] -- Couldn't parse parameters: on_actor(ptr: %016X).", reinterpret_cast<uint64_t>(on_actor));
		
		return false;
	}

	return impl::SwapPhysicsFileImpl(on_actor->formID, old_physics_file_path.c_str(), new_physics_file_path.c_str(), persist, verbose_log);
}

ConsoleRE::BSFixedString hdt::papyrus::QueryCurrentPhysicsFile(ConsoleRE::StaticFunctionTag* base, ConsoleRE::Actor* on_actor, ConsoleRE::TESObjectARMA* on_item, bool verbose_log)
{
	if (!(on_actor && on_item)) 
	{
		if (verbose_log)
			ConsoleRE::ConsoleLog::GetSingleton()->Print("[DynamicHDT] -- Couldn't parse parameters: on_actor(ptr: %016X), on_item(ptr: %016X).", reinterpret_cast<uint64_t>(on_actor), reinterpret_cast<uint64_t>(on_item));
		
		return "";
	}

	return impl::QueryCurrentPhysicsFileImpl(on_actor->formID, on_item->formID, verbose_log).c_str();
}
//
//uint32_t hdt::papyrus::FindOrCreateAnonymousSystem(StaticFunctionTag* base, TESObjectARMA* system_model, bool verbose_log)
//{
//	
//	return uint32_t();
//}
//
//uint32_t hdt::papyrus::AttachAnonymousSystem(StaticFunctionTag* base, Actor* on_actor, uint32_t system_handle, bool verbose_log)
//{
//	if (!on_actor || !system_handle) {
//		if (verbose_log)
//			ConsoleRE::ConsoleLog::GetSingleton()->Print("[DynamicHDT] -- Couldn't parse parameters: on_actor(ptr: %016X), system_handle(%08X).", reinterpret_cast<uint64_t>(on_actor), system_handle);
//		return false;
//	}
//
//
//
//	return uint32_t();
//}
//
//uint32_t hdt::papyrus::DetachAnonymousSystem(StaticFunctionTag* base, Actor* on_actor, uint32_t system_handle, bool verbose_log)
//{
//	if (!on_actor || !system_handle) {
//		if (verbose_log)
//			ConsoleRE::ConsoleLog::GetSingleton()->Print("[DynamicHDT] -- Couldn't parse parameters: on_actor(ptr: %016X), system_handle(%08X).", reinterpret_cast<uint64_t>(on_actor), system_handle);
//		return false;
//	}
//
//	return uint32_t();
//}

bool hdt::papyrus::impl::ReloadPhysicsFileImpl(uint32_t on_actor_formID, uint32_t on_item_formID, std::string physics_file_path, bool persist, bool verbose_log)
{
	const auto& AM = hdt::ActorManager::instance();

	auto& skeletons = AM->getSkeletons();

	bool character_found = false, armor_addon_found = false, succeeded = false;

	std::string old_physics_file_path;

	for (auto& skeleton : skeletons) 
	{
		if (succeeded) 
		{
			break;
		}

		if (!skeleton.skeleton)continue;

		auto owner = skeleton.skeleton->userData;

		if (owner && owner->formID == on_actor_formID) 
		{
			character_found = true;

			auto& armors = skeleton.getArmors();

			for (auto& armor : armors) 
			{
				if (succeeded) 
				{
					break;
				}

				if (!armor.armorWorn)
				{
					continue;
				}

				std::string armorName(armor.armorWorn->name.c_str());

				char buffer[16];
				sprintf_s(buffer, "%08X", on_item_formID);

				if (armorName.find(buffer) != std::string::npos) 
				{
					armor_addon_found = true;
					//Force replacing and reloading. This could lead to assess violation
					try 
					{
						if (armor.physicsFile.first == std::string(physics_file_path)) 
						{
							if (verbose_log)ConsoleRE::ConsoleLog::GetSingleton()->Print("[DynamicHDT] -- Physics file paths are identical, skipping replacing.");
							succeeded = true;
							continue;
						}

						old_physics_file_path = armor.physicsFile.first;
						armor.physicsFile.first = std::string(physics_file_path);
					}
					catch (std::exception& e) 
					{

						ConsoleRE::ConsoleLog::GetSingleton()->Print("[DynamicHDT] ERROR! -- Replacing physics file for ArmorAddon (%08X) on Character (%08X) failed.", on_item_formID, on_actor_formID);

						ConsoleRE::ConsoleLog::GetSingleton()->Print("[DynamicHDT] Error(): %s\nWhat():\n\t%s", typeid(e).name(), e.what());

						return false;
					}

					std::unordered_map<IDStr, IDStr> renameMap = armor.renameMap;

					hdt::Ref<SkyrimSystem> system;

					SkyrimPhysicsWorld::get()->suspendSimulationUntilFinished([&]() 
					{

						if (armor.hasPhysics())
							system = SkyrimSystemCreator().updateSystem(armor.m_physics, skeleton.npc, armor.armorWorn, armor.physicsFile, std::move(renameMap));
						else
							system = SkyrimSystemCreator().createSystem(skeleton.npc, armor.armorWorn, armor.physicsFile, std::move(renameMap));

						if (!system) 
						{
							if (armor.hasPhysics())
								armor.clearPhysics();
						}
						else 
						{
							system->block_resetting = true;

							if (armor.hasPhysics())
								util::transferCurrentPosesBetweenSystems(armor.m_physics, system);

							armor.setPhysics(system, true);

							system->block_resetting = false;
						}
						}
					);

					if (verbose_log)ConsoleRE::ConsoleLog::GetSingleton()->Print("[DynamicHDT] -- Physics file path switched, now is: \"%s\".", armor.physicsFile.first.c_str());

					succeeded = true;
				}
			}
		}
	}

	//Push into global override data
	if (persist) 
	{
		auto OM = Override::OverrideManager::GetSingleton();
		OM->registerOverride(on_actor_formID, old_physics_file_path, std::string(physics_file_path));
	}

	if (verbose_log)
		ConsoleRE::ConsoleLog::GetSingleton()->Print(
			"[DynamicHDT] -- Character (%08X) %s, ArmorAddon (%08X) %s.",
			on_actor_formID,
			character_found ? "found" : "not found",
			on_item_formID,
			armor_addon_found ? "found" : "not found"
		);

	if (verbose_log && succeeded)
		ConsoleRE::ConsoleLog::GetSingleton()->Print(
			"[DynamicHDT] -- ReloadPhysicsFile() succeeded."
		);
	return succeeded;
}

bool hdt::papyrus::impl::SwapPhysicsFileImpl(uint32_t on_actor_formID, std::string old_physics_file_path, std::string new_physics_file_path, bool persist, bool verbose_log)
{
	const auto& AM = hdt::ActorManager::instance();

	auto& skeletons = AM->getSkeletons();

	bool character_found = false, armor_addon_found = false, succeeded = false;

	for (auto& skeleton : skeletons) {
		if (succeeded) 
		{
			break;
		}

		if (!skeleton.skeleton)
		{
			continue;
		}

		auto owner = skeleton.skeleton->userData;

		if (owner && owner->formID == on_actor_formID) {
			character_found = true;

			auto& armors = skeleton.getArmors();

			for (auto& armor : armors) 
			{
				if (succeeded) 
				{
					break;
				}

				if (armor.physicsFile.first == old_physics_file_path.c_str()) 
				{
					armor_addon_found = true;

					//Force replacing and reloading. This could lead to assess violation
					try {
						if (armor.physicsFile.first == std::string(new_physics_file_path)) 
						{
							if (verbose_log)ConsoleRE::ConsoleLog::GetSingleton()->Print("[DynamicHDT] -- Physics file paths are identical, skipping replacing.");
							succeeded = true;
							continue;
						}
						armor.physicsFile.first = std::string(new_physics_file_path);
					}
					catch (std::exception& e) 
					{

						std::string armorName(armor.armorWorn->name.c_str());

						uint32_t form_ID = util::splitArmorAddonFormID(armorName);

						ConsoleRE::ConsoleLog::GetSingleton()->Print("[DynamicHDT] ERROR! -- Replacing physics file for ArmorAddon (%08X) on Character (%08X) failed.", form_ID, on_actor_formID);

						ConsoleRE::ConsoleLog::GetSingleton()->Print("[DynamicHDT] Error(): %s\nWhat():\n\t%s", typeid(e).name(), e.what());

						return false;
					}

					std::unordered_map<IDStr, IDStr> renameMap = armor.renameMap;

					hdt::Ref<SkyrimSystem> system;

					SkyrimPhysicsWorld::get()->suspendSimulationUntilFinished([&]() {

						if (armor.hasPhysics())
							system = SkyrimSystemCreator().updateSystem(armor.m_physics, skeleton.npc, armor.armorWorn, armor.physicsFile, std::move(renameMap));
						else
							system = SkyrimSystemCreator().createSystem(skeleton.npc, armor.armorWorn, armor.physicsFile, std::move(renameMap));

						if (!system) {
							if (armor.hasPhysics())
								armor.clearPhysics();
						}
						else {
							system->block_resetting = true;

							if (armor.hasPhysics())
								util::transferCurrentPosesBetweenSystems(armor.m_physics, system);

							armor.setPhysics(system, true);

							system->block_resetting = false;
						}
						}
					);


					if (verbose_log)ConsoleRE::ConsoleLog::GetSingleton()->Print("[DynamicHDT] -- Physics file path switched, now is: \"%s\".", armor.physicsFile.first.c_str());

					succeeded = true;
				}
			}
		}
	}

	if (persist) {
		auto OM = Override::OverrideManager::GetSingleton();
		OM->registerOverride(on_actor_formID, old_physics_file_path.c_str(), new_physics_file_path.c_str());
	}

	if (verbose_log)
		ConsoleRE::ConsoleLog::GetSingleton()->Print(
			"[DynamicHDT] -- Character (%08X) %s, Physics file path %s.",
			on_actor_formID,
			character_found ? "found" : "not found",
			armor_addon_found ? "found" : "not found"
		);

	if (verbose_log && succeeded)
		ConsoleRE::ConsoleLog::GetSingleton()->Print(
			"[DynamicHDT] -- SwapPhysicsFile() succeeded."
		);
	return succeeded;
}

std::string hdt::papyrus::impl::QueryCurrentPhysicsFileImpl(uint32_t on_actor_formID, uint32_t on_item_formID, bool verbose_log)
{
	const auto& AM = hdt::ActorManager::instance();

	auto& skeletons = AM->getSkeletons();

	bool character_found = false, armor_addon_found = false, succeeded = false;

	std::string physics_file_path;

	for (auto& skeleton : skeletons) {
		if (succeeded)break;
		if (!skeleton.skeleton)continue;

		auto owner = skeleton.skeleton->userData;

		if (owner && owner->formID == on_actor_formID) {
			character_found = true;

			auto& armors = skeleton.getArmors();

			for (auto& armor : armors) {
				if (succeeded)break;
				if (!armor.armorWorn)continue;

				std::string armorName(armor.armorWorn->name.c_str());

				char buffer[16];
				sprintf_s(buffer, "%08X", on_item_formID);

				if (armorName.find(buffer) != std::string::npos) {
					armor_addon_found = true;
					physics_file_path = armor.physicsFile.first;
					succeeded = true;
				}
			}
		}
	}

	if (verbose_log)
		ConsoleRE::ConsoleLog::GetSingleton()->Print(
			"[DynamicHDT] -- Character (%08X) %s, ArmorAddon (%08X) %s.",
			on_actor_formID,
			character_found ? "found" : "not found",
			on_item_formID,
			armor_addon_found ? "found" : "not found"
		);

	if (verbose_log && succeeded)
		ConsoleRE::ConsoleLog::GetSingleton()->Print(
			"[DynamicHDT] -- QueryCurrentPhysicsFile() querying successful."
		);

	return physics_file_path.c_str();
}
