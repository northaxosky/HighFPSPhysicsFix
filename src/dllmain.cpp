#include "PCH.h"

#include <xbyak/xbyak.h>

#include <spdlog/sinks/msvc_sink.h>
#include <spdlog/sinks/rotating_file_sink.h>

namespace
{
	// Tier 3: read [Debug] LogLevel from HighFPSPhysicsFix.ini next to the DLL.
	// We parse this *before* F4SE::Init so the rotating logger picks it up.
	spdlog::level::level_enum ReadInitialLogLevel()
	{
		std::filesystem::path iniPath;
		try {
			wchar_t buf[MAX_PATH]{};
			HMODULE hSelf = nullptr;
			if (::GetModuleHandleExW(
					GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
					reinterpret_cast<LPCWSTR>(&ReadInitialLogLevel),
					&hSelf) &&
				::GetModuleFileNameW(hSelf, buf, MAX_PATH) > 0) {
				iniPath = std::filesystem::path(buf).parent_path() / L"HighFPSPhysicsFix.ini";
			}
		} catch (const std::exception&) {
			return spdlog::level::info;
		}

		if (iniPath.empty() || !std::filesystem::exists(iniPath)) {
			return spdlog::level::info;
		}

		INIReader reader(iniPath.string());
		if (reader.ParseError() < 0) {
			return spdlog::level::info;
		}

		const std::string raw = reader.Get("Debug", "LogLevel", "info");
		auto parsed = spdlog::level::from_str(raw);
		// from_str returns level::off for unknown strings; don't silently mute.
		if (parsed == spdlog::level::off && raw != "off") {
			return spdlog::level::info;
		}
		return parsed;
	}

	// Tier 3: replace F4SE::Init's basic_file_sink with a rotating sink
	// (5 MB x 3 files) writing to Documents\My Games\Fallout4\F4SE.
	void InstallRotatingLogger(spdlog::level::level_enum a_level)
	{
		try {
			wchar_t* known = nullptr;
			if (FAILED(::SHGetKnownFolderPath(FOLDERID_Documents, KF_FLAG_DEFAULT, nullptr, &known)) || !known) {
				if (known) {
					::CoTaskMemFree(known);
				}
				return;
			}
			std::filesystem::path path = known;
			::CoTaskMemFree(known);

			path /= L"My Games";
			path /= L"Fallout4";
			path /= L"F4SE";
			std::error_code ec;
			std::filesystem::create_directories(path, ec);
			path /= L"HighFPSPhysicsFix.log";

			constexpr std::size_t kMaxSize  = 5ull * 1024ull * 1024ull;  // 5 MiB per file
			constexpr std::size_t kMaxFiles = 3;                         // .log + .1.log + .2.log

			std::vector<spdlog::sink_ptr> sinks{
				std::make_shared<spdlog::sinks::msvc_sink_mt>(),
				std::make_shared<spdlog::sinks::rotating_file_sink_mt>(
					path.string(), kMaxSize, kMaxFiles, /*rotate_on_open=*/false),
			};

			auto log = std::make_shared<spdlog::logger>("global", sinks.begin(), sinks.end());
			log->set_level(a_level);
			log->flush_on(a_level);
			spdlog::set_default_logger(std::move(log));
			spdlog::set_pattern("[%T.%e] [%=5t] [%L] %v");
		} catch (const std::exception&) {
			// Logger setup failure is non-fatal; fall back to whatever F4SE
			// configured. We deliberately swallow here to avoid taking down
			// plugin load over a logging issue.
		}
	}

	REX::ELogLevel ToRexLevel(spdlog::level::level_enum a_lvl)
	{
		switch (a_lvl) {
		case spdlog::level::trace:    return REX::ELogLevel::Trace;
		case spdlog::level::debug:    return REX::ELogLevel::Debug;
		case spdlog::level::warn:     return REX::ELogLevel::Warning;
		case spdlog::level::err:      return REX::ELogLevel::Error;
		case spdlog::level::critical: return REX::ELogLevel::Critical;
		case spdlog::level::off:      return REX::ELogLevel::Critical;
		case spdlog::level::info:
		default:                      return REX::ELogLevel::Info;
		}
	}
}

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
	// OG (1.10.163) reads F4SEPlugin_Query and ignores PluginVersionData,
	// so only NG/AE runtimes need to be listed here.
	data.CompatibleVersions({
		F4SE::RUNTIME_1_10_980,  // NG (Next-Gen Update)
		F4SE::RUNTIME_1_10_984,  // NG (Next-Gen Update)
		F4SE::RUNTIME_LATEST,    // AE (1.11.191)
	});

	return data;
}();

extern "C" DLLEXPORT bool F4SEAPI F4SEPlugin_Load(const F4SE::LoadInterface* a_f4se)
{
	const auto logLevel = ReadInitialLogLevel();

	F4SE::InitInfo init{};
	// Suppress F4SE::Init's default basic_file_sink; we install a rotating
	// sink (5 MiB x 3) below so the log can't grow unbounded.
	init.log = false;
	init.logLevel = ToRexLevel(logLevel);
	init.trampoline = true;
	init.trampolineSize = 1 << 12;
	F4SE::Init(a_f4se, init);

	InstallRotatingLogger(logLevel);

	logger::info("{} v{}", Version::PROJECT, Version::DESCRIBE);
	logger::info("Log level: {}", spdlog::level::to_string_view(logLevel));

	const auto ver = a_f4se->RuntimeVersion();
	logger::info("Game version : {}", ver.string());

	// Tier 1: upper-bound runtime gate. We support OG (1.10.163), NG (1.10.984+),
	// and AE (1.11.x). Address Library hides the offset differences, but if a
	// future runtime ships with a fundamentally new layout, our hard-coded
	// behavioral assumptions (xbyak hook bodies, struct field offsets) may
	// silently mis-apply. Refuse to install hooks on anything we have not
	// validated rather than risk a crash in the user's first session.
	{
		const auto mver = REX::FModule::GetExecutingModule().GetFileVersion();
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
			REX::FModule::IsRuntimeOG() ? "OG" : (REX::FModule::IsRuntimeNG() ? "NG" : "AE"));
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
