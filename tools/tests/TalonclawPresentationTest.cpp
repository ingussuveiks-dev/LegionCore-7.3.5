#include "SceneLifecycleFixture.h"
#include <initializer_list>
enum { CLASS_HUNTER = 3, QUEST_STATUS_INCOMPLETE = 3, QUEST_STATUS_COMPLETE = 1, EFFECT_0 = 0, AURA_EFFECT_HANDLE_REAL = 1 };
using AuraEffectHandleModes = int;
namespace ArtifactAcquisition {
#include "Active.inc"
}
struct Scenario {
    uint32 id = 973, step = 12;
    uint32 GetScenarioId() const { return id; }
    uint32 GetCurrentStep() const { return step; }
    void SendCriteriaUpdate(struct CriteriaProgress const*) { }
    void UpdateCurrentStep(bool loading) { assert(!loading); if (step == 12) step = 13; }
};
struct ScenarioManager {
    Scenario scenario; bool present = true;
    Scenario* GetScenario(uint32 instance) { return present && instance == 42 ? &scenario : nullptr; }
} scenarioManager;
auto* sScenarioMgr = &scenarioManager;
struct CriteriaProgress {};
template<class Owner> struct AchievementMgr {
    Owner* owner;
    Owner* GetOwner() const { return owner; }
    void SendCriteriaUpdate(CriteriaProgress const*, uint32, bool) const;
};
#include "ScenarioCriteriaSend.inc"
struct ObjectAccessor {
    static Unit* GetUnit(Unit& context, uint32 guid) {
        Player* player = context.ToPlayer();
        return player && player->IsInWorld() && player->GetGUIDLow() == guid ? player : nullptr;
    }
};
using ObjectList = std::list<Unit*>;
Player* GetScriptCreditPlayer(Unit* unit) { return unit ? unit->ToPlayer() : nullptr; }
enum { SMART_ACTION_UPDATE_ACHIEVEMENT_CRITERIA = 205 };
using CriteriaTypes = uint32;
struct PickupActions {
    Player* recipient;
    Unit* GetBaseObject() const { return recipient; }
    ObjectList* GetTargets(int, Unit*) const { return new ObjectList{recipient}; }
    struct Event {
        struct { struct { uint32 type = 92, misc1 = 47689, misc2 = 0, misc3 = 0; } achievementCriteria; } action;
    };
    ObjectList* GetTargets(Event const&, Unit* unit) const { return GetTargets(0, unit); }
    void Criteria() {
        Event event; Unit* unit = recipient;
        switch (SMART_ACTION_UPDATE_ACHIEVEMENT_CRITERIA) {
#include "PickupCriteria.inc"
        }
    }
    void QueueScene() {
        Player* tempLastInvoker = recipient;
        uint32 targetGUID = recipient->GetGUIDLow(), spellID = 205589;
        bool isTrigger = false;
#include "DeferredInvoker.inc"
    }
};
int warnings = 0;
#define TC_LOG_WARN(...) (++warnings)
struct Aura { Unit* owner = nullptr; Unit* GetUnitOwner() const { return owner; } };
struct AuraScript;
struct ApplyHook {
    std::function<void(AuraScript*, AuraEffect const*, AuraEffectHandleModes)> function;
    ApplyHook& operator+=(ApplyHook const& other) { function = other.function; return *this; }
};
struct UpdateHook {
    std::function<void(AuraScript*, uint32, AuraEffect*)> function;
    UpdateHook& operator+=(UpdateHook const& other) { function = other.function; return *this; }
};
struct AuraScript {
    Aura* m_aura = nullptr;
    ApplyHook AfterEffectApply, AfterEffectRemove;
    UpdateHook OnEffectUpdate;
    std::function<void()> remove;
    virtual bool Validate(SpellInfo const*) { return true; }
    virtual void Register() {}
    Unit* GetUnitOwner() const;
    bool ValidateSpellInfo(std::initializer_list<int> spells) { return spells.size() == 1 && *spells.begin() == 197651; }
    void Remove() { remove(); }
};
#include "Owner.inc"
template<class Script> ApplyHook MakeApply(void(Script::*function)(AuraEffect const*, AuraEffectHandleModes), int effect, int aura, int mode) {
    assert(effect == 0 && aura == 430 && mode == 1);
    return {[function](AuraScript* script, AuraEffect const* value, int handle) { (static_cast<Script*>(script)->*function)(value, handle); }};
}
template<class Script> UpdateHook MakeUpdate(void(Script::*function)(uint32, AuraEffect*), int effect, int aura) {
    assert(effect == 0 && aura == 430);
    return {[function](AuraScript* script, uint32 diff, AuraEffect* value) { (static_cast<Script*>(script)->*function)(diff, value); }};
}
#define PrepareAuraScript(name) public:
#define AuraEffectApplyFn(function,effect,aura,mode) MakeApply(&function,effect,aura,mode)
#define AuraEffectRemoveFn(function,effect,aura,mode) MakeApply(&function,effect,aura,mode)
#define AuraEffectUpdateFn(function,effect,aura) MakeUpdate(&function,effect,aura)
#include "Presentation.inc"
struct Pickup {
    Player player;
    AuraEffect effect;
    Aura aura;
    spell_talonclaw_presentation script;
    uint32 scene = 0;
    Pickup() {
        scenarioManager = {}; aura.owner = &player; script.m_aura = &aura;
        script.Register(); assert(script.Validate(nullptr));
        player.removed = [this](uint32 spell) {
            if (spell == 205589) script.AfterEffectRemove.function(&script, &effect, 1);
        };
        script.remove = [this]() { player.RemoveAurasDueToSpell(205589); };
    }
    void Begin() { scene = player.Start(effect); script.AfterEffectApply.function(&script, &effect, 1); }
    void Tick(uint32 diff) { script.OnEffectUpdate.function(&script, diff, &effect); }
    void Finish() { player.SceneCompleted(scene); }
};
int main() {
    _set_abort_behavior(0, _CALL_REPORTFAULT);
    {
        Pickup pickup;
        pickup.player.cast = [&](uint32 spell) { if (spell == 205589) pickup.Begin(); };
        CriteriaProgress progress; AchievementMgr<Scenario> criteria{&scenarioManager.scenario};
        pickup.player.criteriaUpdate = [&]() { criteria.SendCriteriaUpdate(&progress, 0, false); };
        PickupActions actions{&pickup.player};
        actions.QueueScene(); assert(pickup.player.casts.empty() && pickup.player.delayedInvokerCasts.size() == 1);
        actions.Criteria(); assert(scenarioManager.scenario.step == 13 && pickup.player.casts.empty());
        pickup.player.FlushInvokerCasts(); pickup.Finish(); pickup.Finish();
        assert((pickup.player.casts == std::vector<uint32>{205589, 197651}));
    }
    {
        Pickup pickup; scenarioManager.scenario.step = 13; pickup.Begin(); pickup.Tick(20000);
        assert(pickup.player.casts == std::vector<uint32>{197651});
    }
    for (uint32 invalid = 0; invalid < 3; ++invalid) {
        Pickup pickup;
        pickup.player.cast = [&](uint32 spell) { if (spell == 205589) pickup.Begin(); };
        CriteriaProgress progress; AchievementMgr<Scenario> criteria{&scenarioManager.scenario};
        pickup.player.criteriaUpdate = [&]() { criteria.SendCriteriaUpdate(&progress, 0, false); };
        PickupActions actions{&pickup.player}; actions.QueueScene(); actions.Criteria();
        if (invalid == 0) pickup.player.world = false;
        if (invalid == 1) pickup.player.quest = 0;
        if (invalid == 2) pickup.player.runtimeInstance = 43;
        pickup.player.FlushInvokerCasts(); if (pickup.scene) pickup.Finish(); pickup.Tick(UINT32_MAX);
        assert(pickup.player.casts == (invalid == 0 ? std::vector<uint32>{} : std::vector<uint32>{205589}));
    }
    {
        Pickup pickup; pickup.Begin(); assert(pickup.player.casts.empty());
        pickup.Tick(19999); assert(pickup.player.casts.empty());
        scenarioManager.scenario.step = 13; scenarioManager.present = false;
        pickup.Finish(); pickup.Finish(); pickup.Tick(1);
        assert(pickup.player.casts == std::vector<uint32>{197651});
        assert(pickup.player.effects.empty() && pickup.player.m_sceneInstanceID.empty());
    }
    {
        Pickup pickup; pickup.Begin(); pickup.Tick(19999); int before = warnings;
        pickup.Tick(1); pickup.Tick(UINT32_MAX); pickup.Finish();
        assert(pickup.player.casts == std::vector<uint32>{197651} && warnings == before + 1);
        assert(pickup.player.effects.empty() && pickup.player.m_sceneInstanceID.empty());
    }
    {
        Pickup pickup; pickup.Begin(); pickup.Tick(UINT32_MAX);
        assert(pickup.player.casts.size() == 1 && pickup.player.effects.empty());
    }
    {
        Pickup pickup; pickup.Begin(); WorldSession session; session._player = &pickup.player;
        WorldPackets::Scene::SceneInstance packet{pickup.scene};
        session.HandleScenePlaybackCanceled(packet);
        session.HandleScenePlaybackComplete(packet);
        session.HandleScenePlaybackCanceled(packet);
        assert(pickup.player.casts == std::vector<uint32>{197651});
        assert(pickup.player.effects.empty() && pickup.player.m_sceneInstanceID.empty());
        session._player = nullptr; session.HandleScenePlaybackCanceled(packet);
    }
    for (uint32 invalid = 0; invalid < 13; ++invalid) {
        Pickup pickup;
        switch (invalid) {
            case 0: pickup.player.world = false; break;
            case 1: pickup.player.alive = false; break;
            case 2: pickup.player.teleporting = true; break;
            case 3: pickup.player.playerClass = 1; break;
            case 4: pickup.player.map = 1220; break;
            case 5: pickup.player.quest = 0; break;
            case 6: pickup.player.quest = 5; break;
            case 7: pickup.player.rewarded = true; break;
            case 8: pickup.player.owned = false; break;
            case 9: scenarioManager.present = false; break;
            case 10: scenarioManager.scenario.id = 962; break;
            case 11: scenarioManager.scenario.step = 11; break;
            case 12: scenarioManager.scenario.step = 14; break;
        }
        pickup.Begin(); pickup.Finish(); pickup.Tick(UINT32_MAX);
        assert(pickup.player.casts.empty());
    }
    for (uint32 invalid = 0; invalid < 8; ++invalid) {
        Pickup pickup; pickup.Begin();
        switch (invalid) {
            case 0: pickup.player.world = false; break;
            case 1: pickup.player.alive = false; break;
            case 2: pickup.player.teleporting = true; break;
            case 3: pickup.player.map = 1220; break;
            case 4: pickup.player.quest = 0; break;
            case 5: pickup.player.rewarded = true; break;
            case 6: pickup.player.owned = false; break;
            case 7: pickup.player.runtimeInstance = 43; break;
        }
        pickup.Tick(1); pickup.Finish();
        assert(pickup.player.casts.empty() && pickup.player.effects.empty());
    }
    {
        Pickup pickup; pickup.player.quest = 1; pickup.Begin(); pickup.Finish();
        assert(pickup.player.casts.size() == 1);
    }
    {
        Pickup pickup; pickup.Begin(); uint32 obsolete = pickup.scene;
        pickup.player.RemoveAurasDueToSpell(205589);
        assert(pickup.player.casts.empty());
        spell_talonclaw_presentation retry; retry.m_aura = &pickup.aura; retry.Register();
        pickup.player.removed = [&](uint32) { retry.AfterEffectRemove.function(&retry, &pickup.effect, 1); };
        uint32 current = pickup.player.Start(pickup.effect);
        retry.AfterEffectApply.function(&retry, &pickup.effect, 1);
        pickup.player.SceneCompleted(obsolete);
        if (!pickup.player.casts.empty() || pickup.player.effects.empty()) {
            std::cerr << "FAIL: old scene callback equipped a new retry before its presentation completed\n";
            return 1;
        }
        pickup.player.SceneCompleted(current); assert(pickup.player.casts.size() == 1);
    }
    {
        Pickup pickup; Unit pet; pickup.aura.owner = &pet; pickup.Begin(); pickup.Tick(UINT32_MAX); pickup.Finish();
        assert(pickup.player.casts.empty());
        pickup.aura.owner = nullptr; pickup.script.AfterApply(nullptr, 1); pickup.Tick(UINT32_MAX);
        assert(pickup.player.casts.empty());
    }
    std::cout << "PASS: extracted native scene and Talonclaw aura hooks, actual owner getter, final-stage owned-item admission, complete/cancel-equivalent removal, bounded missing-callback cleanup, invalidation, stale retry, no grant or duplicate scenario credit\n";
    std::cout << "PASS: native200ms invoker queue -> synchronous native criterion dispatcher -> completed-step scene admission/equip, with pre-dispatch logout/abandon/re-entry rejection\n";
}
