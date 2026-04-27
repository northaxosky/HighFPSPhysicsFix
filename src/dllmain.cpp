#include "PCH.h"

#include <xbyak/xbyak.h>

void MessageHandler(F4SE::MessagingInterface::Message* a_message)
{
	switch (a_message->type) {
	case F4SE::MessagingInterface::kPostPostLoad:
		{
			HFPF::IDDispatcher::InitializeDriversPost();
		}
		break;
	case F4SE::MessagingInterface::kGameDataReady:
		{
			HFPF::DWindow::SetupForceMinimizeMP();
			HFPF::DRender::Register();
		}
		break;
	case F4SE::MessagingInterface::kNewGame:
		{
			HFPF::DMisc::SetThreadsNG();
		}
		break;
	default:
		break;
	}
}

// OG F4SE (0.6.23) consumes F4SEPlugin_Query and ignores PluginVersionData.
// NG/AE F4SE consume F4SEPlugin_Version and ignore Query. Export both so the
// same DLL loads under any supported runtime; Address Library handles
// per-runtime address resolution.
extern "C" DLLEXPORT bool F4SEAPI F4SEPlugin_Query(const F4SE::QueryInterface*, F4SE::PluginInfo* a_info)
{
	a_info->infoVersion = F4SE::PluginInfo::kVersion;
	a_info->name = Version::PROJECT.data();
	a_info->version = Version::MAJOR;
	return true;
}

extern "C" DLLEXPORT constinit auto F4SEPlugin_Version = []() noexcept {
	F4SE::PluginVersionData data{};

	data.PluginVersion({ Version::MAJOR, Version::MINOR, Version::PATCH });
	data.PluginName(Version::PROJECT.data());
	data.AuthorName("AntoniX35");
	data.UsesAddressLibrary(true);
	data.UsesSigScanning(false);
	data.IsLayoutDependent(true);
	data.HasNoStructUse(false);
	// Dear-Modding's PluginVersionData helpers only set the NG (1.11.137+) bits.
	// F4SE OG 0.6.23 inspects bit (1<<1) for "address library for 1.10.163" and
	// the corresponding structureIndependence bit for layout compatibility.
	// Set those bits explicitly so OG accepts the plugin alongside NG/AE.
	data.addressIndependence |= (1u << 1);
	data.structureIndependence |= (1u << 1);
	data.CompatibleVersions({ F4SE::RUNTIME_1_10_163, F4SE::RUNTIME_LATEST });

	return data;
}();

extern "C" DLLEXPORT bool F4SEAPI F4SEPlugin_Load(const F4SE::LoadInterface* a_f4se)
{
	F4SE::InitInfo init{};
	init.logLevel = REX::LOG_LEVEL::INFO;
	init.trampoline = true;
	init.trampolineSize = 1 << 12;
	F4SE::Init(a_f4se, init);

	logger::info("{} v{}", Version::PROJECT, Version::NAME);

	const auto ver = a_f4se->RuntimeVersion();
	logger::info("Game version : {}", ver.string());

	// Tier 1: upper-bound runtime gate. We support OG (1.10.163), NG (1.10.984+),
	// and AE (1.11.x). Address Library hides the offset differences, but if a
	// future runtime ships with a fundamentally new layout, our hard-coded
	// behavioral assumptions (xbyak hook bodies, struct field offsets) may
	// silently mis-apply. Refuse to install hooks on anything we have not
	// validated rather than risk a crash in the user's first session.
	{
		const auto mver = REL::Module::GetSingleton()->version();
		const REL::Version k_min{ 1, 10, 163, 0 };
		const REL::Version k_max_exclusive{ 1, 20, 0, 0 };
		if (mver < k_min || !(mver < k_max_exclusive)) {
			logger::warn(
				"Unsupported Fallout 4 runtime {} (supported: 1.10.163, 1.10.984+ NG, 1.11.x AE). "
				"Hooks will NOT be installed.",
				mver.string());
			return false;
		}
		logger::info("Module version: {} ({})",
			mver.string(),
			REL::Module::IsRuntimeOG() ? "OG" : (REL::Module::IsRuntimeNG() ? "NG" : "AE"));
	}

	const auto messaging = F4SE::GetMessagingInterface();
	messaging->RegisterListener(MessageHandler);
	int result1 = HFPF::IConfig::LoadConfiguration();
	if (result1 != 0) {
		logger::warn("Unable to load HighFPSPhysicsFix.ini file. Line: ({})", result1);
	}

	if (HFPF::IConfig::IsCustomLoaded()) {
		logger::info("Custom configuration loaded");
	}
	if (!HFPF::IEvents::Initialize()) {
		return false;
	}
	if (!HFPF::IDDispatcher::InitializeDrivers()) {
		return false;
	}
	HFPF::IConfig::ClearConfiguration();
	return true;
}
