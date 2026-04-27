#include "PCH.h"

#include <xbyak/xbyak.h>

// Tier 0 — xbyak hook bodies (havok.cpp)
//
// Several hooks need per-runtime variants because the surrounding compiled
// code at the patch site uses different registers / immediate widths between
// OG (1.10.163) and NG/AE (1.10.984+ / 1.11.x). Per-runtime branching is
// done at code-gen time via REX::FModule::IsRuntimeOG(); NG follows AE.
//
// Audited differences (OG vs NG/AE), based on the upstream/1-10-163 source:
//   FixWhiteScreen safe_fill   : 0x3C bytes (OG) vs 0x35 bytes (NG/AE)
//   FixStutter1                : xmm3 (OG) vs xmm5 (NG/AE)
//   FixStutter2                : xmm2 + retn+0x6, NOP@0x5 / NOP4@0xE
//                                vs xmm4 + retn+0x8, NOP3@0x5 / NOP8@0x10
//   FixRotationSpeed1          : ptr[rbx+0x38] + dq(&Magic1/2) (OG) vs
//                                ptr[rdi+0x38] + dd(constants)  (NG/AE)
//   FixWSRotation              : mulss xmm1 (OG) vs mulss xmm0 (NG/AE)
//   FixLoadModelSpeed3         : [rbx+0x26C] + xmm8 (OG) vs
//                                [rdi+0x26C] + xmm7 (NG/AE)

namespace HFPF
{
	// Magic constants used by the OG xbyak FixRotationSpeed1 path. The OG
	// branch stored these as globals and embedded their addresses into the
	// generated code via `dq(uintptr_t(MagicN))`; NG/AE inline the values
	// directly with `dd(0x...)`. Defined as inline 32-bit ints so that
	// `&Magic1` is a stable address whose lower 32 bits are loaded by the
	// 4-byte movss in the OG hook body. Bit values come from the named
	// constants on DHavok (see include/havok.h).
	static inline std::int32_t Magic1 = static_cast<std::int32_t>(DHavok::kBits_58_8235);     //  58.8235f
	static inline std::int32_t Magic2 = static_cast<std::int32_t>(DHavok::kBits_Neg58_8235);  // -58.8235f


	static constexpr const char* SECTION_MAIN = "Main";
	static constexpr const char* CKEY_UNTIE = "UntieSpeedFromFPS";
	static constexpr const char* CKEY_COUNTER = "CounterRounding";

	static constexpr const char* SECTION_FIXES = "Fixes";
	static constexpr const char* CKEY_FIXSTUTTER = "FixStuttering";
	static constexpr const char* CKEY_FIXWHITE = "FixWhiteScreen";
	static constexpr const char* CKEY_FIXWIND = "FixWindSpeed";
	static constexpr const char* CKEY_FIXROTATION = "FixRotationSpeed";
	static constexpr const char* CKEY_FIXSITROTATION = "FixSittingRotationSpeed";
	static constexpr const char* CKEY_FIXWSROTATION = "FixWorkshopRotationSpeed";
	static constexpr const char* CKEY_FIXLOADINGMODEL = "FixLoadingModel";
	static constexpr const char* CKEY_FIXSTUCK = "FixStuckAnimation";
	static constexpr const char* CKEY_FIXRESPONSIVE = "FixMotionResponsive";

	DHavok DHavok::m_Instance;

	void DHavok::LoadConfig()
	{
		m_conf.untie_game_speed = GetConfigValue(SECTION_MAIN, CKEY_UNTIE, true);
		m_conf.fix_stuttering = GetConfigValue(SECTION_FIXES, CKEY_FIXSTUTTER, true);
		m_conf.fix_white_screen = GetConfigValue(SECTION_FIXES, CKEY_FIXWHITE, true);
		m_conf.fix_wind_speed = GetConfigValue(SECTION_FIXES, CKEY_FIXWIND, true);
		m_conf.fix_rot_speed = GetConfigValue(SECTION_FIXES, CKEY_FIXROTATION, true);
		m_conf.fix_sit_rot_speed = GetConfigValue(SECTION_FIXES, CKEY_FIXSITROTATION, true);
		m_conf.fix_ws_rot_speed = GetConfigValue(SECTION_FIXES, CKEY_FIXWSROTATION, true);
		m_conf.fix_load_model = GetConfigValue(SECTION_FIXES, CKEY_FIXLOADINGMODEL, true);
		m_conf.fix_stuck_anim = GetConfigValue(SECTION_FIXES, CKEY_FIXSTUCK, true);
		m_conf.fix_responsive = GetConfigValue(SECTION_FIXES, CKEY_FIXRESPONSIVE, true);
	}

	void DHavok::PostLoadConfig()
	{
	}

	void DHavok::RegisterHooks()
	{
		if (m_conf.fix_load_model) {
			IEvents::RegisterForEvent(Event::OnConfigLoad, PostConfigLoad);
		}
	}

	void DHavok::OnGameConfigLoaded()
	{
		if (m_conf.fix_load_model) {
			m_gv.fLoadingModel_TriggerZoomSpeed = RE::GetINISettingAddr<float>("fLoadingModel_TriggerZoomSpeed:Interface");
			if (!m_gv.fLoadingModel_TriggerZoomSpeed) {
				logger::warn("[Havok] INI setting 'fLoadingModel_TriggerZoomSpeed:Interface' missing; FixLoadingModel will be disabled.");
				m_conf.fix_load_model = false;
				return;
			}

			m_gv.fLoadingModel_MouseToRotateSpeed = RE::GetINISettingAddr<float>("fLoadingModel_MouseToRotateSpeed:Interface");
			if (!m_gv.fLoadingModel_MouseToRotateSpeed) {
				logger::warn("[Havok] INI setting 'fLoadingModel_MouseToRotateSpeed:Interface' missing; FixLoadingModel will be disabled.");
				m_conf.fix_load_model = false;
				return;
			}
		}
	}

	void DHavok::PostConfigLoad(Event code, void* data)
	{
		if (!m_Instance.m_conf.fix_load_model) {
			return;
		}
		m_Instance.Patch_FixLoadModelSpeed1();
		m_Instance.Patch_FixLoadModelSpeed2();
	}
	
	void DHavok::Patch()
	{
		if (m_conf.untie_game_speed) {
			REL::safe_write(
				Untie.address(),
				reinterpret_cast<const void*>(Payloads::untie_patch),
				sizeof(Payloads::untie_patch));
		}
		if (m_conf.fix_stuttering) {
			Patch_FixStuttering();
		}
		if (m_conf.fix_white_screen) {
			REL::safe_fill(FixWhiteScreen.address(), Payloads::NOP, REX::FModule::IsRuntimeOG() ? 0x3C : 0x35);
		}
		if (m_conf.fix_wind_speed) {
			Patch_FixWindSpeed();
		}
		if (m_conf.fix_rot_speed) {
			Patch_FixRotationSpeed();
		}
		if (m_conf.fix_sit_rot_speed) {
			Patch_FixSittingRotation();
		}
		if (m_conf.fix_ws_rot_speed) {
			Patch_FixWSRotation();
		}
		if (m_conf.fix_stuck_anim) {
			Patch_FixStuck();
		}
		if (m_conf.fix_responsive) {
			REL::safe_write(FixMotionFeedback.address(), &Payloads::JMP8, 0x1);
		}
	}

	bool DHavok::Prepare()
	{
		return true;
	}

	void DHavok::Patch_FixStuttering()
	{
		{
			struct FixStutter1 : Xbyak::CodeGenerator
			{
				FixStutter1(std::uintptr_t retnAddr)
				{
					Xbyak::Label retnLabel;
					Xbyak::Label magicLabel;

					if (REX::FModule::IsRuntimeOG()) {
						movss(xmm3, dword[rip + magicLabel]);
						cvttss2si(rcx, xmm3);
					} else {
						movss(xmm5, dword[rip + magicLabel]);
						cvttss2si(rcx, xmm5);
					}

					jmp(ptr[rip + retnLabel]);

					L(retnLabel);
					dq(retnAddr + 0x5);

					L(magicLabel);
					dd(DHavok::kBits_One);  // 1.0f
				}
			};
			logger::info("[Havok] [Patch] [Fix stuttering] patching...");
			{
				FixStutter1 code(FixStuttering1.address());
				code.ready();

				auto& trampoline = REL::GetTrampoline();
				trampoline.write_jmp<5>(
					FixStuttering1.address(),
					trampoline.allocate(code));
			}
		}
		REL::safe_write(FixStuttering2.address(), &Payloads::NOP8, 0x8);
		{
			struct FixStutter2 : Xbyak::CodeGenerator
			{
				FixStutter2(std::uintptr_t retnAddr)
				{
					Xbyak::Label retnLabel;

					if (REX::FModule::IsRuntimeOG()) {
						movss(xmm2, xmm6);
					} else {
						movss(xmm4, xmm6);
					}

					jmp(ptr[rip + retnLabel]);

					L(retnLabel);
					dq(retnAddr + (REX::FModule::IsRuntimeOG() ? 0x6 : 0x8));
				}
			};
			{
				FixStutter2 code(FixStuttering3.address());
				code.ready();

				auto& trampoline = REL::GetTrampoline();
				trampoline.write_jmp<5>(
					FixStuttering3.address(),
					trampoline.allocate(code));
			}
			if (REX::FModule::IsRuntimeOG()) {
				REL::safe_write(FixStuttering3.address() + 0x5, &Payloads::NOP, 0x1);
			} else {
				REL::safe_write(FixStuttering3.address() + 0x5, &Payloads::NOP3, 0x3);
			}
		}
		if (REX::FModule::IsRuntimeOG()) {
			REL::safe_write(FixStuttering3.address() + 0xE, &Payloads::NOP4, 0x4);
		} else {
			REL::safe_write(FixStuttering3.address() + 0x10, &Payloads::NOP8, 0x8);
		}
		{
			//fix moving objects
			struct FixStutter3 : Xbyak::CodeGenerator
			{
				FixStutter3(std::uintptr_t retnAddr)
				{
					Xbyak::Label retnLabel;
					Xbyak::Label magicLabel;

					movss(xmm1, dword[rip + magicLabel]);

					jmp(ptr[rip + retnLabel]);

					L(retnLabel);
					dq(retnAddr + 0x5);

					L(magicLabel);
					dd(DHavok::kBits_OneOver60);  // 1.0f/60.0f
				}
			};
			{
				FixStutter3 code(FixObjectsTransfer.address());
				code.ready();

				auto& trampoline = REL::GetTrampoline();
				trampoline.write_jmp<5>(
					FixObjectsTransfer.address(),
					trampoline.allocate(code));
			}
			logger::info("[Havok] [Patch] [Fix stuttering] OK");
		}
	}

	void DHavok::Patch_FixWindSpeed()
	{
		{
			struct FixWind1 : Xbyak::CodeGenerator
			{
				FixWind1(std::uintptr_t retnAddr)
				{
					Xbyak::Label retnLabel;
					Xbyak::Label magicLabel;

					movaps(ptr[rsp + 0x30], xmm6);
					movss(xmm6, dword[rip + magicLabel]);

					jmp(ptr[rip + retnLabel]);

					L(retnLabel);
					dq(retnAddr + 0x8);

					L(magicLabel);
					dd(DHavok::kBits_OneOver60);  // 1.0f/60.0f
				}
			};
			logger::info("[Havok] [Patch] [Fix wind speed] patching...");
			{
				FixWind1 code(FixWindSpeed1.address());
				code.ready();

				auto& trampoline = REL::GetTrampoline();
				trampoline.write_jmp<5>(
					FixWindSpeed1.address(),
					trampoline.allocate(code));
			}
			REL::safe_write(FixWindSpeed1.address() + 0x5, &Payloads::NOP3, 0x3);
		}
		{
			struct FixWind2 : Xbyak::CodeGenerator
			{
				FixWind2(std::uintptr_t retnAddr, std::uintptr_t a_frameTimer)
				{
					Xbyak::Label retnLabel;
					Xbyak::Label timerLabel;

					mov(rcx, ptr[rip + timerLabel]);
					movss(xmm9, dword[rcx]);

					jmp(ptr[rip + retnLabel]);

					L(retnLabel);
					dq(retnAddr + 0x9);

					L(timerLabel);
					dq(a_frameTimer);
				}
			};
			logger::info("[Havok] [Patch] [Fix wind speed] patching...");
			{
				FixWind2 code(FixWindSpeed2.address(), Game::g_frameTimerSlow.address());
				code.ready();

				auto& trampoline = REL::GetTrampoline();
				trampoline.write_jmp<5>(
					FixWindSpeed2.address(),
					trampoline.allocate(code));
			}
			REL::safe_write(FixWindSpeed2.address() + 0x5, &Payloads::NOP4, 0x4);
		}
		{
			struct FixWind3 : Xbyak::CodeGenerator
			{
				FixWind3(std::uintptr_t retnAddr, std::uintptr_t a_frameTimer)
				{
					Xbyak::Label retnLabel;
					Xbyak::Label timerLabel;

					mov(rcx, ptr[rip + timerLabel]);
					movss(xmm9, dword[rcx]);

					jmp(ptr[rip + retnLabel]);

					L(retnLabel);
					dq(retnAddr + 0x9);

					L(timerLabel);
					dq(a_frameTimer);
				}
			};
			{
				FixWind3 code(FixWindSpeed3.address(), Game::g_frameTimerSlow.address());
				code.ready();

				auto& trampoline = REL::GetTrampoline();
				trampoline.write_jmp<5>(
					FixWindSpeed3.address(),
					trampoline.allocate(code));
			}
			REL::safe_write(FixWindSpeed3.address() + 0x5, &Payloads::NOP4, 0x4);
		}
		{
			struct FixWind4 : Xbyak::CodeGenerator
			{
				FixWind4(std::uintptr_t retnAddr, std::uintptr_t a_frameTimer)
				{
					Xbyak::Label retnLabel;
					Xbyak::Label timerLabel;

					mov(r8, ptr[rip + timerLabel]);
					movss(xmm0, dword[r8]);

					jmp(ptr[rip + retnLabel]);

					L(retnLabel);
					dq(retnAddr + 0x8);

					L(timerLabel);
					dq(a_frameTimer);
				}
			};
			{
				FixWind4 code(FixWindSpeed4.address(), Game::g_frameTimerSlow.address());
				code.ready();

				auto& trampoline = REL::GetTrampoline();
				trampoline.write_jmp<5>(
					FixWindSpeed4.address(),
					trampoline.allocate(code));
			}
			logger::info("[Havok] [Patch] [Fix wind speed] OK");
			REL::safe_write(FixWindSpeed4.address() + 0x5, &Payloads::NOP3, 0x3);
		}
	}

	void DHavok::Patch_FixRotationSpeed()
	{
		{
			struct FixRotationSpeed1 : Xbyak::CodeGenerator
			{
				FixRotationSpeed1(std::uintptr_t retnAddr, std::uintptr_t a_frameTimer)
				{
					Xbyak::Label retnLabel;
					Xbyak::Label timerLabel;

					Xbyak::Label jneLabel;
					Xbyak::Label jmpLabel;

					Xbyak::Label forwardLabel;
					Xbyak::Label reverseLabel;

					jne(jneLabel);
					movss(xmm2, dword[rip + forwardLabel]);
					mov(rcx, ptr[rip + timerLabel]);
					mulss(xmm2, dword[rcx]);
					jmp(jmpLabel);
					L(jneLabel);
					movss(xmm2, dword[rip + reverseLabel]);
					mov(rcx, ptr[rip + timerLabel]);
					mulss(xmm2, dword[rcx]);
					L(jmpLabel);
					if (REX::FModule::IsRuntimeOG()) {
						mulss(xmm2, ptr[rbx + 0x38]);
					} else {
						mulss(xmm2, ptr[rdi + 0x38]);
					}

					jmp(ptr[rip + retnLabel]);

					L(retnLabel);
					dq(retnAddr + 0x19);

					L(timerLabel);
					dq(a_frameTimer);

					L(forwardLabel);
					if (REX::FModule::IsRuntimeOG()) {
						dq(uintptr_t(&Magic1));  // 58.8235
					} else {
						dd(DHavok::kBits_2_94118);  //  2.94118f
					}

					L(reverseLabel);
					if (REX::FModule::IsRuntimeOG()) {
						dq(uintptr_t(&Magic2));  // -58.8235
					} else {
						dd(DHavok::kBits_Neg2_94118);  // -2.94118f
					}
				}
			};
			logger::info("[Havok] [Patch] [Fix rotation speed] patching...");
			{
				FixRotationSpeed1 code(FixRotationSpeed.address(), Game::g_frameTimer.address());
				code.ready();

				auto& trampoline = REL::GetTrampoline();
				trampoline.write_jmp<5>(
					FixRotationSpeed.address(),
					trampoline.allocate(code));
			}
			REL::safe_fill(FixRotationSpeed.address() + 0x5, Payloads::NOP, 0x14);
			logger::info("[Havok] [Patch] [Fix rotation speed] OK");
		}
		{
			//fix lockpick rotation speed
			struct FixLockpickSpeed : Xbyak::CodeGenerator
			{
				FixLockpickSpeed(std::uintptr_t retnAddr)
				{
					Xbyak::Label retnLabel;
					Xbyak::Label magicLabel;

					mulss(xmm1, dword[rip + magicLabel]);

					jmp(ptr[rip + retnLabel]);

					L(retnLabel);
					dq(retnAddr + 0x8);

					L(magicLabel);
					dd(DHavok::kBits_OneOver60);  // 1.0f/60.0f
				}
			};
			logger::info("[Havok] [Patch] [Fix lockpick rotation speed] patching...");
			{
				FixLockpickSpeed code(FixLockpickRotation.address());
				code.ready();

				auto& trampoline = REL::GetTrampoline();
				trampoline.write_jmp<5>(
					FixLockpickRotation.address(),
					trampoline.allocate(code));
			}
			REL::safe_write(FixLockpickRotation.address() + 0x5, &Payloads::NOP3, 0x3);
			logger::info("[Havok] [Patch] [Fix lockpick rotation speed] OK");
		}
	}

	void DHavok::Patch_FixSittingRotation()
	{
		{
			struct FixSitRotationX : Xbyak::CodeGenerator
			{
				FixSitRotationX(std::uintptr_t retnAddr, std::uintptr_t a_frameTimer)

				{
					Xbyak::Label retnLabel;
					Xbyak::Label magicLabel;
					Xbyak::Label timerLabel;

					mulss(xmm0, dword[rip + magicLabel]);
					mov(r9, ptr[rip + timerLabel]);
					mulss(xmm0, dword[r9]);
					mulss(xmm0, ptr[rax + 0x4C]);

					jmp(ptr[rip + retnLabel]);

					L(retnLabel);
					dq(retnAddr + 0x5);

					L(magicLabel);
					dq(uintptr_t(Magic1));

					L(timerLabel);
					dq(a_frameTimer);
				}
			};
			logger::info("[Havok] [Patch] [Fix sitting rotation] patching...");
			{
				FixSitRotationX code(FixSittingRotationX.address(), Game::g_frameTimer.address());
				code.ready();

				auto& trampoline = REL::GetTrampoline();
				trampoline.write_jmp<5>(
					FixSittingRotationX.address(),
					trampoline.allocate(code));
			}
		}
		//y
		{
			struct FixSitRotationY : Xbyak::CodeGenerator
			{
				FixSitRotationY(
					std::uintptr_t retnAddr, std::uintptr_t a_frameTimer)

				{
					Xbyak::Label retnLabel;
					Xbyak::Label magicLabel;
					Xbyak::Label timerLabel;

					mulss(xmm1, dword[rip + magicLabel]);
					mov(r9, ptr[rip + timerLabel]);
					mulss(xmm1, dword[r9]);
					movss(xmm0, ptr[rbx + 0x64]);

					jmp(ptr[rip + retnLabel]);

					L(retnLabel);
					dq(retnAddr + 0x5);

					L(magicLabel);
					dq(uintptr_t(Magic1));

					L(timerLabel);
					dq(a_frameTimer);
				}
			};
			{
				FixSitRotationY code(FixSittingRotationY.address(), Game::g_frameTimer.address());
				code.ready();

				auto& trampoline = REL::GetTrampoline();
				trampoline.write_jmp<5>(
					FixSittingRotationY.address(),
					trampoline.allocate(code));
			}
			logger::info("[Havok] [Patch] [Fix sitting rotation] OK");
		}
	}
	void DHavok::Patch_FixWSRotation()
	{
		struct FixWorkshopRotSpeed : Xbyak::CodeGenerator
		{
			FixWorkshopRotSpeed(std::uintptr_t retnAddr, std::uintptr_t a_frameTimer)

			{
				Xbyak::Label retnLabel;
				Xbyak::Label timerLabel;

				mov(rax, ptr[rip + timerLabel]);
				if (REX::FModule::IsRuntimeOG()) {
					mulss(xmm1, dword[rax]);
				} else {
					mulss(xmm0, dword[rax]);
				}

				jmp(ptr[rip + retnLabel]);

				L(retnLabel);
				dq(retnAddr + 0x8);

				L(timerLabel);
				dq(a_frameTimer);
			}
		};
		logger::info("[Havok] [Patch] [Fix workshop rotation speed] patching...");
		{
			FixWorkshopRotSpeed code(FixWSRotationSpeed.address(), Game::g_frameTimer.address());
			code.ready();

			auto& trampoline = REL::GetTrampoline();
			trampoline.write_jmp<5>(
				FixWSRotationSpeed.address(),
				trampoline.allocate(code));
		}
		logger::info("[Havok] [Patch] [Fix workshop rotation speed] OK");
		REL::safe_write(FixWSRotationSpeed.address() + 0x5, &Payloads::NOP3, 0x3);
	}

	void DHavok::Patch_FixLoadModelSpeed1()
	{
		//Fix left trigger zoom speed
		{
			struct FixLoadModelSpeed1 : Xbyak::CodeGenerator
			{
				FixLoadModelSpeed1(std::uintptr_t a_hookTarget, std::uintptr_t a_value, std::uintptr_t a_frameTimer)
				{
					Xbyak::Label retnLabel;
					Xbyak::Label valueLabel;
					Xbyak::Label magicLabel;
					Xbyak::Label timerLabel;

					mov(rcx, ptr[rip + valueLabel]);
					movss(xmm1, ptr[rcx]);
					mulss(xmm1, dword[rip + magicLabel]);
					mov(rcx, ptr[rip + timerLabel]);
					mulss(xmm1, dword[rcx]);

					jmp(ptr[rip + retnLabel]);

					L(retnLabel);
					dq(a_hookTarget + 0x8);

					L(valueLabel);
					dq(a_value);

					L(magicLabel);
					dq(uintptr_t(Magic1));

					L(timerLabel);
					dq(a_frameTimer);
				}
			};
			{
				FixLoadModelSpeed1 code(FixLeftTriggerZoomSpeed.address(), std::uintptr_t(m_gv.fLoadingModel_TriggerZoomSpeed), Game::g_frameTimer.address());
				code.ready();

				auto& trampoline = REL::GetTrampoline();
				trampoline.write_jmp<5>(
					FixLeftTriggerZoomSpeed.address(),
					trampoline.allocate(code));
			}
			REL::safe_write(FixLeftTriggerZoomSpeed.address() + 0x5, &Payloads::NOP3, 0x3);
		}
		//Fix right trigger zoom speed
		{
			struct FixLoadModelSpeed2 : Xbyak::CodeGenerator
			{
				FixLoadModelSpeed2(std::uintptr_t a_hookTarget, std::uintptr_t a_value, std::uintptr_t a_frameTimer)
				{
					Xbyak::Label retnLabel;
					Xbyak::Label valueLabel;
					Xbyak::Label magicLabel;
					Xbyak::Label timerLabel;

					mov(rcx, ptr[rip + valueLabel]);
					movss(xmm1, ptr[rcx]);
					mulss(xmm1, dword[rip + magicLabel]);
					mov(rcx, ptr[rip + timerLabel]);
					mulss(xmm1, dword[rcx]);

					jmp(ptr[rip + retnLabel]);

					L(retnLabel);
					dq(a_hookTarget + 0x8);

					L(valueLabel);
					dq(a_value);

					L(magicLabel);
					dq(uintptr_t(Magic1));

					L(timerLabel);
					dq(a_frameTimer);
				}
			};
			{
				FixLoadModelSpeed2 code(FixRightTriggerZoomSpeed.address(), std::uintptr_t(m_gv.fLoadingModel_TriggerZoomSpeed), Game::g_frameTimer.address());
				code.ready();

				auto& trampoline = REL::GetTrampoline();
				trampoline.write_jmp<5>(
					FixRightTriggerZoomSpeed.address(),
					trampoline.allocate(code));
			}
			REL::safe_write(FixRightTriggerZoomSpeed.address() + 0x5, &Payloads::NOP3, 0x3);
		}
		//Fix repeat rate
		{
			struct FixLoadModelSpeed3 : Xbyak::CodeGenerator
			{
				FixLoadModelSpeed3(std::uintptr_t retnAddr)
				{
					Xbyak::Label retnLabel;
					Xbyak::Label magicLabel;

					if (REX::FModule::IsRuntimeOG()) {
						mov(ecx, ptr[rbx + 0x26C]);
						movss(xmm8, dword[rip + magicLabel]);
					} else {
						mov(ecx, ptr[rdi + 0x26C]);
						movss(xmm7, dword[rip + magicLabel]);
					}

					jmp(ptr[rip + retnLabel]);

					L(retnLabel);
					dq(retnAddr + 0x6);

					L(magicLabel);
					dd(DHavok::kBits_OneOver60);  // 1.0f/60.0f
				}
			};
			{
				FixLoadModelSpeed3 code(FixRepeateRate.address());
				code.ready();

				auto& trampoline = REL::GetTrampoline();
				trampoline.write_jmp<5>(
					FixRepeateRate.address(),
					trampoline.allocate(code));
			}
			REL::safe_write(FixRepeateRate.address() + 0x5, &Payloads::NOP, 0x1);
		}
		//Fix rotation
		REL::safe_write(FixLoadScreenRotationSpeed.address(), &Payloads::NOP6, 0x6);
	}
	void DHavok::Patch_FixLoadModelSpeed2()
	{
		//Fix pan speed
		//Up
		{
			struct FixLoadModelSpeed4 : Xbyak::CodeGenerator
			{
				FixLoadModelSpeed4(std::uintptr_t a_hookTarget, std::uintptr_t a_value, std::uintptr_t a_frameTimer)
				{
					Xbyak::Label retnLabel;
					Xbyak::Label timerLabel;
					Xbyak::Label magicLabel;
					Xbyak::Label valueLabel;

					mov(rcx, ptr[rip + valueLabel]);
					movss(xmm0, ptr[rcx]);
					mulss(xmm0, dword[rip + magicLabel]);
					mov(rcx, ptr[rip + timerLabel]);
					mulss(xmm0, dword[rcx]);

					jmp(ptr[rip + retnLabel]);

					L(retnLabel);
					dq(a_hookTarget + 0x8);

					L(timerLabel);
					dq(a_frameTimer);

					L(magicLabel);
					dq(uintptr_t(Magic1));  // 60.0f

					L(valueLabel);
					dq(a_value);
				}
			};
			{
				FixLoadModelSpeed4 code(FixLoadScreenRotationSpeedUp.address(), std::uintptr_t(m_gv.fLoadingModel_MouseToRotateSpeed), Game::g_frameTimer.address());
				code.ready();

				auto& trampoline = REL::GetTrampoline();
				trampoline.write_jmp<5>(
					FixLoadScreenRotationSpeedUp.address(),
					trampoline.allocate(code));
			}
			REL::safe_write(FixLoadScreenRotationSpeedUp.address() + 0x5, &Payloads::NOP3, 0x3);
		}
		{
			//Down
			struct FixLoadModelSpeed5 : Xbyak::CodeGenerator
			{
				FixLoadModelSpeed5(std::uintptr_t a_hookTarget, std::uintptr_t a_value, std::uintptr_t a_frameTimer)
				{
					Xbyak::Label retnLabel;
					Xbyak::Label timerLabel;
					Xbyak::Label magicLabel;
					Xbyak::Label valueLabel;

					mov(rcx, ptr[rip + valueLabel]);
					movss(xmm0, ptr[rcx]);
					mulss(xmm0, dword[rip + magicLabel]);
					mov(rcx, ptr[rip + timerLabel]);
					mulss(xmm0, dword[rcx]);

					jmp(ptr[rip + retnLabel]);

					L(retnLabel);
					dq(a_hookTarget + 0x8);

					L(timerLabel);
					dq(a_frameTimer);

					L(magicLabel);
					dq(uintptr_t(Magic1));  // 60.0f

					L(valueLabel);
					dq(a_value);
				}
			};
			{
				FixLoadModelSpeed5 code(FixLoadScreenRotationSpeedDown.address(), std::uintptr_t(m_gv.fLoadingModel_MouseToRotateSpeed), Game::g_frameTimer.address());
				code.ready();

				auto& trampoline = REL::GetTrampoline();
				trampoline.write_jmp<5>(
					FixLoadScreenRotationSpeedDown.address(),
					trampoline.allocate(code));
			}
			REL::safe_write(FixLoadScreenRotationSpeedDown.address() + 0x5, &Payloads::NOP3, 0x3);
		}
		{
			//Left
			struct FixLoadModelSpeed6 : Xbyak::CodeGenerator
			{
				FixLoadModelSpeed6(std::uintptr_t a_hookTarget, std::uintptr_t a_value, std::uintptr_t a_frameTimer)
				{
					Xbyak::Label retnLabel;
					Xbyak::Label timerLabel;
					Xbyak::Label magicLabel;
					Xbyak::Label valueLabel;

					mov(rcx, ptr[rip + valueLabel]);
					movss(xmm0, ptr[rcx]);
					mulss(xmm0, dword[rip + magicLabel]);
					mov(rcx, ptr[rip + timerLabel]);
					mulss(xmm0, dword[rcx]);

					jmp(ptr[rip + retnLabel]);

					L(retnLabel);
					dq(a_hookTarget + 0x8);

					L(timerLabel);
					dq(a_frameTimer);

					L(magicLabel);
					dq(uintptr_t(Magic1));  // 60.0f

					L(valueLabel);
					dq(a_value);
				}
			};
			{
				FixLoadModelSpeed6 code(FixLoadScreenRotationSpeedLeft.address(), std::uintptr_t(m_gv.fLoadingModel_MouseToRotateSpeed), Game::g_frameTimer.address());
				code.ready();

				auto& trampoline = REL::GetTrampoline();
				trampoline.write_jmp<5>(
					FixLoadScreenRotationSpeedLeft.address(),
					trampoline.allocate(code));
			}
			REL::safe_write(FixLoadScreenRotationSpeedLeft.address() + 0x5, &Payloads::NOP3, 0x3);
		}
		{
			//Right
			struct FixLoadModelSpeed7 : Xbyak::CodeGenerator
			{
				FixLoadModelSpeed7(std::uintptr_t a_hookTarget, std::uintptr_t a_value, std::uintptr_t a_frameTimer)
				{
					Xbyak::Label retnLabel;
					Xbyak::Label timerLabel;
					Xbyak::Label magicLabel;
					Xbyak::Label valueLabel;

					mov(rcx, ptr[rip + valueLabel]);
					movss(xmm0, ptr[rcx]);
					mulss(xmm0, dword[rip + magicLabel]);
					mov(rcx, ptr[rip + timerLabel]);
					mulss(xmm0, dword[rcx]);

					jmp(ptr[rip + retnLabel]);

					L(retnLabel);
					dq(a_hookTarget + 0x8);

					L(timerLabel);
					dq(a_frameTimer);

					L(magicLabel);
					dq(uintptr_t(Magic1));  // 60.0f

					L(valueLabel);
					dq(a_value);
				}
			};
			{
				FixLoadModelSpeed7 code(FixLoadScreenRotationSpeedRight.address(), std::uintptr_t(m_gv.fLoadingModel_MouseToRotateSpeed), Game::g_frameTimer.address());
				code.ready();

				auto& trampoline = REL::GetTrampoline();
				trampoline.write_jmp<5>(
					FixLoadScreenRotationSpeedRight.address(),
					trampoline.allocate(code));
			}
			REL::safe_write(FixLoadScreenRotationSpeedRight.address() + 0x5, &Payloads::NOP3, 0x3);
			logger::info("[Havok] [Patch] [Fix loading model] OK");
		}
	}
	void DHavok::Patch_FixStuck()
	{
		struct FixStuck : Xbyak::CodeGenerator
		{
			FixStuck(std::uintptr_t retnAddr)

			{
				Xbyak::Label retnLabel;
				Xbyak::Label magicLabel;

				movss(xmm3, dword[rip + magicLabel]);

				jmp(ptr[rip + retnLabel]);

				L(retnLabel);
				dq(retnAddr + 0x5);

				L(magicLabel);
				dd(DHavok::kBits_OneOver58_8235);  // ~1.0f/58.8235f
			}
		};
		logger::info("[Havok] [Patch] [Fix stuck animation] patching...");
		{
			FixStuck code(FixStuckAnim.address());
			code.ready();

			auto& trampoline = REL::GetTrampoline();
			trampoline.write_jmp<5>(
				FixStuckAnim.address(),
				trampoline.allocate(code));
		}
		logger::info("[Havok] [Patch] [Fix stuck animation] OK");
	}
}
