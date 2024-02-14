#include "PluginInterfaceImpl.h"

hdt::PluginInterfaceImpl hdt::g_pluginInterface;

void hdt::PluginInterfaceImpl::addListener(IPreStepListener* l)
{
	if (l)
	{
		m_preStepDispatcher.addListener(l);
	}
}

void hdt::PluginInterfaceImpl::removeListener(IPreStepListener* l)
{
	if (l)
	{
		m_preStepDispatcher.removeListener(l);
	}
}

void hdt::PluginInterfaceImpl::addListener(IPostStepListener* l)
{
	if (l)
	{
		m_postStepDispatcher.addListener(l);
	}
}

void hdt::PluginInterfaceImpl::removeListener(IPostStepListener* l)
{
	if (l)
	{
		m_postStepDispatcher.removeListener(l);
	}
}

void hdt::PluginInterfaceImpl::onPostPostLoad()
{
	//Send ourselves to any plugin that registered during the PostLoad event
	if (m_skseMessagingInterface)
	{
		m_skseMessagingInterface->DispatchLiseners(m_sksePluginHandle, PluginInterface::MSG_STARTUP, static_cast<PluginInterface*>(this), 0, nullptr);
	}
}

void hdt::PluginInterfaceImpl::init(const Interface::QueryInterface* skse)
{
	//We need to have our SKSE plugin handle and the messaging interface in order to reach our plugins later
	if (skse)
	{
		m_sksePluginHandle = skse->QueryHandle();
		m_skseMessagingInterface = reinterpret_cast<Interface::MessagingInterface*>(skse->QueryInterfaceFromID(Interface::MessagingInterface::TypeID));
	}
	if (!m_skseMessagingInterface)
	{
		xUtilty::Log::GetSingleton()->Write(xUtilty::Log::logLevel::kNone,"Failed to get a messaging interface. Plugins will not work.");
	}
}
