#ifndef PCH_H
#define PCH_H

#define WIN32_LEAN_AND_MEAN

#define NOMB
#define NOMINMAX
#define NOSERVICE

#pragma warning(push)
#include "F4SE/F4SE.h"
#include "RE/Fallout.h"
#include "REX/REX/Singleton.h"

#include <spdlog/spdlog.h>

#pragma warning(pop)

#define DLLEXPORT __declspec(dllexport)

#include "OS/SysCall.h"
#include "ext/ICommon.h"
#include "ext/IErrors.h"
#include "ext/ITypes.h"

#include <ext/ID3D11.h>
#include <ext/IHook.h>
#include <ext/INIReader.h>
#include <ext/StrHelpers.h>
#include <ext/stl_containers.h>

#include <algorithm>
#include <atomic>
#include <filesystem>
#include <functional>
#include <map>
#include <mutex>
#include <shared_mutex>
#include <sstream>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include <d3d11.h>
#include <dxgi1_6.h>
#include <shlobj.h>
#include <wrl/client.h>

#include "common.h"
#include "getconfig.h"

#include "drv_base.h"

#include "dispatcher.h"

#include "data.h"

#include "config.h"
#include "drv_ids.h"
#include "events.h"
#include "game.h"
#include "havok.h"
#include "helpers.h"
#include "stats.h"
#include "misc.h"
#include "osd.h"
#include "resources.h"
#include "papyrus.h"
#include "render.h"
#include "window.h"

using namespace std::literals;

// Tier 0: Adapt logging to new commonlibf4 API. The old F4SE::log namespace and
// spdlog wrapper are gone — REX::LOG / REX::INFO / REX::WARN / REX::ERROR /
// REX::CRITICAL replace them. We keep a thin `logger` namespace so existing
// `logger::info(...)` call sites continue to work without a sweeping rewrite.
// Note: <wingdi.h> defines ERROR as a macro; undefine it before referencing
// REX::LOG_LEVEL::ERROR.
#ifdef ERROR
#	undef ERROR
#endif
namespace logger
{
	template <class... Args>
	inline void log(REX::LOG_LEVEL level, std::format_string<Args...> fmt, Args&&... args)
	{
		REX::LOG(std::source_location::current(), level, std::vformat(fmt.get(), std::make_format_args(args...)));
	}

	template <class... Args>
	inline void trace(std::format_string<Args...> fmt, Args&&... args)
	{
		log(REX::LOG_LEVEL::TRACE, fmt, std::forward<Args>(args)...);
	}

	template <class... Args>
	inline void debug(std::format_string<Args...> fmt, Args&&... args)
	{
		log(REX::LOG_LEVEL::DEBUG, fmt, std::forward<Args>(args)...);
	}

	template <class... Args>
	inline void info(std::format_string<Args...> fmt, Args&&... args)
	{
		log(REX::LOG_LEVEL::INFO, fmt, std::forward<Args>(args)...);
	}

	template <class... Args>
	inline void warn(std::format_string<Args...> fmt, Args&&... args)
	{
		log(REX::LOG_LEVEL::WARN, fmt, std::forward<Args>(args)...);
	}

	template <class... Args>
	inline void error(std::format_string<Args...> fmt, Args&&... args)
	{
		log(REX::LOG_LEVEL::ERROR, fmt, std::forward<Args>(args)...);
	}

	template <class... Args>
	inline void critical(std::format_string<Args...> fmt, Args&&... args)
	{
		log(REX::LOG_LEVEL::CRITICAL, fmt, std::forward<Args>(args)...);
	}
}

namespace stl
{
	[[noreturn]] inline void report_and_fail(std::string_view a_msg, std::source_location a_loc = std::source_location::current())
	{
		REX::IMPL::FAIL(a_loc, a_msg);
		std::abort();  // unreachable; keeps the [[noreturn]] contract for older toolchains
	}

	template <class T>
	void write_thunk_call(std::uintptr_t a_src)
	{
		auto& trampoline = REL::GetTrampoline();
		T::func = trampoline.write_call<5>(a_src, T::thunk);
	}

	template <class T>
	void write_thunk_jmp(std::uintptr_t a_src)
	{
		auto& trampoline = REL::GetTrampoline();
		T::func = trampoline.write_jmp<5>(a_src, T::thunk);
	}

	template <class F, size_t index, class T>
	void write_vfunc()
	{
		REL::Relocation<std::uintptr_t> vtbl{ F::VTABLE[index] };
		T::func = vtbl.write_vfunc(T::idx, T::thunk);
	}

	template <class F, class T>
	void write_vfunc()
	{
		write_vfunc<F, 0, T>();
	}
}

// Tier 0 compatibility shims: the new commonlibf4 dropped REL::safe_write,
// REL::safe_fill, and RE::GetINISettingAddr. Plugin code still relies on these
// helpers heavily; rather than rewrite each call site, we provide drop-in
// implementations here. These mirror the semantics of the old CommonLib API.
namespace REL
{
	inline void safe_write(std::uintptr_t a_dst, const void* a_src, std::size_t a_size)
	{
		DWORD oldProtect{};
		::VirtualProtect(reinterpret_cast<void*>(a_dst), a_size, PAGE_EXECUTE_READWRITE, &oldProtect);
		std::memcpy(reinterpret_cast<void*>(a_dst), a_src, a_size);
		::VirtualProtect(reinterpret_cast<void*>(a_dst), a_size, oldProtect, &oldProtect);
	}

	template <class T>
	inline void safe_write(std::uintptr_t a_dst, const T& a_value)
	{
		safe_write(a_dst, std::addressof(a_value), sizeof(T));
	}

	inline void safe_fill(std::uintptr_t a_dst, std::uint8_t a_value, std::size_t a_size)
	{
		DWORD oldProtect{};
		::VirtualProtect(reinterpret_cast<void*>(a_dst), a_size, PAGE_EXECUTE_READWRITE, &oldProtect);
		std::memset(reinterpret_cast<void*>(a_dst), a_value, a_size);
		::VirtualProtect(reinterpret_cast<void*>(a_dst), a_size, oldProtect, &oldProtect);
	}
}

namespace RE
{
	// Returns a pointer to the underlying value union member of the named INI
	// setting (offset 0x08 from RE::Setting; see RE/S/Setting.h). Caller is
	// responsible for matching T to the setting's actual storage type.
	template <class T>
	inline T* GetINISettingAddr(std::string_view a_name)
	{
		auto setting = RE::GetINISetting(a_name);
		if (!setting) {
			return nullptr;
		}
		return reinterpret_cast<T*>(reinterpret_cast<std::byte*>(setting) + 0x08);
	}
}

namespace RE
{
	using FormID = std::uint32_t;
	using RefHandle = std::uint32_t;
	using FormType = ENUM_FORM_ID;
}

#include "Version.h"

#endif  //PCH_H
