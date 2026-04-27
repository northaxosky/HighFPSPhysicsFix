#include "PCH.h"

namespace HFPF
{
	IEvents IEvents::m_Instance;

	bool IEvents::Initialize()
	{
		if (!Hook::Call5(
				REL::GetTrampoline(),
				LoadPluginINI_C.address(),
				reinterpret_cast<std::uintptr_t>(PostLoadPluginINI_Hook),
				m_Instance.LoadPluginINI_O)) {
			// Non-fatal: this hook target's address is not currently validated
			// for every runtime (notably AE 1.11.x). Without this hook the
			// OnConfigLoad event will not fire from the game side, which
			// affects late-binding renderer reconfiguration but does not
			// break core physics/havok patches. Returning false here would
			// cause F4SE to mark the plugin as incompatible and skip it
			// entirely, so log and continue instead.
			logger::warn("[Events] Could not install LoadPluginINI hook on this runtime - continuing without OnConfigLoad event");
			return true;
		}

		logger::info("[Events] Installed event hooks");

		return true;
	}

	void IEvents::RegisterForEvent(Event a_code, EventCallback a_fn)
	{
		m_Instance.m_events.try_emplace(a_code).first->second.emplace_back(a_code, a_fn);
	}

	void IEvents::TriggerEvent(Event a_code, void* a_args)
	{
		const auto it = m_Instance.m_events.find(a_code);
		if (it == m_Instance.m_events.end())
			return;

		for (const auto& evtd : it->second)
			evtd.m_callback(a_code, a_args);
	}

	void IEvents::PostLoadPluginINI_Hook(void* a_unk)
	{
		m_Instance.LoadPluginINI_O(a_unk);
		IDDispatcher::DriversOnGameConfigLoaded();
		m_Instance.TriggerEvent(Event::OnConfigLoad);
	}
}
