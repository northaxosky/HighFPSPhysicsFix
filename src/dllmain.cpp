#include "PCH.h"

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
