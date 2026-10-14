#include "ScriptMgr.h"
#include "ScriptedCreature.h"
#include "Player.h"
#include "ObjectAccessor.h"
#include "QuestData.h"
#include "SpellScript.h"
#include "SpellMgr.h"
#include "SpellAuraEffects.h"
#include "TemporarySummon.h"
#include "MoveSpline.h"
#include "MotionMaster.h"

namespace StormheimNoWings
{
constexpr uint32 Launcher = 138111, Hook = 91971;
struct Branch { uint32 quest, team, entry, spawn; };
Branch const branches[] = {{38318,ALLIANCE,91983,294335}, {38613,HORDE,91975,294296}};
bool Point(Creature* point)
{
    return point && point->IsInWorld() && point->GetMapId() == 1220 &&
        (point->GetEntry() == 91975 || point->GetEntry() == 91983 || point->GetEntry() == 91979 ||
            point->GetEntry() == 92017 || point->GetEntry() == 92072);
}
bool Ready(Player* player, Creature* point)
{
    return player && player->IsInWorld() && player->IsAlive() && !player->IsBeingTeleported() &&
        !player->GetVehicle() && !player->GetTransport() && !player->isInFlight() &&
        Point(point) && point->IsAlive() && !point->IsQuestObjectExpired() &&
        point->HasFlag(UNIT_FIELD_NPC_FLAGS, UNIT_NPC_FLAG_SPELLCLICK) && player->canSeeSpellClickOn(point) &&
        player->GetItemByEntry(Launcher) && player->InSamePhase(point) && player->canSeeOrDetect(point) &&
        player->IsWithinCombatRange(point, player->GetSpellMaxRangeForTarget(point, sSpellMgr->GetSpellInfo(182478))) &&
        player->IsWithinLOSInMap(point);
}
uint32 ActiveQuest(Player* player, Creature* point)
{
    if (!player || !Point(point) || point->GetZoneId() != 7541) return 0;
    for (Branch const& branch : branches)
        if (player->GetTeam() == branch.team && point->GetEntry() == branch.entry &&
            point->GetDBTableGUIDLow() == branch.spawn && player->GetQuestStatus(branch.quest) == QUEST_STATUS_INCOMPLETE)
            if (Quest const* quest = sQuestDataStore->GetQuestTemplate(branch.quest))
                for (QuestObjective const& objective : quest->GetObjectives())
                    if (objective.Type == QUEST_OBJECTIVE_MONSTER && objective.ObjectID == 91484 &&
                        objective.StorageIndex == 0 && objective.Amount == 1 && !(objective.Flags & QUEST_OBJECTIVE_FLAG_OPTIONAL))
                        return branch.quest;
    return 0;
}
uint32 LifetimeQuest(Player* player, Creature* point)
{
    if (uint32 quest = ActiveQuest(player, point)) return quest;
    if (!player || !Point(point) || point->GetZoneId() != 7541 || player->GetInstanceId() != 0) return 0;
    uint32 quest = player->GetTeam() == ALLIANCE ? 38412 : 38618;
    if (player->GetQuestStatus(quest) != QUEST_STATUS_INCOMPLETE) return 0;
    if (Quest const* info = sQuestDataStore->GetQuestTemplate(quest))
    {
        bool across = false, top = false;
        for (QuestObjective const& objective : info->GetObjectives())
            if (objective.Type == QUEST_OBJECTIVE_MONSTER && objective.Amount == 1 && !(objective.Flags & QUEST_OBJECTIVE_FLAG_OPTIONAL))
            {
                across |= objective.ObjectID == 91728 && objective.StorageIndex == 0;
                top |= objective.ObjectID == 91729 && objective.StorageIndex == 1 && (objective.Flags & QUEST_OBJECTIVE_FLAG_SEQUENCED);
            }
        if (across && top) return quest;
    }
    return 0;
}
void RepairLegacy(Player* player)
{
    for (Branch const& branch : branches)
        if (player->GetTeam() == branch.team && player->GetQuestStatus(branch.quest) == QUEST_STATUS_COMPLETE &&
            !player->GetQuestObjectiveData(branch.quest, 91484))
            if (Quest const* quest = sQuestDataStore->GetQuestTemplate(branch.quest))
                for (QuestObjective const& objective : quest->GetObjectives())
                    if (objective.Type == QUEST_OBJECTIVE_MONSTER && objective.ObjectID == 91484 &&
                        objective.StorageIndex == 0 && objective.Amount == 1 && !(objective.Flags & QUEST_OBJECTIVE_FLAG_OPTIONAL))
                        player->SetQuestStatus(branch.quest, QUEST_STATUS_INCOMPLETE);
}
void Arrival(Player* player, uint32 quest, ObjectGuid pointGuid, uint32 instance, uint32 spline, uint32 attempts)
{
    Creature* point = ObjectAccessor::GetCreature(*player, pointGuid);
    if (!Ready(player, point) || player->GetInstanceId() != instance || ActiveQuest(player, point) != quest ||
        player->movespline->GetId() != spline) return;
    if (!player->movespline->Finalized())
    {
        if (attempts)
            player->AddQuestDelayedEvent(quest, 100, [player,quest,pointGuid,instance,spline,attempts]()
                { Arrival(player,quest,pointGuid,instance,spline,attempts-1); });
        return;
    }
    // Finalized alone also describes an interrupted spline. Require its exact
    // destination and the player's actual arrival, including the wall height.
    auto end = player->movespline->FinalDestination();
    if (point->GetExactDist(end.x,end.y,end.z) > 1.0f || !player->IsWithinDistInMap(point, 2.0f) ||
        player->GetQuestObjectiveData(quest, 91484)) return;
    if (Quest const* info = sQuestDataStore->GetQuestTemplate(quest))
        for (QuestObjective const& objective : info->GetObjectives())
            if (objective.Type == QUEST_OBJECTIVE_MONSTER && objective.ObjectID == 91484)
            {
                player->SetQuestObjectiveData(info, &objective, 1);
                player->SendQuestUpdateAddCredit(info, pointGuid, objective, 1);
                if (player->CanCompleteQuest(quest)) player->CompleteQuest(quest);
                return;
            }
}
}

struct npc_stormheim_grapple_hook : ScriptedAI
{
    explicit npc_stormheim_grapple_hook(Creature* creature) : ScriptedAI(creature) { }
    ObjectGuid pointGuid;
    bool consumed = false;
    void SetGUID(ObjectGuid const& guid, int32) override
    {
        pointGuid = guid;
        me->SetReactState(REACT_PASSIVE);
        me->DespawnOrUnsummon(10000); // Native summon has duration 0; never leak a hook.
    }
    ObjectGuid GetGUID(int32) override { return pointGuid; }
    void DoAction(int32 action) override
    {
        if (pointGuid.IsEmpty() || consumed) return;
        consumed = true;
        TempSummon* summon = me->ToTempSummon();
        Player* player = summon && summon->GetSummoner() ? summon->GetSummoner()->ToPlayer() : nullptr;
        Creature* point = ObjectAccessor::GetCreature(*me, pointGuid);
        bool launched = action == 1 && !me->IsQuestObjectExpired() && StormheimNoWings::Ready(player, point) &&
            player->CastSpell(point, 182469, true) == SPELL_CAST_OK;
        //182470's native tether lasts4s. Retain its hook through that effect.
        me->DespawnOrUnsummon(launched ? 5000 : 2000);
    }
};

class spell_stormheim_grapple_start : public SpellScript
{
    PrepareSpellScript(spell_stormheim_grapple_start);
    Creature* Point() { return GetExplTargetUnit() ? GetExplTargetUnit()->ToCreature() : nullptr; }
    SpellCastResult Check()
    {
        if (!StormheimNoWings::Point(Point())) return SPELL_FAILED_BAD_TARGETS;
        Player* player = GetCaster()->ToPlayer();
        return StormheimNoWings::Ready(player, Point()) && !player->HasAura(182468) &&
            player->movespline->Finalized() ? SPELL_CAST_OK : SPELL_FAILED_BAD_TARGETS;
    }
    void Launch(SpellEffIndex index)
    {
        Creature* point = Point();
        if (!StormheimNoWings::Point(point)) return;
        PreventHitDefaultEffect(index);
        Player* player = GetCaster()->ToPlayer();
        if (!StormheimNoWings::Ready(player, point)) return;
        // Keep the real summon + hook aura. An explicit Unit target also fixes
        // TARGET_DEST_NEARBY_ENTRY's native destination fallback to this point.
        auto before = *player->GetSummonList(StormheimNoWings::Hook);
        SpellCastTargets targets;
        targets.SetUnitTarget(point);
        targets.SetDst(*point);
        player->CastSpell(targets, sSpellMgr->GetSpellInfo(182467), nullptr, TRIGGERED_FULL_MASK);
        auto after = *player->GetSummonList(StormheimNoWings::Hook);
        for (ObjectGuid guid : after)
            if (std::find(before.begin(), before.end(), guid) == before.end())
                if (Creature* hook = ObjectAccessor::GetCreature(*player, guid))
                if (auto* ai = dynamic_cast<npc_stormheim_grapple_hook*>(hook->AI()))
                {
                    ai->SetGUID(point->GetGUID(), 0);
                    if (uint32 quest = StormheimNoWings::LifetimeQuest(player, point)) player->BindQuestObject(quest, hook);
                    if (!player->HasAura(182468)) ai->DoAction(2);
                }
    }
    void Register() override
    {
        OnCheckCast += SpellCheckCastFn(spell_stormheim_grapple_start::Check);
        OnEffectLaunchTarget += SpellEffectFn(spell_stormheim_grapple_start::Launch, EFFECT_0, SPELL_EFFECT_TRIGGER_SPELL_4);
    }
};

class aura_stormheim_grapple_hook : public AuraScript
{
    PrepareAuraScript(aura_stormheim_grapple_hook);
    void End(AuraEffect const*, AuraEffectHandleModes)
    {
        Player* player = GetTarget()->ToPlayer();
        if (!player || player->GetMapId() != 1220) return;
        auto hooks = *player->GetSummonList(StormheimNoWings::Hook);
        for (ObjectGuid guid : hooks)
            if (Creature* hook = ObjectAccessor::GetCreature(*player, guid))
                if (auto* ai = dynamic_cast<npc_stormheim_grapple_hook*>(hook->AI()))
                    if (!ai->GetGUID(0).IsEmpty())
                        ai->DoAction(GetTargetApplication()->GetRemoveMode() == AURA_REMOVE_BY_EXPIRE ? 1 : 2);
    }
    void Register() override
    {
        AfterEffectRemove += AuraEffectRemoveFn(aura_stormheim_grapple_hook::End, EFFECT_0, SPELL_AURA_DUMMY, AURA_EFFECT_HANDLE_REAL);
    }
};

// TARGET_DEST_CASTER_SUMMON (18) otherwise overwrites the requested hook
// location with a point behind the caster. Adjust only this scripted gun use.
class spell_stormheim_grapple_summon : public SpellScript
{
    PrepareSpellScript(spell_stormheim_grapple_summon);
    void PositionHook(SpellEffIndex)
    {
        Creature* point = GetExplTargetUnit() ? GetExplTargetUnit()->ToCreature() : nullptr;
        if (StormheimNoWings::Ready(GetCaster()->ToPlayer(), point) && GetHitDest())
            GetHitDest()->Relocate(*point);
    }
    void Register() override
    {
        OnEffectHit += SpellEffectFn(spell_stormheim_grapple_summon::PositionHook, EFFECT_0, SPELL_EFFECT_SUMMON);
    }
};

class spell_stormheim_grapple_jump : public SpellScript
{
    PrepareSpellScript(spell_stormheim_grapple_jump);
    uint32 initialSpline = 0;
    void Before() { initialSpline = GetCaster()->movespline->GetId(); }
    void End()
    {
        Player* player = GetCaster()->ToPlayer();
        Creature* point = GetExplTargetUnit() ? GetExplTargetUnit()->ToCreature() : nullptr;
        uint32 quest = StormheimNoWings::ActiveQuest(player, point);
        if (!quest || !StormheimNoWings::Ready(player, point) || player->movespline->GetId() == initialSpline ||
            player->GetMotionMaster()->GetCurrentMovementGeneratorType() != EFFECT_MOTION_TYPE) return;
        uint32 spline = player->movespline->GetId(), instance = player->GetInstanceId();
        ObjectGuid guid = point->GetGUID();
        player->AddQuestDelayedEvent(quest, 100, [player,quest,guid,instance,spline]()
            { StormheimNoWings::Arrival(player,quest,guid,instance,spline,100); });
    }
    void Register() override
    {
        BeforeCast += SpellCastFn(spell_stormheim_grapple_jump::Before);
        AfterCast += SpellCastFn(spell_stormheim_grapple_jump::End);
    }
};

class player_stormheim_no_wings : public PlayerScript
{
public:
    player_stormheim_no_wings() : PlayerScript("player_stormheim_no_wings") { }
    void OnLogin(Player* player) override { StormheimNoWings::RepairLegacy(player); }
};

void AddSC_stormheim_no_wings()
{
    RegisterCreatureAI(npc_stormheim_grapple_hook);
    RegisterSpellScript(spell_stormheim_grapple_start);
    RegisterAuraScript(aura_stormheim_grapple_hook);
    RegisterSpellScript(spell_stormheim_grapple_summon);
    RegisterSpellScript(spell_stormheim_grapple_jump);
    new player_stormheim_no_wings();
}
