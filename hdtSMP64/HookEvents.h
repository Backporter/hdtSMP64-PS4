#pragma once

#include "EventDispatcherImpl.h"

namespace hdt
{
	struct SkinAllHeadGeometryEvent
	{
		ConsoleRE::NiNode* skeleton = nullptr;
		ConsoleRE::BSFaceGenNiNode* headNode = nullptr;
		bool hasSkinned = false;
	};

	struct SkinSingleHeadGeometryEvent
	{
		ConsoleRE::NiNode* skeleton = nullptr;
		ConsoleRE::BSFaceGenNiNode* headNode = nullptr;
		ConsoleRE::BSGeometry* geometry = nullptr;
	};

	struct ArmorAttachEvent
	{
		ConsoleRE::NiNode* armorModel = nullptr;
		ConsoleRE::NiNode* skeleton = nullptr;
		ConsoleRE::NiAVObject* attachedNode = nullptr;
		bool hasAttached = false;
	};

	struct ArmorDetachEvent
	{
		ConsoleRE::Actor* actor = nullptr;
		bool hasDetached = false;
	};

	struct FrameEvent
	{
		bool gamePaused;
	};

	struct ShutdownEvent
	{
	};

	extern EventDispatcherImpl<FrameEvent> g_frameEventDispatcher;
	extern EventDispatcherImpl<ShutdownEvent> g_shutdownEventDispatcher;
	extern EventDispatcherImpl<ArmorAttachEvent> g_armorAttachEventDispatcher;
	extern EventDispatcherImpl<ArmorDetachEvent> g_armorDetachEventDispatcher;
	extern EventDispatcherImpl<SkinAllHeadGeometryEvent> g_skinAllHeadGeometryEventDispatcher;
	extern EventDispatcherImpl<SkinSingleHeadGeometryEvent> g_skinSingleHeadGeometryEventDispatcher;
}
