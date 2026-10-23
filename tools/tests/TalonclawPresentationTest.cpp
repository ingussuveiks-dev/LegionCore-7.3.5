#include "SceneLifecycleFixture.h"
#include <initializer_list>
enum { CLASS_HUNTER = 3, QUEST_STATUS_INCOMPLETE = 3, QUEST_STATUS_COMPLETE = 1, EFFECT_0 = 0, EFFECT_1 = 1, AURA_EFFECT_HANDLE_REAL = 1, SPELL_CAST_OK = 0 };
using SpellCastResult = uint32;
#ifdef MARKSMANSHIP_PRESENTATION_TEST
constexpr uint32 ScenarioId = 972, FinalStep = 7, PresentationSpell = 200279, EquipSpell = 200262;
constexpr uint32 PresentationScene = 1169, PresentationMap = 1489, PresentationQuest = 40419, PresentationItem = 128826, PickupCriterion = 48210, SceneEffect = 1;
constexpr uint32 ScenePackage = 1535, SceneFlags = 17;
#else
constexpr uint32 ScenarioId = 973, FinalStep = 12, PresentationSpell = 205589, EquipSpell = 197651;
constexpr uint32 PresentationScene = 1220, PresentationMap = 1511, PresentationQuest = 40385, PresentationItem = 128808, PickupCriterion = 47689, SceneEffect = 0;
constexpr uint32 ScenePackage = 1583, SceneFlags = 21;
#endif
using AuraEffectHandleModes = int;
namespace ArtifactAcquisition {
#include "Active.inc"
}
struct Scenario {
    uint32 id = ScenarioId, step = FinalStep;
    uint32 GetScenarioId() const { return id; }
    uint32 GetCurrentStep() const { return step; }
    void SendCriteriaUpdate(struct CriteriaProgress const*) { }
    void UpdateCurrentStep(bool loading) { assert(!loading); if (step == FinalStep) ++step; }
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
        struct { struct { uint32 type = 92, misc1 = PickupCriterion, misc2 = 0, misc3 = 0; } achievementCriteria; } action;
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
        uint32 targetGUID = recipient->GetGUIDLow(), spellID = PresentationSpell;
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
    bool ValidateSpellInfo(std::initializer_list<int> spells) { return spells.size() == 1 && *spells.begin() == EquipSpell; }
    void Remove() { remove(); }
};
#include "Owner.inc"
template<class Script> ApplyHook MakeApply(void(Script::*function)(AuraEffect const*, AuraEffectHandleModes), int effect, int aura, int mode) {
    assert(effect == SceneEffect && aura == 430 && mode == 1);
    return {[function](AuraScript* script, AuraEffect const* value, int handle) { (static_cast<Script*>(script)->*function)(value, handle); }};
}
template<class Script> UpdateHook MakeUpdate(void(Script::*function)(uint32, AuraEffect*), int effect, int aura) {
    assert(effect == SceneEffect && aura == 430);
    return {[function](AuraScript* script, uint32 diff, AuraEffect* value) { (static_cast<Script*>(script)->*function)(diff, value); }};
}
#define PrepareAuraScript(name) public:
#define AuraEffectApplyFn(function,effect,aura,mode) MakeApply(&function,effect,aura,mode)
#define AuraEffectRemoveFn(function,effect,aura,mode) MakeApply(&function,effect,aura,mode)
#define AuraEffectUpdateFn(function,effect,aura) MakeUpdate(&function,effect,aura)
#ifdef MARKSMANSHIP_PRESENTATION_TEST
namespace MarksmanshipClaim {
std::vector<Player*> notices;
void Queue(Player* player) { notices.push_back(player); }
}
#endif
#include "Presentation.inc"
#ifdef MARKSMANSHIP_PRESENTATION_TEST
using PresentationScript = spell_marksmanship_presentation;
#else
using PresentationScript = spell_talonclaw_presentation;
#endif
struct Pickup {
    Player player;
    AuraEffect effect;
    Aura aura;
    PresentationScript script;
    uint32 scene = 0;
    Pickup() {
        scenarioManager = {}; aura.owner = &player; script.m_aura = &aura;
        player.map = PresentationMap; player.expectedQuest = PresentationQuest; player.expectedItem = PresentationItem;
        player.expectedCriterion = PickupCriterion; effect.scene = PresentationScene; effect.spell = PresentationSpell;
        spellManager.scene.SceneScriptPackageID = ScenePackage; spellManager.scene.PlaybackFlags = SceneFlags;
        script.Register(); assert(script.Validate(nullptr));
        player.removed = [this](uint32 spell) {
            if (spell == PresentationSpell) script.AfterEffectRemove.function(&script, &effect, 1);
        };
        script.remove = [this]() { player.RemoveAurasDueToSpell(PresentationSpell); };
    }
    void Begin() { scene = player.Start(effect); script.AfterEffectApply.function(&script, &effect, 1); }
    void Tick(uint32 diff) { script.OnEffectUpdate.function(&script, diff, &effect); }
    void Finish() { player.SceneCompleted(scene); }
};
int main() {
    _set_abort_behavior(0, _CALL_REPORTFAULT);
    #ifdef MARKSMANSHIP_PRESENTATION_TEST
    {
        MarksmanshipClaim::notices.clear();
        Pickup pickup;pickup.Begin();pickup.Finish();pickup.Finish();
        assert(MarksmanshipClaim::notices==std::vector<Player*>{&pickup.player});
    }
    {
        MarksmanshipClaim::notices.clear();
        Pickup pickup;pickup.player.castResult=5;pickup.Begin();pickup.Finish();
        assert(MarksmanshipClaim::notices.empty());
    }
    #endif
    {
        Pickup pickup;
        pickup.player.cast = [&](uint32 spell) { if (spell == PresentationSpell) pickup.Begin(); };
        CriteriaProgress progress; AchievementMgr<Scenario> criteria{&scenarioManager.scenario};
        pickup.player.criteriaUpdate = [&]() { criteria.SendCriteriaUpdate(&progress, 0, false); };
        PickupActions actions{&pickup.player};
        actions.QueueScene(); assert(pickup.player.casts.empty() && pickup.player.delayedInvokerCasts.size() == 1);
        actions.Criteria(); assert(scenarioManager.scenario.step == FinalStep + 1 && pickup.player.casts.empty());
        pickup.player.FlushInvokerCasts(); pickup.Finish(); pickup.Finish();
        assert((pickup.player.casts == std::vector<uint32>{PresentationSpell, EquipSpell}));
    }
    {
        Pickup pickup; scenarioManager.scenario.step = FinalStep + 1; pickup.Begin(); pickup.Tick(20000);
        assert(pickup.player.casts == std::vector<uint32>{EquipSpell});
    }
    for (uint32 invalid = 0; invalid < 3; ++invalid) {
        Pickup pickup;
        pickup.player.cast = [&](uint32 spell) { if (spell == PresentationSpell) pickup.Begin(); };
        CriteriaProgress progress; AchievementMgr<Scenario> criteria{&scenarioManager.scenario};
        pickup.player.criteriaUpdate = [&]() { criteria.SendCriteriaUpdate(&progress, 0, false); };
        PickupActions actions{&pickup.player}; actions.QueueScene(); actions.Criteria();
        if (invalid == 0) pickup.player.world = false;
        if (invalid == 1) pickup.player.quest = 0;
        if (invalid == 2) pickup.player.runtimeInstance = 43;
        pickup.player.FlushInvokerCasts(); if (pickup.scene) pickup.Finish(); pickup.Tick(UINT32_MAX);
        assert(pickup.player.casts == (invalid == 0 ? std::vector<uint32>{} : std::vector<uint32>{PresentationSpell}));
    }
    {
        Pickup pickup; pickup.Begin(); assert(pickup.player.casts.empty());
        pickup.Tick(19999); assert(pickup.player.casts.empty());
        scenarioManager.scenario.step = FinalStep + 1; scenarioManager.present = false;
        pickup.Finish(); pickup.Finish(); pickup.Tick(1);
        assert(pickup.player.casts == std::vector<uint32>{EquipSpell});
        assert(pickup.player.effects.empty() && pickup.player.m_sceneInstanceID.empty());
    }
    {
        Pickup pickup; pickup.Begin(); pickup.Tick(19999); int before = warnings;
        pickup.Tick(1); pickup.Tick(UINT32_MAX); pickup.Finish();
        assert(pickup.player.casts == std::vector<uint32>{EquipSpell} && warnings == before + 1);
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
        assert(pickup.player.casts == std::vector<uint32>{EquipSpell});
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
            case 11: scenarioManager.scenario.step = FinalStep - 1; break;
            case 12: scenarioManager.scenario.step = FinalStep + 2; break;
        }
        pickup.Begin(); pickup.Finish(); pickup.Tick(UINT32_MAX);
        assert(pickup.player.casts.empty());
    }
    for (uint32 step = 0; step <= FinalStep + 3; ++step) {
        if (step == FinalStep || step == FinalStep + 1) continue;
        Pickup pickup; scenarioManager.scenario.step = step; pickup.Begin(); pickup.Finish(); pickup.Tick(UINT32_MAX);
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
        pickup.player.RemoveAurasDueToSpell(PresentationSpell);
        assert(pickup.player.casts.empty());
        PresentationScript retry; retry.m_aura = &pickup.aura; retry.Register();
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
    assert(spellManager.scene.SceneScriptPackageID == ScenePackage && spellManager.scene.PlaybackFlags == SceneFlags);
#ifdef MARKSMANSHIP_PRESENTATION_TEST
    {
        Pickup pickup; pickup.player.castResult = 5;
        pickup.player.cast = [&](uint32 spell) { if (spell == PresentationSpell) pickup.Begin(); };
        CriteriaProgress progress; AchievementMgr<Scenario> criteria{&scenarioManager.scenario};
        pickup.player.criteriaUpdate = [&]() { criteria.SendCriteriaUpdate(&progress, 0, false); };
        PickupActions actions{&pickup.player}; actions.QueueScene(); actions.Criteria(); pickup.player.FlushInvokerCasts();
        pickup.Tick(UINT32_MAX);
        assert(scenarioManager.scenario.step == FinalStep + 1 && pickup.player.casts == std::vector<uint32>{PresentationSpell});
        assert(pickup.player.effects.empty());
    }
    {
        Pickup pickup; pickup.Begin(); pickup.player.castResult = 5; int before = warnings;
        pickup.Finish(); pickup.Finish(); pickup.Tick(UINT32_MAX);
        assert(pickup.player.casts == std::vector<uint32>{EquipSpell} && warnings == before + 1);
        assert(pickup.player.effects.empty());
        PresentationScript retry; retry.m_aura = &pickup.aura; retry.Register(); pickup.player.castResult = 0;
        pickup.player.removed = [&](uint32 spell) { if (spell == PresentationSpell) retry.AfterRemove(&pickup.effect, 1); };
        uint32 scene = pickup.player.Start(pickup.effect); retry.AfterApply(&pickup.effect, 1);
        pickup.player.SceneCompleted(scene);
        assert((pickup.player.casts == std::vector<uint32>{EquipSpell, EquipSpell}));
    }
#endif
    std::cout << "PASS: extracted native scene and route-specific aura hooks, actual owner getter, final-stage owned-item admission, complete/cancel-equivalent removal, bounded missing-callback cleanup, invalidation, stale retry, no grant or duplicate scenario credit\n";
    std::cout << "PASS: native200ms invoker queue -> synchronous native criterion dispatcher -> completed-step scene admission/equip, with pre-dispatch logout/abandon/re-entry rejection\n";
}
