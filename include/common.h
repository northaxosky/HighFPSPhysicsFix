#pragma once

#define FN_ESSENTIAL(x) \
	virtual bool IsEssential() const { return x; };
#define FN_DRVDEF(x)                                     \
	virtual int       GetPriority() const { return x; }; \
	virtual DRIVER_ID GetID() const noexcept { return std::remove_reference_t<decltype(*this)>::ID; }

namespace HFPF
{
	// Convert a target framerate to the corresponding frame interval in
	// microseconds. Centralised so the cast/precision contract is consistent
	// across the limiter, OSD and miscellaneous timers.
	[[nodiscard]] SKMP_FORCEINLINE constexpr std::int64_t FpsToMicros(double a_fps) noexcept
	{
		return a_fps > 0.0 ?
		           static_cast<std::int64_t>(1'000'000.0 / a_fps) :
		           0;
	}
}

