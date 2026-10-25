#define MARKSMANSHIP_PRESENTATION_TEST
#define EARTHWARDER_PRESENTATION_TEST
#include "SceneLifecycleFixture.h"
enum { CLASS_WARRIOR = 1, QUEST_STATUS_INCOMPLETE = 3, QUEST_STATUS_COMPLETE = 1, SPELL_CAST_OK = 0 };
using SpellCastResult = uint32;
int warnings = 0;
#define TC_LOG_WARN(...) (++warnings)
struct Scenario {
    uint32 id = 909, step = 5;
    uint32 GetScenarioId() const { return id; }
    uint32 GetCurrentStep() const { return step; }
};
struct ScenarioManager {
    Scenario scenario;
    bool present = true;
    Scenario* GetScenario(uint32 instance) { return present && instance == 42 ? &scenario : nullptr; }
} scenarioManager;
auto* sScenarioMgr = &scenarioManager;
struct Spell {
    uint32 effectHandleMode = SPELL_EFFECT_HANDLE_HIT_TARGET, m_diffMode = 0;
    Unit* m_caster;
    SpellInfo* m_spellInfo;
    void EffectForceEquipItem(SpellEffIndex);
};
#include "ForceEquip.inc"
struct SceneTriggerScript {
    explicit SceneTriggerScript(std::string name) { assert(name == "scene_earthwarder_pickup"); }
    virtual bool OnTrigger(Player*, SpellScene const*, std::string) { return false; }
};
#include "Presentation.inc"
struct Pickup {
    Player player;
    AuraEffect effect{1215, 205340};
    SpellInfo info;
    Spell equip{SPELL_EFFECT_HANDLE_HIT_TARGET, 0, &player, &info};
    scene_earthwarder_pickup script;
    uint32 instance;
    Pickup() {
        scenarioManager = {}; warnings = 0;
        spellManager.scene = {21, 1575, 1215};
        player.playerClass = 1; player.map = 1495; player.expectedQuest = 39191; player.expectedItem = 128289;
        player.forceEquip = [this]() { equip.EffectForceEquipItem(0); };
        scriptManager.callback = [this](Player* target, std::string trigger) { return script.OnTrigger(target, &spellManager.scene, trigger); };
        instance = player.Start(effect);
    }
    ~Pickup() { scriptManager.callback = {}; }
    void Finish(bool cancel = false) {
        WorldSession session{&player}; WorldPackets::Scene::SceneInstance packet{instance};
        if (cancel) session.HandleScenePlaybackCanceled(packet); else session.HandleScenePlaybackComplete(packet);
    }
};
int main() {
    _set_abort_behavior(0, _CALL_REPORTFAULT);
#ifdef BASELINE
    Pickup baseline;
    baseline.Finish();
    assert(baseline.player.casts.empty() && baseline.player.swaps == 0 && baseline.player.offhand == nullptr);
    std::cout << "BASELINE: pinned792eee91 has no1215 scene hook; native completion does not equip owned Earthwarder. Controlled inventory, not a client traversal.\n";
    return 0;
#else
    for (uint32 step : {5u, 6u}) for (uint32 status : {1u, 3u}) for (bool cancel : {false, true}) {
        Pickup pickup; scenarioManager.scenario.step = step; pickup.player.quest = status;
        pickup.player.TrigerScene(pickup.instance, "Smooth Phase Complete");
        pickup.player.TrigerScene(pickup.instance, "complete");
        assert(pickup.player.swaps == 0 && pickup.player.casts.empty());
        pickup.Finish(cancel); pickup.Finish(cancel);
        assert(pickup.player.swaps == 1 && pickup.player.casts == std::vector<uint32>{208562});
        assert(pickup.player.offhand == &pickup.player.artifact && pickup.player.mainhand == &pickup.player.paired);
        assert(pickup.player.previousWeapon.position == 0xFF20 && pickup.player.paired.position == 0xFF0F);
        assert(pickup.player.effects.empty() && pickup.player.m_sceneInstanceID.empty() && pickup.player.credits.empty() && warnings == 0);
    }
    for (uint32 invalid = 0; invalid < 16; ++invalid) {
        Pickup pickup;
        if (invalid == 0) scenarioManager.scenario.step = 4;
        if (invalid == 1) scenarioManager.scenario.step = 7;
        if (invalid == 2) scenarioManager.scenario.id = 1092;
        if (invalid == 3) scenarioManager.present = false;
        if (invalid == 4) pickup.player.runtimeInstance = 43;
        if (invalid == 5) pickup.player.map = 1220;
        if (invalid == 6) pickup.player.playerClass = 2;
        if (invalid == 7) pickup.player.rewarded = true;
        if (invalid == 8) pickup.player.quest = 0;
        if (invalid == 9) pickup.player.quest = 5;
        if (invalid == 10) pickup.player.alive = false;
        if (invalid == 11) pickup.player.world = false;
        if (invalid == 12) pickup.player.contact = false;
        if (invalid == 13) pickup.player.teleporting = true;
        if (invalid == 14) pickup.player.owned = false;
        if (invalid == 15) pickup.player.bankOnly = true;
        pickup.Finish(); assert(pickup.player.casts.empty() && pickup.player.swaps == 0 && pickup.player.credits.empty());
    }
    for (uint32 failure = 0; failure < 4; ++failure) {
        Pickup pickup;
        if (failure == 0) pickup.player.castResult = 1;
        if (failure == 1) pickup.player.equipResult = EQUIP_ERR_BAG_FULL;
        if (failure == 2) pickup.player.childResult = EQUIP_ERR_CANT_EQUIP;
        if (failure == 3) pickup.player.swapBlocked = true;
        pickup.Finish();
        assert(pickup.player.mainhand == &pickup.player.previousWeapon && pickup.player.offhand == nullptr);
        assert(pickup.player.casts == std::vector<uint32>{208562} && pickup.player.credits.empty() && warnings == 1);
        assert(pickup.player.equipErrors == ((failure == 1 || failure == 2) ? 1u : 0u));
    }
    {
        Pickup pickup;
        uint32 obsolete = pickup.instance; pickup.instance = pickup.player.Start(pickup.effect);
        pickup.player.SceneCompleted(obsolete); assert(pickup.player.casts.empty());
        pickup.Finish(); assert(pickup.player.swaps == 1);
        pickup.player.TrigerScene(obsolete, "complete"); assert(pickup.player.casts.size() == 1);
    }
    {
        Pickup pickup;
        pickup.player.RemoveAurasDueToSpell(205340);
        pickup.Finish(); assert(pickup.player.casts.empty() && pickup.player.swaps == 0);
        SpellScene unrelated{21, 1575, 1220};
        assert(!pickup.script.OnTrigger(&pickup.player, &unrelated, "complete"));
        assert(!pickup.script.OnTrigger(&pickup.player, nullptr, "complete"));
        unrelated = {21, 1583, 1215}; assert(!pickup.script.OnTrigger(&pickup.player, &unrelated, "complete"));
        assert(pickup.script.OnTrigger(nullptr, &spellManager.scene, "complete"));
    }
    {
        Pickup pickup;
        pickup.player.offhand = &pickup.player.artifact; pickup.player.artifact.position = 0xFF10;
        pickup.player.mainhand = &pickup.player.paired; pickup.player.paired.position = 0xFF0F;
        pickup.Finish(); assert(pickup.player.swaps == 0 && warnings == 0);
    }
    {
        Pickup pickup;
        pickup.player.paired.identity = 999;
        pickup.Finish();
        assert(pickup.player.swaps == 1 && pickup.player.offhand == &pickup.player.artifact && warnings == 1);
    }
    {
        Pickup pickup;
        pickup.player.offhand = &pickup.player.artifact; pickup.player.artifact.position = 0xFF10;
        pickup.player.mainhand = nullptr;
        pickup.Finish();
        assert(pickup.player.swaps == 0 && pickup.player.offhand == &pickup.player.artifact && !pickup.player.mainhand && warnings == 1);
    }
    std::cout << "PASS: actual scene/packet lifecycle -> scoped1215/1575 callback -> native208562 effect; existing offhand shield/mainhand child swap, completed/cancelled/forged/stale/repeated/abandoned context and failed equip; native208562 progression/group/ability effects are source-traced separately, not executed by this host\n";
#endif
}
