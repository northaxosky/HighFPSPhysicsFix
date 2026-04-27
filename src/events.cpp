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
			const auto addr = LoadPluginINI_C.address();
			const auto base = REX::FModule::GetExecutingModule().GetBaseAddress();
			const auto firstByte = addr ? *reinterpret_cast<const std::uint8_t*>(addr) : 0;
			logger::critical(
				"[Events] LoadPluginINI hook target 0x{:X} (Fallout4.exe+0x{:X}) byte=0x{:02X} (expected 0xE8 CALL)",
				static_cast<std::uintptr_t>(addr),
				static_cast<std::uintptr_t>(addr - base),
				firstByte);
			return false;
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
