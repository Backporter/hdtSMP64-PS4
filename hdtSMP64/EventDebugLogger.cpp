#include "EventDebugLogger.h"

namespace hdt
{
	ConsoleRE::BSEventNotifyControl EventDebugLogger::ProcessEvent(const ConsoleRE::TESCellAttachDetachEvent* evn, ConsoleRE::BSTEventSource<ConsoleRE::TESCellAttachDetachEvent>* dispatcher)
	{
#ifdef _DEBUG
		if (evn && evn->reference && evn->reference->formType == ConsoleRE::Character::TypeID)
		{
			xUtilty::Log::GetSingleton()->Write(xUtilty::Log::logLevel::kNone,"received TESCellAttachDetachEvent(formID %08llX, name %s, attached=%s)", evn->reference->formID, evn->reference->data.objectReference->GetFormEditorID(), evn->attached ? "true" : "false");
		}
#endif // _DEBUG
		return ConsoleRE::BSEventNotifyControl::kContinue;
	}

	ConsoleRE::BSEventNotifyControl EventDebugLogger::ProcessEvent(const ConsoleRE::TESMoveAttachDetachEvent* evn, ConsoleRE::BSTEventSource<ConsoleRE::TESMoveAttachDetachEvent>* dispatcher)
	{
#ifdef _DEBUG
		if (evn && evn->movedRef && evn->movedRef->formType == ConsoleRE::Character::TypeID)
		{
			xUtilty::Log::GetSingleton()->Write(xUtilty::Log::logLevel::kNone,"received TESMoveAttachDetachEvent(formID %08llX, name %s, attached=%s)", evn->movedRef->formID, evn->movedRef->data.objectReference->GetFormEditorID(), evn->isCellAttached ? "true" : "false");
		}
#endif // _DEBUG
		return ConsoleRE::BSEventNotifyControl::kContinue;
	}

	void EventDebugLogger::onEvent(const ArmorAttachEvent& e)
	{
#ifdef _DEBUG
		xUtilty::Log::GetSingleton()->Write(xUtilty::Log::logLevel::kNone,
			"received ArmorAttachEvent(armorModel=%s (%016llX), skeleton=%s (%016llX), attachedNode=%s (%016llX), hasAttached=%s)",
			e.armorModel ? e.armorModel->name.c_str() : "null",
			(uintptr_t)e.armorModel,
			e.skeleton ? e.skeleton->name.c_str() : "null",
			(uintptr_t)e.skeleton,
			e.attachedNode ? e.attachedNode->name.c_str() : "null",
			(uintptr_t)e.attachedNode,
			static_cast<uintptr_t>(e.hasAttached) ? "true" : "false");
#endif // _DEBUG
	}
}
