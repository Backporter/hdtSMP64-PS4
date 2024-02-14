#include "Hooks.h"
#include "HookEvents.h"
#include "Offsets.h"
#include "ActorManager.h"

#include "../../../OrbisUtil/Third-Party/herumi/xbayk/6.00/xbyak.h"

namespace hdt
{
	class BSFaceGenNiNodeEx : public ConsoleRE::BSFaceGenNiNode
	{
	public:
		void SkinSingleGeometry(ConsoleRE::NiNode* a_skeleton, ConsoleRE::BSGeometry* a_geometry, ConsoleRE::BSTriShape* a_trishape);
		void SkinAllGeometry(ConsoleRE::NiNode* a_skeleton, uint32_t a_unk);
	};

	bool					unequipItem(ConsoleRE::ActorEquipManager*, ConsoleRE::Actor*, ConsoleRE::TESForm*, ConsoleRE::BaseExtraList*, int32_t, ConsoleRE::BGSEquipSlot*, bool, bool, bool, bool, void*);
	ConsoleRE::NiAVObject*	unk001CB0E0(const void*, ConsoleRE::NiNode*, ConsoleRE::NiNode*, void*, char, char, void*);

	//
	REL::Relocation<decltype(&unk001CB0E0)>								_unk001CB0E0;
	REL::Relocation<decltype(&unequipItem)>								_UnequipObject;
	REL::Relocation<decltype(&BSFaceGenNiNodeEx::SkinSingleGeometry)>	_SkinSingleGeometry;
	REL::Relocation<decltype(&BSFaceGenNiNodeEx::SkinAllGeometry)>		_SkinAllGeometry;

	//
	static REL::Relocation<uintptr_t> UpdateHook(0x06A38E3);
	static REL::Relocation<uintptr_t> BoneLimit(0x317E8A); // 0x317E20 / 0x317E8A
	static REL::Relocation<uintptr_t> hookAttachArmorHook1(0x1275E4);
	static REL::Relocation<uintptr_t> hookAttachArmorHook2(0x12761F);
	static REL::Relocation<uintptr_t> SkinSingleGeometryCodeHook1(0x47DEA8);
	static REL::Relocation<uintptr_t> SkinSingleGeometryCodeHook2(0x481EE9);

#if true
	static REL::Relocation<uintptr_t> DetachArmorhook1(0x00000000006E74D3);
	static REL::Relocation<uintptr_t> DetachArmorhook2(0x00000000006E7612);
	static REL::Relocation<uintptr_t> DetachArmorhook3(0x00000000006ED7C1);
	static REL::Relocation<uintptr_t> DetachArmorhook4(0x000000000072307F);
	static REL::Relocation<uintptr_t> DetachArmorhook5(0x000000000074322A);
	static REL::Relocation<uintptr_t> DetachArmorhook6(0x00000000002A6393);
	static REL::Relocation<uintptr_t> DetachArmorhook7(0x000000000075596A);
	static REL::Relocation<uintptr_t> DetachArmorhook8(0x0000000000755A2D);
	static REL::Relocation<uintptr_t> DetachArmorhook9(0x0000000000141833);
	static REL::Relocation<uintptr_t> DetachArmorhook10(0x000000000014F07F);
	static REL::Relocation<uintptr_t> DetachArmorhook11(0x00000000004E33D3);
	static REL::Relocation<uintptr_t> DetachArmorhook12(0x00000000004E3800);
	static REL::Relocation<uintptr_t> DetachArmorhook13(0x00000000004E39B1);
	static REL::Relocation<uintptr_t> DetachArmorhook14(0x000000000062A044);
	static REL::Relocation<uintptr_t> DetachArmorhook15(0x00000000006ED573);
	static REL::Relocation<uintptr_t> DetachArmorhook16(0x000000000074B24A);
	static REL::Relocation<uintptr_t> DetachArmorhook17(0x0000000000755F93);
	static REL::Relocation<uintptr_t> DetachArmorhook18(0x0000000000756328);
	static REL::Relocation<uintptr_t> DetachArmorhook19(0x000000000075636C);
	static REL::Relocation<uintptr_t> DetachArmorhook20(0x0000000000756440);
	static REL::Relocation<uintptr_t> DetachArmorhook21(0x0000000000756501);
	static REL::Relocation<uintptr_t> DetachArmorhook22(0x0000000000789B1F);
	static REL::Relocation<uintptr_t> DetachArmorhook23(0x0000000000789BA6);
	static REL::Relocation<uintptr_t> DetachArmorhook24(0x000000000078E57E);
	static REL::Relocation<uintptr_t> DetachArmorhook25(0x000000000078F13D);
	static REL::Relocation<uintptr_t> DetachArmorhook26(0x000000000079ECC4);
	static REL::Relocation<uintptr_t> DetachArmorhook27(0x00000000007AFBB9);
	static REL::Relocation<uintptr_t> DetachArmorhook28(0x00000000007BB338);
	static REL::Relocation<uintptr_t> DetachArmorhook29(0x00000000007EFB76);
	static REL::Relocation<uintptr_t> DetachArmorhook30(0x0000000000818638);
	static REL::Relocation<uintptr_t> DetachArmorhook31(0x00000000008C68CC);
	static REL::Relocation<uintptr_t> DetachArmorhook32(0x0000000000A1F80E);
	static REL::Relocation<uintptr_t> DetachArmorhook33(0x0000000000A60D73);
	static REL::Relocation<uintptr_t> DetachArmorhook34(0x0000000000B20E1B);
	static REL::Relocation<uintptr_t> DetachArmorhook35(0x0000000000B20F22);
#else
	static REL::Relocation<uintptr_t> UnequipHook(0x755A50);
#endif

	void BSFaceGenNiNodeEx::SkinSingleGeometry(ConsoleRE::NiNode* a_skeleton, ConsoleRE::BSGeometry* a_geometry, ConsoleRE::BSTriShape* a_trishape)
	{
		//
		const char* name = "";
		uint32_t formId = 0x0;

		if (a_skeleton->userData && a_skeleton->userData->data.objectReference)
		{
			auto bname = skyrim_cast<ConsoleRE::TESFullName*>(a_skeleton->userData->data.objectReference);
			if (bname)
			{
				name = bname->GetFullName();
			}

			auto bnpc = skyrim_cast<ConsoleRE::TESNPC*>(a_skeleton->userData->data.objectReference);
			if (bnpc && bnpc->faceNPC)
			{
				formId = bnpc->faceNPC->formID;
			}
		}

		xUtilty::Log::GetSingleton()->Write(xUtilty::Log::logLevel::kNone,"SkinSingleGeometry %s %d - %s, %s, (formid %08x base form %08x head template form %08x)", a_skeleton->name.c_str(), a_skeleton->children._size, a_geometry->name.c_str(), name, a_skeleton->userData ? a_skeleton->userData->formID : 0x0, a_skeleton->userData ? a_skeleton->userData->data.objectReference->formID : 0x0, formId);

		SkinSingleHeadGeometryEvent e;
		e.skeleton = a_skeleton;
		e.geometry = a_geometry;
		e.headNode = this;
		g_skinSingleHeadGeometryEventDispatcher.dispatch(e);
	}

	void BSFaceGenNiNodeEx::SkinAllGeometry(ConsoleRE::NiNode* a_skeleton, uint32_t a_unk)
	{
		const char* name = "";
		uint32_t formId = 0x0;

		if (a_skeleton->userData && a_skeleton->userData->data.objectReference)
		{
			auto bname = skyrim_cast<ConsoleRE::TESFullName*>(a_skeleton->userData->data.objectReference);
			if (bname)
				name = bname->GetFullName();

			auto bnpc = skyrim_cast<ConsoleRE::TESNPC*>(a_skeleton->userData->data.objectReference);
			if (bnpc && bnpc->faceNPC)
			{
				formId = bnpc->faceNPC->formID;
			}
		}

		xUtilty::Log::GetSingleton()->Write(xUtilty::Log::logLevel::kNone,"SkinAllGeometry %s %d, %s, (formid %08x base form %08x head template form %08x)", a_skeleton->name.c_str(), a_skeleton->children._size, name, a_skeleton->userData ? a_skeleton->userData->formID : 0x0, a_skeleton->userData ? a_skeleton->userData->data.objectReference->formID : 0x0, formId);

		SkinAllHeadGeometryEvent e;
		e.skeleton = a_skeleton;
		e.headNode = this;
		g_skinAllHeadGeometryEventDispatcher.dispatch(e);

		_SkinAllGeometry(this, a_skeleton, a_unk);

		e.hasSkinned = true;
		g_skinAllHeadGeometryEventDispatcher.dispatch(e);
	}

	ConsoleRE::NiAVObject* unk001CB0E0(const void* a_this, ConsoleRE::NiNode* armor, ConsoleRE::NiNode* skeleton, void* unk3, char unk4, char unk5, void* unk6)
	{
		ArmorAttachEvent event;
		event.armorModel = armor;
		event.skeleton = skeleton;
		g_armorAttachEventDispatcher.dispatch(event);

		auto ret = _unk001CB0E0(a_this, armor, skeleton, unk3, unk4, unk5, unk6);

		if (ret)
		{
			event.attachedNode = ret;
			event.hasAttached = true;
			g_armorAttachEventDispatcher.dispatch(event);
		}

		return ret;
	}

	void Update(ConsoleRE::Main* const a_this)
	{
		a_this->Update();

		if (a_this->quitGame)
		{
			g_shutdownEventDispatcher.dispatch(ShutdownEvent());
		}
		else
		{
			FrameEvent e;
			e.gamePaused = a_this->freezeTime;
			g_frameEventDispatcher.dispatch(e);
		}
	}

	void hookFaceGen()
	{
		auto& localTrampoline = API::GetTrampoline();

		_SkinSingleGeometry = localTrampoline.write_call<5>(SkinSingleGeometryCodeHook1.address(), stl::unrestricted_cast<uintptr_t>(&BSFaceGenNiNodeEx::SkinSingleGeometry));
		_SkinSingleGeometry = localTrampoline.write_call<5>(SkinSingleGeometryCodeHook2.address(), stl::unrestricted_cast<uintptr_t>(&BSFaceGenNiNodeEx::SkinSingleGeometry));

		//
		REL::Relocation<uintptr_t> vtbl(0x1D45BC0);
		_SkinAllGeometry = vtbl.write_vfunc(0x3F, stl::unrestricted_cast<uintptr_t>(&BSFaceGenNiNodeEx::SkinAllGeometry));

		//
		REL::safe_write(_SkinSingleGeometry.address() + 0x9B, static_cast<uint8_t>(0x7));

		struct StaticCode : Xbyak::CodeGenerator
		{
			StaticCode() : CodeGenerator()
			{
				Xbyak::Label lbl;

				mov(r15d, ptr[rcx + 0x58]);
				
				cmp(r15d, 9);
				jl(lbl);
				mov(r15d, 8);

				//
				L(lbl);
				test(r15, r15);
				jmp(ptr[rip]);
				dq(BoneLimit.address() + 0x7);
			}
		};

		//
		StaticCode code;

		//
		localTrampoline.write_branch<6>(BoneLimit.address(), localTrampoline.allocate(code));
		REL::safe_write(BoneLimit.address() + 6, static_cast<uint8_t>(0x90));
	}

	void hookAttachArmor()
	{
		_unk001CB0E0 = API::GetTrampoline().write_call<5>(hookAttachArmorHook1.address(), unk001CB0E0);
		_unk001CB0E0 = API::GetTrampoline().write_call<5>(hookAttachArmorHook2.address(), unk001CB0E0);
	}

	void hookEngine()
	{
		API::GetTrampoline().write_call<5>(UpdateHook.address(), Update);
	}

	// only one that truely requires a detour/ Xbyak as it has 35+ xrefs.
	bool unequipItem(ConsoleRE::ActorEquipManager* const a_this, ConsoleRE::Actor* actor, ConsoleRE::TESForm* item, ConsoleRE::BaseExtraList* extraData, int32_t count, ConsoleRE::BGSEquipSlot* equipSlot, bool unkFlag1, bool preventEquip, bool unkFlag2, bool unkFlag3, void* unk)
	{
		ArmorDetachEvent event;
		event.actor = actor;
		g_armorDetachEventDispatcher.dispatch(event);

		auto ret = _UnequipObject(a_this, actor, item, extraData, count, equipSlot, unkFlag1, preventEquip, unkFlag2, unkFlag3, unk);

		event.hasDetached = true;
		g_armorDetachEventDispatcher.dispatch(event);
		return ret;
	}

	void hookDetachArmor()
	{
		auto& localTrampoline = API::GetTrampoline();

#if true
		_UnequipObject = localTrampoline.write_call<5>(DetachArmorhook1.address(), unequipItem);
		_UnequipObject = localTrampoline.write_call<5>(DetachArmorhook2.address(), unequipItem);
		_UnequipObject = localTrampoline.write_call<5>(DetachArmorhook3.address(), unequipItem);
		_UnequipObject = localTrampoline.write_call<5>(DetachArmorhook4.address(), unequipItem);
		_UnequipObject = localTrampoline.write_call<5>(DetachArmorhook5.address(), unequipItem);
		_UnequipObject = localTrampoline.write_call<5>(DetachArmorhook6.address(), unequipItem);
		_UnequipObject = localTrampoline.write_call<5>(DetachArmorhook7.address(), unequipItem);
		_UnequipObject = localTrampoline.write_call<5>(DetachArmorhook8.address(), unequipItem);
		_UnequipObject = localTrampoline.write_call<5>(DetachArmorhook9.address(), unequipItem);
		_UnequipObject = localTrampoline.write_call<5>(DetachArmorhook10.address(), unequipItem);
		_UnequipObject = localTrampoline.write_call<5>(DetachArmorhook11.address(), unequipItem);
		_UnequipObject = localTrampoline.write_call<5>(DetachArmorhook12.address(), unequipItem);
		_UnequipObject = localTrampoline.write_call<5>(DetachArmorhook13.address(), unequipItem);
		_UnequipObject = localTrampoline.write_call<5>(DetachArmorhook14.address(), unequipItem);
		_UnequipObject = localTrampoline.write_call<5>(DetachArmorhook15.address(), unequipItem);
		_UnequipObject = localTrampoline.write_call<5>(DetachArmorhook16.address(), unequipItem);
		_UnequipObject = localTrampoline.write_call<5>(DetachArmorhook17.address(), unequipItem);
		_UnequipObject = localTrampoline.write_call<5>(DetachArmorhook18.address(), unequipItem);
		_UnequipObject = localTrampoline.write_call<5>(DetachArmorhook19.address(), unequipItem);
		_UnequipObject = localTrampoline.write_call<5>(DetachArmorhook20.address(), unequipItem);
		_UnequipObject = localTrampoline.write_call<5>(DetachArmorhook21.address(), unequipItem);
		_UnequipObject = localTrampoline.write_call<5>(DetachArmorhook22.address(), unequipItem);
		_UnequipObject = localTrampoline.write_call<5>(DetachArmorhook23.address(), unequipItem);
		_UnequipObject = localTrampoline.write_call<5>(DetachArmorhook24.address(), unequipItem);
		_UnequipObject = localTrampoline.write_call<5>(DetachArmorhook25.address(), unequipItem);
		_UnequipObject = localTrampoline.write_call<5>(DetachArmorhook26.address(), unequipItem);
		_UnequipObject = localTrampoline.write_call<5>(DetachArmorhook27.address(), unequipItem);
		_UnequipObject = localTrampoline.write_call<5>(DetachArmorhook28.address(), unequipItem);
		_UnequipObject = localTrampoline.write_call<5>(DetachArmorhook29.address(), unequipItem);
		_UnequipObject = localTrampoline.write_call<5>(DetachArmorhook30.address(), unequipItem);
		_UnequipObject = localTrampoline.write_call<5>(DetachArmorhook31.address(), unequipItem);
		_UnequipObject = localTrampoline.write_call<5>(DetachArmorhook32.address(), unequipItem);
		_UnequipObject = localTrampoline.write_call<5>(DetachArmorhook33.address(), unequipItem);
		_UnequipObject = localTrampoline.write_call<5>(DetachArmorhook34.address(), unequipItem);
		_UnequipObject = localTrampoline.write_call<5>(DetachArmorhook35.address(), unequipItem);
#else
		struct DUMMY : Xbyak::CodeGenerator
		{
			DUMMY() : Xbyak::CodeGenerator()
			{
				//
				push(rbp);
				mov(rbp, rsp);
				push(r15);

				//
				jmp(ptr[rip]);
				dq(uintptr_t(unequipItem));
			}
		};

		auto& localTrampoline = API::GetTrampoline();

		//
		DUMMY code;

		//
		localTrampoline.write_branch<6>(UnequipHook.address(), localTrampoline.allocate(code));
#endif
	}

	void hookAll()
	{
		hookEngine();
		hookAttachArmor();
		hookDetachArmor();

		//
		hookFaceGen();
	}

	void unhookAll()
	{
	}
}
