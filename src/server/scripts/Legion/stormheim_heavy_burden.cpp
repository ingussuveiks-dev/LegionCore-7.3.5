#include "ScriptMgr.h"
#include "SmartAI.h"
#include "SpellScript.h"
#include "Player.h"
#include "GameObject.h"
#include "ObjectAccessor.h"
#include "QuestData.h"
#include <map>
#include <mutex>

namespace StormheimHeavyBurden
{
constexpr uint32 QuestId = 40078, Fish = 193563, Skewer = 193576, CreditId = 98239;
// The barrel was absent. Restore it at the native quest POI3220,1555 with
// native MMAP ground180.773; the spear retains its existing elevated placement.
Position const Barrel = {3220.0f, 1555.0f, 180.773f, 0};
Position const Spear = {3194.48f, 1519.95f, 183.274f, 0.685995f};
struct Run
{
    std::weak_ptr<uint8> lifetime;
    ObjectGuid spear;
    std::shared_ptr<uint8> cast;
};
std::map<ObjectGuid, Run> runs;
std::recursive_mutex mutex;
bool Scope(Player* p)
{
    return p && p->IsInWorld() && p->GetMapId() == 1220 && !p->GetInstanceId() &&
        p->GetZoneId() == 7541 && !p->HasPhaseId(8306);
}
bool Definition()
{
    auto q = sQuestDataStore->GetQuestTemplate(QuestId);
    if (!q || q->StartScript || q->HasSpecialFlag(QUEST_SPECIAL_FLAGS_EXPLORATION_OR_EVENT)) return false;
    uint32 count = 0;
    for (auto const& o : q->GetObjectives())
    {
        if (o.Type != QUEST_OBJECTIVE_MONSTER || o.ObjectID != CreditId || o.StorageIndex != 1 || o.Amount != 1 || o.Flags) return false;
        ++count;
    }
    return count == 1;
}
bool Ready(Player* p)
{
    return Scope(p) && Definition() && p->IsAlive() && !p->IsBeingTeleported() &&
        !p->GetVehicle() && !p->GetTransport() && !p->isInFlight() && !p->isInCombat() &&
        p->GetQuestStatus(QuestId) == QUEST_STATUS_INCOMPLETE && !p->GetQuestObjectiveData(QuestId, CreditId);
}
bool Current(Player* p, Run const& r)
{
    auto s = p->getQuestStatus(QuestId);
    return Ready(p) && s && !r.lifetime.expired() && s->ScriptLifetime == r.lifetime.lock();
}
uint32 Identity(GameObject* go)
{
    if (!go || !go->IsInWorld() || go->GetMapId() != 1220 || go->GetInstanceId() || go->GetZoneId() != 7541 ||
        go->HasPhaseId(8306) || go->HasPhaseId(7510) || go->GetGoType() != GAMEOBJECT_TYPE_GOOBER ||
        go->GetGOInfo()->goober.questID != QuestId) return 0;
    if (go->GetEntry() == 244867 && go->GetGOInfo()->goober.spell == Fish && go->GetExactDist(&Barrel) < 1) return 1;
    if (go->GetEntry() == 244872 && go->GetDBTableGUIDLow() == 115015 &&
        go->GetGOInfo()->goober.spell == 193577 && go->GetExactDist(&Spear) < 1) return 2;
    return 0;
}
bool Source(Player* p, GameObject* go, uint32 kind)
{
    return Ready(p) && Identity(go) == kind && go->isSpawned() &&
        !go->HasFlag(GAMEOBJECT_FIELD_FLAGS, GO_FLAG_IN_USE | GO_FLAG_NOT_SELECTABLE) &&
        p->InSamePhase(go) && p->canSeeOrDetect(go) && p->GetExactDist(go) <= go->GetInteractionDistance() &&
        p->IsWithinLOSInMap(go);
}
void Clear(Player* p)
{
    runs.erase(p->GetGUID());
    // Remove the native slow/carry aura, including its client-owned animation.
    // Preserve foreign casters, independent slows, pets, movers and viewpoints.
    p->RemoveAurasDueToSpell(Fish, p->GetGUID());
}
bool Carrying(Player* p, Run const& r)
{
    return Current(p, r) && p->GetAura(Fish, p->GetGUID());
}
bool Delivering(Player* p, Run const& r)
{
    return Carrying(p, r) && Source(p, ObjectAccessor::GetGameObject(*p, r.spear), 2);
}
void Use(Player* p, GameObject* go, uint32 kind)
{
    if (!Source(p, go, kind) || p->HasUnitState(UNIT_STATE_CASTING)) return;
    auto it = runs.find(p->GetGUID());
    if (it != runs.end() && !Carrying(p, it->second)) { Clear(p); it = runs.end(); }
    if (kind == 1)
    {
        if (it != runs.end()) return;
        // Never adopt a saved/reapplied aura as proof of collecting this run's
        // fish. Its instant native spell applies the slow and carry animation.
        p->RemoveAurasDueToSpell(Fish, p->GetGUID());
        p->AddQuestDelayedEvent(QuestId, 0, [] {});
        Run r; r.lifetime = p->getQuestStatus(QuestId)->ScriptLifetime;
        p->CastSpell(p, Fish, true);
        if (p->GetAura(Fish, p->GetGUID())) runs.emplace(p->GetGUID(), r);
        return;
    }
    if (it == runs.end() || !Carrying(p, it->second)) return;
    auto& r = it->second;
    r.spear = go->GetGUID(); r.cast = std::make_shared<uint8>();
    // Native193577 only queues193576 with its native1500ms cast time. Start
    // that same cast directly so its source/acceptance ticket is unambiguous.
    // Interruptions retain the fish; the next use replaces the old cast ticket.
    if (p->CastSpell(p, Skewer, false) != SPELL_CAST_OK) r.cast.reset();
}
}

struct go_stormheim_heavy_burden : SmartGameObjectAI
{
    explicit go_stormheim_heavy_burden(GameObject* object) : SmartGameObjectAI(object) { }
    bool GossipHello(Player* p, bool isUse) override
    {
        using namespace StormheimHeavyBurden;
        std::lock_guard<std::recursive_mutex> lock(mutex);
        auto kind = Identity(go);
        if (!kind) return SmartGameObjectAI::GossipHello(p, isUse);
        // AI dispatch precedes the native hidden/in-use admission. Check it
        // here and consume invalid base interactions, never falling through to
        // the old no-fish/group-credit native goober path.
        if (isUse) Use(p, go, kind);
        return true;
    }
};

class spell_stormheim_heavy_burden : public SpellScript
{
    PrepareSpellScript(spell_stormheim_heavy_burden);
    bool controlled = false, checked = false;
    std::weak_ptr<uint8> cast, lifetime;
    ObjectGuid source;
    SpellCastResult Check()
    {
        using namespace StormheimHeavyBurden;
        std::lock_guard<std::recursive_mutex> lock(mutex);
        auto p = GetCaster() ? GetCaster()->ToPlayer() : nullptr;
        // Native CheckCast can run again at completion. Never adopt a newer
        // collection/cast ticket during that second check.
        if (checked) return !controlled || (p && Valid(p)) ? SPELL_CAST_OK : SPELL_FAILED_BAD_TARGETS;
        checked = true;
        auto it = p ? runs.find(p->GetGUID()) : runs.end();
        if (it == runs.end() || !it->second.cast) return SPELL_CAST_OK; // unrelated native uses
        controlled = true;
        auto& r = it->second;
        // TARGET_UNIT_CASTER1 needs no explicit unit. InitExplicitTargets
        // removes the redundant self target before this hook executes.
        if (!Delivering(p, r) || (GetExplTargetUnit() && GetExplTargetUnit() != p)) return SPELL_FAILED_BAD_TARGETS;
        cast = r.cast; lifetime = r.lifetime; source = r.spear;
        return SPELL_CAST_OK;
    }
    bool Valid(Player* p)
    {
        using namespace StormheimHeavyBurden;
        auto it = p ? runs.find(p->GetGUID()) : runs.end();
        return it != runs.end() && !cast.expired() && !lifetime.expired() &&
            it->second.cast == cast.lock() && it->second.lifetime.lock() == lifetime.lock() &&
            it->second.spear == source && Delivering(p, it->second);
    }
    void RemoveFish(SpellEffIndex index)
    {
        // Native effect0 queues broad193563 removal after10ms. Keep carry
        // ownership until the validated credit effect; consume only our aura.
        if (controlled) PreventHitDefaultEffect(index);
    }
    void Complete(SpellEffIndex index)
    {
        using namespace StormheimHeavyBurden;
        if (!controlled) return;
        PreventHitDefaultEffect(index); // never native134's group reward
        std::lock_guard<std::recursive_mutex> lock(mutex);
        auto p = GetCaster() ? GetCaster()->ToPlayer() : nullptr;
        if (!p || GetHitUnit() != p || !Valid(p)) return;
        auto q = sQuestDataStore->GetQuestTemplate(QuestId);
        Clear(p);
        for (auto const& o : q->GetObjectives())
        {
            p->SetQuestObjectiveData(q, &o, 1);
            p->SendQuestUpdateAddCredit(q, source, o, 1);
            if (p->CanCompleteQuest(QuestId)) p->CompleteQuest(QuestId);
        }
    }
    void Register() override
    {
        OnCheckCast += SpellCheckCastFn(spell_stormheim_heavy_burden::Check);
        OnEffectHitTarget += SpellEffectFn(spell_stormheim_heavy_burden::RemoveFish, EFFECT_0, SPELL_EFFECT_REMOVE_AURA_2);
        OnEffectHitTarget += SpellEffectFn(spell_stormheim_heavy_burden::Complete, EFFECT_1, SPELL_EFFECT_KILL_CREDIT2);
    }
};

class player_stormheim_heavy_burden : public PlayerScript
{
public:
    player_stormheim_heavy_burden() : PlayerScript("player_stormheim_heavy_burden") { }
    void OnUpdate(Player* p, uint32) override
    {
        using namespace StormheimHeavyBurden;
        std::lock_guard<std::recursive_mutex> lock(mutex);
        auto it = runs.find(p->GetGUID());
        if (it != runs.end() && !Carrying(p, it->second)) Clear(p);
    }
    void OnLogout(Player* p) override
    {
        using namespace StormheimHeavyBurden;
        std::lock_guard<std::recursive_mutex> lock(mutex);
        if (runs.count(p->GetGUID())) Clear(p);
    }
    void OnMapChanged(Player* p) override
    {
        using namespace StormheimHeavyBurden;
        std::lock_guard<std::recursive_mutex> lock(mutex);
        if (runs.count(p->GetGUID())) Clear(p);
    }
    void OnLogin(Player* p) override
    {
        using namespace StormheimHeavyBurden;
        std::lock_guard<std::recursive_mutex> lock(mutex);
        runs.erase(p->GetGUID());
        // Fresh incomplete logins collect again. A completed quest needs only
        // Havi's reward; native reward227837 remains unchanged as a fallback.
        if (!p->HasPhaseId(8306) && (Scope(p) ||
            p->GetQuestStatus(QuestId) == QUEST_STATUS_INCOMPLETE || p->GetQuestStatus(QuestId) == QUEST_STATUS_COMPLETE)) Clear(p);
    }
};
void AddSC_stormheim_heavy_burden()
{
    RegisterGameObjectAI(go_stormheim_heavy_burden);
    RegisterSpellScript(spell_stormheim_heavy_burden);
    new player_stormheim_heavy_burden();
}
