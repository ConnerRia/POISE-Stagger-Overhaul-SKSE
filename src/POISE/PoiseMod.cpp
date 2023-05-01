#include "PoiseMod.h"
#include "../ActorCache.h"
#include <ctime>


void Loki::PoiseMod::ReadPoiseTOML() {

    constexpr auto path = L"Data/SKSE/Plugins/loki_POISE";
    constexpr auto ext = L".toml";
    constexpr auto basecfg = L"Data/SKSE/Plugins/loki_POISE/loki_POISE_RaceSettings.toml";

    auto dataHandle = RE::TESDataHandler::GetSingleton();

    const auto readToml = [&](std::filesystem::path path) {
        logger::info("Reading {}...", path.string());
        try {

            const auto tbl = toml::parse_file(path.c_str());
            auto& arr = *tbl.get_as<toml::array>("race");
            for (auto&& elem : arr) {
                auto& raceTable = *elem.as_table();

                auto formID = raceTable["FormID"].value<RE::FormID>();
                logger::info("FormID -> {0:#x}", *formID);
                auto plugin = raceTable["Plugin"].value<std::string_view>();
                logger::info("plugin -> {}", *plugin);
                auto race = dataHandle->LookupForm<RE::TESRace>(*formID, *plugin);

                auto poiseValues = raceTable["PoiseValues"].as_array();
                if (poiseValues) {
                    std::vector<float> vals = {};
                    for (auto& value : *poiseValues) {
                        logger::info("value -> {}", *value.value<float>());
                        vals.push_back(*value.value<float>());
                    }
                    poiseRaceMap.insert_or_assign(race ? race : nullptr, vals);
                }
            }

        } catch (const toml::parse_error& e) {
			logger::error(
				"Error parsing file \'{}\':\n"
				"\t{}\n"
				"\t\t({})",
				*e.source().path, e.description(), e.source().begin.line
			);
        } catch (const std::exception& e) {
            logger::error("{}", e.what());
        } catch (...) {
            logger::error("Unknown failure");
        }
    };

    logger::info("Reading .toml files");

    auto baseToml = std::filesystem::path(basecfg);
    readToml(baseToml);
    if (std::filesystem::is_directory(path)) {
        for (const auto& file : std::filesystem::directory_iterator(path)) {
            if (std::filesystem::is_regular_file(file) && file.path().extension() == ext) {
                auto filePath = file.path();
                if (filePath != basecfg) {
                    readToml(filePath);
                }
            }
        }
    }

    logger::info("Success");

    return;

}

void Loki::PoiseMod::ReadStaggerList() {
	logger::info("Reading Poisebreaker stagger whitelist...");

	constexpr auto whitelistpath = L"Data/SKSE/Plugins/Poisebreaker_EffectWhitelist";
	constexpr auto ext = L".ini";
	const auto dataHandler = RE::TESDataHandler::GetSingleton();

    const auto ReadInis = [&](std::filesystem::path path) {
		logger::info("  Reading {}...", path.string());
		CSimpleIniA ini;
		ini.SetUnicode();
		ini.LoadFile(path.string().c_str());
		CSimpleIniA::TNamesDepend StaggerEffects;
		ini.GetAllValues("StaggerWhitelist", "Effect", StaggerEffects);
		for (const auto& entry : StaggerEffects) {
			std::string str = entry.pItem;
			auto split = str.find(':');
			auto modName = str.substr(0, split);
			auto formIDstr = str.substr(split + 1, str.find(' ', split + 1) - (split + 1));
			RE::FormID formID = std::stoi(formIDstr.data(), 0, 16);
			auto Effect = dataHandler->LookupForm<RE::EffectSetting>(formID, modName);
			if (Effect) {
				StaggerEffectList.emplace(Effect);
			}
		}
	};

	if (std::filesystem::is_directory(whitelistpath))
	{
		for (const auto& file : std::filesystem::directory_iterator(whitelistpath)) 
		{
			if (std::filesystem::is_regular_file(file) && file.path().extension() == ext) {
				auto path = file.path();
                ReadInis(path);
			}
		}
	}

    logger::info("Poisebreaker whitelist read complete");
}



Loki::PoiseMod::PoiseMod() {

    Loki::PoiseMod::ReadPoiseTOML();
	Loki::PoiseMod::ReadStaggerList();

    CSimpleIniA ini;
    ini.SetUnicode();
    auto filename = L"Data/SKSE/Plugins/loki_POISE.ini";
    [[maybe_unused]] SI_Error rc = ini.LoadFile(filename);

    this->ConsoleInfoDump = ini.GetBoolValue("DEBUG", "bConsoleInfoDump", false);

    this->PlayerPoiseEnabled    = ini.GetBoolValue("MAIN", "bPlayerPoiseEnabled", false);
    this->NPCPoiseEnabled       = ini.GetBoolValue("MAIN", "bNPCPoiseEnabled", false);
    this->PlayerRagdollReplacer = ini.GetBoolValue("MAIN", "bPlayerRagdollReplacer", false);
    this->NPCRagdollReplacer    = ini.GetBoolValue("MAIN", "bNPCRagdollReplacer", false);
    this->PoiseRegenEnabled     = ini.GetBoolValue("MAIN", "bPoiseRegen", false);
    this->TrueHUDBars           = ini.GetBoolValue("MAIN", "bTrueHUDBars", false);
	this->ForceThirdPerson      = ini.GetBoolValue("MAIN", "bForceFirstPersonStagger", false);
	this->SpellPoise            = ini.GetBoolValue("MAIN", "bNPCSpellPoise", false);
	this->PlayerSpellPoise      = ini.GetBoolValue("MAIN", "bPlayerSpellPoise", false);
	this->StaggerDelay          = (uint32_t)ini.GetLongValue("MAIN", "fStaggerDelay", 3);

    this->poiseBreakThreshhold0 = (float)ini.GetDoubleValue("MAIN", "fPoiseBreakThreshhold0", -1.00f);
	this->poiseBreakThreshhold1 = (float)ini.GetDoubleValue("MAIN", "fPoiseBreakThreshhold1", -1.00f);
	this->poiseBreakThreshhold2 = (float)ini.GetDoubleValue("MAIN", "fPoiseBreakThreshhold2", -1.00f);

   // this->HyperArmourMult = (float)ini.GetDoubleValue("HYPERARMOR", "fHyperArmourMult", -1.00f);
	this->NPCHyperArmourMult = (float)ini.GetDoubleValue("HYPERARMOR", "fNPCHyperArmourMult", -1.00f);
	//this->SpellHyperArmourMult = (float)ini.GetDoubleValue("HYPERARMOR", "fSpellHyperArmourMult", -1.00f);
	this->NPCSpellHyperArmourMult = (float)ini.GetDoubleValue("HYPERARMOR", "fNPCSpellHyperArmourMult", -1.00f);

    this->PowerAttackMult = (float)ini.GetDoubleValue("MECHANICS", "fPowerAttackMult", -1.00f);
    this->BlockedMult     = (float)ini.GetDoubleValue("MECHANICS", "fBlockedMult", -1.00f);
    this->BashMult        = (float)ini.GetDoubleValue("MECHANICS", "fBashMult", -1.00f);
	this->BashParryMult   = (float)ini.GetDoubleValue("MECHANICS", "fBashParryMult", -1.00f);

    this->ScalePHealthGlobal      = ini.GetBoolValue("GLOBALMULT", "bScalePHealthGlobal", false);
	this->ScalePDmgGlobal         = ini.GetBoolValue("GLOBALMULT", "bScalePDmgGlobal", false);
	this->GlobalPHealthMult       = (float)ini.GetDoubleValue("GLOBALMULT", "fGlobalPHealthMult", -1.00f);
	this->GlobalPDmgMult          = (float)ini.GetDoubleValue("GLOBALMULT", "fGlobalPDmgMult", -1.00f);
	this->GlobalPlayerPHealthMult = (float)ini.GetBoolValue("GLOBALMULT", "fGlobalPlayerPHealthMult", false);
	this->GlobalPlayerPDmgMult    = (float)ini.GetBoolValue("GLOBALMULT", "fGlobalPlayerPDmgMult", false);

    this->BowMult         = (float)ini.GetDoubleValue("WEAPON", "fBowMult", -1.00f);
    this->CrossbowMult    = (float)ini.GetDoubleValue("WEAPON", "fCrossbowMult", -1.00f);
    this->Hand2Hand       = (float)ini.GetDoubleValue("WEAPON", "fHand2HandMult", -1.00f);
    this->OneHandAxe      = (float)ini.GetDoubleValue("WEAPON", "fOneHandAxeMult", -1.00f);
    this->OneHandDagger   = (float)ini.GetDoubleValue("WEAPON", "fOneHandDaggerMult", -1.00f);
    this->OneHandMace     = (float)ini.GetDoubleValue("WEAPON", "fOneHandMaceMult", -1.00f);
    this->OneHandSword    = (float)ini.GetDoubleValue("WEAPON", "fOneHandSwordMult", -1.00f);
    this->TwoHandAxe      = (float)ini.GetDoubleValue("WEAPON", "fTwoHandAxeMult", -1.00f);
    this->TwoHandSword    = (float)ini.GetDoubleValue("WEAPON", "fTwoHandSwordMult", -1.00f);

    this->RapierMult   = (float)ini.GetDoubleValue("ANIMATED_ARMOURY", "fRapierMult", -1.00f);
    this->PikeMult     = (float)ini.GetDoubleValue("ANIMATED_ARMOURY", "fPikeMult", -1.00f);
    this->SpearMult    = (float)ini.GetDoubleValue("ANIMATED_ARMOURY", "fSpearMult", -1.00f);
    this->HalberdMult  = (float)ini.GetDoubleValue("ANIMATED_ARMOURY", "fHalberdMult", -1.00f);
    this->QtrStaffMult = (float)ini.GetDoubleValue("ANIMATED_ARMOURY", "fQtrStaffMult", -1.00f);
    this->CaestusMult  = (float)ini.GetDoubleValue("ANIMATED_ARMOURY", "fCaestusMult", -1.00f);
    this->ClawMult     = (float)ini.GetDoubleValue("ANIMATED_ARMOURY", "fClawMult", -1.00f);
    this->WhipMult     = (float)ini.GetDoubleValue("ANIMATED_ARMOURY", "fWhipMult", -1.00f);

    this->TwinbladeMult = (float)ini.GetDoubleValue("CUSTOM_WEAPONS", "fTwinbladeMult", -1.00f);

    this->UseOldFormula             = ini.GetBoolValue("FORMULAE", "bUseOldFormula", false);
	this->PhysicalDmgWeight         = (float)ini.GetDoubleValue("FORMULAE", "fPhysicalDmgWeight", -1.00f);
	this->PhysicalDmgWeightPlayer   = (float)ini.GetDoubleValue("FORMULAE", "fPhysicalDmgWeightPlayer", -1.00f);
	this->MaxPoiseLevelWeight       = (float)ini.GetDoubleValue("FORMULAE", "fMaxPoiseLevelWeight", -1.00f);
	this->MaxPoiseLevelWeightPlayer = (float)ini.GetDoubleValue("FORMULAE", "fMaxPoiseLevelWeightPlayer", -1.00f);
	this->ArmorLogarithmSlope       = (float)ini.GetDoubleValue("FORMULAE", "fMaxPoiseArmorRatingSlope", -1.00f);
	this->ArmorLogarithmSlopePlayer = (float)ini.GetDoubleValue("FORMULAE", "fMaxPoiseArmorRatingSlopePlayer", -1.00f);
	this->HyperArmorLogSlope        = (float)ini.GetDoubleValue("FORMULAE", "fHyperArmorSlopePlayer", -1.00f);
	this->SpellHyperLogSlope        = (float)ini.GetDoubleValue("FORMULAE", "fSpellHyperArmorSlopePlayer", -1.00f);
	this->CreatureHPMultiplier      = (float)ini.GetDoubleValue("FORMULAE", "fCreatureTOMLMaxPoiseMult", -1.00f);
	this->CreatureDMGMultiplier     = (float)ini.GetDoubleValue("FORMULAE", "fCreatureTOMLPoiseDmgMult", -1.00f);
	this->SpellPoiseEffectWeight    = (float)ini.GetDoubleValue("FORMULAE", "fSpellPoiseEffectWeight", -1.00f);
	this->SpellPoiseEffectWeightP   = (float)ini.GetDoubleValue("FORMULAE", "fSpellPoiseEffectWeightPlayer", -1.00f);
	this->SpellPoiseConcMult        = (float)ini.GetDoubleValue("FORMULAE", "fSpellPoiseConcentrationMult", -1.00f);
	this->WardPowerWeight           = (float)ini.GetDoubleValue("FORMULAE", "fWardPowerWeight", -1.00f);


    if (auto dataHandle = RE::TESDataHandler::GetSingleton(); dataHandle) {
        poiseDelaySpell  = dataHandle->LookupForm<RE::SpellItem>(0xD62, "loki_POISE.esp");
        poiseDelayEffect = dataHandle->LookupForm<RE::EffectSetting>(0xD63, "loki_POISE.esp");
		HardcodeFus1     = dataHandle->LookupForm<RE::EffectSetting>(0x7F82E, "Skyrim.esm");
		HardcodeFus2     = dataHandle->LookupForm<RE::EffectSetting>(0x13E08, "Skyrim.esm");
		HardcodeDisarm1  = dataHandle->LookupForm<RE::EffectSetting>(0x8BB26, "Skyrim.esm");
		HardcodeDisarm2  = dataHandle->LookupForm<RE::EffectSetting>(0x0CD08, "Skyrim.esm");
        //Conner: only the 3rd word of shouts get handled correctly by stagger processing, so we have to hardcode the first few words of the stagger shouts.
       
        //PoiseDmgNerf     = dataHandle->LookupForm<RE::BGSKeyword>(0x433C, "loki_POISE.esp");
        //PoiseDmgBuff     = dataHandle->LookupForm<RE::BGSKeyword>(0x433B, "loki_POISE.esp");
        //PoiseHPNerf      = dataHandle->LookupForm<RE::BGSKeyword>(0x433A, "loki_POISE.esp");
        //PoiseHPBuff      = dataHandle->LookupForm<RE::BGSKeyword>(0x4339, "loki_POISE.esp");

        kCreature = dataHandle->LookupForm<RE::BGSKeyword>(0x13795, "Skyrim.esm");
        kDragon   = dataHandle->LookupForm<RE::BGSKeyword>(0x35d59, "Skyrim.esm");
        kGiant    = dataHandle->LookupForm<RE::BGSKeyword>(0x10E984, "Skyrim.esm");
        kGhost    = dataHandle->LookupForm<RE::BGSKeyword>(0xd205e, "Skyrim.esm");
        kDwarven  = dataHandle->LookupForm<RE::BGSKeyword>(0x1397a, "Skyrim.esm");
        kTroll    = dataHandle->LookupForm<RE::BGSKeyword>(0xf5d16, "Skyrim.esm");
        WeapMaterialSilver = dataHandle->LookupForm<RE::BGSKeyword>(0x10aa1a, "Skyrim.esm");
    }

}

Loki::PoiseMod* Loki::PoiseMod::GetSingleton() {
    static Loki::PoiseMod* singleton = new Loki::PoiseMod();
    return singleton;
}

void Loki::PoiseMod::InstallStaggerHook() {
	REL::Relocation<std::uintptr_t> StaggerHook{ RELOCATION_ID(37673, 38627), REL::Relocate(0x3C0, 0x4A8) };

	auto& trampoline = SKSE::GetTrampoline();
	_ProcessHitEvent = trampoline.write_call<5>(StaggerHook.address(), ProcessHitEvent);

	logger::info("ProcessHitEvent hook injected");
}

void Loki::PoiseMod::InstallWaterHook() {
	REL::Relocation<std::uintptr_t> ActorUpdate{ RELOCATION_ID(36357, 37348), REL::Relocate(0x6D3, 0x674) };
	// last ditch effort
	auto& trampoline = SKSE::GetTrampoline();
	_GetSubmergedLevel = trampoline.write_call<5>(ActorUpdate.address(), GetSubmergedLevel);

	logger::info("Update hook injected");
}

void Loki::PoiseMod::InstallIsActorKnockdownHook() {
	REL::Relocation<std::uintptr_t> isActorKnockdown{ RELOCATION_ID(38858, 39895), REL::Relocate(0x7E, 0x68) };

	auto& trampoline = SKSE::GetTrampoline();
	_IsActorKnockdown = trampoline.write_call<5>(isActorKnockdown.address(), IsActorKnockdown);

	logger::info("isActorKnockdown hook injected");
}

/*
void Loki::PoiseMod::InstallMagicEventSink() {
    auto sourceHolder = RE::ScriptEventSourceHolder::GetSingleton();
    if (sourceHolder) { sourceHolder->AddEventSink(PoiseMagicDamage::GetSingleton()); }
}
*/

/*
* im keeping this here because i always forget how to hook vfuncs
void Loki::PoiseMod::InstallVFuncHooks() {
    REL::Relocation<std::uintptr_t> CharacterVtbl{ REL::ID(261397) };  // 165DA40
    _HandleHealthDamage_Character = CharacterVtbl.write_vfunc(0x104, HandleHealthDamage_Character);

    REL::Relocation<std::uintptr_t> PlayerCharacterVtbl{ RE::Offset::PlayerCharacter::Vtbl };
    _HandleHealthDamage_PlayerCharacter = PlayerCharacterVtbl.write_vfunc(0x104, HandleHealthDamage_PlayerCharacter);
}
*/

Loki::PoiseMagicDamage* Loki::PoiseMagicDamage::GetSingleton() {
    static PoiseMagicDamage singleton;
    return &singleton;
}

bool Loki::PoiseMod::IsTargetVaild(RE::Actor* a_this, RE::TESObjectREFR& a_target)
{
	bool isValid = true;
	if (a_target.Is(RE::FormType::ActorCharacter)) {
		auto target = static_cast<RE::Actor*>(&a_target);
		if (!target->IsDead() && !target->IsHostileToActor(a_this)) {
			auto targetOwnerHandle = target->GetCommandingActor();
			auto thisOwnerHandle = a_this->GetCommandingActor();
			auto targetOwner = targetOwnerHandle ? targetOwnerHandle.get() : nullptr;
			auto thisOwner = thisOwnerHandle ? thisOwnerHandle.get() : nullptr;

			bool isThisPlayer = a_this->IsPlayerRef();
			bool isThisTeammate = a_this->IsPlayerTeammate();
			bool isThisSummonedByPC = a_this->IsSummoned() && thisOwner && thisOwner->IsPlayerRef();
			bool isTargetPlayer = target->IsPlayerRef();
			bool isTargetTeammate = target->IsPlayerTeammate();
			bool isTargetSummonedByPC = target->IsSummoned() && targetOwner && targetOwner->IsPlayerRef();
			//bool isTargetAGuard = target->IsGuard();
			bool isTargetAMount = target->IsAMount();

			// Prevent player's team from hitting each other, a guard or a mount.
			if ((isThisPlayer || isThisTeammate || isThisSummonedByPC) &&
				(isTargetPlayer || isTargetTeammate || isTargetSummonedByPC || isTargetAMount))
				isValid = false;

			// If the target is commanded by the hostile actor, do not protect it
			auto targetCommanderHandle = target->GetCommandingActor();
			if (targetCommanderHandle) {
				auto targetCommander = targetCommanderHandle.get();
				if (targetCommander->IsHostileToActor(a_this))
					isValid = true;
				else
					isValid = false;
			} else
				isValid = false;
		}
	}

	return isValid;
}

void Loki::PoiseMagicDamage::PoiseCalculateMagic(RE::MagicCaster* a_magicCaster, RE::Projectile* a_projectile, RE::TESObjectREFR* a_target)
{
	if (!a_magicCaster || !a_projectile) {
		//RE::ConsoleLog::GetSingleton()->Print("no caster or no projectile.");
		return;
    }

	if (!a_target) {
		//logger::info("Spell Poise: no target found.");
		return;
    } 
    else {	   
        auto MAttacker = a_magicCaster->GetCasterAsActor();
		if (MAttacker == NULL) {
			return;
        }           
		else if (MAttacker) {
			if (auto actor = a_target->As<RE::Actor>(); actor) {
				if (!PoiseMod::IsTargetVaild(MAttacker, *a_target)) {
					return;
				}

                auto ptr = Loki::PoiseMod::GetSingleton();

				auto nowTime = uint32_t(std::time(nullptr) % UINT32_MAX);
				auto lastStaggerTime = actor->GetActorRuntimeData().pad1EC;
				if (lastStaggerTime > nowTime) {
					actor->GetActorRuntimeData().pad1EC = nowTime;
					lastStaggerTime = 0;
				} 
				else {
					if (nowTime - lastStaggerTime < ptr->StaggerDelay) {
						//logger::info("{} last stagger time {} is less than stagger delay {}", actor->GetName(), lastStaggerTime, ptr->StaggerDelay);
						return;
					}
				}

				auto EffectSetting = a_projectile->GetProjectileRuntimeData().avEffect->As<RE::EffectSetting>();

				if (!EffectSetting) {
					//logger::info("Could not find MagicEffect from projectile");
					return;
				}

				static RE::BSFixedString str = NULL;
				auto cam = RE::PlayerCamera::GetSingleton();

				// if stagger effect is whitelisted skip other nullchecks and instantly stagger.
				if (ptr->StaggerEffectList.contains(EffectSetting) || EffectSetting == ptr->HardcodeFus1 || EffectSetting == ptr->HardcodeFus2 || EffectSetting == ptr->HardcodeDisarm1 || EffectSetting == ptr->HardcodeDisarm2) {
					actor->GetActorRuntimeData().pad0EC = 0;
					auto hitPos = MAttacker->GetPosition();
					auto heading = actor->GetHeadingAngle(hitPos, false);
					auto stagDir = (heading >= 0.0f) ? heading / 360.0f : (360.0f + heading) / 360.0f;
					if (actor->GetHandle() == MAttacker->GetHandle()) {
						stagDir = 0.0f;
					}  // 0 when self-hit

					auto caster = actor->GetMagicCaster(RE::MagicSystem::CastingSource::kInstant);
					//nullcheck
					if (caster) {
						caster->CastSpellImmediate(ptr->poiseDelaySpell, false, actor, 1.0f, false, 0.0f, 0);
					}

					if (ptr->ForceThirdPerson && actor->IsPlayerRef()) {
						if (cam->IsInFirstPerson()) {
							cam->ForceThirdPerson();  //if player is in first person, stagger them in thirdperson.
						}
					}
					actor->SetGraphVariableFloat(ptr->staggerDire, stagDir);  // set direction
					//actor->pad0EC = static_cast<std::uint32_t>(maxPoise); // remember earlier when we calculated max poise health?
					if (TrueHUDControl::GetSingleton()->g_trueHUD) {
						TrueHUDControl::GetSingleton()->g_trueHUD->FlashActorSpecialBar(SKSE::GetPluginHandle(), actor->GetHandle(), false);
					}
					if (actor->HasKeyword(ptr->kCreature) || actor->HasKeyword(ptr->kDwarven)) {  // if creature, use normal beh
						actor->SetGraphVariableFloat(ptr->staggerMagn, 1.00f);
						actor->NotifyAnimationGraph(ptr->ae_Stagger);  // play animation
					} 
					else {
						if (stagDir > 0.25f && stagDir < 0.75f) {
							str = ptr->poiseMedFwd;
						} 
						else {
							str = ptr->poiseMedBwd;
						}
						actor->NotifyAnimationGraph(str); //instant stagger
					}
					return;
				}


				//logger::info("if not whitelisted stagger, then valid attacker and aggressor, projectile calculation being attempted!");
                float a_result = 0.00f;
				float SpellMult = ptr->SpellPoiseEffectWeight;
                if (MAttacker->IsPlayerRef()) {
					SpellMult = ptr->SpellPoiseEffectWeightP;
                }

                if (!ptr->SpellPoise && !MAttacker->IsPlayerRef()) {
					return;
                }

                if (!ptr->PlayerSpellPoise && MAttacker->IsPlayerRef()) {
					return;
                }

				auto avHealth = actor->AsActorValueOwner()->GetActorValue(RE::ActorValue::kHealth);
				auto avParalysis = actor->AsActorValueOwner()->GetActorValue(RE::ActorValue::kParalysis);
				if (avHealth <= 0.05f || actor->IsInKillMove() || avParalysis) {
					//logger::info("target is in state ineligible for poise dmg");
					return;
				}

                auto Spell = a_projectile->GetProjectileRuntimeData().spell->As<RE::MagicItem>();
                if (!Spell) {
					//logger::info("Could not find spell from projectile");
					return;
                }
				auto Effect = Spell->GetCostliestEffectItem();
                if (!Effect) {
					//logger::info("Could not find applied effect from projectile");
					return;
                }

				float Magnitude = Effect->GetMagnitude();

                if (EffectSetting->IsHostile() && EffectSetting->IsDetrimental()) {
					a_result = Magnitude * SpellMult;
                    if (Spell->GetCastingType() == RE::MagicSystem::CastingType::kConcentration) {
						float SpellConcMult = ptr->SpellPoiseConcMult;
						a_result *= SpellConcMult;
                    }
					//RE::ConsoleLog::GetSingleton()->Print("Poise Damage From Cum-> %f", a_result);
				}
                else {
					//RE::ConsoleLog::GetSingleton()->Print("spell is not hostile nor in whitelist, no poise dmg");
					return;
                }

                const auto zeffect = MAttacker->AsMagicTarget()->GetActiveEffectList();
				if (zeffect) {
					for (const auto& aeffect : *zeffect) {
						if (!aeffect) {
							continue;
						}
						if (!aeffect->GetBaseObject()) {
							continue;
						}
						if ((!aeffect->flags.all(RE::ActiveEffect::Flag::kInactive)) && aeffect->GetBaseObject()->HasKeywordString("zzzSpellPoiseDamageBuffFlat")) {
							auto buffFlat = (aeffect->magnitude);
							a_result += buffFlat;  //aggressor has buff that gives them flat spell poise damage 
						}
						if ((!aeffect->flags.all(RE::ActiveEffect::Flag::kInactive)) && aeffect->GetBaseObject()->HasKeywordString("zzzSpellPoiseDamageBuffMult")) {
							auto buffPercent = (aeffect->magnitude / 100.00f);	// convert to percentage
							auto resultingBuff = (a_result * buffPercent);
							a_result += resultingBuff;	// aggressor has buff that multiplies their spell poise damage.
						}
						if ((!aeffect->flags.all(RE::ActiveEffect::Flag::kInactive)) && aeffect->GetBaseObject()->HasKeywordString("zzzSpellPoiseDamageNerfFlat")) {
							auto NerfFlat = (aeffect->magnitude);
							a_result -= NerfFlat;  //aggressor has nerf that reduces their spell poise damage by flat value
						}
						if ((!aeffect->flags.all(RE::ActiveEffect::Flag::kInactive)) && aeffect->GetBaseObject()->HasKeywordString("zzzSpellPoiseDamageNerfMult")) {
							auto nerfPercent = (aeffect->magnitude / 100.00f);	// convert to percentage
							auto resultingNerf = (a_result * nerfPercent);
							a_result -= resultingNerf;	// aggressor has nerf that nerfs their spell poise damage by multiplier.
						}
					}
				}

				bool blk;
				actor->GetGraphVariableBool("IsBlocking", blk);

				//bool shout;
				//actor->GetGraphVariableBool("IsShouting", shout);

				if (a_result < 0.00f) {
					logger::info("Magic damage was negative... somehow?");
					a_result = 0.00f;
				}

                if (blk) {
					if (actor->IsPlayerRef()) {
						float BlockMult = (actor->AsActorValueOwner()->GetBaseActorValue(RE::ActorValue::kBlock));
						if (BlockMult >= 20.0f) {
							float fLogarithm = ((BlockMult - 20) / 50 + 1);
							double PlayerBlockMult = (0.7 - log10(fLogarithm));
							PlayerBlockMult = (PlayerBlockMult >= 0.0) ? PlayerBlockMult : 0.0;
							a_result *= (float)PlayerBlockMult;
						} 
                        else {
							a_result *= 0.7f;
						}
					} 
                    else {
						a_result *= ptr->BlockedMult;
					}
				}

				if (actor->HasKeyword(ptr->kGhost)) {
					a_result = 0.00f;
				}

                
				//if (shout) {
				//	a_result = 0.00f;
				//}

				uint32_t oriPoise = 0;
				uint32_t poise = 0;
				float maxPoise = ptr->CalculateMaxPoise(actor);

				if (ptr->StaggerEffectList.contains(EffectSetting)) {
					actor->GetActorRuntimeData().pad0EC = 0;
				} else {
					auto poiseRef = std::atomic_ref(actor->GetActorRuntimeData().pad0EC);
					oriPoise = poiseRef.load();
					uint32_t newPoise = 0;
					uint32_t newPoiseSet = 0; 
					do {
						poise = poiseRef.load();
						if (poise > oriPoise){
							return;
						}
						newPoise = poise - (int)a_result;
						newPoiseSet = newPoise;
						if (newPoiseSet <= 0) {
							newPoiseSet = (int)maxPoise;
						}
						oriPoise = poise;
					} while (!poiseRef.compare_exchange_strong(poise, newPoiseSet));
					oriPoise = poise;
					poise = newPoise;

					// after change pad0EC's typedef from std::uint32_t to std::int32_t, this case shouldn't happen anymore
					if (poise > 100000) {
						//logger::info("PoiseCalculateMagic strange poise value {}", poise);
						actor->GetActorRuntimeData().pad0EC = 0;
					}
				}

				if (poise <= 0) {
					if (!std::atomic_ref(actor->GetActorRuntimeData().pad1EC).compare_exchange_strong(lastStaggerTime, nowTime)) {
						//logger::info("PoiseCalculateMagic prevent stagger lock", poise);
						return;
					}
				}

                auto threshhold0 = maxPoise * ptr->poiseBreakThreshhold0;
				if (threshhold0 <= 2) {
					threshhold0 = 2;
				}
                auto threshhold1 = maxPoise * ptr->poiseBreakThreshhold1;
				if (threshhold1 <= 5) {
					threshhold1 = 5;
				}
                auto threshhold2 = maxPoise * ptr->poiseBreakThreshhold2;
				if (threshhold2 <= 10) {
					threshhold2 = 10;
				}

				auto hitPos = MAttacker->GetPosition();
				auto heading = actor->GetHeadingAngle(hitPos, false);
                auto stagDir = (heading >= 0.0f) ? heading / 360.0f : (360.0f + heading) / 360.0f;
                if (actor->GetHandle() == MAttacker->GetHandle()) { stagDir = 0.0f; } // 0 when self-hit

                auto caster = actor->GetMagicCaster(RE::MagicSystem::CastingSource::kInstant);
                //nullcheck
				if (caster) {
					caster->CastSpellImmediate(ptr->poiseDelaySpell, false, actor, 1.0f, false, 0.0f, 0);
				}

                //Conner: for the stagger function and the modification of actor->pad0EC, we should move the code block to a new function, and lock it probably. Do later. I added 3DLoaded check i guess.

                if (Spell->GetCastingType() == RE::MagicSystem::CastingType::kConcentration) {
					//RE::ConsoleLog::GetSingleton()->Print("magic event is from concentration spell");
					if (poise <= 0) {
						if (ptr->ForceThirdPerson && actor->IsPlayerRef()) {
							if (cam->IsInFirstPerson()) {
								cam->ForceThirdPerson();  //if player is in first person, stagger them in thirdperson.
							}
						}
						actor->SetGraphVariableFloat(ptr->staggerDire, stagDir);  // set direction
						//actor->pad0EC = static_cast<std::uint32_t>(maxPoise);	  // remember earlier when we calculated max poise health?
						if (TrueHUDControl::GetSingleton()->g_trueHUD) {
							TrueHUDControl::GetSingleton()->g_trueHUD->FlashActorSpecialBar(SKSE::GetPluginHandle(), actor->GetHandle(), false);
						}
						if (actor->HasKeyword(ptr->kCreature) || actor->HasKeyword(ptr->kDwarven)) {  // if creature, use normal beh
							actor->SetGraphVariableFloat(ptr->staggerMagn, 1.00f);
							actor->NotifyAnimationGraph(ptr->ae_Stagger);  // play animation
						} else {
							if (actor->HasKeyword(ptr->kDragon)) {
								if (stagDir > 0.25f && stagDir < 0.75f) {
									str = ptr->poiseLargestFwd;
								} else {
									str = ptr->poiseLargestBwd;
								}
								actor->NotifyAnimationGraph(str);  // if those, play tier 4
							} else {
								if (stagDir > 0.25f && stagDir < 0.75f) {
									str = ptr->poiseMedFwd;
								} else {
									str = ptr->poiseMedBwd;
								}
								actor->NotifyAnimationGraph(str);  // if not those, play tier 2
							}
						}                    
                    } else {
						return; // Concentration spells spam magic poise calculation. To prevent stupid staggerlocks, we only stagger once per Poise meter for concentration spells. 
                    }
                }


                if (poise <= 0) {
					if (ptr->ForceThirdPerson && actor->IsPlayerRef()) {
						if (cam->IsInFirstPerson()) {
							cam->ForceThirdPerson();    //if player is in first person, stagger them in thirdperson.
						}
					}
                    actor->SetGraphVariableFloat(ptr->staggerDire, stagDir); // set direction
                    //actor->pad0EC = static_cast<std::uint32_t>(maxPoise); // remember earlier when we calculated max poise health?
                    if (TrueHUDControl::GetSingleton()->g_trueHUD) {
                        TrueHUDControl::GetSingleton()->g_trueHUD->
                            FlashActorSpecialBar(SKSE::GetPluginHandle(), actor->GetHandle(), false);
                    }
                    if (actor->HasKeyword(ptr->kCreature) || actor->HasKeyword(ptr->kDwarven)) { // if creature, use normal beh
                        actor->SetGraphVariableFloat(ptr->staggerMagn, 1.00f);
                        actor->NotifyAnimationGraph(ptr->ae_Stagger);          // play animation
                    } else {
						if (actor->HasKeyword(ptr->kDragon)) {
                            if (stagDir > 0.25f && stagDir < 0.75f) {
                                str = ptr->poiseLargestFwd;
                            } 
                            else {
                                str = ptr->poiseLargestBwd;
                            }
                            actor->NotifyAnimationGraph(str);         // if those, play tier 4
                        } else {
                            if (stagDir > 0.25f && stagDir < 0.75f) {
                                str = ptr->poiseMedFwd;
                            } 
                            else {
                                str = ptr->poiseMedBwd;
                            }
                            actor->NotifyAnimationGraph(str);  // if not those, play tier 2
                        }
                    }
                } else if (poise <= threshhold0) {
					if (oriPoise <= threshhold0) {
						return;
					}
					if (ptr->ForceThirdPerson && actor->IsPlayerRef()) {
						if (cam->IsInFirstPerson()) {
							cam->ForceThirdPerson();
						}
					}
                    actor->SetGraphVariableFloat(ptr->staggerDire, stagDir); // set direction
                    if (actor->HasKeyword(ptr->kCreature) || actor->HasKeyword(ptr->kDwarven)) { // if creature, use normal beh
                        actor->SetGraphVariableFloat(ptr->staggerMagn, 0.75f);
                        actor->NotifyAnimationGraph(ptr->ae_Stagger);
                    } else {
						if (actor->HasKeyword(ptr->kDragon)) {	// check if explosion, dragon, giant attack or dwarven
                            if (stagDir > 0.25f && stagDir < 0.75f) {
                                str = ptr->poiseLargeFwd;
                            } 
                            else {
                                str = ptr->poiseLargeBwd;
                            }
                            actor->NotifyAnimationGraph(str);           // if those, play tier 3
                        } 
                        else {
                            if (stagDir > 0.25f && stagDir < 0.75f) {
                                str = ptr->poiseMedFwd;
                            } 
                            else {
                                str = ptr->poiseMedBwd;
                            }
                            actor->NotifyAnimationGraph(str); // if block, set pushback, ! play tier 2
                        }
                    }
                } else if (poise <= threshhold1) {
					if (oriPoise <= threshhold1) {
						return;
					}
                    actor->SetGraphVariableFloat(ptr->staggerDire, stagDir); // set direction
                    if (actor->HasKeyword(ptr->kCreature) || actor->HasKeyword(ptr->kDwarven)) {
                        actor->SetGraphVariableFloat(ptr->staggerMagn, 0.50f);
                        actor->NotifyAnimationGraph(ptr->ae_Stagger);
                    } else {
                        if (actor->HasKeyword(ptr->kDragon)) {
                            if (stagDir > 0.25f && stagDir < 0.75f) {
                                str = ptr->poiseLargeFwd;
                            } 
                            else {
                                str = ptr->poiseLargeBwd;
                            }
                            actor->NotifyAnimationGraph(str);  // play tier 3 again
                        } 
                        else {
                            if (stagDir > 0.25f && stagDir < 0.75f) {
                                str = ptr->poiseSmallFwd;
                            } 
                            else {
                                str = ptr->poiseSmallBwd;
                            }
                            actor->NotifyAnimationGraph(str);
                        }
                    }
				} else if (poise <= threshhold2) {
					if (oriPoise <= threshhold2) {
						return;
					}
                    actor->SetGraphVariableFloat(ptr->staggerDire, stagDir); // set direction
                    if (actor->HasKeyword(ptr->kCreature) || actor->HasKeyword(ptr->kDwarven)) {
                        actor->SetGraphVariableFloat(ptr->staggerMagn, 0.25f);
                        actor->NotifyAnimationGraph(ptr->ae_Stagger);
                    } 
                    else {
                        if (stagDir > 0.25f && stagDir < 0.75f) {
                            str = ptr->poiseSmallFwd;
                        } 
                        else {
                            str = ptr->poiseSmallBwd;
                        }
                        actor->NotifyAnimationGraph(str);
                    }
                }
            }
        }
    }
    return;
}

void Loki::PoiseMagicDamage::PoiseCalculateExplosion(ExplosionHitData* a_hitData, RE::TESObjectREFR* a_target)
{
	if (!a_target) {
		return;

	} else {
		auto MAttacker = a_hitData->caster->GetCasterAsActor();
		if (MAttacker == NULL) {
			return;
		} else if (MAttacker) {
			if (auto actor = a_target->As<RE::Actor>(); actor) {
				if (!PoiseMod::IsTargetVaild(MAttacker, *a_target)) {
					return;
				}
				auto ptr = Loki::PoiseMod::GetSingleton();

				auto nowTime = uint32_t(std::time(nullptr) % UINT32_MAX);
				auto lastStaggerTime = actor->GetActorRuntimeData().pad1EC;
				if (lastStaggerTime > nowTime) {
					actor->GetActorRuntimeData().pad1EC = nowTime;
					lastStaggerTime = 0;
				} else {
					if (nowTime - lastStaggerTime < ptr->StaggerDelay) {
						//logger::info("{} last stagger time {} is less than stagger delay {}", actor->GetName(), lastStaggerTime, ptr->StaggerDelay);
						return;
					}
				}

				float a_result = 0.00f;
				float SpellMult = ptr->SpellPoiseEffectWeight;
				if (MAttacker->IsPlayerRef()) {
					SpellMult = ptr->SpellPoiseEffectWeightP;
				}

				if (!ptr->SpellPoise && !MAttacker->IsPlayerRef()) {
					return;
				}

				if (!ptr->PlayerSpellPoise && MAttacker->IsPlayerRef()) {
					return;
				}

				auto avHealth = actor->AsActorValueOwner()->GetActorValue(RE::ActorValue::kHealth);
				auto avParalysis = actor->AsActorValueOwner()->GetActorValue(RE::ActorValue::kParalysis);
				if (avHealth <= 0.05f || actor->IsInKillMove() || avParalysis) {
					return;
				}

				auto Spell = a_hitData->spell->As<RE::MagicItem>();
				if (!Spell) {
					return;
				}
				auto Effect = Spell->GetCostliestEffectItem();
				if (!Effect) {
					return;
				}

				float Magnitude = Effect->GetMagnitude();

				auto EffectSetting = a_hitData->mainEffect->baseEffect->As<RE::EffectSetting>();
				if (!EffectSetting) {
					return;
				}


				if (ptr->StaggerEffectList.contains(EffectSetting)) {
					a_result = 8.00f;
				} else if (EffectSetting->IsHostile() && EffectSetting->IsDetrimental()) {
					a_result = Magnitude * SpellMult;
					if (Spell->GetCastingType() == RE::MagicSystem::CastingType::kConcentration) {
						float SpellConcMult = ptr->SpellPoiseConcMult;
						a_result *= SpellConcMult;
					}
				} else {
					return;
				}

				const auto zeffect = MAttacker->AsMagicTarget()->GetActiveEffectList();
				if (zeffect) {
					for (const auto& aeffect : *zeffect) {
						if (!aeffect) {
							continue;
						}
						if (!aeffect->GetBaseObject()) {
							continue;
						}
						if ((!aeffect->flags.all(RE::ActiveEffect::Flag::kInactive)) && aeffect->GetBaseObject()->HasKeywordString("zzzSpellPoiseDamageBuffFlat")) {
							auto buffFlat = (aeffect->magnitude);
							a_result += buffFlat;  //aggressor has buff that gives them flat spell poise damage
						}
						if ((!aeffect->flags.all(RE::ActiveEffect::Flag::kInactive)) && aeffect->GetBaseObject()->HasKeywordString("zzzSpellPoiseDamageBuffMult")) {
							auto buffPercent = (aeffect->magnitude / 100.00f);	// convert to percentage
							auto resultingBuff = (a_result * buffPercent);
							a_result += resultingBuff;	// aggressor has buff that multiplies their spell poise damage.
						}
						if ((!aeffect->flags.all(RE::ActiveEffect::Flag::kInactive)) && aeffect->GetBaseObject()->HasKeywordString("zzzSpellPoiseDamageNerfFlat")) {
							auto NerfFlat = (aeffect->magnitude);
							a_result -= NerfFlat;  //aggressor has nerf that reduces their spell poise damage by flat value
						}
						if ((!aeffect->flags.all(RE::ActiveEffect::Flag::kInactive)) && aeffect->GetBaseObject()->HasKeywordString("zzzSpellPoiseDamageNerfMult")) {
							auto nerfPercent = (aeffect->magnitude / 100.00f);	// convert to percentage
							auto resultingNerf = (a_result * nerfPercent);
							a_result -= resultingNerf;	// aggressor has nerf that nerfs their spell poise damage by multiplier.
						}
					}
				}

				bool blk;
				actor->GetGraphVariableBool("IsBlocking", blk);

				//bool shout;
				//actor->GetGraphVariableBool("IsShouting", shout);

				if (a_result < 0.00f) {
					logger::info("Magic damage was negative... somehow?");
					a_result = 0.00f;
				}

				if (blk) {
					if (actor->IsPlayerRef()) {
						float BlockMult = (actor->AsActorValueOwner()->GetBaseActorValue(RE::ActorValue::kBlock));
						if (BlockMult >= 20.0f) {
							float fLogarithm = ((BlockMult - 20) / 50 + 1);
							double PlayerBlockMult = (0.7 - log10(fLogarithm));
							PlayerBlockMult = (PlayerBlockMult >= 0.0) ? PlayerBlockMult : 0.0;
							a_result *= (float)PlayerBlockMult;
						} else {
							a_result *= 0.7f;
						}
					} else {
						a_result *= ptr->BlockedMult;
					}
				}

				if (actor->HasKeyword(ptr->kGhost)) {
					a_result = 0.00f;
				}


				//if (shout) {
				//	a_result = 0.00f;
				//}

				uint32_t poise = 0;
				uint32_t oriPoise = 0;
				float maxPoise = ptr->CalculateMaxPoise(actor);

				if (ptr->StaggerEffectList.contains(EffectSetting)) {
					actor->GetActorRuntimeData().pad0EC = 0;
				} else {
					auto poiseRef = std::atomic_ref(actor->GetActorRuntimeData().pad0EC);
					oriPoise = poiseRef.load();
					uint32_t newPoise = 0;
					uint32_t newPoiseSet = 0;
					do {
						poise = poiseRef.load();
						if (poise > oriPoise) {
							return;
						}
						newPoise = poise - (int)a_result;
						newPoiseSet = newPoise;
						if (newPoiseSet <= 0) {
							newPoiseSet = (int)maxPoise;
						}
					} while (!poiseRef.compare_exchange_strong(poise, newPoiseSet));
					oriPoise = poise;
					poise = newPoise;
					if (poise > 100000) {
						//logger::info("PoiseCalculateExplosion strange poise value {}", poise);
						actor->GetActorRuntimeData().pad0EC = 0;
					}
				}

				if (poise <= 0) {
					if (!std::atomic_ref(actor->GetActorRuntimeData().pad1EC).compare_exchange_strong(lastStaggerTime, nowTime)) {
						//logger::info("PoiseCalculateExplosion prevent stagger lock", poise);
						return;
					}
				}

				auto threshhold0 = maxPoise * ptr->poiseBreakThreshhold0;
				if (threshhold0 <= 2) {
					threshhold0 = 2;
				}
				auto threshhold1 = maxPoise * ptr->poiseBreakThreshhold1;
				if (threshhold1 <= 5) {
					threshhold1 = 5;
				}
				auto threshhold2 = maxPoise * ptr->poiseBreakThreshhold2;
				if (threshhold2 <= 10) {
					threshhold2 = 10;
				}

				auto hitPos = MAttacker->GetPosition();
				auto heading = actor->GetHeadingAngle(hitPos, false);
				auto stagDir = (heading >= 0.0f) ? heading / 360.0f : (360.0f + heading) / 360.0f;
				if (actor->GetHandle() == MAttacker->GetHandle()) {
					stagDir = 0.0f;
				}  // 0 when self-hit

				auto caster = actor->GetMagicCaster(RE::MagicSystem::CastingSource::kInstant);
				//nullcheck
				if (caster) {
					caster->CastSpellImmediate(ptr->poiseDelaySpell, false, actor, 1.0f, false, 0.0f, 0);
				}

				//Conner: for the stagger function and the modification of actor->pad0EC, we should move the code block to a new function, and lock it probably. Do later. I added 3DLoaded check i guess.
				static RE::BSFixedString str = NULL;
				auto cam = RE::PlayerCamera::GetSingleton();

				if (Spell->GetCastingType() == RE::MagicSystem::CastingType::kConcentration) {
					if (poise <= 0) {
						if (ptr->ForceThirdPerson && actor->IsPlayerRef()) {
							if (cam->IsInFirstPerson()) {
								cam->ForceThirdPerson();  //if player is in first person, stagger them in thirdperson.
							}
						}
						actor->SetGraphVariableFloat(ptr->staggerDire, stagDir);  // set direction
						//actor->pad0EC = static_cast<std::uint32_t>(maxPoise);  // remember earlier when we calculated max poise health?	
						if (TrueHUDControl::GetSingleton()->g_trueHUD) {
							TrueHUDControl::GetSingleton()->g_trueHUD->FlashActorSpecialBar(SKSE::GetPluginHandle(), actor->GetHandle(), false);
						}
						if (actor->HasKeyword(ptr->kCreature) || actor->HasKeyword(ptr->kDwarven)) {  // if creature, use normal beh
							actor->SetGraphVariableFloat(ptr->staggerMagn, 1.00f);
							actor->NotifyAnimationGraph(ptr->ae_Stagger);  // play animation
						} else {
							if (actor->HasKeyword(ptr->kDragon)) {
								if (stagDir > 0.25f && stagDir < 0.75f) {
									str = ptr->poiseLargestFwd;
								} else {
									str = ptr->poiseLargestBwd;
								}
								actor->NotifyAnimationGraph(str);  // if those, play tier 4
							} else {
								if (stagDir > 0.25f && stagDir < 0.75f) {
									str = ptr->poiseMedFwd;
								} else {
									str = ptr->poiseMedBwd;
								}
								actor->NotifyAnimationGraph(str);  // if not those, play tier 2
							}
						}
					} else {
						return;	 // Concentration spells spam magic poise calculation. To prevent stupid staggerlocks, we only stagger once per Poise meter for concentration spells.
					}
				}


				if (poise <= 0) {
					if (ptr->ForceThirdPerson && actor->IsPlayerRef()) {
						if (cam->IsInFirstPerson()) {
							cam->ForceThirdPerson();  //if player is in first person, stagger them in thirdperson.
						}
					}
					actor->SetGraphVariableFloat(ptr->staggerDire, stagDir);  // set direction
					//actor->pad0EC = static_cast<std::uint32_t>(maxPoise);	  // remember earlier when we calculated max poise health?
					if (TrueHUDControl::GetSingleton()->g_trueHUD) {
						TrueHUDControl::GetSingleton()->g_trueHUD->FlashActorSpecialBar(SKSE::GetPluginHandle(), actor->GetHandle(), false);
					}
					if (actor->HasKeyword(ptr->kCreature) || actor->HasKeyword(ptr->kDwarven)) {  // if creature, use normal beh
						actor->SetGraphVariableFloat(ptr->staggerMagn, 1.00f);
						actor->NotifyAnimationGraph(ptr->ae_Stagger);  // play animation
					} else {
						if (actor->HasKeyword(ptr->kDragon)) {
							if (stagDir > 0.25f && stagDir < 0.75f) {
								str = ptr->poiseLargestFwd;
							} else {
								str = ptr->poiseLargestBwd;
							}
							actor->NotifyAnimationGraph(str);  // if those, play tier 4
						} else {
							if (stagDir > 0.25f && stagDir < 0.75f) {
								str = ptr->poiseMedFwd;
							} else {
								str = ptr->poiseMedBwd;
							}
							actor->NotifyAnimationGraph(str);  // if not those, play tier 2
						}
					}
				} else if (poise <= threshhold0) {
					if (oriPoise <= threshhold0) {
						return;
					}
					if (ptr->ForceThirdPerson && actor->IsPlayerRef()) {
						if (cam->IsInFirstPerson()) {
							cam->ForceThirdPerson();
						}
					}
					actor->SetGraphVariableFloat(ptr->staggerDire, stagDir);					  // set direction
					if (actor->HasKeyword(ptr->kCreature) || actor->HasKeyword(ptr->kDwarven)) {  // if creature, use normal beh
						actor->SetGraphVariableFloat(ptr->staggerMagn, 0.75f);
						actor->NotifyAnimationGraph(ptr->ae_Stagger);
					} else {
						if (actor->HasKeyword(ptr->kDragon)) {	// check if explosion, dragon, giant attack or dwarven
							if (stagDir > 0.25f && stagDir < 0.75f) {
								str = ptr->poiseLargeFwd;
							} else {
								str = ptr->poiseLargeBwd;
							}
							actor->NotifyAnimationGraph(str);  // if those, play tier 3
						} else {
							if (stagDir > 0.25f && stagDir < 0.75f) {
								str = ptr->poiseMedFwd;
							} else {
								str = ptr->poiseMedBwd;
							}
							actor->NotifyAnimationGraph(str); 
						}
					}
				} else if (poise <= threshhold1) {
					if (oriPoise <= threshhold1) {
						return;
					}
					actor->SetGraphVariableFloat(ptr->staggerDire, stagDir);  // set direction
					if (actor->HasKeyword(ptr->kCreature) || actor->HasKeyword(ptr->kDwarven)) {
						actor->SetGraphVariableFloat(ptr->staggerMagn, 0.50f);
						actor->NotifyAnimationGraph(ptr->ae_Stagger);
					} else {
						if (actor->HasKeyword(ptr->kDragon)) {
							if (stagDir > 0.25f && stagDir < 0.75f) {
								str = ptr->poiseLargeFwd;
							} else {
								str = ptr->poiseLargeBwd;
							}
							actor->NotifyAnimationGraph(str);  // play tier 3 again
						} else {
							if (stagDir > 0.25f && stagDir < 0.75f) {
								str = ptr->poiseSmallFwd;
							} else {
								str = ptr->poiseSmallBwd;
							}
							actor->NotifyAnimationGraph(str);
						}
					}
				} else if (poise <= threshhold2) {
					if (oriPoise <= threshhold2) {
						return;
					}
					actor->SetGraphVariableFloat(ptr->staggerDire, stagDir);  // set direction
					if (actor->HasKeyword(ptr->kCreature) || actor->HasKeyword(ptr->kDwarven)) {
						actor->SetGraphVariableFloat(ptr->staggerMagn, 0.25f);
						actor->NotifyAnimationGraph(ptr->ae_Stagger);
					} else {
						if (stagDir > 0.25f && stagDir < 0.75f) {
							str = ptr->poiseSmallFwd;
						} else {
							str = ptr->poiseSmallBwd;
						}
						actor->NotifyAnimationGraph(str);
					}
				}
			}
		}
	}
	return;

}



/*
    a_result = ((weaponWeight x weaponTypeMul x effectMul) x blockedMul) x hyperArmrMul
*/


float Loki::PoiseMod::CalculatePoiseDamage(RE::HitData& a_hitData, RE::Actor* a_actor) {

    // this whole function is BAD and DIRTY but i cant think of any other way at the moment
    auto ptr = Loki::PoiseMod::GetSingleton();
	bool blk, atk, castleft, castright, castdual, aggressorattack;
	a_actor->GetGraphVariableBool("IsBlocking", blk);
	a_actor->GetGraphVariableBool("IsAttacking", atk);
	a_actor->GetGraphVariableBool("IsCastingDual", castdual);
	a_actor->GetGraphVariableBool("IsCastingRight", castright);
	a_actor->GetGraphVariableBool("IsCastingLeft", castleft);
    auto aggressor = a_hitData.aggressor.get().get();
	aggressor->GetGraphVariableBool("IsAttacking", aggressorattack);
	//bool shout;
	//a_actor->GetGraphVariableBool("IsShouting", shout);

    auto weap = a_hitData.weapon;
    float a_result = 8.00f;
    bool isSilver = false;

    if (!weap) {
        if (!aggressor) {
            a_result = 8.00f;
        }
        else {
            auto attackerWeap = aggressor->GetAttackingWeapon();
            if (!attackerWeap) {
                a_result = 8.00f;
            }
            else {
                a_result = attackerWeap->GetWeight();
            }
        }
    }
    else {
        a_result = weap->weight;

        switch (weap->weaponData.animationType.get()) {
        
            case RE::WEAPON_TYPE::kBow: {
                a_result *= ptr->BowMult;
                break;
            }
            case RE::WEAPON_TYPE::kCrossbow: {
                a_result *= ptr->CrossbowMult;
                break;
            }
            case RE::WEAPON_TYPE::kHandToHandMelee: {
                if (weap->HasKeywordID(0x19AAB3)) {
                    a_result *= ptr->CaestusMult;
                }
                else if (weap->HasKeywordID(0x19AAB4)) {
                    a_result *= ptr->ClawMult;
                }
                else {
					if (!aggressor) {
						a_result = 8.00f;  //if nullcheck fail just set it to 10 meh. 
					}
                    else {
						a_result = (aggressor->GetActorRuntimeData().equippedWeight / 2);  //conner: for npc we use 1/2 of equip weight. Clamped to 50.						                                                                    
						if (a_result > 50.0f) {
							a_result = 50.0f;
						} 
                        if (aggressor->IsPlayerRef()) {
							a_result = (aggressor->AsActorValueOwner()->GetActorValue(RE::ActorValue::kUnarmedDamage) / 2);  //conner: we calculate player poise dmg as half of unarmed damage. Clamp to 25 max.
							if (a_result > 25.0f) {
								a_result = 25.0f;
							}
                        }
                    }
                }
                a_result *= ptr->Hand2Hand;
                break;
            }
            case RE::WEAPON_TYPE::kOneHandAxe: {
                a_result *= ptr->OneHandAxe;
                break;
            }
            case RE::WEAPON_TYPE::kOneHandDagger: {
                a_result *= ptr->OneHandDagger;
                break;
            }
            case RE::WEAPON_TYPE::kOneHandMace: {
                a_result *= ptr->OneHandMace;
                break;
            }
            case RE::WEAPON_TYPE::kOneHandSword: {
                if (weap->HasKeywordID(0x801)) {
                    a_result *= ptr->RapierMult;
                    break;
                }
                a_result *= ptr->OneHandSword;
                break;
            }
            case RE::WEAPON_TYPE::kTwoHandAxe: {
                if (weap->HasKeywordID(0xE4580)) {
                    a_result *= ptr->HalberdMult;
                    break;
                }
                if (weap->HasKeywordID(0xE4581)) {
                    a_result *= ptr->QtrStaffMult;
                    break;
                }
                a_result *= ptr->TwoHandAxe;
                break;
            }
            case RE::WEAPON_TYPE::kTwoHandSword: {
                if (weap->HasKeywordID(0xE457E)) {
                    a_result *= ptr->PikeMult;
                    break;
                }
                if (weap->HasKeywordID(0xE457F)) {
                    a_result *= ptr->SpearMult;
                    break;
                }
				if (weap->HasKeywordString("WeapTypeTwinblade")) {
					a_result *= ptr->TwinbladeMult;
					RE::ConsoleLog::GetSingleton()->Print("weapon is twinblade");
					break;
				}
                a_result *= ptr->TwoHandSword;
                break;
            }
        
        }
        if (weap->HasKeyword(ptr->WeapMaterialSilver)) isSilver = true;

        //imo bound weapons shouldnt be affected by weapon multipliers.
		if (weap->weaponData.flags2.any(RE::TESObjectWEAP::Data::Flag2::kBoundWeapon)) {
			if (aggressor) {
				a_result = (8 + (aggressor->AsActorValueOwner()->GetBaseActorValue(RE::ActorValue::kConjuration) * 0.12f));
				//RE::ConsoleLog::GetSingleton()->Print("Using bound weapon flag, poise damage is %f", a_result);
			}
		}
    }

    //if we dont use original recipe, we add a portion of base physical damage to poise damage.
    if (!ptr->UseOldFormula) {
		float Damage = a_hitData.physicalDamage;
		float Plevel = ptr->PhysicalDmgWeight;
		float Plevel2 = ptr->PhysicalDmgWeightPlayer;
		//RE::ConsoleLog::GetSingleton()->Print("Physical damage done by attack is %f", Damage);
		if (aggressor->IsPlayerRef()) {
			a_result += (Damage * Plevel2);
		} else {
			a_result += (Damage * Plevel);
        }
	}


    if (aggressor->HasKeywordString("ActorTypeCreature") || aggressor->HasKeywordString("ActorTypeDwarven")) {
	    for (auto idx : ptr->poiseRaceMap) {
			if (aggressor) {
				RE::TESRace* a_actorRace = aggressor->GetActorRuntimeData().race;
			    RE::TESRace* a_mapRace = idx.first;
			    if (a_actorRace && a_mapRace) {
				    if (a_actorRace->formID == a_mapRace->formID) {
						float CreatureMult = ptr->CreatureDMGMultiplier;
				        a_result = CreatureMult * idx.second[1];
						break;
			        } 
			    }
		    }
	    }
    }

    const auto zeffect = aggressor->AsMagicTarget()->GetActiveEffectList();
	if (zeffect) {
		for (const auto& aeffect : *zeffect) {
			if (!aeffect) {
				continue;
			}
			if (!aeffect->GetBaseObject()) {
				continue;
			}
			if ((!aeffect->flags.all(RE::ActiveEffect::Flag::kInactive)) && aeffect->GetBaseObject()->HasKeywordString("zzzPoiseDamageFlat")) {
				auto buffFlat = (aeffect->magnitude);
				a_result += buffFlat;  //conner: additive instead of multiplicative buff.
			}
			if ((!aeffect->flags.all(RE::ActiveEffect::Flag::kInactive)) && aeffect->GetBaseObject()->HasKeywordString("zzzPoiseDamageIncrease")) {
				auto buffPercent = (aeffect->magnitude / 100.00f);	// convert to percentage
				auto resultingBuff = (a_result * buffPercent);
				a_result += resultingBuff;	// aggressor has buff that makes them do more poise damage
			}
			if ((!aeffect->flags.all(RE::ActiveEffect::Flag::kInactive)) && aeffect->GetBaseObject()->HasKeywordString("zzzPoiseDamageDecrease")) {
				auto nerfPercent = (aeffect->magnitude / 100.00f);	// convert to percentage
				auto resultingNerf = (a_result * nerfPercent);
				a_result -= resultingNerf;	// aggressor has nerf that makes them do less poise damage
			}
		}
	}

    //ward buff is applied after keyword buffs but before hyperarmor and other modifiers 
	float WardPower = a_actor->AsActorValueOwner()->GetActorValue(RE::ActorValue::kWardPower);
	float WardPowerMult = ptr->WardPowerWeight;
	a_result -= (WardPower * WardPowerMult);
	(a_result > 0.0f) ? a_result : (a_result = 0.0f);


    if (ptr->ScalePDmgGlobal) {
		if (a_actor->IsPlayerRef()) {
			a_result *= ptr->GlobalPlayerPDmgMult;
		} else {
			a_result *= ptr->GlobalPDmgMult;
		}
	}


    if (auto data = a_hitData.attackData.get()) {
        if (data->data.flags == RE::AttackData::AttackFlag::kPowerAttack) {
            a_result *= ptr->PowerAttackMult;
        }

        if (data->data.flags == RE::AttackData::AttackFlag::kBashAttack) {
			if (aggressorattack) {
				a_result *= ptr->BashParryMult;   
                //RE::ConsoleLog::GetSingleton()->Print("BASH PARRY!! VERY FANTASTIC!");
				//logger::info("BASH PARRY!! VERY FANTASTIC!");
            } 
            else {
                a_result *= ptr->BashMult;
			}
        }
    }
    
    if (blk) {

		if (a_actor->IsPlayerRef()) {
			float BlockMult = (a_actor->AsActorValueOwner()->GetBaseActorValue(RE::ActorValue::kBlock));
			if (BlockMult >= 20.0f) {
				float fLogarithm = ((BlockMult - 20) / 50 + 1);
				double PlayerBlockMult = (0.7 - log10(fLogarithm));
				PlayerBlockMult = (PlayerBlockMult >= 0.0) ? PlayerBlockMult : 0.0;
				a_result *= (float)PlayerBlockMult;
			}
            else {
				a_result *= 0.7f;
            }
		} 
        else {
			a_result *= ptr->BlockedMult;
		}
    }

    if (atk) {
		if (a_actor->IsPlayerRef()) {
			float LightArmorMult = (a_actor->AsActorValueOwner()->GetBaseActorValue(RE::ActorValue::kLightArmor));
			float Slope = ptr->HyperArmorLogSlope; //default = 50
			if (LightArmorMult >= 20.0f) {
				float fLogarithm = ( (LightArmorMult - 20) / Slope + 1);
				double PlayerHyperArmourMult = (0.8 - log2(fLogarithm) * 0.4);
				//RE::ConsoleLog::GetSingleton()->Print("logarithm value: %f", fLogarithm);
				//RE::ConsoleLog::GetSingleton()->Print("Player hyper armor LOG Calculation: %f", PlayerHyperArmourMult);
				PlayerHyperArmourMult = PlayerHyperArmourMult >= 0.0 ? PlayerHyperArmourMult : 0.0;  //if value is negative, player is at a dumb level so just give them full damage mitigation. 
				a_result *= (float)PlayerHyperArmourMult;
				//RE::ConsoleLog::GetSingleton()->Print("Player hyper armor multipler: %f", (float)PlayerHyperArmourMult);
				//RE::ConsoleLog::GetSingleton()->Print("Damage after calculation of hyperarmor: %f", a_result);
				//logger::info("player hyperarmor scaled to light armor");
			} 
            else {
				a_result *= 0.8f;
				//RE::ConsoleLog::GetSingleton()->Print("Player light armor is 20 or below, using default hyperarmor of %f", ptr->HyperArmourMult);
				//logger::info("player hyperarmor NOT scaled to light armor");
			}

        } 
        else {
			a_result *= ptr->NPCHyperArmourMult;
		}

    }

    if (castleft || castright || castdual) {
		if (a_actor->IsPlayerRef()) { 
		float LightArmorMult = (a_actor->AsActorValueOwner()->GetBaseActorValue(RE::ActorValue::kLightArmor));
        float Slope = ptr->SpellHyperLogSlope; //default = 60
		   if (LightArmorMult >= 20.0f) {
			   float fLogarithm = ((LightArmorMult - 20) / Slope + 1);
			   double PlayerHyperArmourMult = (1.5 - log2(fLogarithm) * 0.8);
			   //RE::ConsoleLog::GetSingleton()->Print("spell logarithm value: %f", fLogarithm);
			   //RE::ConsoleLog::GetSingleton()->Print("Player spell HyperArmor LOG Calculation: %f", PlayerHyperArmourMult);
			   PlayerHyperArmourMult = PlayerHyperArmourMult >= 0.0 ? PlayerHyperArmourMult : PlayerHyperArmourMult = 0.0;	//if value is negative, player is at a dumb level so just give them full damage mitigation.
			   a_result *= (float)PlayerHyperArmourMult;
			   //RE::ConsoleLog::GetSingleton()->Print("Player spell hyperarmor multipler: %f", (float)PlayerHyperArmourMult);
			   //RE::ConsoleLog::GetSingleton()->Print("Damage after calculation of hyperarmor: %f", a_result);
			   //logger::info("player SPELL hyperarmor scaled to light armor");
		   }
		   else {
               a_result *= 1.5f;
			   //RE::ConsoleLog::GetSingleton()->Print("Player light armor is 20 or below, using default spell hyperarmor of %f", ptr->HyperArmourMult);
			   //logger::info("player SPELL hyperarmor NOT scaled to light armor");
		   }
        } 
        else {
			a_result *= ptr->NPCSpellHyperArmourMult;
			//logger::info("npc spell hyperarmor calculated");
			//RE::ConsoleLog::GetSingleton()->Print("npc spell hyperarmor %f", ptr->NPCSpellHyperArmourMult);
		}
    }

    if (a_actor->HasKeyword(ptr->kGhost) && !isSilver) {
        a_result = 0.00f;
    }


    //shout check is broken doesnt work, lets revisit this at another time!
    //if (shout) {
	//	a_result = 0.00f;
	//	RE::ConsoleLog::GetSingleton()->Print("Shout MULT active");
	//}

    //quick mathematics: Base hyperarmor is 20% damage reduction. Base spell multiplier is 1.5x. Light armor buffs hyperarmor. 
    //First 20 levels of ActorValue do not count for scaling. The remaining 80 levels scale damage reduction logarithmically


    return a_result;

}

float Loki::PoiseMod::CalculateMaxPoise(RE::Actor* a_actor) {
    auto ptr = Loki::PoiseMod::GetSingleton();

	//Scales logarithmically with armorrating.
	float level = a_actor->GetLevel();
	float levelweight = ptr->MaxPoiseLevelWeight;
    float levelweightplayer = ptr->MaxPoiseLevelWeightPlayer;
	float ArmorRating = a_actor->CalcArmorRating();
	float ArmorWeight = ptr->ArmorLogarithmSlope;
	float ArmorWeightPlayer = ptr->ArmorLogarithmSlopePlayer;
	float RealWeight = ActorCache::GetSingleton()->GetOrCreateCachedWeight(a_actor);

	level = (level < 100 ? level : 100);
	ArmorRating = (ArmorRating > 0 ? ArmorRating : 0);
	float a_result = (RealWeight + (levelweight * level) + (a_actor->AsActorValueOwner()->GetBaseActorValue(RE::ActorValue::kHeavyArmor) * 0.2f)) * (1 + log10(ArmorRating / ArmorWeight + 1));
	if (a_actor->IsPlayerRef()) {
		level = (level < 60 ? level : 60);
		a_result = (RealWeight + (levelweightplayer * level) + (a_actor->AsActorValueOwner()->GetBaseActorValue(RE::ActorValue::kHeavyArmor) * 0.2f)) * (1 + log10(ArmorRating / ArmorWeightPlayer + 1));
    }

        


    //KFC Original Recipe.
    if (ptr->UseOldFormula) {
		a_result = (RealWeight + (a_actor->AsActorValueOwner()->GetBaseActorValue(RE::ActorValue::kHeavyArmor) * 0.20f));
    }

    if (a_actor && a_actor->GetActorRuntimeData().race->HasKeywordString("ActorTypeCreature") || a_actor->GetActorRuntimeData().race->HasKeywordString("ActorTypeDwarven")) {
	    for (auto idx : ptr->poiseRaceMap) {
			if (a_actor && a_actor->GetActorRuntimeData().race) {
			   RE::TESRace* a_actorRace = a_actor->GetActorRuntimeData().race;
			    RE::TESRace* a_mapRace = idx.first;
			    if (a_actorRace && a_mapRace) {
				    if (a_actorRace->formID == a_mapRace->formID) {
						float CreatureMult = ptr->CreatureHPMultiplier;
						a_result = CreatureMult * idx.second[0];
						break;
			        } 
			    }
		    }
	    }
    }

    if (ptr->ScalePHealthGlobal) {
		if (a_actor->IsPlayerRef()) {
			a_result *= ptr->GlobalPlayerPHealthMult;
		} 
        else {
			a_result *= ptr->GlobalPHealthMult;
		}
	}

    const auto effect = a_actor->AsMagicTarget()->GetActiveEffectList();
	if (effect) {
		for (const auto& veffect : *effect) {
			if (!veffect) {
				continue;
			}
			if (!veffect->GetBaseObject()) {
				continue;
			}
			if ((!veffect->flags.all(RE::ActiveEffect::Flag::kInactive)) && veffect->GetBaseObject()->HasKeywordString("zzzMaxPoiseHealthFlat")) {
				auto buffFlat = (veffect->magnitude);
				a_result += buffFlat; 				        //conner: additive instead of multiplicative buff.
				//RE::ConsoleLog::GetSingleton()->Print("flat buff = %f", buffFlat);
			}
			if ((!veffect->flags.all(RE::ActiveEffect::Flag::kInactive)) && veffect->GetBaseObject()->HasKeywordString("MagicArmorSpell")) {
				auto armorflesh = (0.1f * veffect->magnitude);
				a_result += armorflesh;
				//RE::ConsoleLog::GetSingleton()->Print("armor flesh poise buff = %f", armorflesh);
			}
			if ((!veffect->flags.all(RE::ActiveEffect::Flag::kInactive)) && veffect->GetBaseObject()->HasKeywordString("zzzMaxPoiseIncrease")) {
				auto buffPercent = (veffect->magnitude / 100.00f);	// convert to percentage
				auto resultingBuff = (a_result * buffPercent);
				a_result += resultingBuff;	// victim has poise hp buff
			}
			if ((!veffect->flags.all(RE::ActiveEffect::Flag::kInactive)) && veffect->GetBaseObject()->HasKeywordString("zzzMaxPoiseDecrease")) {
				auto nerfPercent = (veffect->magnitude / 100.00f);	// convert to percentage
				auto resultingNerf = (a_result * nerfPercent);
				a_result -= resultingNerf;	// victim poise hp nerf
			}
		}
	}

    return a_result;

}

bool Loki::PoiseMod::IsActorKnockdown(RE::Character* a_this, std::int64_t a_unk) {

    auto ptr = Loki::PoiseMod::GetSingleton();

    auto avHealth = a_this->AsActorValueOwner()->GetActorValue(RE::ActorValue::kHealth);
    if (a_this->IsOnMount() || a_this->IsInMidair() || avHealth <= 0.05f) {
        return _IsActorKnockdown(a_this, a_unk);
    }

	auto kGrabbed = a_this->AsActorValueOwner()->GetActorValue(RE::ActorValue::kGrabbed);
	if (kGrabbed == 1) {
		return _IsActorKnockdown(a_this, a_unk);
	} 

    static RE::BSFixedString str = NULL;

	if ((a_this->IsPlayerRef() && ptr->PlayerRagdollReplacer) || (!a_this->IsPlayerRef() && ptr->NPCRagdollReplacer)) {
        float knockdownDirection = 0.00f;
        a_this->GetGraphVariableFloat("staggerDirection", knockdownDirection);
        if (knockdownDirection > 0.25f && knockdownDirection < 0.75f) {
            str = ptr->poiseLargestFwd;
        } else {
            str = ptr->poiseLargestBwd;
        }
        if (TrueHUDControl::GetSingleton()->g_trueHUD) {
            TrueHUDControl::GetSingleton()->g_trueHUD->
                FlashActorSpecialBar(SKSE::GetPluginHandle(), a_this->GetHandle(), true);
        }
        a_this->NotifyAnimationGraph(str);
        return false;
    } 

    return _IsActorKnockdown(a_this, a_unk);

}

float Loki::PoiseMod::GetSubmergedLevel(RE::Actor* a_actor, float a_zPos, RE::TESObjectCELL* a_cell)
{

    auto ptr = Loki::PoiseMod::GetSingleton();

    auto avHealth = a_actor->AsActorValueOwner()->GetActorValue(RE::ActorValue::kHealth);
    if (avHealth <= 0.05f || !ptr->PoiseRegenEnabled) { return _GetSubmergedLevel(a_actor, a_zPos, a_cell); }

    if (!a_actor->AsMagicTarget()->HasMagicEffect(ptr->poiseDelayEffect)) {
        auto a_result = (int)CalculateMaxPoise(a_actor);
		if (a_result > 100000) {
			//logger::info("GetSubmergedLevel strange poise value {}", a_result);
			a_result = 0;
		}
		a_actor->GetActorRuntimeData().pad0EC = a_result;
    }

    return _GetSubmergedLevel(a_actor, a_zPos, a_cell);

}

void Loki::PoiseMod::ProcessHitEvent(RE::Actor* a_actor, RE::HitData& a_hitData) {
    auto ptr = Loki::PoiseMod::GetSingleton();

	auto nowTime = uint32_t(std::time(nullptr) % UINT32_MAX);
	auto lastStaggerTime = a_actor->GetActorRuntimeData().pad1EC;
	if (lastStaggerTime > nowTime) {
		a_actor->GetActorRuntimeData().pad1EC = nowTime;
		lastStaggerTime = 0;
	} else {
		if (nowTime - lastStaggerTime < ptr->StaggerDelay) {
			//logger::info("{} last stagger time {} is less than stagger delay {}", a_actor->GetName(), lastStaggerTime, ptr->StaggerDelay);
			return _ProcessHitEvent(a_actor, a_hitData);
		}
	}

    using HitFlag = RE::HitData::Flag;
    RE::FormID kLurker = 0x14495;

    auto avHealth = a_actor->AsActorValueOwner()->GetActorValue(RE::ActorValue::kHealth);
    auto avParalysis = a_actor->AsActorValueOwner()->GetActorValue(RE::ActorValue::kParalysis);
    if (avHealth <= 0.05f || a_actor->IsInKillMove() || avParalysis) { return _ProcessHitEvent(a_actor, a_hitData); }

    if (a_actor->IsPlayerRef() && !ptr->PlayerPoiseEnabled) { return _ProcessHitEvent(a_actor, a_hitData); }
    if (!a_actor->IsPlayerRef() && !ptr->NPCPoiseEnabled) { return _ProcessHitEvent(a_actor, a_hitData); }

	if (!IsTargetVaild(a_hitData.aggressor.get().get(), *a_actor)) {
		return _ProcessHitEvent(a_actor, a_hitData);
	}

    float dmg = CalculatePoiseDamage(a_hitData, a_actor);

    if (dmg < 0.00f) {
        //logger::info("Damage was negative... somehow?");
        dmg = 0.00f;
    }

	uint32_t poise = 0;
	uint32_t oriPoise = 0;
	float maxPoise = CalculateMaxPoise(a_actor);

	{
		auto poiseRef = std::atomic_ref(a_actor->GetActorRuntimeData().pad0EC);
		oriPoise = poiseRef.load();
		auto newPoise = 0u;
		auto newPoiseSet = 0u;
		do {
			poise = poiseRef.load();
			if (poise > oriPoise) {
				return _ProcessHitEvent(a_actor, a_hitData);
			}
			newPoise = poise - (int)dmg;
			newPoiseSet = newPoise;
			if (newPoiseSet <= 0) {
				newPoiseSet = (int)maxPoise;
			}
		} while (!poiseRef.compare_exchange_strong(poise, newPoiseSet));
		oriPoise = poise;
		poise = newPoise;
		if (poise > 100000) {
			//logger::info("ProcessHitEvent strange poise value {}", poise);
			a_actor->GetActorRuntimeData().pad0EC = 0;
		}
	}

	if (poise <= 0) {
		if (!std::atomic_ref(a_actor->GetActorRuntimeData().pad1EC).compare_exchange_strong(lastStaggerTime, nowTime)) {
			//logger::info("ProcessHitEvent prevent stagger lock", poise);
			return _ProcessHitEvent(a_actor, a_hitData);
		}
	}

    if (ptr->ConsoleInfoDump) {
        RE::ConsoleLog::GetSingleton()->Print("---");
        RE::ConsoleLog::GetSingleton()->Print("Aggressor's Weight: %f", a_hitData.aggressor.get()->GetWeight());
		RE::ConsoleLog::GetSingleton()->Print("Aggressor's EquipWeight: %f", ActorCache::GetSingleton()->GetOrCreateCachedWeight(a_hitData.aggressor.get().get()));
		RE::ConsoleLog::GetSingleton()->Print("Aggressor's Current Poise Health: %d", a_hitData.aggressor.get()->GetActorRuntimeData().pad0EC);
        RE::ConsoleLog::GetSingleton()->Print("Aggresssor's Max Poise Health: %f", CalculateMaxPoise(a_hitData.aggressor.get().get()));
        RE::ConsoleLog::GetSingleton()->Print("Aggressor's Poise Damage: %f", dmg);
        RE::ConsoleLog::GetSingleton()->Print("-");
        RE::ConsoleLog::GetSingleton()->Print("Victim's Weight: %f", a_actor->GetWeight());
		RE::ConsoleLog::GetSingleton()->Print("Victim's EquipWeight: %f", ActorCache::GetSingleton()->GetOrCreateCachedWeight(a_actor));
		RE::ConsoleLog::GetSingleton()->Print("Victim's Current Poise Health: %d %d", poise, a_actor->GetActorRuntimeData().pad0EC);
        RE::ConsoleLog::GetSingleton()->Print("Victim's Max Poise Health %f", CalculateMaxPoise(a_actor));
        RE::ConsoleLog::GetSingleton()->Print("---");
    }

    auto hitPos = a_hitData.aggressor.get()->GetPosition();
    auto heading = a_actor->GetHeadingAngle(hitPos, false);
    auto stagDir = (heading >= 0.0f) ? heading / 360.0f : (360.0f + heading) / 360.0f;
    if (a_actor->GetHandle() == a_hitData.aggressor) { stagDir = 0.0f; } // 0 when self-hit

    auto a = a_actor->GetMagicCaster(RE::MagicSystem::CastingSource::kInstant);
	if (a) {
		a->CastSpellImmediate(ptr->poiseDelaySpell, false, a_actor, 1.0f, false, 0.0f, 0);
	}
    
	auto threshhold0 = maxPoise * ptr->poiseBreakThreshhold0;
	if (threshhold0 <= 2) {
		threshhold0 = 2;
	}
	auto threshhold1 = maxPoise * ptr->poiseBreakThreshhold1;
	if (threshhold1 <= 5) {
		threshhold1 = 5;
	}
	auto threshhold2 = maxPoise * ptr->poiseBreakThreshhold2;
	if (threshhold2 <= 10) {
		threshhold2 = 10;
	}

	//Conner: for the stagger function and the modification of actor->pad0EC, we should move the code block to a new function, and lock it probably. Do later. I added 3DLoaded check i guess.
	bool isBlk = false;
    static RE::BSFixedString str = NULL;
    a_actor->GetGraphVariableBool(ptr->isBlocking, isBlk);

	auto cam = RE::PlayerCamera::GetSingleton();

    if (poise <= 0.00f && a_actor->Is3DLoaded()) {
		if (ptr->ForceThirdPerson && a_actor->IsPlayerRef()) {
			if (cam->IsInFirstPerson()) {
		         cam->ForceThirdPerson();  //if player is in first person, stagger them in thirdperson.
            }
        }
        a_actor->SetGraphVariableFloat(ptr->staggerDire, stagDir); // set direction
        //a_actor->pad0EC = static_cast<std::uint32_t>(maxPoise); // remember earlier when we calculated max poise health?
        if (TrueHUDControl::GetSingleton()->g_trueHUD) {
            TrueHUDControl::GetSingleton()->g_trueHUD->
                FlashActorSpecialBar(SKSE::GetPluginHandle(), a_actor->GetHandle(), false);
        }
        if (a_actor->HasKeyword(ptr->kCreature) || a_actor->HasKeyword(ptr->kDwarven)) { // if creature, use normal beh
            a_actor->SetGraphVariableFloat(ptr->staggerMagn, 1.00f);
            a_actor->NotifyAnimationGraph(ptr->ae_Stagger);          // play animation
        } 
        else {
            if (a_hitData.flags == HitFlag::kExplosion || a_hitData.aggressor.get()->HasKeyword(ptr->kDragon)
                || a_hitData.aggressor.get()->HasKeyword(ptr->kGiant) || a_hitData.aggressor.get()->HasKeyword(ptr->kDwarven)
                || a_hitData.aggressor.get()->HasKeyword(ptr->kTroll) || a_hitData.aggressor.get()->GetActorRuntimeData().race->formID == kLurker) {  // check if explosion, dragon, giant attack or dwarven
                if (stagDir > 0.25f && stagDir < 0.75f) {
                    str = ptr->poiseLargestFwd;
                } 
                else {
                    str = ptr->poiseLargestBwd;
                }
                a_actor->NotifyAnimationGraph(str);         // if those, play tier 4
            } 
            else {
                if (stagDir > 0.25f && stagDir < 0.75f) {
                    str = ptr->poiseMedFwd;
                } 
                else {
                    str = ptr->poiseMedBwd;
                }
                a_actor->NotifyAnimationGraph(str);  // if not those, play tier 2
            }
        }
    } 
    else if (a_actor->Is3DLoaded() && poise < threshhold0) {
		if (oriPoise <= threshhold0) {
			return _ProcessHitEvent(a_actor, a_hitData);
		}
		if (a_actor->IsPlayerRef()) {
			if (cam->IsInFirstPerson()) {
				cam->ForceThirdPerson();
			}
		}
        a_actor->SetGraphVariableFloat(ptr->staggerDire, stagDir); // set direction
        if (a_actor->HasKeyword(ptr->kCreature) || a_actor->HasKeyword(ptr->kDwarven)) { // if creature, use normal beh
            a_actor->SetGraphVariableFloat(ptr->staggerMagn, 0.75f);
            a_actor->NotifyAnimationGraph(ptr->ae_Stagger);
        } 
        else {
            if (a_hitData.flags == HitFlag::kExplosion || a_hitData.aggressor.get()->HasKeyword(ptr->kDragon)
                || a_hitData.aggressor.get()->HasKeyword(ptr->kGiant) || a_hitData.aggressor.get()->HasKeyword(ptr->kDwarven)
                || a_hitData.aggressor.get()->HasKeyword(ptr->kTroll) || a_hitData.aggressor.get()->GetActorRuntimeData().race->formID == kLurker) {  // check if explosion, dragon, giant attack or dwarven
                if (stagDir > 0.25f && stagDir < 0.75f) {
                    str = ptr->poiseLargeFwd;
                } 
                else {
                    str = ptr->poiseLargeBwd;
                }
                a_actor->NotifyAnimationGraph(str);           // if those, play tier 3
            } 
            else {
                if (stagDir > 0.25f && stagDir < 0.75f) {
                    str = ptr->poiseMedFwd;
                } 
                else {
                    str = ptr->poiseMedBwd;
                }
                isBlk ? a_hitData.pushBack = 5.00 : a_actor->NotifyAnimationGraph(str); // if block, set pushback, ! play tier 2
            }
        }
    } 
    else if (a_actor->Is3DLoaded() && poise < threshhold1) {
		if (oriPoise <= threshhold1) {
			return _ProcessHitEvent(a_actor, a_hitData);
		}
        a_actor->SetGraphVariableFloat(ptr->staggerDire, stagDir); // set direction
        if (a_actor->HasKeyword(ptr->kCreature) || a_actor->HasKeyword(ptr->kDwarven)) {
            a_actor->SetGraphVariableFloat(ptr->staggerMagn, 0.50f);
            a_actor->NotifyAnimationGraph(ptr->ae_Stagger);
        } 
        else {
            if (a_hitData.flags == HitFlag::kExplosion || a_hitData.aggressor.get()->HasKeyword(ptr->kDragon)
                || a_hitData.aggressor.get()->HasKeyword(ptr->kGiant) || a_hitData.aggressor.get()->HasKeyword(ptr->kDwarven) 
				|| a_hitData.aggressor.get()->HasKeyword(ptr->kTroll) || a_hitData.aggressor.get()->GetActorRuntimeData().race->formID == kLurker) {
                if (stagDir > 0.25f && stagDir < 0.75f) {
                    str = ptr->poiseLargeFwd;
                } 
                else {
                    str = ptr->poiseLargeBwd;
                }
                a_actor->NotifyAnimationGraph(str);  // play tier 3 again
            } 
            else {
                if (stagDir > 0.25f && stagDir < 0.75f) {
                    str = ptr->poiseSmallFwd;
                } 
                else {
                    str = ptr->poiseSmallBwd;
                }
                isBlk ? a_hitData.pushBack = 3.75 : a_actor->NotifyAnimationGraph(str);
            }
        }
    } 
    else if (a_actor->Is3DLoaded() && poise < threshhold2) {
		if (oriPoise <= threshhold2) {
			return _ProcessHitEvent(a_actor, a_hitData);
		}
        a_actor->SetGraphVariableFloat(ptr->staggerDire, stagDir); // set direction
        if (a_actor->HasKeyword(ptr->kCreature) || a_actor->HasKeyword(ptr->kDwarven)) {
            a_actor->SetGraphVariableFloat(ptr->staggerMagn, 0.25f);
            a_actor->NotifyAnimationGraph(ptr->ae_Stagger);
        } 
        else {
            if (stagDir > 0.25f && stagDir < 0.75f) {
                str = ptr->poiseSmallFwd;
            } 
            else {
                str = ptr->poiseSmallBwd;
            }
            isBlk ? a_hitData.pushBack = 2.50f : a_actor->NotifyAnimationGraph(str);
        }
    }

    return _ProcessHitEvent(a_actor, a_hitData);

}