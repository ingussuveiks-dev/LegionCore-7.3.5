#define WARSWORDS_SCENE_EXPIRY_TEST
#include "WarswordsPresentationTest.cpp"
using uint8 = uint8_t;
enum { AURA_EFFECT_HANDLE_REAL = 1 };
#include "AuraRemoveModes.inc"
struct AuraApplication {
    Unit* target; uint32 reason = AURA_REMOVE_BY_EXPIRE;
    Unit* GetTarget() const { return target; }
    uint32 GetRemoveMode() const { return reason; }
};
#include "AuraScene.inc"
int main() {
    _set_abort_behavior(0, _CALL_REPORTFAULT);
    uint32 cases = 0;
#ifdef SCENE_EXPIRY_BASELINE
    Pickup pickup;
    AuraApplication application{&pickup.player};
    pickup.effect.HandleAuraActivateScene(&application, AURA_EFFECT_HANDLE_REAL, false);
    pickup.Finish();
    assert(!pickup.player.HasSceneStatus(1218, SCENE_COMPLETE) && pickup.player.casts.empty() && pickup.player.m_sceneInstanceID.empty());
    std::cout << "COUNTERFACTUAL pinnedcb9cc208e full aura430 remove/native scene lifecycle: natural finite expiry cancels before completion,late packet cannot equip/acknowledge.\n";
    return 0;
#else
    for (bool packetFirst : {false,true}) for (uint32 step : {6u,7u}) {
        Pickup pickup; scenarioManager.scenario.step = step;
        AuraApplication application{&pickup.player};
        if (packetFirst) pickup.Finish();
        pickup.effect.HandleAuraActivateScene(&application,AURA_EFFECT_HANDLE_REAL,false);
        pickup.Finish();
        pickup.effect.HandleAuraActivateScene(&application,AURA_EFFECT_HANDLE_REAL,false);
        assert(pickup.player.HasSceneStatus(1218,SCENE_COMPLETE));
        assert(pickup.player.casts == std::vector<uint32>{194201} && pickup.player.swaps == 1 && pickup.player.m_sceneInstanceID.empty());
        ++cases;
    }
    for (uint32 invalid = 0;invalid < 12;++invalid) {
        Pickup pickup; AuraApplication application{&pickup.player};
        if(invalid == 0)application.reason = AURA_REMOVE_BY_DEFAULT;
        if(invalid == 1)application.reason = AURA_REMOVE_BY_DEATH;
        if(invalid == 2)application.reason = AURA_REMOVE_BY_CANCEL;
        if(invalid == 3)pickup.effect.spell = 205340;
        if(invalid == 4)pickup.effect.scene = 1215;
        if(invalid == 5)pickup.player.playerClass = 3;
        if(invalid == 6)pickup.player.map = 1495;
        if(invalid == 7)spellManager.scene.SceneScriptPackageID = 1583;
        if(invalid == 8)spellManager.scene.PlaybackFlags = 1;
        if(invalid == 9)spellManager.scene.CustomDuration = 1;
        if(invalid == 10)application.target = nullptr;
        Unit npc;if(invalid == 11)application.target = &npc;
        pickup.effect.HandleAuraActivateScene(&application,AURA_EFFECT_HANDLE_REAL,false);
        assert(!pickup.player.HasSceneStatus(1218,SCENE_COMPLETE) && pickup.player.casts.empty());
        ++cases;
    }
    for(uint32 invalid = 0;invalid < 6;++invalid) {
        Pickup pickup;AuraApplication application{&pickup.player};
        if(invalid == 0)pickup.player.owned = false;
        if(invalid == 1)pickup.player.bankOnly = true;
        if(invalid == 2)pickup.player.quest = 0;
        if(invalid == 3)pickup.player.rewarded = true;
        if(invalid == 4)scenarioManager.scenario.step = 5;
        if(invalid == 5)pickup.player.alive = false;
        pickup.effect.HandleAuraActivateScene(&application,AURA_EFFECT_HANDLE_REAL,false);
        assert(pickup.player.HasSceneStatus(1218,SCENE_COMPLETE) && pickup.player.casts.empty() && !pickup.player.swaps);
        ++cases;
    }
    {
        Pickup pickup;uint32 obsolete = pickup.instance;pickup.instance = pickup.player.Start(pickup.effect);
        pickup.player.m_sceneInstanceID[500] = 999;
        AuraApplication application{&pickup.player};pickup.effect.HandleAuraActivateScene(&application,AURA_EFFECT_HANDLE_REAL,false);
        pickup.player.SceneCompleted(obsolete);pickup.Finish();
        assert(pickup.player.swaps == 1 && pickup.player.casts.size() == 1 && pickup.player.m_sceneInstanceID.count(500));++cases;
    }
    {
        Pickup pickup;AuraApplication application{&pickup.player};
        pickup.effect.HandleAuraActivateScene(&application,0,false);
        assert(pickup.player.m_sceneInstanceID.count(pickup.instance) && pickup.player.casts.empty());++cases;
    }
    for(bool matchingMap : {false,true}) {
        Pickup pickup;pickup.player.RemoveAurasDueToSpell(205443);
        pickup.player.map = matchingMap ? 1495 : 1511;
        spellManager.scene = {21,1575,1215};
        pickup.effect.scene = 1215;pickup.effect.spell = 205340;
        pickup.instance = pickup.player.Start(pickup.effect);
        AuraApplication application{&pickup.player};
        pickup.effect.HandleAuraActivateScene(&application,AURA_EFFECT_HANDLE_REAL,false);
        assert(pickup.player.HasSceneStatus(1215,SCENE_COMPLETE) == matchingMap);
        assert(pickup.player.casts.empty()); // Fury's equip bridge must not consume Protection.
        ++cases;
    }
    std::cout << "PASS " << cases << " full extracted aura430/native scene lifecycle cases: natural expiry or early packet completes once;late/duplicate/stale instances harmless,exact spell/scene/map/class/native metadata,early death/cancel/removal,invalid progression/no item/no grants. Actor/inventory/packet endpoints controlled;full real finite Aura expiry checked separately.\n";
#endif
}
