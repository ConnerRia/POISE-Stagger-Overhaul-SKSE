#pragma once

#include <shared_mutex>
#include <unordered_set>
#include "..\Loki_PluginTools.h"
#include "TrueHUDControl.h"

namespace Loki {
    class PoiseMod {

    public:
        bool ConsoleInfoDump;

        float poiseBreakThreshhold0, poiseBreakThreshhold1, poiseBreakThreshhold2;
        float BowMult, CrossbowMult, Hand2Hand, OneHandAxe, OneHandDagger, OneHandMace, OneHandSword, TwoHandAxe, TwoHandSword;
		float RapierMult, PikeMult, SpearMult, HalberdMult, QtrStaffMult, CaestusMult, ClawMult, WhipMult, TwinbladeMult;
		float PowerAttackMult, BlockedMult, BashMult, HyperArmourMult, NPCHyperArmourMult, SpellHyperArmourMult, NPCSpellHyperArmourMult;
		float BashParryMult, GlobalPHealthMult, GlobalPDmgMult, GlobalPlayerPHealthMult, GlobalPlayerPDmgMult;
		float MaxPoiseLevelWeight, MaxPoiseLevelWeightPlayer, PhysicalDmgWeight, PhysicalDmgWeightPlayer, SpellPoiseEffectWeight, SpellPoiseEffectWeightP, SpellPoiseConcMult;
		float ArmorLogarithmSlope, ArmorLogarithmSlopePlayer, WardPowerWeight, CreatureHPMultiplier, CreatureDMGMultiplier, HyperArmorLogSlope, SpellHyperLogSlope; 
        bool PlayerPoiseEnabled, NPCPoiseEnabled, PlayerRagdollReplacer, NPCRagdollReplacer, PoiseRegenEnabled, TrueHUDBars;
		bool ScalePHealthGlobal, ScalePDmgGlobal, SpellPoise, PlayerSpellPoise, ForceThirdPerson, UseOldFormula; 
		uint32_t StaggerDelay;

        const RE::BSFixedString ae_Stagger = "staggerStart";
        const RE::BSFixedString staggerDire = "staggerDirection";
        const RE::BSFixedString staggerMagn = "staggerMagnitude";
        const RE::BSFixedString isBlocking = "IsBlocking";
        const RE::BSFixedString isAttacking = "IsAttacking";
        const RE::BSFixedString setStaggerDire = "setstagger";

        const RE::BSFixedString poiseSmallBwd = "poise_small_start";
        const RE::BSFixedString poiseSmallFwd = "poise_small_start_fwd";

        const RE::BSFixedString poiseMedBwd = "poise_med_start";
        const RE::BSFixedString poiseMedFwd = "poise_med_start_fwd";

        const RE::BSFixedString poiseLargeBwd = "poise_large_start";
        const RE::BSFixedString poiseLargeFwd = "poise_large_start_fwd";

        const RE::BSFixedString poiseLargestBwd = "poise_largest_start";
        const RE::BSFixedString poiseLargestFwd = "poise_largest_start_fwd";

        RE::SpellItem* poiseDelaySpell = NULL;
        RE::EffectSetting* poiseDelayEffect = NULL;
        RE::BGSKeyword* kCreature = NULL;
        RE::BGSKeyword* kDragon = NULL;
        RE::BGSKeyword* kGiant = NULL;
        RE::BGSKeyword* kGhost = NULL;
        RE::BGSKeyword* kDwarven = NULL;
        RE::BGSKeyword* kTroll = NULL;
        RE::BGSKeyword* WeapMaterialSilver = NULL;

        RE::EffectSetting* HardcodeFus1 = NULL;
		RE::EffectSetting* HardcodeFus2 = NULL;
		RE::EffectSetting* HardcodeDisarm1 = NULL;
		RE::EffectSetting* HardcodeDisarm2 = NULL;
        //hardcode fus and disarm because the first two words of a shout will not get processed by whitelist for whatever reason
 
 
        //RE::BGSKeyword* PoiseDmgNerf = NULL;
        //RE::BGSKeyword* PoiseDmgBuff = NULL;
        //RE::BGSKeyword* PoiseHPNerf = NULL;
        //RE::BGSKeyword* PoiseHPBuff = NULL;

        static inline std::unordered_map<RE::TESRace*, std::vector<float>> poiseRaceMap;

        PoiseMod();
        static void ReadPoiseTOML();
		static void ReadStaggerList();
        static PoiseMod* GetSingleton();

        static void InstallStaggerHook();
        static void InstallWaterHook();
        static void InstallIsActorKnockdownHook();
        //static void InstallMagicEventSink();

        static float CalculatePoiseDamage(RE::HitData& a_hitData, RE::Actor* a_actor);
        static float CalculateMaxPoise(RE::Actor* a_actor);
		static inline std::unordered_set<RE::EffectSetting*> StaggerEffectList;

		static bool IsTargetVaild(RE::Actor* a_this, RE::TESObjectREFR& a_target);

    private:
        static bool IsActorKnockdown(RE::Character* a_this, std::int64_t a_unk);
		static float GetSubmergedLevel(RE::Actor* a_ref, float a_zPos, RE::TESObjectCELL* a_cell);
        static void ProcessHitEvent(RE::Actor* a_actor, RE::HitData& a_hitData);

        static inline REL::Relocation<decltype(GetSubmergedLevel)> _GetSubmergedLevel;
        static inline REL::Relocation<decltype(ProcessHitEvent)> _ProcessHitEvent;
        static inline REL::Relocation<decltype(IsActorKnockdown)> _IsActorKnockdown;

    protected:

    };

    class PoiseMagicDamage
	{
	public:
		static PoiseMagicDamage* GetSingleton();
		static void PoiseCalculateMagic(RE::MagicCaster* a_magicCaster, RE::Projectile* a_projectile, RE::TESObjectREFR* a_target);

		//auto ProcessEvent(const RE::TESHitEvent* a_event, RE::BSTEventSource<RE::TESHitEvent>* a_eventSource)->RE::BSEventNotifyControl override;

		// This hook includes magic projectile from spells and enchanted arrows
		struct MagicProjectileHitHook
		{
			static void thunk(RE::MagicCaster* a_magicCaster, void* a_unk1, RE::Projectile* a_projectile, RE::TESObjectREFR* a_target, float a_unk2, float a_unk3)
			{
				//auto attacker = a_magicCaster->GetCaster();

				func(a_magicCaster, a_unk1, a_projectile, a_target, a_unk2, a_unk3);
				GetSingleton()->PoiseCalculateMagic(a_magicCaster, a_projectile, a_target);
			}
			static inline REL::Relocation<decltype(thunk)> func;
		};

		// This struct is not available in CommonLibSSE
		struct ExplosionHitData
		{
			RE::MagicCaster* caster;	   // 00
			uint64_t unk08;				   // 08
			RE::Effect* applyEffect;	   // 10
			RE::Effect* mainEffect;		   // 18
			RE::MagicItem* spell;		   // 20
			uint64_t unk28;				   // 28
			RE::TESObjectREFR* aggressor;  // 30
			float magnitudeOverride;	   // 38
			uint32_t unk3C;				   // 3C
			RE::NiPoint3* location;		   // 40
			float area;					   // 48
			uint32_t unk4C;				   // 4C
			uint64_t unk50;				   // 50
			uint32_t unk58;				   // 58
			bool unk5C;					   // 5C
			bool unk5D;					   // 5D
			bool unk5E;					   // 5E
			bool unk5F;					   // 5F
		};

		static_assert(sizeof(ExplosionHitData) == 0x60);
		static void PoiseCalculateExplosion(ExplosionHitData* a_hitData, RE::TESObjectREFR* a_target);

		struct MagicExplosionHitHook
		{
			// SE version patch
			struct Patch : public Xbyak::CodeGenerator
			{
				Patch(size_t a_retAddr)
				{
					Xbyak::Label callLbl;
					Xbyak::Label retLbl;
					Xbyak::Label retNullLbl;
					Xbyak::Label retNoFFLbl;

					test(r15, r15);
					je("NULL");

					// HitData is at register r12
					mov(rcx, r12);
					mov(rdx, r15);
					call(ptr[rip + callLbl]);

					test(al, al);
					je("NoFF");

					jmp(ptr[rip + retLbl]);

					L("NoFF");
					jmp(ptr[rip + retNoFFLbl]);

					L("NULL");
					jmp(ptr[rip + retNullLbl]);

					L(callLbl);
					dq((size_t)&thunk);

					L(retLbl);
					dq(a_retAddr);

					L(retNullLbl);
					dq(a_retAddr + 0x9);

					L(retNoFFLbl);
					dq(a_retAddr + 0x287);

					ready();
				}
			};

#ifdef SKYRIM_AE
			static uint32_t thunk(ExplosionHitData* a_hitData, RE::TESObjectREFR* a_target)
			{
				GetSingleton()->PoiseCalculateExplosion(a_hitData, a_target);
				return func(a_hitData, a_target);
			}
#else
			static bool thunk(ExplosionHitData* a_hitData, RE::TESObjectREFR* a_target)
			{
				// Cancel all effects if at least one of effect is hostile and target is friendly
				GetSingleton()->PoiseCalculateExplosion(a_hitData, a_target);
				return true;
			}
#endif
			static inline REL::Relocation<decltype(thunk)> func;
		};


		static void InstallNFACMagicHook()
		{
#ifdef SKYRIM_AE
			stl::write_thunk_call<MagicProjectileHitHook>(REL::ID(44206).address() + 0x218);
			stl::write_thunk_call<MagicExplosionHitHook>(REL::ID(34410).address() + 0xB9A);
#else
			stl::write_thunk_call<MagicProjectileHitHook>(REL::ID(43015).address() + 0x216);
			auto& trampoline = SKSE::GetTrampoline();
			REL::Relocation<uintptr_t> hook(REL::ID(33686), 0x73);
			MagicExplosionHitHook::Patch patch(hook.address() + 5);
			SKSE::AllocTrampoline(14 + patch.getSize());
			uintptr_t code = (uintptr_t)trampoline.allocate(patch);
			trampoline.write_branch<5>(hook.address(), code);
#endif
		}


	protected:
		PoiseMagicDamage() = default;
		PoiseMagicDamage(const PoiseMagicDamage&) = delete;
		PoiseMagicDamage(PoiseMagicDamage&&) = delete;
		virtual ~PoiseMagicDamage() = default;

		auto operator=(const PoiseMagicDamage&) -> PoiseMagicDamage& = delete;
		auto operator=(PoiseMagicDamage&&) -> PoiseMagicDamage& = delete;
	};
};