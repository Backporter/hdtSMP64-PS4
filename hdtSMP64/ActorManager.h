#pragma once

#include "../hdtSSEUtils/NetImmerseUtils.h"
#include "../hdtSSEUtils/FrameworkUtils.h"

#include "hdtSkyrimSystem.h"

#include "IEventListener.h"
#include "HookEvents.h"
#include "Offsets.h"

#include <mutex>

#include "DynamicHDT.h"

namespace hdt
{
	class ActorManager
		: public IEventListener<ArmorAttachEvent>
		, public IEventListener<ArmorDetachEvent>
		, public IEventListener<SkinSingleHeadGeometryEvent>
		, public IEventListener<SkinAllHeadGeometryEvent>
		, public IEventListener<FrameEvent>
		, public IEventListener<ShutdownEvent>
	{
		using IDType = uint32_t;

	public:

		enum class ItemState
		{
			e_NoPhysics,
			e_Inactive,
			e_Active
		};

		// Overall skeleton state, purely for console debug info
		enum class SkeletonState
		{
			// Note order: inactive states must come before e_SkeletonActive, and active states after
			e_InactiveNotInScene,
			e_InactiveUnseenByPlayer,
			e_InactiveTooFar,
			e_SkeletonActive,
			e_ActiveNearPlayer,
			e_ActiveIsPlayer
		};
		int activeSkeletons = 0;

	private:
		int maxActiveSkeletons = 10;
		int frameCount = 0;
		float rollingAverage = 0;
		struct Skeleton;

		struct PhysicsItem
		{
			DefaultBBP::PhysicsFile physicsFile;

			void setPhysics(Ref<SkyrimSystem>& system, bool active);
			void clearPhysics();
			bool hasPhysics() const { return m_physics; }
			ActorManager::ItemState state() const;

			const std::vector<Ref<SkinnedMeshBody>>& meshes() const;

			void updateActive(bool active);

			// Update windfactor for all armors attached to skeleton.
			// a_windFactor is a percentage [0,1] with 0 being no wind efect to 1 being full wind effect.
			void setWindFactor(float a_windFactor);

			Ref<SkyrimSystem> m_physics;
		};

		struct Head
		{
			struct HeadPart : public PhysicsItem
			{
				Ref<ConsoleRE::BSGeometry> headPart;
				Ref<ConsoleRE::NiNode> origPartRootNode;
				std::set<IDStr> renamedBonesInUse;
			};

			IDType id;
			Ref<IString> prefix;
			Ref<ConsoleRE::BSFaceGenNiNode> headNode;
			Ref<ConsoleRE::BSFadeNode> npcFaceGeomNode;
			std::vector<HeadPart> headParts;
			std::unordered_map<IDStr, IDStr> renameMap;
			std::unordered_map<IDStr, uint8_t> nodeUseCount;
			bool isFullSkinning;
			bool isActive = true; // false when hidden by a wig
		};

		struct Armor : public PhysicsItem
		{
			IDType id;
			Ref<IString> prefix;
			Ref<ConsoleRE::NiAVObject> armorWorn;
			std::unordered_map<IDStr, IDStr> renameMap;
		};

		struct Skeleton
		{
			ConsoleRE::NiPointer<ConsoleRE::TESObjectREFR> skeletonOwner;
			Ref<ConsoleRE::NiNode> skeleton;
			Ref<ConsoleRE::NiNode> npc;
			Head head;
			SkeletonState state;

			std::string name();
			void addArmor(ConsoleRE::NiNode* armorModel);
			void attachArmor(ConsoleRE::NiNode* armorModel, ConsoleRE::NiAVObject* attachedNode);

			void cleanArmor();
			void cleanHead(bool cleanAll = false);
			void clear();

			// @brief This calculates and sets the distance from skeleton to player, and a value that is the cosinus
			// between the camera orientation vector and the camera to skeleton vector, multiplied by the length
			// of the camera to skeleton vector; that value is very fast to compute as it is a dot product, and it
			// can be directly used for our needs later; the distance is provided squared for performance reasons.
			// @param sourcePosition the position of the camera
			// @param sourceOrientation the orientation of the camera
			void calculateDistanceAndOrientationDifferenceFromSource(ConsoleRE::NiPoint3 sourcePosition, ConsoleRE::NiPoint3 sourceOrientation);

			bool isPlayerCharacter() const;
			bool isInPlayerView();
			bool hasPhysics = false;

			bool position(ConsoleRE::NiPoint3&) const;

			// @brief Update windfactor for skeleton
			// @param a_windFactor is a percentage [0,1] with 0 being no wind efect to 1 being full wind effect.
			void updateWindFactor(float a_windFactor);
			// @brief Get windfactor for skeleton
			float getWindFactor();

			// @brief Updates the states and activity of skeletons, their heads parts and armors.
			// @param playerCell The skeletons not in the player cell are automatically inactive.
			// @param deactivate If set to true, the concerned skeleton will be inactive, regardless of other elements.
			bool updateAttachedState(const ConsoleRE::NiNode* playerCell, bool deactivate);

			// bool deactivate(); // FIXME useless?
			void reloadMeshes();

			void scanHead();
			void processGeometry(ConsoleRE::BSFaceGenNiNode* head, ConsoleRE::BSGeometry* geometry);

			static void doSkeletonMerge(ConsoleRE::NiNode* dst, ConsoleRE::NiNode* src, IString* prefix, std::unordered_map<IDStr, IDStr>& map);
			static void doSkeletonClean(ConsoleRE::NiNode* dst, IString* prefix);
			static ConsoleRE::NiNode* cloneNodeTree(ConsoleRE::NiNode* src, IString* prefix, std::unordered_map<IDStr, IDStr>& map);
			static void renameTree(ConsoleRE::NiNode* root, IString* prefix, std::unordered_map<IDStr, IDStr>& map);

			std::vector<Armor>& getArmors() { return armors; }

			// @brief This is the squared distance between the skeleton and the camera.
			float m_distanceFromCamera2 = std::numeric_limits<float>::max();

			// @brief This is |camera2SkeletonVector|*cos(angle between that vector and the camera direction).
			float m_cosAngleFromCameraDirectionTimesSkeletonDistance = -1.;

		private:
			bool isActiveInScene() const;
			bool checkPhysics();

			bool isActive = false;
			float currentWindFactor = 0.f;
			std::vector<Armor> armors;
		};

		bool m_shutdown = false;
		std::recursive_mutex m_lock;
		std::vector<Skeleton> m_skeletons;

		Skeleton& getSkeletonData(ConsoleRE::NiNode* skeleton);
		ActorManager::Skeleton* get3rdPersonSkeleton(ConsoleRE::Actor* actor);
		static void setHeadActiveIfNoHairArmor(ConsoleRE::Actor* actor, Skeleton* skeleton);
	public:
		ActorManager() = default;
		~ActorManager() = default;

		static ActorManager* instance();

		static IDStr armorPrefix(IDType id);
		static IDStr headPrefix(IDType id);

		void onEvent(const ArmorAttachEvent& e) override;
		void onEvent(const ArmorDetachEvent& e) override;

		// @brief On this event, we decide which skeletons will be active for physics this frame.
		void onEvent(const FrameEvent& e) override;

		void onEvent(const ConsoleRE::MenuOpenCloseEvent&);
		void onEvent(const ShutdownEvent&) override;
		void onEvent(const SkinSingleHeadGeometryEvent&) override;
		void onEvent(const SkinAllHeadGeometryEvent&) override;

		bool skeletonNeedsParts(ConsoleRE::NiNode* skeleton);
		std::vector<Skeleton>& getSkeletons();//Altered by Dynamic HDT

		bool m_skinNPCFaceParts = true;
		bool m_autoAdjustMaxSkeletons = true; // Whether to dynamically change the maxActive skeletons to maintain min_fps
		int m_maxActiveSkeletons = 20; // The maximum active skeletons; hard limit
		int m_sampleSize = 5; // how many samples (each sample taken every second) for determining average time per activeSkeleton.

		// @brief Depending on this setting, we avoid to calculate the physics of the PC when it is in 1st person view.
		bool m_disable1stPersonViewPhysics = false;
	private:
		void setSkeletonsActive();
	};
}
