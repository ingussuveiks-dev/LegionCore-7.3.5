#include "ScriptMgr.h"
#include "SmartAI.h"
#include "SpellScript.h"
#include "Player.h"
#include "ObjectAccessor.h"
#include "ObjectMgr.h"
#include "QuestData.h"
#include "MotionMaster.h"
#include <list>
#include <set>

namespace StormheimSundered
{
constexpr uint32 QuestId = 39850, Lantern = 192493;
struct Fragment { uint32 entry; ObjectGuid::LowType spawn; uint32 credit, pose, conversation; Position point; };
Fragment const Fragments[] = {
    {97356,146728276,97482,192214,212902,{231.622f,378.858f,6.28104f,1.41372f}},
    {97570,146728280,97483,192485,212903,{109.45f,449.408f,-14.6794f,1.25664f}},
    {97571,146728281,97484,192486,212904,{112.366f,704.08f,17.823f,5.49779f}}
};
Fragment const* Identity(Creature* c)
{
    if (c && c->IsInWorld() && c->GetMapId() == 1463 && !c->GetInstanceId() && c->GetZoneId() == 7588 &&
        sObjectMgr->GetScriptName(c->GetCreatureTemplate()->ScriptID) == "npc_stormheim_sundered")
        for (auto const& f : Fragments)
            if (c->GetEntry() == f.entry && c->GetDBTableGUIDLow() == f.spawn && c->GetExactDist(&f.point) < 1)
                return &f;
    return nullptr;
}
bool Scope(Player* p)
{
    return p && p->IsInWorld() && p->GetMapId() == 1463 && !p->GetInstanceId() &&
        p->GetZoneId() == 7588 && !p->HasPhaseId(8306);
}
bool Definition()
{
    auto q = sQuestDataStore->GetQuestTemplate(QuestId);
    if (!q || q->StartScript || q->HasSpecialFlag(QUEST_SPECIAL_FLAGS_EXPLORATION_OR_EVENT)) return false;
    uint32 count = 0;
    for (auto const& o : q->GetObjectives())
    {
        bool known = false;
        for (uint32 i = 0; i < 3; ++i)
            if (o.Type == QUEST_OBJECTIVE_MONSTER && o.ObjectID == Fragments[i].credit &&
                o.StorageIndex == i + 1 && o.Amount == 1 && !o.Flags) known = true;
        if (!known) return false;
        ++count;
    }
    return count == 3;
}
bool Active(Player* p)
{
    return Scope(p) && Definition() && p->IsAlive() && !p->IsBeingTeleported() && !p->GetVehicle() &&
        !p->GetTransport() && !p->isInFlight() && !p->isInCombat() && p->GetQuestStatus(QuestId) == QUEST_STATUS_INCOMPLETE;
}
bool Valid(Player* p, Creature* c)
{
    auto f = Identity(c);
    return f && Active(p) && c->IsAlive() && c->HasFlag(UNIT_FIELD_NPC_FLAGS, UNIT_NPC_FLAG_SPELLCLICK) &&
        !p->GetQuestObjectiveData(QuestId, f->credit) && !c->ShouldHideFor(p->GetGUID()) &&
        p->InSamePhase(c) && p->canSeeOrDetect(c) && p->GetExactDist(c) <= 10 && p->IsWithinLOSInMap(c);
}
bool Collected(Player* p, Fragment const& f)
{
    if (!Scope(p) || !Definition()) return false;
    auto s = p->GetQuestStatus(QuestId);
    return (s == QUEST_STATUS_INCOMPLETE || s == QUEST_STATUS_COMPLETE) && p->GetQuestObjectiveData(QuestId, f.credit);
}
void Release(Player* p, Creature* source)
{
    auto const& f = *Identity(source);
    auto q = sQuestDataStore->GetQuestTemplate(QuestId);
    for (auto const& o : q->GetObjectives()) if (o.ObjectID == f.credit)
    {
        // Individual delivery, independent of optional conversations/summons.
        p->SetQuestObjectiveData(q, &o, 1);
        p->SendQuestUpdateAddCredit(q, source->GetGUID(), o, 1);
        if (p->CanCompleteQuest(QuestId)) p->CompleteQuest(QuestId);
        break;
    }
    // Never release/despawn the public actor or remove its limb auras: they
    // are still needed by other players. Only this player's copy falls.
    if (auto copy = p->SummonCreature(f.entry, f.point, TEMPSUMMON_TIMED_DESPAWN, 5000, 0, p->GetGUID()))
    {
        p->BindQuestObject(QuestId, copy);
        copy->AddPlayerInPersonnalVisibilityList(p->GetGUID());
        copy->RemoveFlag(UNIT_FIELD_NPC_FLAGS, UNIT_NPC_FLAG_SPELLCLICK);
        copy->CastSpell(copy, 192495, true);
        for (uint32 id : {192195u,192196u,192198u,192200u,192214u,192485u,192486u}) copy->RemoveAurasDueToSpell(id);
        copy->SetDisableGravity(false);
        copy->SetAnimTier(AnimTier::Ground); // Addon byte3=3 can outlive the pose aura.
        copy->GetMotionMaster()->MoveFall();
    }
    p->CastSpell(source, f.conversation, true);
    if (source->IsAIEnabled) source->AI()->SetGUID(p->GetGUID(), Lantern);
}
}

struct npc_stormheim_sundered : SmartAI
{
    explicit npc_stormheim_sundered(Creature* c) : SmartAI(c) { }
    std::set<ObjectGuid> hidden;
    uint32 timer = 0;
    bool initialized = false;

    void SetGUID(ObjectGuid const& guid, int32 id) override
    {
        using namespace StormheimSundered;
        auto f = Identity(me);
        if (!f || id != Lantern) { SmartAI::SetGUID(guid, id); return; }
        auto p = ObjectAccessor::GetPlayer(*me, guid);
        if (!p || !Collected(p, *f)) return;
        // Track the hide immediately: abandonment/logout before the next
        //500ms scan must not leave an untracked permanent hide-list entry.
        hidden.insert(guid);
        me->AddToHideList(guid);
        p->UpdateVisibilityOf(me);
    }

    void UpdateAI(uint32 diff) override
    {
        using namespace StormheimSundered;
        auto f = Identity(me);
        if (!f) { SmartAI::UpdateAI(diff); return; }
        if (!initialized) { initialized = true; me->CastSpell(me, 192500, true); }
        if (timer > diff) { timer -= diff; return; }
        timer = 500;
        for (auto it = hidden.begin(); it != hidden.end();)
        {
            auto p = ObjectAccessor::GetPlayer(*me, *it);
            if (p && Collected(p, *f) && p->InSamePhase(me) && p->GetExactDist(me) < 150) { ++it; continue; }
            me->RemoveFromHideList(*it);
            if (p) p->UpdateVisibilityOf(me);
            it = hidden.erase(it);
        }
        std::list<Player*> players;
        me->GetPlayerListInGrid(players, 120);
        for (auto p : players)
            if (Collected(p, *f) && p->InSamePhase(me) && hidden.insert(p->GetGUID()).second)
            {
                me->AddToHideList(p->GetGUID());
                p->UpdateVisibilityOf(me);
            }
    }
    void SpellHit(Unit* caster, SpellInfo const* spell) override
    {
        // Progress is delivered by the validated spell effect, not a shared
        // nonrepeatable SmartAI event or an OnSpellClick after a failed cast.
        if (StormheimSundered::Identity(me) && spell->Id == StormheimSundered::Lantern) return;
        SmartAI::SpellHit(caster, spell);
    }
};

class spell_stormheim_sundered : public SpellScript
{
    PrepareSpellScript(spell_stormheim_sundered);
    ObjectGuid fragment;
    std::weak_ptr<uint8> lifetime;
    bool captured = false, scoped = false;
    bool Valid()
    {
        using namespace StormheimSundered;
        auto p = GetCaster()->ToPlayer();
        auto status = p ? p->getQuestStatus(QuestId) : nullptr;
        auto c = p ? ObjectAccessor::GetCreature(*p, fragment) : nullptr;
        return captured && status && !lifetime.expired() && status->ScriptLifetime == lifetime.lock() && StormheimSundered::Valid(p, c);
    }
    SpellCastResult Check()
    {
        using namespace StormheimSundered;
        if (!captured)
        {
            auto target = GetExplTargetUnit();
            auto c = target ? target->ToCreature() : nullptr;
            scoped = Identity(c) != nullptr;
            if (!scoped) return SPELL_CAST_OK;
            auto p = GetCaster()->ToPlayer();
            if (!StormheimSundered::Valid(p, c)) return SPELL_FAILED_BAD_TARGETS;
            p->AddQuestDelayedEvent(QuestId, 0, [] {});
            fragment = c->GetGUID(); lifetime = p->getQuestStatus(QuestId)->ScriptLifetime; captured = true;
        }
        return Valid() ? SPELL_CAST_OK : SPELL_FAILED_BAD_TARGETS;
    }
    void Hit(SpellEffIndex)
    {
        if (!scoped) return;
        auto hit = GetHitUnit();
        if (!Valid() || !hit || hit->GetGUID() != fragment) { PreventHitDefaultEffect(EFFECT_0); return; }
        StormheimSundered::Release(GetCaster()->ToPlayer(), hit->ToCreature());
    }
    void Register() override
    {
        OnCheckCast += SpellCheckCastFn(spell_stormheim_sundered::Check);
        OnEffectHitTarget += SpellEffectFn(spell_stormheim_sundered::Hit, EFFECT_0, SPELL_EFFECT_DUMMY);
    }
};

void AddSC_stormheim_sundered()
{
    RegisterCreatureAI(npc_stormheim_sundered);
    RegisterSpellScript(spell_stormheim_sundered);
}
