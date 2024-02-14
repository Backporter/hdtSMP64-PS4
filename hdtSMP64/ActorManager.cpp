
#include "WeatherManager.h"
#include "ActorManager.h"
#include "hdtSkyrimPhysicsWorld.h"
#include "hdtDefaultBBP.h"
#include <cinttypes>
#include "Offsets.h"

//NiAVObject* Actor::CalculateLOS_1405FD2C0(Actor *aActor, NiPoint3 *aTargetPosition, NiPoint3 *aRayHitPosition, float aViewCone)
//Used to ray cast from the actor. Will return nonNull if it hits something with position at aTargetPosition.
//Pass in 2pi to aViewCone to ignore LOS of actor.
typedef ConsoleRE::NiAVObject* (*_Actor_CalculateLOS)(ConsoleRE::Actor* aActor, ConsoleRE::NiPoint3* aTargetPosition, ConsoleRE::NiPoint3* aRayHitPosition, float aViewCone);
typedef bool(*_TESNPC_GetFaceGeomPath)(ConsoleRE::TESNPC* a_npc, char* a_buf);

REL::Relocation<_Actor_CalculateLOS> Actor_CalculateLOS(0x71A970);
REL::Relocation<_TESNPC_GetFaceGeomPath> TESNPC_GetFaceGeomPath(0x317110);

ConsoleRE::TESForm*	GetWornForm(ConsoleRE::Actor* thisActor, uint32_t mask)
{
	ConsoleRE::TESBoundObject*	s_form = nullptr;
	ConsoleRE::ExtraDataList*	s_List = nullptr;
	
	if (thisActor)
	{
		Helper::EquipmentSlotMatcher matcher(mask);
		ConsoleRE::ExtraContainerChanges* pContainerChanges = thisActor->extraList.GetByType<ConsoleRE::ExtraContainerChanges>();
		if (pContainerChanges)
		{
			pContainerChanges->FindEquipped(matcher, s_form, s_List);

			return s_form;
		}
	}

	return nullptr;
}

namespace hdt
{
	ActorManager* ActorManager::instance()
	{
		static ActorManager s;
		return &s;
	}

	IDStr ActorManager::armorPrefix(ActorManager::IDType id)
	{
		char buffer[128];
		sprintf_s(buffer, "hdtSSEPhysics_AutoRename_Armor_%08X ", id);
		return IDStr(buffer);
	}

	IDStr ActorManager::headPrefix(ActorManager::IDType id)
	{
		char buffer[128];
		sprintf_s(buffer, "hdtSSEPhysics_AutoRename_Head_%08X ", id);
		return IDStr(buffer);
	}

	inline bool isFirstPersonSkeleton(ConsoleRE::NiNode* npc)
	{
		if (!npc)
		{
			return false;
		}

		return findNode(npc, "Camera1st [Cam1]") ? true : false;
	}

	ConsoleRE::NiNode* getNpcNode(ConsoleRE::NiNode* skeleton)
	{
		// TODO: replace this with a generic skeleton fixing configuration option
		// hardcode an exception for lurker skeletons because they are made incorrectly
		auto shouldFix = false;
		if (skeleton->userData && skeleton->userData->data.objectReference)
		{
			auto npcForm = skyrim_cast<ConsoleRE::TESNPC*>(skeleton->userData->data.objectReference);
			if (npcForm && npcForm->race && !strcasecmp(npcForm->race->skeletonModels[0].GetModel(), "Actors\\DLC02\\BenthicLurker\\Character Assets\\skeleton.nif"))
			{
				shouldFix = true;
			}
		}

		return findNode(skeleton, shouldFix ? "NPC Root [Root]" : "NPC");
	}

	void ActorManager::onEvent(const ArmorAttachEvent& e)
	{
		// No armor is ever attached to a lurker skeleton, thus we don't need to test.
		if (e.skeleton == nullptr || !findNode(e.skeleton, "NPC"))
		{
			return;
		}

		std::lock_guard<decltype(m_lock)> l(m_lock);
		if (m_shutdown) 
		{
			return;
		}

		auto& skeleton = getSkeletonData(e.skeleton);
		if (e.hasAttached)
		{
			// Check override data for current armoraddon
			if (e.skeleton->userData)
			{
				auto actor_formID = e.skeleton->userData->formID;
				if (actor_formID) 
				{
					auto old_physics_file = skeleton.getArmors().back().physicsFile.first;
					std::string physics_file_path_override = hdt::Override::OverrideManager::GetSingleton()->checkOverride(actor_formID, old_physics_file);
					if (!physics_file_path_override.empty()) 
					{
						skeleton.getArmors().back().physicsFile.first = physics_file_path_override;
					}
				}
			}

			skeleton.attachArmor(e.armorModel, e.attachedNode);
		}
		else
		{
			skeleton.addArmor(e.armorModel);
		}
	}

	void ActorManager::onEvent(const ArmorDetachEvent& e)
	{
		if (!e.actor || !e.hasDetached)
		{
			return;
		}

		std::lock_guard<decltype(m_lock)> l(m_lock);
		if (m_shutdown)
		{
			return;
		}

		Skeleton* s = get3rdPersonSkeleton(e.actor);
		setHeadActiveIfNoHairArmor(e.actor, s);
	}

	// @brief To avoid calculating headparts when they're hidden by a wig,
	// we mark the head as not active when there is an armor on hair or long hair slots.
	// We do this during the attach/detach armor events, and on the events leading to scanning the head.
	// Then when checking which skeletons are active to calculate the frame,
	// we only allow the activation of headparts that are on active heads.
	// @param Actor * actor is expected not null.
	void ActorManager::setHeadActiveIfNoHairArmor(ConsoleRE::Actor* actor, Skeleton* skeleton)
	{
		const int hairslot = 1 << 1;
		const int longhairslot = 1 << 11;
		auto worn = GetWornForm(actor, hairslot | longhairslot);
		
		if (skeleton)
		{
			skeleton->head.isActive = !worn;
		}
	}

	// @brief This happens on a closing RaceSex menu, and on 'smp reset'.
	void ActorManager::onEvent(const ConsoleRE::MenuOpenCloseEvent&)
	{
		// The ActorManager members are protected from parallel events by ActorManager.m_lock.
		std::lock_guard<decltype(m_lock)> l(m_lock);
		if (m_shutdown) 
		{
			return;
		}

		setSkeletonsActive();

		for (auto& i : m_skeletons)
		{
			i.reloadMeshes();
		}
	}

	void ActorManager::onEvent(const FrameEvent& e)
	{
		// Other events have to be managed. The FrameEvent is the only event that we can drop,
		// we always have one later where we'll be able to manage the passed time.
		// We drop this execution when the lock is already taken; in that case, we would execute the code later.
		// It is better to drop it now, and let the next frame manage it.
		// Moreover, dropping a locked part of the code allows to reduce the total wait times.
		// Finally, some skse mods issue FrameEvents, this mechanism manages the case where they issue too many.
		std::unique_lock<decltype(m_lock)> lock(m_lock, std::try_to_lock);
		if (!lock.owns_lock())
		{
			return;
		}

		setSkeletonsActive();
	}

	// @brief This function is called by different events, with different locking needs, and is therefore extracted from the events.
	void ActorManager::setSkeletonsActive()
	{
		if (m_shutdown)
		{
			return;
		}

		// We get the player character and its cell.
		// TODO Isn't there a more performing way to find the PC?? A singleton? And if it's the right way, why isn't it in utils functions?
		const auto& playerCharacter = std::find_if(m_skeletons.begin(), m_skeletons.end(), [](Skeleton& s) { return s.isPlayerCharacter(); });
		auto playerCell = (playerCharacter != m_skeletons.end() && playerCharacter->skeleton->parent) ? playerCharacter->skeleton->parent->parent : nullptr;
		// We get the camera, its position and orientation.
		// TODO Can this be reconciled between VR and AE/SE?

		const auto cameraNode = ConsoleRE::PlayerCamera::GetSingleton()->cameraRoot;
		const auto cameraTransform = cameraNode->world;
		const auto cameraPosition = cameraTransform.translate;
		const auto cameraOrientation = cameraTransform.rotate * ConsoleRE::NiPoint3(0., 1., 0.); // The camera matrix is relative to the world.

		std::for_each(m_skeletons.begin(), m_skeletons.end(), [&](Skeleton& skel)
		{
			skel.calculateDistanceAndOrientationDifferenceFromSource(cameraPosition, cameraOrientation);
		});


		// We sort the skeletons depending on the angle and distance.
		std::sort(m_skeletons.begin(), m_skeletons.end(), [](auto&& a_lhs, auto&& a_rhs) 
		{
			auto cr = a_rhs.m_cosAngleFromCameraDirectionTimesSkeletonDistance;
			auto cl = a_lhs.m_cosAngleFromCameraDirectionTimesSkeletonDistance;
			auto dr = a_rhs.m_distanceFromCamera2;
			auto dl = a_lhs.m_distanceFromCamera2;
			return
			// If one of the skeletons is at distance zero (1st person player) from the camera
			(btFuzzyZero(dl) || btFuzzyZero(dr))
			// then it is first.
			? (dl < dr)

			// If one of the skeletons is exacly on the side of the camera (cos = 0)
			: (btFuzzyZero(cl) || btFuzzyZero(cr))
			// then it is last.
			? abs(cl) > abs(cr)

			// If both are on the same side of the camera (product of cos > 0):
			// we want first the smallest angle (so the highest cosinus), and the smallest distance,
			// so we want the smallest distance / cosinus.
			// cl = cosinus * distance, dl = distance� => distance / cosinus = dl/cl
			// So we want dl/cl < dr/cr.
			// Moreover, this test manages the case where one of the skeletons is behind the camera and the other in front of the camera too;
			// the one behind the camera is last (the one with cos(angle) = cr < 0).
			: (dl * cr < dr * cl);
		});
		// We set which skeletons are active and we count them.
		activeSkeletons = 0;
		for (auto& i : m_skeletons)
		{
			if (i.skeleton->refcount == 1)
			{
				i.clear();
				i.skeleton = nullptr;
			}
			else if (i.hasPhysics && i.updateAttachedState(playerCell, activeSkeletons >= maxActiveSkeletons)) 
			{
				activeSkeletons++;
				//check wind obstructions
				const auto world = SkyrimPhysicsWorld::get();
				const auto wind = getWindDirection();
				if (world->m_enableWind && wind && !(btFuzzyZero(hdt::magnitude(*wind)))) 
				{
					const auto owner = skyrim_cast<ConsoleRE::Actor*>(i.skeletonOwner.get());
					if (owner) 
					{
						auto windray = *wind * -1; // reverse wind raycast to find obstruction
						ConsoleRE::NiPoint3 hitLocation;
						//Raycast for object in direction of wind
						const auto object = Actor_CalculateLOS(owner, &windray, &hitLocation, 6.28f);
						if (object) 
						{ //object found
								auto diff = (owner->data.location - hitLocation);
								diff.z = 0;	//remove z component difference
								const auto dist = hdt::magnitude(diff);
							// wind is a linear reduction, with a minimum floor since objects may have a minimum distance
							// windfactor = 0 when dist <= m_distanceForNoWind, = 1 when dist >= m_distanceForMaxWind, and is linear with dist between these 2 values.
							const auto windFactor = std::clamp((dist - world->m_distanceForNoWind) / (world->m_distanceForMaxWind - world->m_distanceForNoWind), 0.f, 1.f);
							if (!btFuzzyZero(windFactor - i.getWindFactor())) 
							{
								xUtilty::Log::GetSingleton()->Write(xUtilty::Log::logLevel::kNone, "%s blocked by %s with distance %2.2g; setting windFactor %2.2g", i.name().c_str(), object->name.c_str(), dist, windFactor);
								i.updateWindFactor(windFactor);
							}
						}
					}
					else 
					{
						xUtilty::Log::GetSingleton()->Write(xUtilty::Log::logLevel::kNone,"%s is active skeleton but failed to cast to Actor, no wind obstruction check possible", i.name().c_str());
					}
				}
			}
		}
		m_skeletons.erase(std::remove_if(m_skeletons.begin(), m_skeletons.end(), [](Skeleton& i) { return !i.skeleton; }), m_skeletons.end());

		for (auto& i : m_skeletons)
		{
			i.cleanArmor();
			i.cleanHead();
		}

		const auto world = SkyrimPhysicsWorld::get();
		if (!world->isSuspended() && // do not do metrics while paused
			frameCount++ % world->min_fps == 0) // check every min-fps frames (i.e., a stable 60 fps should wait for 1 second)
		{
			const auto processing_time = world->m_averageProcessingTime;
			// 30% of processing time is in hdt per profiling;
			// Setting it higher provides more time for hdt processing and can activate more skeletons.
			const auto target_time = world->m_timeTick * world->m_percentageOfFrameTime;
			auto averageTimePerSkeleton = 0.f;
			if (activeSkeletons > 0) 
			{
				averageTimePerSkeleton = processing_time / activeSkeletons;
				// calculate rolling average
				rollingAverage += (averageTimePerSkeleton - rollingAverage) / m_sampleSize;
			}

			xUtilty::Log::GetSingleton()->Write(xUtilty::Log::logLevel::kNone,"msecs/activeSkeleton %2.2g rollingAverage %2.2g activeSkeletons/maxActive/total %d/%d/%d processTime/targetTime %2.2g/%2.2g", averageTimePerSkeleton, rollingAverage, activeSkeletons, maxActiveSkeletons, m_skeletons.size(), processing_time, target_time);
			if (m_autoAdjustMaxSkeletons) 
			{
				maxActiveSkeletons = processing_time > target_time ? activeSkeletons - 2 : static_cast<int>(target_time / rollingAverage);
				// clamp the value to the m_maxActiveSkeletons value
				maxActiveSkeletons = std::clamp(maxActiveSkeletons, 1, m_maxActiveSkeletons);
				frameCount = 1;
			}
		}
	}

	void ActorManager::onEvent(const ShutdownEvent&)
	{
		m_shutdown = true;
		std::lock_guard<decltype(m_lock)> l(m_lock);
		m_skeletons.clear();
	}

	void ActorManager::onEvent(const SkinSingleHeadGeometryEvent& e)
	{
		// This case never happens to a lurker skeleton, thus we don't need to test.
		auto npc = findNode(e.skeleton, "NPC");
		if (!npc)
		{
			return;
		}

		std::lock_guard<decltype(m_lock)> l(m_lock);
		if (m_shutdown)
		{
			return;
		}

		auto& skeleton = getSkeletonData(e.skeleton);
		skeleton.npc = getNpcNode(e.skeleton);
		skeleton.processGeometry(e.headNode, e.geometry);
		auto headPartIter = std::find_if(skeleton.head.headParts.begin(), skeleton.head.headParts.end(), [e](const Head::HeadPart& p) { return p.headPart == e.geometry; });
		if (headPartIter != skeleton.head.headParts.end())
		{
			if (headPartIter->origPartRootNode)
			{
#ifdef _DEBUG
				xUtilty::Log::GetSingleton()->Write(xUtilty::Log::logLevel::kNone, "renaming nodes in original part %s back", headPartIter->origPartRootNode->name.c_str());
#endif // _DEBUG

				for (auto& entry : skeleton.head.renameMap)
				{
					// This case never happens to a lurker skeleton, thus we don't need to test.
					auto node = findNode(headPartIter->origPartRootNode, entry.second->cstr());
					if (node)
					{
#ifdef _DEBUG
						xUtilty::Log::GetSingleton()->Write(xUtilty::Log::logLevel::kNone,"rename node %s -> %s", entry.second->cstr(), entry.first->cstr());
#endif // _DEBUG
						setNiNodeName(node, entry.first->cstr());
					}
				}
			}

			headPartIter->origPartRootNode = nullptr;
		}

		if (!skeleton.head.isFullSkinning)
		{
			skeleton.scanHead();
		}
	}

	void ActorManager::onEvent(const SkinAllHeadGeometryEvent& e)
	{
		// This case never happens to a lurker skeleton, thus we don't need to test.
		auto npc = findNode(e.skeleton, "NPC");
		if (!npc) 
		{
			return;
		}

		std::lock_guard<decltype(m_lock)> l(m_lock);
		if (m_shutdown) 
		{
			return;
		}

		auto& skeleton = getSkeletonData(e.skeleton);
		skeleton.npc = npc;
		if (e.skeleton->userData)
		{
			skeleton.skeletonOwner = e.skeleton->userData;
		}

		if (e.hasSkinned)
		{
			skeleton.scanHead();
			skeleton.head.isFullSkinning = false;

			if (skeleton.head.npcFaceGeomNode)
			{
#ifdef _DEBUG
				xUtilty::Log::GetSingleton()->Write(xUtilty::Log::logLevel::kNone,"npc face geom no longer needed, clearing ref");
#endif // _DEBUG
				skeleton.head.npcFaceGeomNode = nullptr;
			}
		}
		else
		{
			skeleton.head.isFullSkinning = true;
		}
	}

	void ActorManager::PhysicsItem::setPhysics(Ref<SkyrimSystem>& system, bool active)
	{
		clearPhysics();
		
		m_physics = system;
		if (active)
		{
			SkyrimPhysicsWorld::get()->addSkinnedMeshSystem(m_physics);
		}
	}

	void ActorManager::PhysicsItem::clearPhysics()
	{
		if (state() == ItemState::e_Active)
		{
			m_physics->m_world->removeSkinnedMeshSystem(m_physics);
		}
		m_physics = nullptr;
	}

	ActorManager::ItemState ActorManager::PhysicsItem::state() const
	{
		return m_physics ? (m_physics->m_world ? ItemState::e_Active : ItemState::e_Inactive) : ItemState::e_NoPhysics;
	}

	const std::vector<Ref<SkinnedMeshBody>>& ActorManager::PhysicsItem::meshes() const
	{
		return m_physics->meshes();
	}

	void ActorManager::PhysicsItem::updateActive(bool active)
	{
		if (active && state() == ItemState::e_Inactive)
		{
			SkyrimPhysicsWorld::get()->addSkinnedMeshSystem(m_physics);
		}
		else if (!active && state() == ItemState::e_Active)
		{
			m_physics->m_world->removeSkinnedMeshSystem(m_physics);
		}
	}

	void ActorManager::PhysicsItem::setWindFactor(float a_windFactor)
	{
		if (m_physics)
		{
			m_physics->m_windFactor = a_windFactor;
		}
	}

	std::vector<ActorManager::Skeleton>& ActorManager::getSkeletons()
	{
		return m_skeletons;
	}

	bool ActorManager::skeletonNeedsParts(ConsoleRE::NiNode* skeleton)
	{
		return !isFirstPersonSkeleton(skeleton);
		/*
		auto iter = std::find_if(m_skeletons.begin(), m_skeletons.end(), [=](Skeleton& i)
		{
			return i.skeleton == skeleton;
		});
		if (iter != m_skeletons.end())
		{
			return (iter->head.headNode == 0);
		}
		*/
	}

	ActorManager::Skeleton& ActorManager::getSkeletonData(ConsoleRE::NiNode* skeleton)
	{
		auto iter = std::find_if(m_skeletons.begin(), m_skeletons.end(), [=](Skeleton& i) { return i.skeleton == skeleton; });
		if (iter != m_skeletons.end())
		{
			return *iter;
		}

		if (!isFirstPersonSkeleton(skeleton))
		{
			auto ownerIter = std::find_if(m_skeletons.begin(), m_skeletons.end(), [=](Skeleton& i) { return !isFirstPersonSkeleton(i.skeleton) && i.skeletonOwner && skeleton->userData && i.skeletonOwner.get() == skeleton->userData; });
			if (ownerIter != m_skeletons.end())
			{
#ifdef _DEBUG
				xUtilty::Log::GetSingleton()->Write(xUtilty::Log::logLevel::kNone,"new skeleton found for formid %08x", skeleton->userData->formID);
#endif // _DEBUG
				ownerIter->cleanHead(true);
			}
		}

		m_skeletons.push_back(Skeleton());
		m_skeletons.back().skeleton = skeleton;
		return m_skeletons.back();
	}

	ActorManager::Skeleton* ActorManager::get3rdPersonSkeleton(ConsoleRE::Actor* actor)
	{
		for (auto& i : m_skeletons)
		{
			const auto owner = skyrim_cast<ConsoleRE::Actor*>(i.skeletonOwner.get());
			if (actor == owner && i.skeleton && !isFirstPersonSkeleton(i.skeleton))
			{
				return &i;
			}
		}

		return 0;
	}

	void ActorManager::Skeleton::doSkeletonMerge(ConsoleRE::NiNode* dst, ConsoleRE::NiNode* src, IString* prefix, std::unordered_map<IDStr, IDStr>& map)
	{
		for (int i = 0; i < src->children._capacity; ++i)
		{
			auto srcChild = castNiNode(src->children._data[i].get());
			if (!srcChild)
			{
				continue;
			}

			if (!srcChild->name)
			{
				doSkeletonMerge(dst, srcChild, prefix, map);
				continue;
			}

			// FIXME: This was previously only in doHeadSkeletonMerge.
			// But surely non-head skeletons wouldn't have this anyway?
			if (!strcasecmp(srcChild->name.c_str(), "BSFaceGenNiNodeSkinned"))
			{
#ifdef _DEBUG
				xUtilty::Log::GetSingleton()->Write(xUtilty::Log::logLevel::kNone,"skipping facegen ninode in skeleton merge");
#endif // _DEBUG
				continue;
			}

			// TODO check it's not a lurker skeleton
			auto dstChild = findNode(dst, srcChild->name);
			if (dstChild)
			{
				doSkeletonMerge(dstChild, srcChild, prefix, map);
			}
			else
			{
				dst->AttachChild(cloneNodeTree(srcChild, prefix, map), false);
			}
		}
	}

	ConsoleRE::NiNode* ActorManager::Skeleton::cloneNodeTree(ConsoleRE::NiNode* src, IString* prefix, std::unordered_map<IDStr, IDStr>& map)
	{
		ConsoleRE::NiCloningProcess c;
		auto ret = static_cast<ConsoleRE::NiNode*>(src->CreateClone(c));
		src->ProcessClone(c);

		// FIXME: cloneHeadNodeTree just did this for ret, not both. Don't know if that matters. Armor parts need it on both.
		renameTree(src, prefix, map);
		renameTree(ret, prefix, map);

		return ret;
	}

	void ActorManager::Skeleton::renameTree(ConsoleRE::NiNode* root, IString* prefix, std::unordered_map<IDStr, IDStr>& map)
	{
		if (root->name)
		{
			std::string newName(prefix->cstr(), prefix->size());
			newName += root->name.c_str();

#ifdef _DEBUG
			if (map.insert(std::make_pair<IDStr, IDStr>(root->mame.c_str(), newName)).second)
			{
				xUtilty::Log::GetSingleton()->Write(xUtilty::Log::logLevel::kNone, "Rename Bone %s -> %s", root->name.c_str(), newName.c_str());
			}
#else
			map.insert(std::make_pair<IDStr, IDStr>(root->name.c_str(), newName));
#endif // _DEBUG

			setNiNodeName(root, newName.c_str());
		}

		for (int i = 0; i < root->children._capacity; ++i)
		{
			auto child = castNiNode(root->children._data[i].get());
			if (child)
			{
				renameTree(child, prefix, map);
			}
		}
	}

	void ActorManager::Skeleton::doSkeletonClean(ConsoleRE::NiNode* dst, IString* prefix)
	{
		for (int i = dst->children._capacity - 1; i >= 0; --i)
		{
			auto child = castNiNode(dst->children._data[i].get());
			if (!child) 
			{
				continue;
			}

			if (child->name && !strncmp(child->name, prefix->cstr(), prefix->size()))
			{
				dst->DetachChildAt2(i++);
			}
			else
			{
				doSkeletonClean(child, prefix);
			}
		}
	}

	// returns the name of the skeleton owner
	std::string ActorManager::Skeleton::name()
	{
		if (skeleton->userData && skeleton->userData->data.objectReference)
		{
			auto bname = skyrim_cast<ConsoleRE::TESFullName*>(skeleton->userData->data.objectReference);
			if (bname)
			{
				return bname->GetFullName();
			}
		}

		return "";
	}

	void ActorManager::Skeleton::addArmor(ConsoleRE::NiNode* armorModel)
	{
		IDType id = armors.size() ? armors.back().id + 1 : 0;
		auto prefix = armorPrefix(id);
		// FIXME we probably could simplify this by using findNode as surely we don't merge Armors with lurkers skeleton?
		npc = getNpcNode(skeleton);
		auto physicsFile = DefaultBBP::instance()->scanBBP(armorModel);
		armors.push_back(Armor());
		armors.back().id = id;
		armors.back().prefix = prefix;
		armors.back().physicsFile = physicsFile;
		doSkeletonMerge(npc, armorModel, prefix, armors.back().renameMap);
	}

	void ActorManager::Skeleton::attachArmor(ConsoleRE::NiNode* armorModel, ConsoleRE::NiAVObject* attachedNode)
	{
#ifdef _DEBUG
		if (armors.size() == 0 || armors.back().hasPhysics())
		{
			xUtilty::Log::GetSingleton()->Write(xUtilty::Log::logLevel::kNone, "Not attaching armor - no record or physics already exists");
		}

#endif // _DEBUG

		Armor& armor = armors.back();
		armor.armorWorn = attachedNode;

		if (!isFirstPersonSkeleton(skeleton))
		{
			std::unordered_map<IDStr, IDStr> renameMap = armor.renameMap;

			// FIXME we probably could simplify this by using findNode as surely we don't attach Armors to lurkers skeleton?
			auto system = SkyrimSystemCreator().createSystem(getNpcNode(skeleton), attachedNode, armor.physicsFile, std::move(renameMap));
			if (system)
			{
				armor.setPhysics(system, isActive);
				hasPhysics = true;
			}
		}
		if (skeleton && skeleton->userData)
		{
			ConsoleRE::TESForm* form = ConsoleRE::TESForm::LookupByID(skeleton->userData->formID);
			ConsoleRE::Actor* actor = skyrim_cast<ConsoleRE::Actor*>(form);
	
			if (actor)
			{
				ActorManager::setHeadActiveIfNoHairArmor(actor, this);
			}
		}
	}

	void ActorManager::Skeleton::cleanArmor()
	{
		for (auto& i : armors)
		{
			if (!i.armorWorn)
			{
				continue;
			}

			if (i.armorWorn->parent)
			{
				continue;
			}

			i.clearPhysics();
			if (npc)
			{
				doSkeletonClean(npc, i.prefix);
			}

			i.prefix = nullptr;
		}
		armors.erase(std::remove_if(armors.begin(), armors.end(), [](Armor& i) { return !i.prefix; }), armors.end());
	}

	void ActorManager::Skeleton::cleanHead(bool cleanAll)
	{
		for (auto& headPart : head.headParts)
		{
			if (!headPart.headPart->parent || cleanAll)
			{
#ifdef _DEBUG
				if (cleanAll)
					xUtilty::Log::GetSingleton()->Write(xUtilty::Log::logLevel::kNone,"cleaning headpart %s due to clean all", headPart.headPart->name.c_str());
				else
					xUtilty::Log::GetSingleton()->Write(xUtilty::Log::logLevel::kNone,"headpart %s disconnected", headPart.headPart->name.c_str());
#endif // _DEBUG

				auto renameIt = this->head.renameMap.begin();
				while (renameIt != this->head.renameMap.end())
				{
					bool erase = false;
					if (headPart.renamedBonesInUse.count(renameIt->first) != 0)
					{
						auto findNode = this->head.nodeUseCount.find(renameIt->first);
						if (findNode != this->head.nodeUseCount.end())
						{
							findNode->second -= 1;
#ifdef _DEBUG
							xUtilty::Log::GetSingleton()->Write(xUtilty::Log::logLevel::kNone,"decrementing use count by 1, it is now %d", findNode->second);
#endif // _DEBUG
							if (findNode->second <= 0)
							{
#ifdef _DEBUG
								xUtilty::Log::GetSingleton()->Write(xUtilty::Log::logLevel::kNone,"node no longer in use, cleaning from skeleton");
#endif // _DEBUG
								auto removeObj = findObject(npc, renameIt->second->cstr());
								if (removeObj)
								{
#ifdef _DEBUG
									xUtilty::Log::GetSingleton()->Write(xUtilty::Log::logLevel::kNone,"found node %s, removing", removeObj->name.c_str());
#endif // _DEBUG
									auto parent = removeObj->parent;
									if (parent)
									{
										parent->DetachChild2(removeObj);
										removeObj->DecRefCount();
									}

								}

								this->head.nodeUseCount.erase(findNode);
								erase = true;
							}
						}
					}

					if (erase)
					{
						renameIt = this->head.renameMap.erase(renameIt);
					}
					else
					{
						++renameIt;
					}
				}

				headPart.headPart = nullptr;
				headPart.origPartRootNode = nullptr;
				headPart.clearPhysics();
				headPart.renamedBonesInUse.clear();
			}
		}

		head.headParts.erase(std::remove_if(head.headParts.begin(), head.headParts.end(), [](Head::HeadPart& i) { return !i.headPart; }), head.headParts.end());
	}

	void ActorManager::Skeleton::clear()
	{
		std::for_each(armors.begin(), armors.end(), [](Armor& armor) { armor.clearPhysics(); });
		SkyrimPhysicsWorld::get()->removeSystemByNode(npc);
		cleanHead();
		head.headParts.clear();
		head.headNode = nullptr;
		armors.clear();
	}

	void ActorManager::Skeleton::calculateDistanceAndOrientationDifferenceFromSource(ConsoleRE::NiPoint3 sourcePosition, ConsoleRE::NiPoint3 sourceOrientation)
	{
		if (isPlayerCharacter())
		{
			m_distanceFromCamera2 = 0.f;
			return;
		}
		ConsoleRE::NiPoint3 pos;
		if (!position(pos))
		{
			m_distanceFromCamera2 = std::numeric_limits<float>::max();
			return;
		}

		// We calculate the vector between camera and the skeleton feets.
		const auto camera2SkeletonVector = pos - sourcePosition;
		// This is the distance (squared) between the camera and the skeleton feets.
		m_distanceFromCamera2 = camera2SkeletonVector.x * camera2SkeletonVector.x + camera2SkeletonVector.y * camera2SkeletonVector.y + camera2SkeletonVector.z * camera2SkeletonVector.z;
		// This is |camera2SkeletonVector|*cos(angle between both vectors)
		m_cosAngleFromCameraDirectionTimesSkeletonDistance = camera2SkeletonVector.x * sourceOrientation.x + camera2SkeletonVector.y * sourceOrientation.y + camera2SkeletonVector.z * sourceOrientation.z;
	}

	// Is called to print messages only
	bool ActorManager::Skeleton::checkPhysics()
	{
		hasPhysics = false;
		std::for_each(armors.begin(), armors.end(), [=](Armor& armor) { if (armor.state() != ItemState::e_NoPhysics) { hasPhysics = true; }});
		if (!hasPhysics)
		{
			std::for_each(head.headParts.begin(), head.headParts.end(), [=](Head::HeadPart& headPart) { if (headPart.state() != ItemState::e_NoPhysics) { hasPhysics = true; }});
		}

		xUtilty::Log::GetSingleton()->Write(xUtilty::Log::logLevel::kNone, "%s isDrawn %d: %d", name().c_str(), hasPhysics);
		return hasPhysics;
	}

	bool ActorManager::Skeleton::isActiveInScene() const
	{
		// TODO: do this better
		// When entering/exiting an interior, NPCs are detached from the scene but not unloaded, so we need to check two levels up.
		// This properly removes exterior cell armors from the physics world when entering an interior, and vice versa.
		return skeleton->parent && skeleton->parent->parent && skeleton->parent->parent->parent;
	}

	bool ActorManager::Skeleton::isPlayerCharacter() const
	{
		constexpr uint32_t playerFormID = 0x14;
		return skeletonOwner.get() == ConsoleRE::PlayerCharacter::GetSingleton() || (skeleton->userData && skeleton->userData->formID == playerFormID);
	}

	bool ActorManager::Skeleton::isInPlayerView()
	{
		// This function is called only when the skeleton isn't the player character.
		// This might change in the future; in that case this test will have to be enabled.
		//if (isPlayerCharacter())
		//	return true;

		// We don't enable the skeletons behind the camera or on its side.
		if (m_cosAngleFromCameraDirectionTimesSkeletonDistance <= 0)
		{
			return false;
		}

		// We enable only the skeletons that the PC sees.
		bool unk1 = 0;
		return ConsoleRE::PlayerCharacter::GetSingleton()->HasLineOfSight(skeleton->userData, unk1);
	}

	bool ActorManager::Skeleton::position(ConsoleRE::NiPoint3& dst) const
	{
		if (npc)
		{
			// This works for lurker skeletons.
			auto rootNode = findNode(npc, "NPC Root [Root]");
			if (rootNode)
			{
				dst = rootNode->world.translate;
				return true;
			}
		}

		return false;
	}

	void ActorManager::Skeleton::updateWindFactor(float a_windFactor)
	{
		this->currentWindFactor = a_windFactor;
		std::for_each(armors.begin(), armors.end(), [=](Armor& armor) { armor.setWindFactor(a_windFactor); });
		std::for_each(head.headParts.begin(), head.headParts.end(), [=](Head::HeadPart& headPart) { headPart.setWindFactor(a_windFactor); });
	}

	float ActorManager::Skeleton::getWindFactor()
	{
		return this->currentWindFactor;
	}

	bool ActorManager::Skeleton::updateAttachedState(const ConsoleRE::NiNode* playerCell, bool deactivate = false)
	{
		// 1- Skeletons that aren't active in any scene are always detached, unless they are in the
		// same cell as the player character (workaround for issue in Ancestor Glade).
		// 2- Player character is always attached.
		// 3- Otherwise, attach only if both the camera and this skeleton have a position,
		// the distance between them is below the threshold value,
		// and the angle difference between the camera orientation and the skeleton orientation is below the threshold value.
		isActive = false;
		state = SkeletonState::e_InactiveNotInScene;

		if (deactivate)
		{
			state = SkeletonState::e_InactiveTooFar;
		}
		else if (isActiveInScene() || skeleton->parent && skeleton->parent->parent == playerCell)
		{
			if (isPlayerCharacter())
			{
				// That setting defines whether we don't set the PC skeleton as active
				// when it is in 1st person view, to avoid calculating physics uselessly.
				if (!(instance()->m_disable1stPersonViewPhysics // disabling?
					&& ConsoleRE::PlayerCamera::GetSingleton()->currentState == ConsoleRE::PlayerCamera::GetSingleton()->cameraStates[0])) // 1st person view
				{
					isActive = true;
					state = SkeletonState::e_ActiveIsPlayer;
				}
			}
			else if (isInPlayerView())
			{
				isActive = true;
				state = SkeletonState::e_ActiveNearPlayer;
			}
			else
			{
				state = SkeletonState::e_InactiveUnseenByPlayer;
			}
		}

		// We update the activity state of armors and head parts, and add and remove SkinnedMeshSystems to these parts in consequence.
		// We set headparts as not active if the head isn't active (for example because it's hidden by a wig).
		std::for_each(armors.begin(), armors.end(), [=](Armor& armor) { armor.updateActive(isActive); });
		const bool isHeadActive = head.isActive;
		std::for_each(head.headParts.begin(), head.headParts.end(), [=](Head::HeadPart& headPart) { headPart.updateActive(isHeadActive && isActive); });
		return isActive;
	}

	void ActorManager::Skeleton::reloadMeshes()
	{
		for (auto& i : armors)
		{
			i.clearPhysics();
			if (!isFirstPersonSkeleton(skeleton))
			{
				std::unordered_map<IDStr, IDStr> renameMap = i.renameMap;
				auto system = SkyrimSystemCreator().createSystem(npc, i.armorWorn, i.physicsFile, std::move(renameMap));
				if (system)
				{
					i.setPhysics(system, isActive);
					hasPhysics = true;
				}
			}
		}

		scanHead();
	}

	void ActorManager::Skeleton::scanHead()
	{
		if (isFirstPersonSkeleton(this->skeleton))
		{
#ifdef _DEBUG
			xUtilty::Log::GetSingleton()->Write(xUtilty::Log::logLevel::kNone,"not scanning head of first person skeleton");
#endif // _DEBUG
			return;
		}

		if (!this->head.headNode)
		{
#ifdef _DEBUG
			xUtilty::Log::GetSingleton()->Write(xUtilty::Log::logLevel::kNone,"actor has no head node");
#endif // _DEBUG
			return;
		}

		std::unordered_set<std::string> physicsDupes;
		if (skeleton && skeleton->userData)
		{
			ConsoleRE::TESForm* form = ConsoleRE::TESForm::LookupByID(skeleton->userData->formID);
			ConsoleRE::Actor* actor = skyrim_cast<ConsoleRE::Actor*>(form);
			if (actor)
			{
				ActorManager::setHeadActiveIfNoHairArmor(actor, this);
			}
		}

		for (auto& headPart : this->head.headParts)
		{
			// always regen physics for all head parts
			headPart.clearPhysics();
	
			if (headPart.physicsFile.first.empty())
			{
#ifdef _DEBUG
				xUtilty::Log::GetSingleton()->Write(xUtilty::Log::logLevel::kNone, ,"no physics file for headpart %s", headPart.headPart->name.c_str());
#endif // _DEBUG
				continue;
			}

			if (physicsDupes.count(headPart.physicsFile.first))
			{
#ifdef _DEBUG
				xUtilty::Log::GetSingleton()->Write(xUtilty::Log::logLevel::kNone, ,"previous head part generated physics system for file %s, skipping", headPart.physicsFile.first.c_str());
#endif // _DEBUG
				continue;
			}

			std::unordered_map<IDStr, IDStr> renameMap = this->head.renameMap;

#ifdef _DEBUG
				xUtilty::Log::GetSingleton()->Write(xUtilty::Log::logLevel::kNone, "try create system for headpart %s physics file %s", headPart.headPart->name.c_str(), headPart.physicsFile.first.c_str());
#endif // _DEBUG

			physicsDupes.insert(headPart.physicsFile.first);
			auto system = SkyrimSystemCreator().createSystem(npc, this->head.headNode, headPart.physicsFile, std::move(renameMap));
			if (system)
			{

#ifdef _DEBUG
				xUtilty::Log::GetSingleton()->Write(xUtilty::Log::logLevel::kNone,"success");
#endif // _DEBUG

				headPart.setPhysics(system, isActive);
				hasPhysics = true;
			}
		}
	}

	void ActorManager::Skeleton::processGeometry(ConsoleRE::BSFaceGenNiNode* headNode, ConsoleRE::BSGeometry* geometry)
	{
		if (this->head.headNode && this->head.headNode != headNode)
		{
#ifdef _DEBUG
			xUtilty::Log::GetSingleton()->Write((xUtilty::Log::logLevel::kNone,"completely new head attached to skeleton, clearing tracking");
#endif // _DEBUG

			for (auto& headPart : this->head.headParts)
			{
				headPart.clearPhysics();
				headPart.headPart = nullptr;
				headPart.origPartRootNode = nullptr;
			}

			this->head.headParts.clear();

			if (npc)
			{
				doSkeletonClean(npc, this->head.prefix);
			}

			this->head.prefix = nullptr;
			this->head.headNode = nullptr;
			this->head.renameMap.clear();
			this->head.nodeUseCount.clear();
		}


		// clean swapped out headparts
		cleanHead();
		this->head.headNode = headNode;
		++this->head.id;
		this->head.prefix = headPrefix(this->head.id);
		
		auto it = std::find_if(this->head.headParts.begin(), this->head.headParts.end(), [geometry](const Head::HeadPart& p) { return p.headPart == geometry; });
		if (it != this->head.headParts.end())
		{
#ifdef _DEBUG
			xUtilty::Log::GetSingleton()->Write(xUtilty::Log::logLevel::kNone, "geometry is already added as head part");
#endif // _DEBUG
			return;
		}

		this->head.headParts.push_back(Head::HeadPart());

		head.headParts.back().headPart = geometry;
		head.headParts.back().clearPhysics();

		// Skinning
#ifdef _DEBUG
		xUtilty::Log::GetSingleton()->Write(xUtilty::Log::logLevel::kNone, "skinning geometry to skeleton");
#endif // _DEBUG

		if (!geometry->skinInstance || !geometry->skinInstance->skinData)
		{
			xUtilty::Log::GetSingleton()->Write(xUtilty::Log::logLevel::kNone,"geometry is missing skin instance - how?");
			return;
		}

		auto fmd = static_cast<ConsoleRE::BSFaceGenModelExtraData*>(geometry->GetExtraData("FMD"));

		ConsoleRE::BSGeometry* origGeom = nullptr;
		ConsoleRE::NiGeometry* origNiGeom = nullptr;

		if (fmd && fmd->m_model && fmd->m_model->modelMeshData && fmd->m_model->modelMeshData->faceNode)
		{
#ifdef _DEBUG
			xUtilty::Log::GetSingleton()->Write(xUtilty::Log::logLevel::kNone, "orig part node found via fmd");
#endif // _DEBUG

			auto origRootNode = fmd->m_model->modelMeshData->faceNode->AsNode();
			head.headParts.back().physicsFile = DefaultBBP::instance()->scanBBP(origRootNode);
			head.headParts.back().origPartRootNode = origRootNode;
	
			for (int i = 0; i < origRootNode->children._size; i++)
			{
				if (origRootNode->children._data[i])
				{
					const auto geo = origRootNode->children._data[i]->AsGeometry();
					if (geo)
					{
						origGeom = geo;
						break;
					}
				}
			}
		}
		else
		{
#ifdef _DEBUG
			xUtilty::Log::GetSingleton()->Write(xUtilty::Log::logLevel::kNone, "no fmd available, loading original facegeom");
#endif // _DEBUG
	
			if (!head.npcFaceGeomNode)
			{
				if (skeleton->userData && skeleton->userData->data.objectReference)
				{
					auto npc = skyrim_cast<ConsoleRE::TESNPC*>(skeleton->userData->data.objectReference);
					if (npc)
					{
						char filePath[MAX_PATH];
						if (TESNPC_GetFaceGeomPath(npc, filePath))
						{
#ifdef _DEBUG
							xUtilty::Log::GetSingleton()->Write(xUtilty::Log::logLevel::kNone, "loading facegeom from path %s", filePath);
#endif // _DEBUG
							uint8_t niStreamMemory[sizeof(ConsoleRE::NiStream)]{ 0 };
							ConsoleRE::NiStream* niStream = (ConsoleRE::NiStream*)niStreamMemory;
							niStream->Ctor();

							ConsoleRE::BSResourceNiBinaryStream binaryStream(filePath);
							if (!binaryStream.good())
							{
								xUtilty::Log::GetSingleton()->Write(xUtilty::Log::logLevel::kNone,"somehow npc facegeom was not found");
								niStream->Dtor();
							}
							else
							{
								niStream->Load1(&binaryStream);
								if (niStream->topObjects[0])
								{
									auto rootFadeNode = niStream->topObjects[0]->AsFadeNode();
									if (rootFadeNode)
									{
#ifdef _DEBUG
										xUtilty::Log::GetSingleton()->Write(xUtilty::Log::logLevel::kNone, "npc root fadenode found");
#endif // _DEBUG
										head.npcFaceGeomNode = rootFadeNode;
									}
#ifdef _DEBUG
									else
									{
										xUtilty::Log::GetSingleton()->Write(xUtilty::Log::logLevel::kNone, "npc facegeom root wasn't fadenode as expected");
									}
#endif // _DEBUG
								}

								niStream->Dtor();
							}
						}
					}
				}
			}
#ifdef _DEBUG
			else
			{
				xUtilty::Log::GetSingleton()->Write(xUtilty::Log::logLevel::kNone, "using cached facegeom");
			}
#endif // _DEBUG
			if (head.npcFaceGeomNode)
			{
				head.headParts.back().physicsFile = DefaultBBP::instance()->scanBBP(head.npcFaceGeomNode);
				
				auto obj = findObject(head.npcFaceGeomNode, geometry->name);
				if (obj)
				{
					auto ob = obj->AsGeometry();
					if (ob)
					{
						origGeom = ob;
					}
					else
					{
						auto on = obj->AsNiGeometry();
						if (on)
						{
							origNiGeom = on;
						}
					}
				}
			}
		}

		bool hasMerged = false;
		bool hasRenames = false;
		for (int boneIdx = 0; boneIdx < geometry->skinInstance->skinData->bones; boneIdx++)
		{
			ConsoleRE::BSFixedString boneName("");
			// skin the way the game does via FMD
			if (boneIdx <= 7)
			{
				if (fmd)
				{
					boneName = fmd->bones[boneIdx];
				}
			}

			if (!*boneName.c_str())
			{
				if (origGeom)
				{
					boneName = origGeom->skinInstance->bones[boneIdx]->name;
				}
				else if (origNiGeom)
				{
					boneName = origNiGeom->m_spSkinInstance->bones[boneIdx]->name;
				}
			}

			auto renameIt = this->head.renameMap.find(boneName.c_str());
			if (renameIt != this->head.renameMap.end())
			{
#ifdef _DEBUG
				xUtilty::Log::GetSingleton()->Write(xUtilty::Log::logLevel::kNone, "found renamed bone %s -> %s", boneName.c_str(), renameIt->second->cstr());
#endif // _DEBUG
				boneName = renameIt->second->cstr();
				hasRenames = true;
			}

			auto boneNode = findNode(this->npc, boneName);
			if (!boneNode && !hasMerged)
			{
#ifdef _DEBUG
				xUtilty::Log::GetSingleton()->Write(xUtilty::Log::logLevel::kNone, "bone not found on skeleton, trying skeleton merge");
#endif // _DEBUG
				if (this->head.headParts.back().origPartRootNode)
				{
					doSkeletonMerge(npc, head.headParts.back().origPartRootNode, head.prefix, head.renameMap);
				}
				else if (this->head.npcFaceGeomNode)
				{
					// Facegen data doesn't have any tree structure to the skeleton. We need to make any new
					// nodes children of the head node, so that they move properly when there's no physics.
					// This case never happens to a lurker skeleton, thus we don't need to test.
					auto headNode = findNode(head.npcFaceGeomNode, "NPC Head [Head]");
					if (headNode)
					{
						ConsoleRE::NiTransform invTransform = headNode->local.Invert();
						for (int i = 0; i < head.npcFaceGeomNode->children._capacity; ++i)
						{
							Ref<ConsoleRE::NiNode> child = castNiNode(head.npcFaceGeomNode->children._data[i].get());
							// This case never happens to a lurker skeleton, thus we don't need to test.
							if (child && !findNode(npc, child->name))
							{
								child->local = invTransform * child->local;
								head.npcFaceGeomNode->DetachChildAt2(i);
								headNode->AttachChild(child, false);
							}
						}
					}
					doSkeletonMerge(npc, this->head.npcFaceGeomNode, head.prefix, head.renameMap);
				}

				hasMerged = true;

				auto postMergeRenameIt = this->head.renameMap.find(boneName.c_str());
				if (postMergeRenameIt != this->head.renameMap.end())
				{
#ifdef _DEBUG
					xUtilty::Log::GetSingleton()->Write(xUtilty::Log::logLevel::kNone,"found renamed bone %s -> %s", boneName.c_str(), postMergeRenameIt->second->cstr());
#endif // _DEBUG
					boneName = postMergeRenameIt->second->cstr();
					hasRenames = true;
				}

				boneNode = findNode(this->npc, boneName);
			}

	
			if (!boneNode)
			{
				xUtilty::Log::GetSingleton()->Write(xUtilty::Log::logLevel::kNone,"bone %s not found after skeleton merge, geometry cannot be fully skinned", boneName.c_str());
				continue;
			}

			geometry->skinInstance->bones[boneIdx] = boneNode;
			geometry->skinInstance->boneWorldTransforms[boneIdx] = &boneNode->world;
		}

		geometry->skinInstance->rootParent = headNode;

		if (hasRenames)
		{
			for (auto& entry : head.renameMap)
			{
				if ((this->head.headParts.back().origPartRootNode && findObject(this->head.headParts.back().origPartRootNode, entry.first->cstr())) || (this->head.npcFaceGeomNode && findObject(this->head.npcFaceGeomNode, entry.first->cstr())))
				{
					auto findNode = this->head.nodeUseCount.find(entry.first);
					if (findNode != this->head.nodeUseCount.end())
					{
						findNode->second += 1;
#ifdef _DEBUG
						xUtilty::Log::GetSingleton()->Write(xUtilty::Log::logLevel::kNone,"incrementing use count by 1, it is now %d", findNode->second);
#endif // _DEBUG
					}
					else
					{
						this->head.nodeUseCount.insert(std::make_pair(entry.first, 1));
#ifdef _DEBUG
						xUtilty::Log::GetSingleton()->Write(xUtilty::Log::logLevel::kNone, "first use of bone, count 1");
#endif // _DEBUG
					}

					head.headParts.back().renamedBonesInUse.insert(entry.first);
				}
			}
		}

#ifdef _DEBUG
		xUtilty::Log::GetSingleton()->Write(xUtilty::Log::logLevel::kNone, "done skinning part");
#endif // _DEBUG
	}
}
