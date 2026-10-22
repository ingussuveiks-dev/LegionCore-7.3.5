#ifndef HIGHMOUNTAIN_HUNTING_H
#define HIGHMOUNTAIN_HUNTING_H

#include "SpellAuraEffects.h"
#include "TemporarySummon.h"
#include "MotionMaster.h"
#include <set>

namespace AddieHunting
{
constexpr uint32 QuestId = 40216;
constexpr uint32 Controller = 188723;
constexpr uint32 SummonSpell = 188720;
constexpr uint32 Companion = 99040;
constexpr uint32 LevelCredits[] = {97354,99935,99936};
constexpr int8 SkillSlots[] = {0,5,6};

uint8 Stage(Player* player)
{
    for (uint8 i=0;i<3;++i)
        if (!player->GetQuestObjectiveData(QuestId,LevelCredits[i]))
            return i;
    return 3;
}

void Advance(Player* player, uint32 points)
{
    Quest const* quest = sQuestDataStore->GetQuestTemplate(QuestId);
    if (!quest || player->GetQuestStatus(QuestId) != QUEST_STATUS_INCOMPLETE ||
        !player->GetQuestObjectiveData(QuestId,99034))
        return;

    // The three native optional counters have separate persistent storage.
    // Give each level credit only when its own skill threshold is reached.
    for (uint8 stage=Stage(player);stage<3;++stage)
    {
        QuestObjective const* skill=nullptr;
        for (QuestObjective const& objective:quest->GetObjectives())
            if (objective.StorageIndex==SkillSlots[stage] && objective.ObjectID==97441)
                skill=&objective;
        if (!skill || skill->Amount<=0)
            return;
        uint32 count=player->GetQuestObjectiveData(quest,skill->StorageIndex);
        uint32 remaining=count<uint32(skill->Amount)?uint32(skill->Amount)-count:0;
        uint32 earned=std::min(points,remaining);
        if (earned)
        {
            count+=earned;
            points-=earned;
            player->SetQuestObjectiveData(quest,skill,count);
            player->SendQuestUpdateAddCredit(quest,ObjectGuid::Empty,*skill,count);
        }
        if (count<uint32(skill->Amount))
            break;
        player->KilledMonsterCredit(LevelCredits[stage]);
    }
    // This is an event quest (SpecialFlags 2): CanCompleteQuest intentionally
    // refuses automatic completion while its event is still incomplete.
    // Finish the event only after every training stage and native required
    // objective, rather than invoking the old blanket-completion helper.
    if (Stage(player)!=3)
        return;
    for (QuestObjective const& objective:quest->GetObjectives())
        if (!(objective.Flags & (QUEST_OBJECTIVE_FLAG_OPTIONAL | QUEST_OBJECTIVE_FLAG_HIDE_ITEM_GAINS |
            QUEST_OBJECTIVE_FLAG_PART_OF_PROGRESS_BAR)) && !player->HasQuestObjectiveComplete(quest,objective))
            return;
    player->CompleteQuest(QuestId);
}

uint32 SkillSpell(uint32 entry)
{
    // Hunting credit spells carry 5/10 points in SpellEffect.db2.
    switch (entry)
    {
        case 94238: return 0; // Skittish Doe: native one-point template credit
        case 94198: case 98808: return 196679; // hillstriders
        case 94149: case 94151: case 99481: case 95437:
        case 96146: case 96264: case 102384: case 102385:
        case 102800: case 102825: return 196680;
        default: return 0;
    }
}
bool IsPrey(uint32 entry) { return entry==94238 || SkillSpell(entry)!=0; }
}

// The native controller summon is a generic Pinerock Hunter. Supply its
// missing server actor initialization using the installed Addie template.
struct npc_highmountain_addie_hunt : ScriptedAI
{
    npc_highmountain_addie_hunt(Creature* creature):ScriptedAI(creature) { }
    ObjectGuid ownerGuid;
    uint32 timer=0, shot=0;
    void IsSummonedBy(Unit* owner) override
    {
        Player* player=owner?owner->ToPlayer():nullptr;
        if (!player || me->GetUInt32Value(UNIT_FIELD_CREATED_BY_SPELL)!=AddieHunting::SummonSpell)
            return;
        ownerGuid=player->GetGUID();
        uint32 originalEntry=me->GetEntry();
        me->UpdateEntry(AddieHunting::Companion);
        player->GetSummonList(originalEntry)->remove(me->GetGUID());
        player->GetSummonList(me->GetEntry())->push_back(me->GetGUID());
        me->SetUInt32Value(UNIT_FIELD_NPC_FLAGS,0);
        me->setFaction(player->getFaction());
        me->SetReactState(REACT_DEFENSIVE);
        me->AddPlayerInPersonnalVisibilityList(ownerGuid);
        me->GetMotionMaster()->MoveFollow(player,2.0f,1.5f);
    }
    void UpdateAI(uint32 diff) override
    {
        if (ownerGuid.IsEmpty())
            return;
        if (shot>diff) shot-=diff; else shot=0;
        if (timer>diff) {timer-=diff;return;}
        timer=500;
        Player* player=ObjectAccessor::GetPlayer(*me,ownerGuid);
        if (!player || !player->HasAura(AddieHunting::Controller))
        {
            me->DespawnOrUnsummon();
            return;
        }
        uint8 stage=AddieHunting::Stage(player);
        uint32 bar=stage==0?192381:stage==1?195742:197877;
        for (uint32 aura:{192381u,195742u,197877u})
            if (aura!=bar) me->RemoveAurasDueToSpell(aura);
        if (!me->HasAura(bar)) me->CastSpell(me,bar,true);
        if (Quest const* quest=sQuestDataStore->GetQuestTemplate(AddieHunting::QuestId))
            for (QuestObjective const& objective:quest->GetObjectives())
                if (objective.StorageIndex==AddieHunting::SkillSlots[std::min<uint8>(stage,2)])
                {
                    me->SetMaxPower(POWER_ALTERNATE,objective.Amount);
                    me->SetPower(POWER_ALTERNATE,player->GetQuestObjectiveData(quest,objective.StorageIndex));
                }
        Unit* target=player->getVictim();
        if (target && target->IsCreature() && AddieHunting::IsPrey(target->GetEntry()) &&
            me->IsValidAttackTarget(target) && me->IsWithinDistInMap(target,30.0f))
        {
            if (me->getVictim()!=target) AttackStart(target);
            if (!shot) {me->CastSpell(target,192568,false);shot=3000;}
        }
        else if (!me->isInCombat() && me->GetMotionMaster()->GetCurrentMovementGeneratorType()!=FOLLOW_MOTION_TYPE)
            me->GetMotionMaster()->MoveFollow(player,2.0f,1.5f);
    }
};

class aura_highmountain_addie_hunt : public AuraScript
{
    PrepareAuraScript(aura_highmountain_addie_hunt);
    std::set<ObjectGuid> credited;
    uint32 retry=2000;
    void Proc(AuraEffect const*,ProcEventInfo& event)
    {
        PreventDefaultAction();
        Player* player=GetTarget()->ToPlayer();
        Unit* victim=event.GetActionTarget();
        if (!player || !(event.GetTypeMask()&PROC_FLAG_KILL) || !victim ||
            !victim->IsCreature() || !AddieHunting::IsPrey(victim->GetEntry()) ||
            player->GetMapId()!=1220 || !DB2Manager::IsInArea(victim->GetAreaId(),7732) ||
            !player->IsWithinDistInMap(victim,100.0f))
            return;
        Creature* addie=player->GetMinionByEntry(AddieHunting::Companion);
        if (!addie || addie->GetUInt32Value(UNIT_FIELD_CREATED_BY_SPELL)!=AddieHunting::SummonSpell ||
            !addie->IsWithinDistInMap(victim,100.0f) || !credited.insert(victim->GetGUID()).second)
            return;
        uint32 points=victim->GetEntry()==94238?1:0;
        if (uint32 spell=AddieHunting::SkillSpell(victim->GetEntry()))
            if (SpellInfo const* info=sSpellMgr->GetSpellInfo(spell))
                points=info->Effects[EFFECT_0]->BasePoints;
        AddieHunting::Advance(player,points);
    }
    void Update(uint32 diff)
    {
        Player* player=GetUnitOwner()?GetUnitOwner()->ToPlayer():nullptr;
        if (!player || (player->GetQuestStatus(AddieHunting::QuestId)!=QUEST_STATUS_INCOMPLETE &&
            player->GetQuestStatus(AddieHunting::QuestId)!=QUEST_STATUS_COMPLETE))
        {
            Remove();
            return;
        }
        if (retry>diff) {retry-=diff;return;}
        retry=2000;
        if (player->GetMapId()==1220 && DB2Manager::IsInArea(player->GetAreaId(),7732) &&
            player->GetQuestObjectiveData(AddieHunting::QuestId,99034) &&
            !player->GetMinionByEntry(AddieHunting::Companion))
            player->CastSpell(player,AddieHunting::SummonSpell,true);
    }
    void Register() override
    {
        OnEffectProc+=AuraEffectProcFn(aura_highmountain_addie_hunt::Proc,EFFECT_0,SPELL_AURA_SUMMON_CONTROLLER);
        OnAuraUpdate+=AuraUpdateFn(aura_highmountain_addie_hunt::Update);
    }
};

class spell_highmountain_hunting_skill : public SpellScript
{
    PrepareSpellScript(spell_highmountain_hunting_skill);
    void Credit(SpellEffIndex)
    {
        PreventHitDefaultEffect(EFFECT_0);
        Player* player=GetHitUnit()?GetHitUnit()->ToPlayer():nullptr;
        Unit* caster=GetCaster();
        if (!player || !caster || caster->GetCharmerOrOwnerPlayerOrPlayerItself()!=player ||
            !player->HasAura(AddieHunting::Controller) || player->GetMapId()!=1220 ||
            !DB2Manager::IsInArea(player->GetAreaId(),7732))
            return;
        Creature* addie=player->GetMinionByEntry(AddieHunting::Companion);
        if (!addie || addie->GetUInt32Value(UNIT_FIELD_CREATED_BY_SPELL)!=AddieHunting::SummonSpell ||
            !addie->IsWithinDistInMap(player,100.0f))
            return;
        AddieHunting::Advance(player,GetSpellInfo()->Effects[EFFECT_0]->BasePoints);
    }
    void Register() override
    {
        OnEffectHitTarget+=SpellEffectFn(spell_highmountain_hunting_skill::Credit,EFFECT_0,SPELL_EFFECT_KILL_CREDIT2);
    }
};
#endif
