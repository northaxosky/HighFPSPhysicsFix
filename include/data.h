#pragma once

#include "runtime_value.h"

// Multi-runtime address table for OG (1.10.163), NG (1.10.984), AE (1.11.x).
// AIDs use the COMMONLIB_RUNTIMECOUNT=3 triplet form { og, ng, ae } (when only
// 2 are supplied, the AE value also covers NG, matching upstream's per-DLL split
// where the NG and AE DLLs shared sources). Offsets use the RuntimeOffset helper
// which selects the correct value at runtime from the loaded module.

namespace HFPF
{
	namespace AID
	{
		// Sourced from upstream/master (AE) and upstream/1-10-163 (OG) branches.
		// NG slot mirrors AE per upstream design.
		constexpr REL::ID FPS_Cap_Patch1                 { 133902,  2228907 };
		constexpr REL::ID FullScreen_Patch1              { 1547437, 2227231 };
		// AE has standalone MovRaxRcx AID; OG inlines it inside ResizeBuffers.
		constexpr REL::ID MovRaxRcx                      { 212827,  2276824 };
		constexpr REL::ID ResizeTarget                   { 796949,  2276825 };
		// AE has standalone CreateDXGIFactory AID; OG uses D3D11Create as the parent symbol.
		constexpr REL::ID CreateDXGIFactory              { 224250,  4492363 };
		constexpr REL::ID FrameTimer                     { 922988,  4803789 };
		constexpr REL::ID CreateWindowEx_a               { 193854,  2276814 };
		constexpr REL::ID BlackLoadingScreens            { 991513,  2249217 };
		constexpr REL::ID LoadingScreens                 { 132841,  2227631 };
		constexpr REL::ID PostLoadInject                 { 1289136, 2248711 };
		constexpr REL::ID PresentThread                  { 700869,  2276834 };
		constexpr REL::ID Untie                          { 462873,  2267969 };
		constexpr REL::ID ExtInt                         { 359440,  359440  };
		constexpr REL::ID FixStuttering1                 { 12890,   2277709 };
		constexpr REL::ID FixStuttering2                 { 1395106, 2277710 };
		constexpr REL::ID ObjectsTransfer                { 754666,  2255886 };
		constexpr REL::ID FixWhiteScreen                 { 703643,  2258401 };
		constexpr REL::ID FixWindSpeed1                  { 1469635, 2278751 };
		constexpr REL::ID FixWindSpeed2                  { 1164603, 2277711 };
		constexpr REL::ID FixRotationSpeed               { 457276,  2234879 };
		constexpr REL::ID FixLockpickRotation            { 676000,  2249260 };
		constexpr REL::ID FixWSRotationSpeed             { 1144472, 2195211 };
		constexpr REL::ID FixRepeateRate                 { 618896,  2249218 };
		// OG branch named this FixTriggerZoomSpeed (one trigger handles both); AE split into Left/Right.
		constexpr REL::ID FixLeftTriggerZoomSpeed        { 629736,  2249220 };
		constexpr REL::ID FixLoadScreenRotationSpeed     { 22234,   2249233 };
		constexpr REL::ID FixStuckAnim                   { 463133,  2302542 };
		constexpr REL::ID FixMotionFeedback              { 1201084, 2196089 };
		constexpr REL::ID FixSittingRotationX            { 533372,  2248271 };
		constexpr REL::ID Upscale                        { 288964,  2276833 };
		constexpr REL::ID ActorFade                      { 295466,  2214659 };
		constexpr REL::ID PlayerFade                     { 202079,  2248393 };
		constexpr REL::ID BudgetGame                     { 759508,  2251303 };
		constexpr REL::ID BudgetUI                       { 1343068, 2251305 };
		constexpr REL::ID Budget                         { 890788,  2251306 };
		// OG branch names this Write_iLocationX (single AID for both X/Y axes).
		constexpr REL::ID Write_iLocation                { 719782,  8695006 };
		// Hook target for plugin INI loading. OG and AE use different parent symbols
		// (OG: LoadPluginINI_C function; AE: FPS_Cap_Patch1 + 0x631) but resolve to the
		// same absolute address. Pair with Offsets::LoadPluginINI_C below.
		constexpr REL::ID LoadPluginINI_C                { 1266784, 2228907 };
	}

	namespace Offsets
	{
		// Each offset selects the value for the active runtime via RuntimeOffset.
		// Format: { OG offset, AE offset } (NG mirrors AE).
		// Source: upstream/master (AE) and upstream/1-10-163 (OG) Offsets namespaces.
		static inline const RuntimeOffset FPS_Cap_Patch1                  { 0xAA,  0xC37  };
		static inline const RuntimeOffset FPS_Cap_Patch2                  { 0xB3,  0xC40  };
		static inline const RuntimeOffset PresentThreadBlock              { 0x30,  0x30   };  // Used on OG; AE may not use it.
		static inline const RuntimeOffset Borderless_Patch                { 0x5C,  0xBEE  };
		static inline const RuntimeOffset FullScreen_Patch1               { 0xD0,  0xCF   };
		static inline const RuntimeOffset FullScreen_Patch3               { 0x101, 0x100  };
		static inline const RuntimeOffset Screen_Patch                    { 0x51,  0xBE4  };
		static inline const RuntimeOffset MovRaxRcx                       { 0x1B5, 0x1D6  };
		static inline const RuntimeOffset ResizeBuffersDisable            { 0x27,  0x28   };
		static inline const RuntimeOffset ResizeTarget                    { 0x119, 0x108  };
		static inline const RuntimeOffset ResizeTargetDisable             { 0x25,  0x27   };
		static inline const RuntimeOffset ResizeBuffers                   { 0x1A0, 0x1C1  };
		static inline const RuntimeOffset CreateDXGIFactory               { 0x2B,  0x32   };
		static inline const RuntimeOffset D3D11CreateDeviceAndSwapChain   { 0x419, 0x410  };
		static inline const RuntimeOffset CreateWindowEx_a                { 0x187, 0x285  };
		static inline const RuntimeOffset BlackLoadingScreens             { 0x116, 0x116  };
		static inline const RuntimeOffset LoadingScreens                  { 0x19D, 0x223  };
		static inline const RuntimeOffset PostLoadInject                  { 0x2B,  0x2E   };
		static inline const RuntimeOffset PresentThread                   { 0x30,  0x30   };
		static inline const RuntimeOffset BethesdaVsync                   { 0x332, 0x331  };
		static inline const RuntimeOffset LoadScreenPlusLimiterInject     { 0x0E,  0x0E   };
		static inline const RuntimeOffset PresentInject                   { 0x48,  0x48   };
		static inline const RuntimeOffset Untie                           { 0x6B,  0x61   };
		static inline const RuntimeOffset FixStuttering1                  { 0x196, 0x169  };
		static inline const RuntimeOffset FixStuttering2                  { 0x1A1, 0x19B  };
		static inline const RuntimeOffset FixStuttering3                  { 0x145, 0x122  };
		static inline const RuntimeOffset FixWhiteScreen                  { 0x13,  0x10   };
		static inline const RuntimeOffset FixWindSpeed1                   { 0x21,  0x24   };
		static inline const RuntimeOffset FixWindSpeed2                   { 0x9E,  0x115  };
		static inline const RuntimeOffset FixWindSpeed3                   { 0x147, 0x1B7  };
		static inline const RuntimeOffset FixWindSpeed4                   { 0x32B, 0x3BD  };
		static inline const RuntimeOffset FixRotationSpeed                { 0xE1,  0x6E   };
		static inline const RuntimeOffset FixLockpickRotation             { 0x42,  0x42   };
		static inline const RuntimeOffset FixWSRotationSpeed              { 0xA2,  0x94   };
		static inline const RuntimeOffset FixRepeateRate                  { 0x354, 0x3F5  };
		static inline const RuntimeOffset FixLeftTriggerZoomSpeed         { 0xFF,  0xDD   };
		static inline const RuntimeOffset FixRightTriggerZoomSpeed        { 0x163, 0x11E  };
		static inline const RuntimeOffset FixLoadScreenRotationSpeedUp    { 0x46C, 0x525  };
		static inline const RuntimeOffset FixLoadScreenRotationSpeedDown  { 0x4C6, 0x58A  };
		static inline const RuntimeOffset FixLoadScreenRotationSpeedLeft  { 0x51F, 0x5EE  };
		static inline const RuntimeOffset FixLoadScreenRotationSpeedRight { 0x584, 0x65E  };
		static inline const RuntimeOffset FixLoadScreenRotationSpeed      { 0xBF,  0xBA   };
		static inline const RuntimeOffset FixStuckAnim                    { 0xA9,  0xA9   };
		static inline const RuntimeOffset FixMotionFeedback               { 0x9F7, 0x9FE  };
		static inline const RuntimeOffset FixSittingRotationX             { 0xC0,  0xC0   };
		static inline const RuntimeOffset FixSittingRotationY             { 0xD7,  0xDE   };
		static inline const RuntimeOffset Upscale                         { 0x1BE, 0x1BA  };
		static inline const RuntimeOffset ActorFade                       { 0x663, 0xAED  };
		static inline const RuntimeOffset PlayerFade                      { 0x162, 0x169  };
		static inline const RuntimeOffset iSizeW                          { 0x69,  0xBF8  };
		static inline const RuntimeOffset iSizeH                          { 0x78,  0xC02  };
		static inline const RuntimeOffset BudgetGame                      { 0x3C,  0x3C   };
		static inline const RuntimeOffset BudgetUI                        { 0xB4,  0xB4   };
		static inline const RuntimeOffset Budget                          { 0xB4,  0xB4   };
		static inline const RuntimeOffset LoadPluginINI_C                 { 0xEE,  0x631  };
		static inline const RuntimeOffset Write_iLocationX                { 0x269, 0x1DA  };
		static inline const RuntimeOffset Write_iLocationY                { 0x2A4, 0x215  };
	}

	namespace Payloads
	{
		static inline constexpr std::uint8_t JMP8 = { 0xEB };
		static inline constexpr std::uint8_t INT3 = { 0xCC };
		static inline constexpr std::uint8_t NOP = { 0x90 };
		static inline constexpr std::uint8_t NOP2[] = { 0x66, 0x90 };
		static inline constexpr std::uint8_t NOP3[] = { 0x0F, 0x1F, 0x00 };
		static inline constexpr std::uint8_t NOP4[] = { 0x0F, 0x1F, 0x40, 0x00 };
		static inline constexpr std::uint8_t NOP5[] = { 0x0F, 0x1F, 0x44, 0x00, 0x00 };
		static inline constexpr std::uint8_t NOP6[] = { 0x66, 0x0F, 0x1F, 0x44, 0x00, 0x00 };
		static inline constexpr std::uint8_t NOP8[] = { 0x0F, 0x1F, 0x84, 0x00, 0x00, 0x00, 0x00, 0x00 };

		static inline constexpr std::uint8_t loading_patch[] = { 0xF2, 0x48, 0x90, 0xF2, 0x48, 0x90, 0xF2, 0x48, 0x90, 0x90 };
		static inline constexpr std::uint8_t screen_patch[] = { 0xB8, 0x01, 0x00, 0x00, 0x00, 0x90, 0x90 };
		static inline constexpr std::uint8_t fullscreen1_patch[] = { 0x41, 0x80, 0xFB, 0x01, 0x90, 0x90, 0x90 };
		static inline constexpr std::uint8_t fullscreenJMP_patch[] = { 0xEB, 0x18 };
		static inline constexpr std::uint8_t fullscreenNOP_patch[] = { 0x90, 0x90 };
		static inline constexpr std::uint8_t untie_patch[] = { 0x00 };
		static inline constexpr std::uint8_t ifpsclamp_patch[] = { 0x38 };
		static inline constexpr std::uint8_t disable_blackloading_patch[] = { 0xEB };
		// AE (master) and OG branch use different short-jump targets here. Selected at runtime.
		static inline const std::uint8_t* ResizeBuffersDisable() noexcept
		{
			static constexpr std::uint8_t og[]{ 0xE9, 0x46, 0x02, 0x00, 0x00, 0x90 };
			static constexpr std::uint8_t ae[]{ 0xE9, 0x6A, 0x03, 0x00, 0x00, 0x90 };
			return static_cast<std::uint8_t>(REL::Module::GetRuntimeIndex()) == 0 ? og : ae;
		}
		static inline const std::uint8_t* ResizeTargetDisable() noexcept
		{
			static constexpr std::uint8_t og[]{ 0xE9, 0x4F, 0x01, 0x00, 0x00, 0x90 };
			static constexpr std::uint8_t ae[]{ 0xE9, 0x3C, 0x01, 0x00, 0x00, 0x90 };
			return static_cast<std::uint8_t>(REL::Module::GetRuntimeIndex()) == 0 ? og : ae;
		}
		static inline constexpr std::size_t ResizeBuffersDisable_size = 6;
		static inline constexpr std::size_t ResizeTargetDisable_size  = 6;

		static inline constexpr std::uint8_t res_patch[] = { 0xB8, 0x00, 0x00, 0x00, 0x00, 0x90 };

		static inline constexpr std::uint8_t SkipNoINI[] = { 0x48, 0x8B, 0xCF };

		// player_fade_jmp: AE uses 0x6C; OG uses 0x6F (both are short jumps with NOP padding).
		static inline const std::uint8_t* player_fade_jmp() noexcept
		{
			static constexpr std::uint8_t og[]{ 0xEB, 0x6F, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90 };
			static constexpr std::uint8_t ae[]{ 0xEB, 0x6C, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90 };
			return static_cast<std::uint8_t>(REL::Module::GetRuntimeIndex()) == 0 ? og : ae;
		}
		static inline constexpr std::size_t player_fade_jmp_size = 8;
	}
}
