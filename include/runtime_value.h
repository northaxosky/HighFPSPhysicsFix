#pragma once

#include <cstdint>

#include "REL/Module.h"

namespace HFPF
{
	// RuntimeOffset: holds an offset value per supported runtime (OG, NG, AE) and
	// implicitly converts to the active-runtime value at use site. AE/NG share IDs
	// in upstream's design, so a 2-arg form { og, ae } is provided where the AE
	// value is also used for NG.
	struct RuntimeOffset
	{
		std::uintptr_t og{};
		std::uintptr_t ng{};
		std::uintptr_t ae{};

		constexpr RuntimeOffset(std::uintptr_t v) noexcept :
			og(v), ng(v), ae(v) {}

		constexpr RuntimeOffset(std::uintptr_t a_og, std::uintptr_t a_ae) noexcept :
			og(a_og), ng(a_ae), ae(a_ae) {}

		constexpr RuntimeOffset(std::uintptr_t a_og, std::uintptr_t a_ng, std::uintptr_t a_ae) noexcept :
			og(a_og), ng(a_ng), ae(a_ae) {}

		[[nodiscard]] std::uintptr_t value() const noexcept
		{
			const auto idx = static_cast<std::uint8_t>(REL::Module::GetRuntimeIndex());
			switch (idx) {
			case 0:  return og;
			case 1:  return ng;
			default: return ae;
			}
		}

		operator std::uintptr_t() const noexcept   { return value(); }
		operator std::ptrdiff_t() const noexcept   { return static_cast<std::ptrdiff_t>(value()); }
	};
}
