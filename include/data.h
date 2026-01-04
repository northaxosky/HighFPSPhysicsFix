#pragma once

namespace HFPF
{
	namespace AID
	{
        constexpr REL::ID FPS_Cap_Patch1                                              (2228907); // Fallout4.exe+0x00C2FAD0
        constexpr REL::ID FullScreen_Patch1                                           (2227231); // Fallout4.exe+0x00BB4F40
        constexpr REL::ID MovRaxRcx                                                   (2276824); // Fallout4.exe+0x018173D0
        constexpr REL::ID ResizeTarget                                                (2276825); // Fallout4.exe+0x01817780
        constexpr REL::ID CreateDXGIFactory                                           (4492363); // Fallout4.exe+0x01824350
        constexpr REL::ID FrameTimer                                                  (4803789); // Fallout4.exe+0x0326F040
        constexpr REL::ID CreateWindowEx_a                                            (2276814); // Fallout4.exe+0x01815A90
        constexpr REL::ID BlackLoadingScreens                                         (2249217); // Fallout4.exe+0x01066450
        constexpr REL::ID LoadingScreens                                              (2227631); // Fallout4.exe+0x00BD6F10
        constexpr REL::ID PostLoadInject                                              (2248711); // Fallout4.exe+0x010454A0
        constexpr REL::ID PresentThread                                               (2276834); // Fallout4.exe+0x01817F60
        constexpr REL::ID Untie                                                       (2267969); // Fallout4.exe+0x0165B5F0
        constexpr REL::ID ExtInt                                                      (359440);  // Fallout4.exe+0x02F28530
        constexpr REL::ID FixStuttering1                                              (2277709); // Fallout4.exe+0x01873C80
        constexpr REL::ID FixStuttering2                                              (2277710); // Fallout4.exe+0x01873E60
        constexpr REL::ID ObjectsTransfer                                             (2255886); // Fallout4.exe+0x012767C0
        constexpr REL::ID FixWhiteScreen                                              (2258401); // Fallout4.exe+0x013B1170
        constexpr REL::ID FixWindSpeed1                                               (2278751); // Fallout4.exe+0x018BE4E0
        constexpr REL::ID FixWindSpeed2                                               (2277711); // Fallout4.exe+0x01874B60
        constexpr REL::ID FixRotationSpeed                                            (2234879); // Fallout4.exe+0x00DDF410
        constexpr REL::ID FixLockpickRotation                                         (2249260); // Fallout4.exe+0x0106AF50
        constexpr REL::ID FixWSRotationSpeed                                          (2195211); // Fallout4.exe+0x003A1C50
        constexpr REL::ID FixRepeateRate                                              (2249218); // Fallout4.exe+0x01066610
        constexpr REL::ID FixLeftTriggerZoomSpeed                                     (2249220); // Fallout4.exe+0x01066D30
        constexpr REL::ID FixLoadScreenRotationSpeed                                  (2249233); // Fallout4.exe+0x010682A0
        constexpr REL::ID FixStuckAnim                                                (2302542); // Fallout4.exe+0x01E7AA30
        constexpr REL::ID FixMotionFeedback                                           (2196089); // Fallout4.exe+0x003E6960
        constexpr REL::ID FixSittingRotationX                                         (2248271); // Fallout4.exe+0x010216B0
        constexpr REL::ID Upscale                                                     (2276833); // Fallout4.exe+0x01817D10
        constexpr REL::ID ActorFade                                                   (2214659); // Fallout4.exe+0x0081CE50
        constexpr REL::ID PlayerFade                                                  (2248393); // Fallout4.exe+0x01028CC0
        constexpr REL::ID BudgetGame                                                  (2251303); // Fallout4.exe+0x010E9A70
        constexpr REL::ID BudgetUI                                                    (2251305); // Fallout4.exe+0x010E9D00
        constexpr REL::ID Budget                                                      (2251306); // Fallout4.exe+0x010E9E10
        constexpr REL::ID Write_iLocation                                             (8695006); // Fallout4.exe+0x00C3A15F
        constexpr REL::ID Write_iLocation_1_11_169                                    (7267008); // Fallout4.exe+0x00C35BCF
        constexpr REL::ID Write_iLocation_1_11_159                                    (5988687); // Fallout4.exe+0x00C3571F

	}

	namespace Offsets
	{
		static inline constexpr std::uintptr_t FPS_Cap_Patch1 = 0xC37;                   // Fallout4.exe+0x00C30707
		static inline constexpr std::uintptr_t FPS_Cap_Patch2 = 0xC40;                   // Fallout4.exe+0x00C30710
		static inline constexpr std::uintptr_t Borderless_Patch = 0xBEE;                 // Fallout4.exe+0x00C306BE
		static inline constexpr std::uintptr_t FullScreen_Patch1 = 0xCF;                 // Fallout4.exe+0x00BB500F
		static inline constexpr std::uintptr_t FullScreen_Patch3 = 0x100;                // Fallout4.exe+0x00BB5040
		static inline constexpr std::uintptr_t Screen_Patch = 0xBE4;                     // Fallout4.exe+0x00C306B4
		static inline constexpr std::uintptr_t MovRaxRcx = 0x1D6;                        // Fallout4.exe+0x018175A6
		static inline constexpr std::uintptr_t ResizeBuffersDisable = 0x28;              // Fallout4.exe+0x018173F8
		static inline constexpr std::uintptr_t ResizeTarget = 0x108;                     // Fallout4.exe+0x01817888
		static inline constexpr std::uintptr_t ResizeTargetDisable = 0x27;               // Fallout4.exe+0x018177A7
		static inline constexpr std::uintptr_t ResizeBuffers = 0x1C1;                    // Fallout4.exe+0x01817591
		static inline constexpr std::uintptr_t CreateDXGIFactory = 0x32;                 // Fallout4.exe+0x01824382
		static inline constexpr std::uintptr_t D3D11CreateDeviceAndSwapChain = 0x410;    // Fallout4.exe+0x01824760
		static inline constexpr std::uintptr_t CreateWindowEx_a = 0x285;                 // Fallout4.exe+0x01815D15
		static inline constexpr std::uintptr_t BlackLoadingScreens = 0x116;              // Fallout4.exe+0x01066566
		static inline constexpr std::uintptr_t LoadingScreens = 0x223;                   // Fallout4.exe+0x00BD7133
		static inline constexpr std::uintptr_t PostLoadInject = 0x2E;                    // Fallout4.exe+0x010454CE
		static inline constexpr std::uintptr_t PresentThread = 0x30;                     // Fallout4.exe+0x01817F90
		static inline constexpr std::uintptr_t BethesdaVsync = 0x331;                    // Fallout4.exe+0x01824681
		static inline constexpr std::uintptr_t LoadScreenPlusLimiterInject = 0xE;        // Fallout4.exe+0x01817F6E
		static inline constexpr std::uintptr_t PresentInject = 0x48;                     // Fallout4.exe+0x01817FA8
		static inline constexpr std::uintptr_t Untie = 0x61;                             // Fallout4.exe+0x0165B651
		static inline constexpr std::uintptr_t FixStuttering1 = 0x169;                   // Fallout4.exe+0x01873DE9
		static inline constexpr std::uintptr_t FixStuttering2 = 0x19B;                   // Fallout4.exe+0x01873FFB
		static inline constexpr std::uintptr_t FixStuttering3 = 0x122;                   // Fallout4.exe+0x01873DA2
		static inline constexpr std::uintptr_t FixWhiteScreen = 0x10;                    // Fallout4.exe+0x013B1180
		static inline constexpr std::uintptr_t FixWindSpeed1 = 0x24;                     // Fallout4.exe+0x018BE504
		static inline constexpr std::uintptr_t FixWindSpeed2 = 0x115;                    // Fallout4.exe+0x01874C75
		static inline constexpr std::uintptr_t FixWindSpeed3 = 0x1B7;                    // Fallout4.exe+0x01874D17
		static inline constexpr std::uintptr_t FixWindSpeed4 = 0x3BD;                    // Fallout4.exe+0x01874F1D
		static inline constexpr std::uintptr_t FixRotationSpeed = 0x6E;                  // Fallout4.exe+0x00DDF47E
		static inline constexpr std::uintptr_t FixLockpickRotation = 0x42;               // Fallout4.exe+0x0106AF92
		static inline constexpr std::uintptr_t FixWSRotationSpeed = 0x94;                // Fallout4.exe+0x003A1CE4
		static inline constexpr std::uintptr_t FixRepeateRate = 0x3F5;                   // Fallout4.exe+0x01066A05
		static inline constexpr std::uintptr_t FixLeftTriggerZoomSpeed = 0xDD;           // Fallout4.exe+0x01066E0D
		static inline constexpr std::uintptr_t FixRightTriggerZoomSpeed = 0x11E;         // Fallout4.exe+0x01066E4E
		static inline constexpr std::uintptr_t FixLoadScreenRotationSpeedUp = 0x525;     // Fallout4.exe+0x01066B35
		static inline constexpr std::uintptr_t FixLoadScreenRotationSpeedDown = 0x58A;   // Fallout4.exe+0x01066B9A
		static inline constexpr std::uintptr_t FixLoadScreenRotationSpeedLeft = 0x5EE;   // Fallout4.exe+0x01066BFE
		static inline constexpr std::uintptr_t FixLoadScreenRotationSpeedRight = 0x65E;  // Fallout4.exe+0x01066C6E
		static inline constexpr std::uintptr_t FixLoadScreenRotationSpeed = 0xBA;        // Fallout4.exe+0x0106835A
		static inline constexpr std::uintptr_t FixStuckAnim = 0xA9;                      // Fallout4.exe+0x01E7AAD9
		static inline constexpr std::uintptr_t FixMotionFeedback = 0x9FE;                // Fallout4.exe+0x003E735E
		static inline constexpr std::uintptr_t FixSittingRotationX = 0xC0;               // Fallout4.exe+0x01021770
		static inline constexpr std::uintptr_t FixSittingRotationY = 0xDE;               // Fallout4.exe+0x0102178E
		static inline constexpr std::uintptr_t Upscale = 0x1BA;                          // Fallout4.exe+0x01817ECA
		static inline constexpr std::uintptr_t ActorFade = 0xAED;                        // Fallout4.exe+0x0081D93D
		static inline constexpr std::uintptr_t PlayerFade = 0x169;                       // Fallout4.exe+0x01028E29
		static inline constexpr std::uintptr_t iSizeW = 0xBF8;                           // Fallout4.exe+0x00C306C8
		static inline constexpr std::uintptr_t iSizeH = 0xC02;                           // Fallout4.exe+0x00C306D2
		static inline constexpr std::uintptr_t BudgetGame = 0x3C;                        // Fallout4.exe+0x010E9AAC
		static inline constexpr std::uintptr_t BudgetUI = 0xB4;                          // Fallout4.exe+0x010E9DB4
		static inline constexpr std::uintptr_t Budget = 0xB4;                            // Fallout4.exe+0x010E9EC4
		static inline constexpr std::uintptr_t LoadPluginINI_C = 0x631;                  // Fallout4.exe+0x00C30101
		static inline constexpr std::uintptr_t Write_iLocationX = 0x1DA;                 // Fallout4.exe+0x00C3A339
		static inline constexpr std::uintptr_t Write_iLocationY = 0x215;                 // Fallout4.exe+0x00C3A374
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
		static inline constexpr std::uint8_t ResizeBuffersDisable[] = { 0xE9, 0x6A, 0x03, 0x00, 0x00, 0x90 };
		static inline constexpr std::uint8_t ResizeTargetDisable[] = { 0xE9, 0x3C, 0x01, 0x00, 0x00, 0x90 };
		static inline constexpr std::uint8_t res_patch[] = { 0xB8, 0x00, 0x00, 0x00, 0x00, 0x90 };

		static inline constexpr std::uint8_t SkipNoINI[] = { 0x48, 0x8B, 0xCF };

		static inline constexpr std::uint8_t player_fade_jmp[] = { 0xEB, 0x6C, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90 };
	}
}
