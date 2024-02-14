#pragma once

#include "EventDispatcherImpl.h"
#include "PluginAPI.h"

namespace hdt
{
	class PluginInterfaceImpl final : public PluginInterface
	{
	public:
		PluginInterfaceImpl() = default;
		~PluginInterfaceImpl() = default;

		PluginInterfaceImpl(const PluginInterfaceImpl&) = delete;
		PluginInterfaceImpl& operator=(const PluginInterfaceImpl&) = delete;

		virtual const VersionInfo& getVersionInfo() const { return m_versionInfo; }

		virtual void addListener(IPreStepListener* l) override;
		virtual void removeListener(IPreStepListener* l) override;

		virtual void addListener(IPostStepListener* l) override;
		virtual void removeListener(IPostStepListener* l) override;

		void onPostPostLoad();

		void onPreStep(const PreStepEvent& e) { m_preStepDispatcher.dispatch(e); }
		void onPostStep(const PostStepEvent& e) { m_postStepDispatcher.dispatch(e); }

		void init(const Interface::QueryInterface* skse);

	private:
		VersionInfo m_versionInfo{ INTERFACE_VERSION, BULLET_VERSION };
		EventDispatcherImpl<PreStepEvent> m_preStepDispatcher;
		EventDispatcherImpl<PostStepEvent> m_postStepDispatcher;

		size_t m_sksePluginHandle;
		Interface::MessagingInterface* m_skseMessagingInterface;
	};

	extern PluginInterfaceImpl g_pluginInterface;
}