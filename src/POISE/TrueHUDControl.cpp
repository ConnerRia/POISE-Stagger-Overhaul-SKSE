#include "TrueHUDControl.h"
#include "../ActorCache.h"

Loki::TrueHUDControl::TrueHUDControl() {
    CSimpleIniA ini;
    ini.SetUnicode();
    auto filename = L"Data/SKSE/Plugins/loki_POISE.ini";
    [[maybe_unused]] SI_Error rc = ini.LoadFile(filename);

    this->TrueHUDBars = ini.GetBoolValue("MAIN", "bTrueHUDBars", false);
}

Loki::TrueHUDControl* Loki::TrueHUDControl::GetSingleton() {
    static Loki::TrueHUDControl* singleton = new Loki::TrueHUDControl();
    return singleton;
}

float Loki::TrueHUDControl::GetMaxSpecial([[maybe_unused]] RE::Actor* a_actor) {

    auto ptr = Loki::PoiseMod::GetSingleton();
	float a_result = ptr->CalculateMaxPoise(a_actor);
    return a_result;

}

float Loki::TrueHUDControl::GetCurrentSpecial([[maybe_unused]] RE::Actor* a_actor) {
    return (float)a_actor->GetActorRuntimeData().pad0EC;
}