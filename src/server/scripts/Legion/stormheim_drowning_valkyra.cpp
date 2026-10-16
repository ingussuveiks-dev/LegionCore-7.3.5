#include "ScriptMgr.h"
#include "SmartAI.h"
#include "SpellScript.h"
#include "Player.h"
#include "ObjectAccessor.h"
#include "ObjectMgr.h"
#include "QuestData.h"
#include <algorithm>
#include <map>

namespace StormheimDrowningValkyra
{
constexpr uint32 QuestId = 39851, CreditId = 97472, Lantern = 192387;
constexpr uint32 HideDelay = 4500, Cycle = 18500;
struct Site { ObjectGuid::LowType spawn; Position point; };
Site const Sites[] = {
    {11546786,{307.359f,457.002f,4.34966f,1.80312f}},
    {11546787,{300.068f,617.618f,6.08967f,2.27148f}},
    {11546788,{328.184f,510.083f,12.3617f,0.17433f}},
    {11546789,{201.566f,470.658f,6.83275f,5.58173f}},
    {11546790,{186.927f,432.241f,10.4137f,0.757764f}},
    {11546791,{151.613f,511.788f,7.98752f,0.576571f}},
    {11546792,{129.092f,586.227f,8.89817f,0.6908f}},
    {11546793,{81.1476f,611.356f,21.6815f,1.80078f}},
    {11546794,{155.33f,646.333f,10.3065f,5.28428f}},
    {11546795,{249.057f,705.366f,8.14049f,5.03742f}}
};
bool Scope(Player* p)
{
    return p && p->IsInWorld() && p->GetMapId() == 1463 && !p->GetInstanceId() &&
        p->GetZoneId() == 7588 && !p->HasPhaseId(8306);
}
bool Identity(Creature* c)
{
    if (!c || !c->IsInWorld() || c->GetEntry() != 97469 || c->GetMapId() != 1463 ||
        c->GetInstanceId() || c->GetZoneId() != 7588 ||
        sObjectMgr->GetScriptName(c->GetCreatureTemplate()->ScriptID) != "npc_stormheim_drowning_valkyra") return false;
    for (auto const& site : Sites)
        if (c->GetDBTableGUIDLow() == site.spawn && c->GetExactDist(&site.point) < 1) return true;
    return false;
}
bool Definition()
{
    auto q = sQuestDataStore->GetQuestTemplate(QuestId);
    if (!q || q->StartScript || q->HasSpecialFlag(QUEST_SPECIAL_FLAGS_EXPLORATION_OR_EVENT) || q->GetObjectives().size() != 1) return false;
    auto const& o = q->GetObjectives().front();
    return o.Type == QUEST_OBJECTIVE_MONSTER && o.ObjectID == CreditId && o.StorageIndex == 1 && o.Amount == 8 && !o.Flags;
}
bool Active(Player* p)
{
    return Scope(p) && Definition() && p->IsAlive() && !p->IsBeingTeleported() && !p->GetVehicle() &&
        !p->GetTransport() && !p->isInFlight() && !p->isInCombat() && p->GetQuestStatus(QuestId) == QUEST_STATUS_INCOMPLETE &&
        p->GetQuestObjectiveData(QuestId, CreditId) < 8;
}
bool Available(Player* p, Creature* c);
bool Valid(Player* p, Creature* c)
{
    return Active(p) && Identity(c) && c->IsAIEnabled && c->IsAlive() &&
        c->HasFlag(UNIT_FIELD_NPC_FLAGS, UNIT_NPC_FLAG_SPELLCLICK) && !c->ShouldHideFor(p->GetGUID()) &&
        p->InSamePhase(c) && p->canSeeOrDetect(c) && p->GetExactDist(c) <= 10 && p->IsWithinLOSInMap(c) && Available(p, c);
}
void Release(Player* p, Creature* c);
}

struct npc_stormheim_drowning_valkyra : SmartAI
{
    explicit npc_stormheim_drowning_valkyra(Creature* c) : SmartAI(c) { }
    struct Visit { std::weak_ptr<uint8> lifetime; uint32 age = 0; bool hidden = false; };
    std::map<ObjectGuid, Visit> visits;
    bool Available(Player* p) const
    {
        auto it = visits.find(p->GetGUID());
        auto status = p->getQuestStatus(StormheimDrowningValkyra::QuestId);
        return it == visits.end() || !status || it->second.lifetime.expired() || status->ScriptLifetime != it->second.lifetime.lock();
    }
    void Release(Player* p)
    {
        using namespace StormheimDrowningValkyra;
        if (!Valid(p, me)) return;
        auto q = sQuestDataStore->GetQuestTemplate(QuestId);
        auto const& o = q->GetObjectives().front();
        auto status = p->getQuestStatus(QuestId);
        visits[p->GetGUID()] = {status->ScriptLifetime, 0, false}; // Before any reentrant optional dialogue.
        auto count = p->GetQuestObjectiveData(QuestId, CreditId) + 1;
        p->SetQuestObjectiveData(q, &o, count);
        p->SendQuestUpdateAddCredit(q, me->GetGUID(), o, count);
        if (p->CanCompleteQuest(QuestId)) p->CompleteQuest(QuestId);
        Talk(0, p->GetGUID()); // Preserve the native random text group and SAY behavior.
    }
    void SpellHit(Unit* caster, SpellInfo const* spell) override
    {
        if (StormheimDrowningValkyra::Identity(me) && spell->Id == StormheimDrowningValkyra::Lantern) return;
        SmartAI::SpellHit(caster, spell);
    }
    void UpdateAI(uint32 diff) override
    {
        using namespace StormheimDrowningValkyra;
        // Keep AIName=SmartAI and the original event/TAL for later Warrior
        // spawns. Their CALL_TIMED_ACTIONLIST admission checks that AIName.
        if (!Identity(me)) { SmartAI::UpdateAI(diff); return; }
        for (auto it = visits.begin(); it != visits.end();)
        {
            auto p = ObjectAccessor::GetPlayer(*me, it->first);
            auto status = p ? p->getQuestStatus(QuestId) : nullptr;
            auto& visit = it->second;
            visit.age += std::min(diff, Cycle);
            auto questStatus = p ? p->GetQuestStatus(QuestId) : QUEST_STATUS_NONE;
            if (!Scope(p) || !status || visit.lifetime.expired() || status->ScriptLifetime != visit.lifetime.lock() ||
                (questStatus != QUEST_STATUS_INCOMPLETE && questStatus != QUEST_STATUS_COMPLETE) ||
                !p->InSamePhase(me) || p->GetExactDist(me) > 150 || visit.age >= Cycle)
            {
                if (visit.hidden) { me->RemoveFromHideList(it->first); if (p) p->UpdateVisibilityOf(me); }
                it = visits.erase(it);
                continue;
            }
            if (!visit.hidden && visit.age >= HideDelay)
            {
                visit.hidden = true;
                me->AddToHideList(it->first);
                p->UpdateVisibilityOf(me);
            }
            ++it;
        }
    }
};

namespace StormheimDrowningValkyra
{
bool Available(Player* p, Creature* c)
{
    auto ai = dynamic_cast<npc_stormheim_drowning_valkyra*>(c->AI());
    return ai && ai->Available(p);
}
void Release(Player* p, Creature* c)
{
    if (auto ai = dynamic_cast<npc_stormheim_drowning_valkyra*>(c->AI())) ai->Release(p);
}
}

class spell_stormheim_drowning_valkyra : public SpellScript
{
    PrepareSpellScript(spell_stormheim_drowning_valkyra);
    ObjectGuid actor;
    std::weak_ptr<uint8> lifetime;
    bool captured = false, scoped = false;
    bool Valid()
    {
        using namespace StormheimDrowningValkyra;
        auto p = GetCaster()->ToPlayer();
        auto status = p ? p->getQuestStatus(QuestId) : nullptr;
        return captured && status && !lifetime.expired() && status->ScriptLifetime == lifetime.lock() &&
            StormheimDrowningValkyra::Valid(p, ObjectAccessor::GetCreature(*p, actor));
    }
    SpellCastResult Check()
    {
        using namespace StormheimDrowningValkyra;
        if (!captured)
        {
            auto target = GetExplTargetUnit(); auto c = target ? target->ToCreature() : nullptr;
            scoped = Identity(c);
            if (!scoped) return SPELL_CAST_OK;
            auto p = GetCaster()->ToPlayer();
            if (!StormheimDrowningValkyra::Valid(p, c)) return SPELL_FAILED_BAD_TARGETS;
            p->AddQuestDelayedEvent(QuestId, 0, [] {});
            actor = c->GetGUID(); lifetime = p->getQuestStatus(QuestId)->ScriptLifetime; captured = true;
        }
        return Valid() ? SPELL_CAST_OK : SPELL_FAILED_BAD_TARGETS;
    }
    void Hit(SpellEffIndex)
    {
        if (!scoped) return;
        auto hit = GetHitUnit();
        if (!Valid() || !hit || hit->GetGUID() != actor) { PreventHitDefaultEffect(EFFECT_0); return; }
        StormheimDrowningValkyra::Release(GetCaster()->ToPlayer(), hit->ToCreature());
    }
    void Register() override
    {
        OnCheckCast += SpellCheckCastFn(spell_stormheim_drowning_valkyra::Check);
        OnEffectHitTarget += SpellEffectFn(spell_stormheim_drowning_valkyra::Hit, EFFECT_0, SPELL_EFFECT_DUMMY);
    }
};

void AddSC_stormheim_drowning_valkyra()
{
    RegisterCreatureAI(npc_stormheim_drowning_valkyra);
    RegisterSpellScript(spell_stormheim_drowning_valkyra);
}
