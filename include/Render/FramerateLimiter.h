#pragma once

// Tier 4: waitable-object FPS limiter.
//
// CreateWaitableTimerExW(... CREATE_WAITABLE_TIMER_HIGH_RESOLUTION ...) gives
// us ~0.5ms wake granularity on Windows 10 1803+ (vs the legacy ~15.6ms tick
// floor). We use a relative SetWaitableTimer + WaitForSingleObject for the
// bulk of the wait, then a tiny QPC-driven microspin to converge on the
// deadline. If the high-resolution timer can't be created on this platform
// we fall back to Sleep(1) + spin against QPC, preserving the original
// frametime targeting semantics.

// CREATE_WAITABLE_TIMER_HIGH_RESOLUTION was added in the Windows 10 1803
// SDK. Define it manually if the toolchain SDK is older — the kernel will
// just reject the flag at runtime and we'll fall through to the legacy
// timer or the Sleep+spin path.
#ifndef CREATE_WAITABLE_TIMER_HIGH_RESOLUTION
#	define CREATE_WAITABLE_TIMER_HIGH_RESOLUTION 0x00000002
#endif

namespace HFPF
{
	class FramerateLimiter
	{
	public:
		FramerateLimiter() :
			m_lastTimePoint(IPerfCounter::Query()),
			m_hTimer(::CreateWaitableTimerExW(
				nullptr, nullptr,
				CREATE_WAITABLE_TIMER_HIGH_RESOLUTION,
				TIMER_ALL_ACCESS))
		{
			if (!m_hTimer) {
				// Pre-1803: retry without the high-res flag. Granularity
				// will be coarser but the API still works.
				m_hTimer = ::CreateWaitableTimerExW(
					nullptr, nullptr, 0, TIMER_ALL_ACCESS);
			}
		}

		~FramerateLimiter()
		{
			if (m_hTimer) {
				::CloseHandle(m_hTimer);
				m_hTimer = nullptr;
			}
		}

		FramerateLimiter(const FramerateLimiter&) = delete;
		FramerateLimiter& operator=(const FramerateLimiter&) = delete;

		void Wait(long long a_limit)
		{
			auto       now      = IPerfCounter::Query();
			const auto interval = IPerfCounter::delta_us(m_lastTimePoint, now);
			const auto waitTime = a_limit - interval;

			if (waitTime > 0) {
				const long long deadline = now + IPerfCounter::T(waitTime);

				if (m_hTimer) {
					WaitWithTimer(waitTime, deadline);
				} else {
					WaitFallback(deadline);
				}

				now = IPerfCounter::Query();
				if (now < deadline) {
					now = deadline;
				}
			}

			m_lastTimePoint = now;
		}

	private:
		// Relative wait via SetWaitableTimer (negative liDueTime == relative,
		// in 100ns units). We aim a hair short of the deadline and finish
		// with a microspin to keep frame pacing tight.
		SKMP_FORCEINLINE void WaitWithTimer(long long a_waitTime_us, long long a_deadline_qpc)
		{
			constexpr long long kMicrospinReserve_us = 250;

			long long sleepBudget_us = a_waitTime_us;
			if (sleepBudget_us > kMicrospinReserve_us * 2) {
				sleepBudget_us -= kMicrospinReserve_us;
			}

			LARGE_INTEGER dueTime;
			dueTime.QuadPart = -(sleepBudget_us * 10LL);

			if (::SetWaitableTimer(m_hTimer, &dueTime, 0, nullptr, nullptr, FALSE) != FALSE) {
				::WaitForSingleObject(m_hTimer, INFINITE);
			}

			while (IPerfCounter::Query() < a_deadline_qpc) {
				_mm_pause();
			}
		}

		// Sleep(1)+spin fallback used only when the waitable-timer API
		// completely fails (e.g. handle creation failed). Mirrors the prior
		// limiter's pacing behaviour.
		SKMP_FORCEINLINE void WaitFallback(long long a_deadline_qpc)
		{
			while (true) {
				const auto t = IPerfCounter::Query();
				if (t >= a_deadline_qpc) {
					break;
				}

				const long long remaining_us = IPerfCounter::delta_us(t, a_deadline_qpc);
				if (remaining_us > 1500) {
					::Sleep(1);
				} else {
					_mm_pause();
				}
			}
		}

		long long m_lastTimePoint;
		HANDLE    m_hTimer;
	};
}
