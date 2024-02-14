#pragma once

#include "HookEvents.h"
#include "IEventListener.h"

namespace hdt
{
	class EventDebugLogger: 
		public IEventListener<ArmorAttachEvent>, 
		public ConsoleRE::BSTEventSink<ConsoleRE::TESCellAttachDetachEvent>, 
		public ConsoleRE::BSTEventSink<ConsoleRE::TESMoveAttachDetachEvent>
	{
	protected:
		ConsoleRE::BSEventNotifyControl ProcessEvent(const ConsoleRE::TESCellAttachDetachEvent* evn, ConsoleRE::BSTEventSource<ConsoleRE::TESCellAttachDetachEvent>* dispatcher) override;
		ConsoleRE::BSEventNotifyControl ProcessEvent(const ConsoleRE::TESMoveAttachDetachEvent* evn, ConsoleRE::BSTEventSource<ConsoleRE::TESMoveAttachDetachEvent>* dispatcher) override;

		void onEvent(const ArmorAttachEvent&) override;
	};
}
