#pragma once

namespace HFPF
{
	class DHavok :
		public IDriver,
		IConfig
	{
	public:
		static inline constexpr auto ID = DRIVER_ID::HAVOK;

		FN_NAMEPROC("HAVOK");
		FN_ESSENTIAL(false);
		FN_DRVDEF(6);

	private:
		virtual void LoadConfig() override;
		virtual void PostLoadConfig() override;
		virtual void RegisterHooks() override;
		virtual void Patch() override;
		virtual bool Prepare() override;
		virtual void OnGameConfigLoaded() override;

		static void PostConfigLoad(Event code, void* data);

		struct
		{
			bool untie_game_speed;
			bool fix_stuttering;
			bool fix_white_screen;
			bool fix_wind_speed;
			bool fix_rot_speed;
			bool fix_sit_rot_speed;
			bool fix_ws_rot_speed;
			bool fix_load_model;
			bool fix_stuck_anim;
			bool fix_responsive;
		} m_conf;

		struct
		{
			float* fLoadingModel_TriggerZoomSpeed{ nullptr };
			float* fLoadingModel_MouseToRotateSpeed{ nullptr };
		} m_gv;

		void Patch_FixStuttering();
		void Patch_FixWindSpeed();
		void Patch_FixRotationSpeed();
		void Patch_FixSittingRotation();
		void Patch_FixWSRotation();
		void Patch_FixLoadModelSpeed1();
		void Patch_FixLoadModelSpeed2();
		void Patch_FixStuck();

		inline static DWORD_PTR ft4handle;

	public:
		// Tier 4: named replacements for the physics magic constants that
		// previously appeared as raw 32-bit hex literals throughout the xbyak
		// hook bodies. The `kBits_*` form is what the code generators emit
		// (via `dd(...)`) so we preserve the EXACT little-endian byte
		// sequence regardless of how the compiler rounds the equivalent
		// `kFloat_*` literal. Comment shows the float value each pattern
		// represents.
		static constexpr float        kFloat_60_0          = 60.0f;
		static constexpr float        kFloat_OneOver60     = 1.0f / 60.0f;
		static constexpr float        kFloat_NegOneOver60  = -1.0f / 60.0f;
		static constexpr float        kFloat_58_8235       = 58.8235f;
		static constexpr float        kFloat_Neg58_8235    = -58.8235f;
		static constexpr float        kFloat_2_94118       = 2.94118f;
		static constexpr float        kFloat_Neg2_94118    = -2.94118f;

		static constexpr std::uint32_t kBits_OneOver60   = 0x3c88888aU;  //  1.0f/60.0f  (~0.0166667f)
		static constexpr std::uint32_t kBits_58_8235     = 0x426b4b44U;  //  58.8235f
		static constexpr std::uint32_t kBits_Neg58_8235  = 0xc26b4b44U;  // -58.8235f
		static constexpr std::uint32_t kBits_2_94118     = 0x403c3c3cU;  //  2.94118f
		static constexpr std::uint32_t kBits_Neg2_94118  = 0xc03c3c4bU;  // -2.94118f
		static constexpr std::uint32_t kBits_One         = 0x3f800000U;  //  1.0f
		static constexpr std::uint32_t kBits_OneOver58_8235 = 0x3c8b4396U;  // ~1.0f/58.8235f (~0.017f)

	private:
		inline static REL::Relocation<std::uintptr_t> Untie{ AID::Untie, Offsets::Untie };
		inline static REL::Relocation<std::uintptr_t> FixStuttering1{ AID::FixStuttering1, Offsets::FixStuttering1 };
		inline static REL::Relocation<std::uintptr_t> FixStuttering2{ AID::FixStuttering2, Offsets::FixStuttering2 };
		inline static REL::Relocation<std::uintptr_t> FixStuttering3{ AID::FixStuttering1, Offsets::FixStuttering3 };
		inline static REL::Relocation<std::uintptr_t> FixObjectsTransfer{ AID::ObjectsTransfer };
		inline static REL::Relocation<std::uintptr_t> FixWhiteScreen{ AID::FixWhiteScreen, Offsets::FixWhiteScreen };
		inline static REL::Relocation<std::uintptr_t> FixWindSpeed1{ AID::FixWindSpeed1, Offsets::FixWindSpeed1 };
		inline static REL::Relocation<std::uintptr_t> FixWindSpeed2{ AID::FixWindSpeed2, Offsets::FixWindSpeed2 };
		inline static REL::Relocation<std::uintptr_t> FixWindSpeed3{ AID::FixWindSpeed2, Offsets::FixWindSpeed3 };
		inline static REL::Relocation<std::uintptr_t> FixWindSpeed4{ AID::FixWindSpeed2, Offsets::FixWindSpeed4 };
		inline static REL::Relocation<std::uintptr_t> FixRotationSpeed{ AID::FixRotationSpeed, Offsets::FixRotationSpeed };
		inline static REL::Relocation<std::uintptr_t> FixLockpickRotation{ AID::FixLockpickRotation, Offsets::FixLockpickRotation };
		inline static REL::Relocation<std::uintptr_t> FixWSRotationSpeed{ AID::FixWSRotationSpeed, Offsets::FixWSRotationSpeed };
		inline static REL::Relocation<std::uintptr_t> FixRepeateRate{ AID::FixRepeateRate, Offsets::FixRepeateRate };
		inline static REL::Relocation<std::uintptr_t> FixLeftTriggerZoomSpeed{ AID::FixLeftTriggerZoomSpeed, Offsets::FixLeftTriggerZoomSpeed };
		inline static REL::Relocation<std::uintptr_t> FixRightTriggerZoomSpeed{ AID::FixLeftTriggerZoomSpeed, Offsets::FixRightTriggerZoomSpeed };
		inline static REL::Relocation<std::uintptr_t> FixLoadScreenRotationSpeedUp{ AID::FixRepeateRate, Offsets::FixLoadScreenRotationSpeedUp };
		inline static REL::Relocation<std::uintptr_t> FixLoadScreenRotationSpeedDown{ AID::FixRepeateRate, Offsets::FixLoadScreenRotationSpeedDown };
		inline static REL::Relocation<std::uintptr_t> FixLoadScreenRotationSpeedLeft{ AID::FixRepeateRate, Offsets::FixLoadScreenRotationSpeedLeft };
		inline static REL::Relocation<std::uintptr_t> FixLoadScreenRotationSpeedRight{ AID::FixRepeateRate, Offsets::FixLoadScreenRotationSpeedRight };
		inline static REL::Relocation<std::uintptr_t> FixLoadScreenRotationSpeed{ AID::FixLoadScreenRotationSpeed, Offsets::FixLoadScreenRotationSpeed };
		inline static REL::Relocation<std::uintptr_t> FixStuckAnim{ AID::FixStuckAnim, Offsets::FixStuckAnim };
		inline static REL::Relocation<std::uintptr_t> FixMotionFeedback{ AID::FixMotionFeedback, Offsets::FixMotionFeedback };
		inline static REL::Relocation<std::uintptr_t> FixSittingRotationX{ AID::FixSittingRotationX, Offsets::FixSittingRotationX };
		inline static REL::Relocation<std::uintptr_t> FixSittingRotationY{ AID::FixSittingRotationX, Offsets::FixSittingRotationY };

		static DHavok m_Instance;
	};

}
