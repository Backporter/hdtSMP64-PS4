#include "../../../OrbisUtil/include/RelocationManager.h"
#include "../../../OrbisUtil/include/Logger.h"
#include "../../../OrbisUtil/include/FileSystem.h"

extern "C" __declspec (dllexport) int module_start(size_t argc, const void* argv) { xUtilty::RelocationManager(); return 0; }
extern "C" __declspec (dllexport) int module_stop(size_t argc, const void* argv) { return 0; }

#include "ActorManager.h"
#include "config.h"
#include "EventDebugLogger.h"
#include "hdtSkyrimPhysicsWorld.h"
#include "Hooks.h"
#include "Offsets.h"
#include "HookEvents.h"
#include "PluginInterfaceImpl.h"

#include <numeric>

#include "WeatherManager.h"

namespace hdt
{
	EventDebugLogger	g_eventDebugLogger;
	size_t				g_PluginHandle;

	class FreezeEventHandler : public ConsoleRE::BSTEventSink<ConsoleRE::MenuOpenCloseEvent>
	{
	public:
		FreezeEventHandler() = default;
		~FreezeEventHandler() override = default;

		static FreezeEventHandler* Get()
		{
			static FreezeEventHandler singleton;
			return std::addressof(singleton);
		}

		ConsoleRE::BSEventNotifyControl ProcessEvent(const ConsoleRE::MenuOpenCloseEvent* evn, ConsoleRE::BSTEventSource<ConsoleRE::MenuOpenCloseEvent>* dispatcher) override
		{
			auto mm = ConsoleRE::UI::GetSingleton();

			if (evn && evn->opening && (!strcasecmp(evn->menuName._data, "Loading Menu") || !strcasecmp(evn->menuName._data, "RaceSex Menu")))
			{
				xUtilty::Log::GetSingleton()->Write(xUtilty::Log::logLevel::kNone,"loading menu/racesexmenu detected(%s), scheduling physics reset on world un-suspend", evn->menuName._data);
				SkyrimPhysicsWorld::get()->suspend(true);
			}

			if (evn && !evn->opening && !strcasecmp(evn->menuName._data, "RaceSex Menu"))
			{
				xUtilty::Log::GetSingleton()->Write(xUtilty::Log::logLevel::kNone,"racemenu closed, reloading meshes");
				ActorManager::instance()->onEvent(*evn);
			}

			return ConsoleRE::BSEventNotifyControl::kContinue;
		}
	};

	void checkOldPlugins()
	{
	}

	ConsoleRE::NiPointer<ConsoleRE::NiSourceTexture>* GetTextureFromIndex(ConsoleRE::BSLightingShaderMaterial* material, uint32_t index)
	{
		switch (index)
		{
		case 0:
			return &material->diffuseTexture;
			break;
		case 1:
			return &material->normalTexture;
			break;
		case 2:
		{
			if (material->GetFeature() == ConsoleRE::BSShaderMaterial::Feature::kFaceGen)
			{
				return &static_cast<ConsoleRE::BSLightingShaderMaterialFacegen*>(static_cast<ConsoleRE::BSLightingShaderMaterialBase*>(material))->subsurfaceTexture;
			}

			if (material->GetFeature() == ConsoleRE::BSShaderMaterial::Feature::kGlowMap)
			{
				return &static_cast<ConsoleRE::BSLightingShaderMaterialFacegen*>(static_cast<ConsoleRE::BSLightingShaderMaterialBase*>(material))->subsurfaceTexture;
			}

			return &material->rimSoftLightingTexture;
		}
		break;
		case 3:
		{
			if (material->GetFeature() == ConsoleRE::BSShaderMaterial::Feature::kFaceGen)
			{
				return &static_cast<ConsoleRE::BSLightingShaderMaterialFacegen*>(static_cast<ConsoleRE::BSLightingShaderMaterialBase*>(material))->detailTexture;
			}
			if (material->GetFeature() == ConsoleRE::BSShaderMaterial::Feature::kParallax)
			{
				return &static_cast<ConsoleRE::BSLightingShaderMaterialParallax*>(static_cast<ConsoleRE::BSLightingShaderMaterialBase*>(material))->heightTexture;
			}
			if (material->GetFeature() == ConsoleRE::BSShaderMaterial::Feature::kParallax || material->GetFeature() == ConsoleRE::BSShaderMaterial::Feature::kParallaxOcc)
			{
				return &static_cast<ConsoleRE::BSLightingShaderMaterialParallaxOcc*>(static_cast<ConsoleRE::BSLightingShaderMaterialBase*>(material))->heightTexture;
			}
		}
		break;
		case 4:
		{
			if (material->GetFeature() == ConsoleRE::BSShaderMaterial::Feature::kEye)
			{
				return &static_cast<ConsoleRE::BSLightingShaderMaterialEye*>(static_cast<ConsoleRE::BSLightingShaderMaterialBase*>(material))->envTexture;
			}
			if (material->GetFeature() == ConsoleRE::BSShaderMaterial::Feature::kEnvironmentMap)
			{
				return &static_cast<ConsoleRE::BSLightingShaderMaterialEnvmap*>(static_cast<ConsoleRE::BSLightingShaderMaterialBase*>(material))->envTexture;
			}
			if (material->GetFeature() == ConsoleRE::BSShaderMaterial::Feature::kMultilayerParallax)
			{
				return &static_cast<ConsoleRE::BSLightingShaderMaterialMultiLayerParallax*>(static_cast<ConsoleRE::BSLightingShaderMaterialBase*>(material))->envTexture;
			}
		}
		break;
		case 5:
		{
			if (material->GetFeature() == ConsoleRE::BSShaderMaterial::Feature::kEye)
			{
				return &static_cast<ConsoleRE::BSLightingShaderMaterialEye*>(static_cast<ConsoleRE::BSLightingShaderMaterialBase*>(material))->envMaskTexture;
			}
			if (material->GetFeature() == ConsoleRE::BSShaderMaterial::Feature::kEnvironmentMap)
			{
				return &static_cast<ConsoleRE::BSLightingShaderMaterialEnvmap*>(static_cast<ConsoleRE::BSLightingShaderMaterialBase*>(material))->envTexture;
			}
			if (material->GetFeature() == ConsoleRE::BSShaderMaterial::Feature::kMultilayerParallax)
			{
				return &static_cast<ConsoleRE::BSLightingShaderMaterialMultiLayerParallax*>(static_cast<ConsoleRE::BSLightingShaderMaterialBase*>(material))->envMaskTexture;
			}
		}
		break;
		case 6:
		{
			if (material->GetFeature() == ConsoleRE::BSShaderMaterial::Feature::kFaceGen)
			{
				return &static_cast<ConsoleRE::BSLightingShaderMaterialFacegen*>(static_cast<ConsoleRE::BSLightingShaderMaterialBase*>(material))->tintTexture;
			}
			if (material->GetFeature() == ConsoleRE::BSShaderMaterial::Feature::kMultilayerParallax)
			{
				return &static_cast<ConsoleRE::BSLightingShaderMaterialMultiLayerParallax*>(static_cast<ConsoleRE::BSLightingShaderMaterialBase*>(material))->layerTexture;
			}
		}
		break;
		case 7:
			return &material->specularBackLightingTexture;
			break;
		}

		return nullptr;
	}

	void DumpNodeChildren(ConsoleRE::NiAVObject* node)
	{
		xUtilty::Log::GetSingleton()->Write(xUtilty::Log::logLevel::kNone, "{%s} {%s} {%X} [%f, %f, %f]", node->GetRTTI()->name, node->name.c_str(), node, node->world.translate.x, node->world.translate.y, node->world.translate.z);
		if (node->extraDataSize > 0)
		{
			for (uint16_t i = 0; i < node->extraDataSize; i++)
			{
				xUtilty::Log::GetSingleton()->Write(xUtilty::Log::logLevel::kNone,"{%s} {%s} {%X}", node->extra[i]->GetRTTI()->name, node->extra[i]->name.c_str(), node);
			}
		}

		ConsoleRE::NiNode* niNode = node->AsNode();
		if (niNode && niNode->children._freeidx > 0)
		{
			for (int i = 0; i < niNode->children._freeidx; i++)
			{
				ConsoleRE::NiAVObject* object = niNode->children._data[i].get();
				if (object)
				{
					ConsoleRE::NiNode* childNode = object->AsNode();
					ConsoleRE::BSGeometry* geometry = object->AsGeometry();
					if (geometry)
					{
						xUtilty::Log::GetSingleton()->Write(xUtilty::Log::logLevel::kNone,"{%s} {%s} {%X} [%f, %f, %f] - Geometry", object->GetRTTI()->name, object->name.c_str(), object, geometry->world.translate.x, geometry->world.translate.y, geometry->world.translate.z);
						if (geometry->skinInstance && geometry->skinInstance->skinData)
						{
							for (int i = 0; i < geometry->skinInstance->skinData->bones; i++)
							{
								auto bone = geometry->skinInstance->bones[i];
								xUtilty::Log::GetSingleton()->Write(xUtilty::Log::logLevel::kNone, "Bone %d - {%s} {%s} {%X} [%f, %f, %f]", i, bone->GetRTTI()->name, bone->name.c_str(), bone, bone->world.translate.x, bone->world.translate.y, bone->world.translate.z);
							}
						}

						ConsoleRE::NiPointer<ConsoleRE::BSShaderProperty> shaderProperty = static_cast<ConsoleRE::BSShaderProperty*>(geometry->properties[ConsoleRE::BSGeometry::States::kEffect].get());
						if (shaderProperty)
						{
							ConsoleRE::BSLightingShaderProperty* lightingShader = netimmerse_cast<ConsoleRE::BSLightingShaderProperty*>(shaderProperty.get());
							if (lightingShader)
							{
								ConsoleRE::BSLightingShaderMaterial* material = static_cast<ConsoleRE::BSLightingShaderMaterial*>(lightingShader->material);
								for (int i = 0; i < static_cast<int>(ConsoleRE::BSTextureSet::Texture::kTotal); ++i)
								{
									const char* texturePath = material->textureSet->GetTexturePath(static_cast<ConsoleRE::BSTextureSet::Texture>(i));
									if (!texturePath)
									{
										continue;
									}

									const char* textureName = "";
									ConsoleRE::NiPointer<ConsoleRE::NiSourceTexture>* texture = GetTextureFromIndex(material, i);
									if (texture && texture->get())
									{
										textureName = texture->get()->name;
									}

									xUtilty::Log::GetSingleton()->Write(xUtilty::Log::logLevel::kNone,"Texture %d - %s (%s)", i, texturePath, textureName);
								}
								xUtilty::Log::GetSingleton()->Write(xUtilty::Log::logLevel::kNone,"Flags - %08X %08X", lightingShader->flags, lightingShader->flags);
							}
						}
					}
					else if (childNode)
					{
						DumpNodeChildren(childNode);
					}
					else
					{
						xUtilty::Log::GetSingleton()->Write(xUtilty::Log::logLevel::kNone,"{%s} {%s} {%X} [%f, %f, %f]", object->GetRTTI()->name, object->name.c_str(), object, object->world.translate.x, object->world.translate.y, object->world.translate.z);
					}
				}
			}
		}
	}

	void SMPDebug_PrintDetailed(bool includeItems)
	{
		static std::map<ActorManager::SkeletonState, char*> stateStrings = { { ActorManager::SkeletonState::e_InactiveNotInScene, "Not in scene"}, {ActorManager::SkeletonState::e_InactiveUnseenByPlayer, "Unseen by player"}, {ActorManager::SkeletonState::e_InactiveTooFar, "Deactivated for performance"}, {ActorManager::SkeletonState::e_ActiveIsPlayer, "Is player character"}, {ActorManager::SkeletonState::e_ActiveNearPlayer, "Is near player"} };

		auto skeletons = ActorManager::instance()->getSkeletons();
		std::vector<int>order(skeletons.size());
		std::iota(order.begin(), order.end(), 0);
		std::sort(order.begin(), order.end(), [&](int a, int b) { return skeletons[a].state < skeletons[b].state; });

		for (int i : order)
		{
			auto& skeleton = skeletons[i];

			ConsoleRE::TESObjectREFR* skelOwner = nullptr;
			ConsoleRE::TESFullName* ownerName = nullptr;

			if (skeleton.skeleton->userData)
			{
				skelOwner = skeleton.skeleton->userData;
				if (skelOwner->data.objectReference)
					ownerName = skyrim_cast<ConsoleRE::TESFullName*>(skelOwner->data.objectReference);
			}

			ConsoleRE::ConsoleLog::GetSingleton()->Print("[HDT-SMP] %s skeleton - owner %s (refr formid %08x, base formid %08x) - %s",
				skeleton.state > ActorManager::SkeletonState::e_SkeletonActive ? "active" : "inactive",
				ownerName ? ownerName->GetFullName() : "unk_name",
				skelOwner ? skelOwner->formID : 0x00000000,
				skelOwner && skelOwner->data.objectReference ? skelOwner->data.objectReference->formID : 0x00000000,
				stateStrings[skeleton.state]
			);

			if (includeItems)
			{
				for (auto armor : skeleton.getArmors())
				{
					ConsoleRE::ConsoleLog::GetSingleton()->Print("[HDT-SMP] -- tracked armor addon %s, %s",
						armor.armorWorn->name.c_str(),
						armor.state() != ActorManager::ItemState::e_NoPhysics
						? armor.state() == ActorManager::ItemState::e_Active
						? "has active physics system"
						: "has inactive physics system"
						: "has no physics system");

					if (armor.state() != ActorManager::ItemState::e_NoPhysics)
					{
						for (auto mesh : armor.meshes())
							ConsoleRE::ConsoleLog::GetSingleton()->Print("[HDT-SMP] ---- has collision mesh %s", mesh->m_name->cstr());
					}
				}

				if (skeleton.head.headNode)
				{
					for (auto headPart : skeleton.head.headParts)
					{
						ConsoleRE::ConsoleLog::GetSingleton()->Print("[HDT-SMP] -- tracked headpart %s, %s",
							headPart.headPart->name.c_str(),
							headPart.state() != ActorManager::ItemState::e_NoPhysics
							? headPart.state() == ActorManager::ItemState::e_Active
							? "has active physics system"
							: "has inactive physics system"
							: "has no physics system");

						if (headPart.state() != ActorManager::ItemState::e_NoPhysics)
						{
							for (auto mesh : headPart.meshes())
								ConsoleRE::ConsoleLog::GetSingleton()->Print("[HDT-SMP] ---- has collision mesh %s", mesh->m_name->cstr());
						}
					}
				}
			}

		}
	}

	bool SMPDebug_Execute(const ConsoleRE::SCRIPT_PARAMETER* a_paramInfo, ConsoleRE::SCRIPT_FUNCTION::ScriptData* a_scriptData, ConsoleRE::TESObjectREFR* a_thisObj, ConsoleRE::TESObjectREFR* a_containingObj, ConsoleRE::Script* a_scriptObj, ConsoleRE::ScriptLocals* a_locals, double& a_result, uint32_t& a_opcodeOffsetPtr)
	{
		char buffer[MAX_PATH];
		memset(buffer, 0, MAX_PATH);
		
		char buffer2[MAX_PATH];
		memset(buffer2, 0, MAX_PATH);

		if (!ConsoleRE::SCRIPT_FUNCTION::ParseParameters(a_paramInfo, a_scriptData, a_opcodeOffsetPtr, a_thisObj, a_containingObj, a_scriptObj, a_locals, buffer, buffer2))
		{
			return false;
		}

		if (strncasecmp(buffer, "reset", MAX_PATH) == 0)
		{
			ConsoleRE::ConsoleLog::GetSingleton()->Print("running full smp reset");
			hdt::loadConfig();
			SkyrimPhysicsWorld::get()->resetTransformsToOriginal();
			const ConsoleRE::MenuOpenCloseEvent e { "", false };
			ActorManager::instance()->onEvent(e);
			SkyrimPhysicsWorld::get()->resetSystems();
			return true;
		}
		if (strncasecmp(buffer, "dumptree", MAX_PATH) == 0)
		{
			if (a_thisObj)
			{
				ConsoleRE::ConsoleLog::GetSingleton()->Print("dumping targeted reference's node tree");
				DumpNodeChildren(a_thisObj->Get3D1(0));
			}
			else
			{
				ConsoleRE::ConsoleLog::GetSingleton()->Print("error: you must target a reference to dump their node tree");
			}

			return true;
		}
		if (strncasecmp(buffer, "detail", MAX_PATH) == 0)
		{
			SMPDebug_PrintDetailed(true);
			return true;
		}
		if (strncasecmp(buffer, "list", MAX_PATH) == 0)
		{
			SMPDebug_PrintDetailed(false);
			return true;
		}
		if (strncasecmp(buffer, "on", MAX_PATH) == 0)
		{
			SkyrimPhysicsWorld::get()->disabled = false;
			{
				ConsoleRE::ConsoleLog::GetSingleton()->Print("HDT-SMP enabled");
			}
			return true;
		}
		if (strncasecmp(buffer, "off", MAX_PATH) == 0)
		{
			SkyrimPhysicsWorld::get()->disabled = true;
			{
				ConsoleRE::ConsoleLog::GetSingleton()->Print("HDT-SMP disabled");
			}
			return true;
		}

		if (strncasecmp(buffer, "QueryOverride", MAX_PATH) == 0)
		{
			ConsoleRE::ConsoleLog::GetSingleton()->Print(hdt::Override::OverrideManager::GetSingleton()->queryOverrideData().c_str());
			return true;
		}

		auto skeletons = ActorManager::instance()->getSkeletons();

		size_t activeSkeletons = 0;
		size_t armors = 0;
		size_t headParts = 0;
		size_t activeArmors = 0;
		size_t activeHeadParts = 0;
		size_t activeCollisionMeshes = 0;

		for (auto skeleton : skeletons)
		{
			if (skeleton.state > ActorManager::SkeletonState::e_SkeletonActive)
				activeSkeletons++;

			for (const auto armor : skeleton.getArmors())
			{
				armors++;

				if (armor.state() == ActorManager::ItemState::e_Active)
				{
					activeArmors++;

					activeCollisionMeshes += armor.meshes().size();
				}
			}

			if (skeleton.head.headNode)
			{
				for (const auto headpart : skeleton.head.headParts)
				{
					headParts++;

					if (headpart.state() == ActorManager::ItemState::e_Active)
					{
						activeHeadParts++;

						activeCollisionMeshes += headpart.meshes().size();
					}
				}
			}
		}

		ConsoleRE::ConsoleLog::GetSingleton()->Print("[HDT-SMP] tracked skeletons: %d", skeletons.size());
		ConsoleRE::ConsoleLog::GetSingleton()->Print("[HDT-SMP] active skeletons: %d", activeSkeletons);
		ConsoleRE::ConsoleLog::GetSingleton()->Print("[HDT-SMP] tracked armor addons: %d", armors);
		ConsoleRE::ConsoleLog::GetSingleton()->Print("[HDT-SMP] tracked head parts: %d", headParts);
		ConsoleRE::ConsoleLog::GetSingleton()->Print("[HDT-SMP] active armor addons: %d", activeArmors);
		ConsoleRE::ConsoleLog::GetSingleton()->Print("[HDT-SMP] active head parts: %d", activeHeadParts);
		ConsoleRE::ConsoleLog::GetSingleton()->Print("[HDT-SMP] active collision meshes: %d", activeCollisionMeshes);

		return true;
	}
}

namespace
{
	void InitializeLog()
	{
#if true
		xUtilty::Log::GetSingleton()->OpenRelitive(xUtilty::FileSystem::MIRA, "Plugins/hdtSMP64.log");
#else
		xUtilty::Log::GetSingleton()->OpenRelitive(xUtilty::FileSystem::Download, "OSEL/Plugins/hdtSMP64/hdtSMP64.log");
#endif
	}
}

extern "C" 
{
	constexpr uint32_t hdtSMP64Version = 105004; // patch version + 10^2 * minor version + 10^5 * major version

	__declspec(dllexport) bool Query(const Interface::QueryInterface* skse, PluginInfo* info)
	{
		// populate info structure
		info->SetPluginName("hdtSMP64");
		info->SetPluginVersion(hdtSMP64Version);

		return true;
	}

	__declspec(dllexport) bool Load(const Interface::QueryInterface* skse)
	{
		InitializeLog();

		API::Init(skse);
		API::AllocTrampoline(1 << 7);

		hdt::g_PluginHandle = API::GetPluginHandle();

		hdt::g_frameEventDispatcher.addListener(hdt::ActorManager::instance());
		hdt::g_frameEventDispatcher.addListener(hdt::SkyrimPhysicsWorld::get());
		hdt::g_shutdownEventDispatcher.addListener(hdt::ActorManager::instance());
		hdt::g_shutdownEventDispatcher.addListener(hdt::SkyrimPhysicsWorld::get());
		hdt::g_armorAttachEventDispatcher.addListener(hdt::ActorManager::instance());
		hdt::g_armorDetachEventDispatcher.addListener(hdt::ActorManager::instance());
		hdt::g_skinSingleHeadGeometryEventDispatcher.addListener(hdt::ActorManager::instance());
		hdt::g_skinAllHeadGeometryEventDispatcher.addListener(hdt::ActorManager::instance());

		hdt::hookAll();

		hdt::g_pluginInterface.init(skse);

		const auto messageInterface = reinterpret_cast<Interface::MessagingInterface*>(skse->QueryInterfaceFromID(Interface::MessagingInterface::TypeID));
		if (messageInterface)
		{
			const auto cameraDispatcher = static_cast<ConsoleRE::BSTEventSource<SKSE::CameraEvent>*>(messageInterface->QueryEventSource(Interface::MessagingInterface::EventSourceType::kCameraEvent));
			if (cameraDispatcher)
			{
				cameraDispatcher->AddEventSink(hdt::SkyrimPhysicsWorld::get());
			}

			messageInterface->RegisterPluginCallback(hdt::g_PluginHandle, "SELF", [](Interface::MessagingInterface::Message* msg)
				{

					if (msg && msg->type == Interface::MessagingInterface::kInputLoaded)
					{
						auto* mm = ConsoleRE::UI::GetSingleton();
						if (mm)
						{
							mm->AddEventSink(hdt::FreezeEventHandler::Get());
						}

						hdt::checkOldPlugins();

// I think we only have _DEBUG now...
#ifdef DEBUG
						hdt::g_armorAttachEventDispatcher.addListener(&hdt::g_eventDebugLogger);
						GetEventDispatcherList()->unk1B8.AddEventSink(&hdt::g_eventDebugLogger);
						GetEventDispatcherList()->unk840.AddEventSink(&hdt::g_eventDebugLogger);
#endif
					}

					// If we receive a SaveGame message, we serialize our data and save it in our dedicated save files.
					if (msg && msg->type == Interface::MessagingInterface::kSaveGame)
					{
						auto data = hdt::Override::OverrideManager::GetSingleton()->Serialize();
						if (!data.str().empty()) 
						{
							std::string save_name = reinterpret_cast<char*>(msg->data);
							std::ofstream ofs(OVERRIDE_SAVE_PATH + save_name + ".dhdt", std::ios::out);
							if (ofs && ofs.is_open())
							{
								ofs << data.str();
							}
						}
					}

					// If we receive a PreLoadGame message, we take our data in our dedicated save files and deserialize it.
					if (msg && msg->type == Interface::MessagingInterface::kPreLoadGame)
					{
						std::string save_name = reinterpret_cast<char*>(msg->data);
						save_name = save_name.substr(0, save_name.find_last_of("."));

						std::ifstream ifs(OVERRIDE_SAVE_PATH + save_name + ".dhdt", std::ios::in);
						if (ifs && ifs.is_open())
						{
							std::stringstream data;
							data << ifs.rdbuf();
							hdt::Override::OverrideManager::GetSingleton()->Deserialize(data);
						}
					}

					//Send our public interface to registered plugins
					if (msg && msg->type == Interface::MessagingInterface::kPostPostLoad)
					{
						hdt::g_pluginInterface.onPostPostLoad();
					}

					// required
					if (msg && msg->type == Interface::MessagingInterface::kDataLoaded)
					{
						if (hdt::SkyrimPhysicsWorld::get()->m_enableWind)
						{
#if KUSE_POSIX_TYPES
							static auto lamda = [](void* arg)
							{
								std::pair<pthread_t, pthread_attr_t>* ThreadPair = reinterpret_cast<std::pair<pthread_t, pthread_attr_t>*>(arg);
								
								//
								hdt::WeatherCheck();

								//
#if _DEBUG
								char threadname[32];
								scePthreadGetname(ThreadPair->first, threadname);
								PRINT_FMT_N("Thread %s Exiting... normal?", threadname);
#endif

								scePthreadExit(nullptr);
								return reinterpret_cast<void*>(0);
							};

							static std::pair<pthread_t, pthread_attr_t> ThreadPair;
							
							if (!scePthreadAttrInit(&ThreadPair.second))
							{
								if (!scePthreadCreate(&ThreadPair.first, nullptr, lamda, (void*)&ThreadPair, "HDT Weather Thread"))
								{
									scePthreadDetach(ThreadPair.first);
									xUtilty::Log::GetSingleton()->Write(xUtilty::Log::logLevel::kNone, "Wind enabled");
								}
							}
#elif
							std::thread t(hdt::WeatherCheck);
							t.detach();
							xUtilty::Log::GetSingleton()->Write(xUtilty::Log::logLevel::kNone, "Wind enabled");
#endif

						}
					}
				});
		}


		static ConsoleRE::SCRIPT_PARAMETER params[1];
		params[0].paramType = static_cast<int>(ConsoleRE::SCRIPT_PARAM_TYPE::kChar);
		params[0].paramName = "String (optional)";
		params[0].optional = 1;

		ConsoleRE::SCRIPT_FUNCTION cmd;
		cmd.functionName = "SMPDebug";
		cmd.shortName = "smp";
		cmd.helpString = "smp <reset>";
		cmd.referenceFunction = 0;
		cmd.numParams = 1;
		cmd.params = params;
		cmd.executeFunction = hdt::SMPDebug_Execute;
		cmd.editorFilter = 0;

		Interface::ConsoleInterface* inf = API::GetConsoleInterface();
		if (inf)
		{
			inf->AddCommand(cmd);
		}

		hdt::papyrus::RegisterAllFunctions(reinterpret_cast<Interface::PapyrusInterface*>(skse->QueryInterfaceFromID(Interface::PapyrusInterface::TypeID)));
		hdt::loadConfig();

		return true;
	}

	__declspec(dllexport) bool Revert() { return true; }
}
