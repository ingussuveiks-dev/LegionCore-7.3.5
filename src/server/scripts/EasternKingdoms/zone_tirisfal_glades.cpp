#include "ScriptMgr.h"
#include "MotionMaster.h"
#include "Player.h"
#include "ScriptedCreature.h"
#include "SpellScript.h"
#include "TemporarySummon.h"
#include "Vehicle.h"

namespace
{
    constexpr uint32 RecruitmentQuest = 26800;
    constexpr uint32 Darnell = 49337;
    constexpr uint32 ScarletCorpse = 49340;
    constexpr uint32 SummonDarnell = 91938;
    constexpr uint32 RideDarnell = 46598;

    Player* GetDarnellOwner(Creature* creature)
    {
        auto summon = creature->ToTempSummon();
        if (!summon || creature->GetUInt32Value(UNIT_FIELD_CREATED_BY_SPELL) != SummonDarnell)
            return nullptr;
        if (Unit* owner = summon->GetSummoner())
            return owner->ToPlayer();
        return nullptr;
    }

    bool PickupInFlight(WorldObject const& object, ObjectGuid guid)
    {
        if (Creature* copy = ObjectAccessor::GetCreature(object, guid))
            return copy->IsAlive() && !copy->IsDespawn();
        return false;
    }
}

struct npc_scarlet_corpse_q26800 : public ScriptedAI
{
    npc_scarlet_corpse_q26800(Creature* creature) : ScriptedAI(creature)
    {
        me->SetReactState(REACT_PASSIVE);
    }

    void Reset() override
    {
        _collected = false;
        _pendingCopy.Clear();
    }

    void IsSummonedBy(Unit* /*summoner*/) override
    {
        me->RemoveFlag(UNIT_FIELD_NPC_FLAGS, UNIT_NPC_FLAG_SPELLCLICK);
        me->RemoveAura(92230);
    }

    bool CanCollect(Player const& player) const
    {
        return !_collected && me->GetDBTableGUIDLow() && me->GetMapId() == 0 && me->GetAreaId() == 154 &&
            me->IsAlive() && !me->IsDespawn() && me->HasFlag(UNIT_FIELD_NPC_FLAGS, UNIT_NPC_FLAG_SPELLCLICK) &&
            player.IsAlive() && player.GetQuestStatus(RecruitmentQuest) == QUEST_STATUS_INCOMPLETE &&
            me->IsWithinDistInMap(&player, INTERACTION_DISTANCE) && me->IsWithinLOSInMap(&player);
    }

    TempSummon* CreatePickup(Player& player)
    {
        if (PickupInFlight(*me, _pendingCopy))
            return nullptr;

        TempSummon* copy = me->SummonCreature(ScarletCorpse, me->GetPosition(),
            TEMPSUMMON_TIMED_DESPAWN, 3000, 0, player.GetGUID());
        if (copy)
        {
            player.BindQuestObject(RecruitmentQuest, copy);
            _pendingCopy = copy->GetGUID();
        }
        return copy;
    }

    bool ConfirmPickup(Player& player, TempSummon& copy)
    {
        if (_pendingCopy != copy.GetGUID() || !CanCollect(player))
            return false;

        _collected = true;
        _pendingCopy.Clear();
        copy.SetTempSummonType(TEMPSUMMON_MANUAL_DESPAWN);
        player.KilledMonsterCredit(ScarletCorpse);
        me->DespawnOrUnsummon(100);
        return true;
    }

    void OnSpellClick(Unit* who) override;

private:
    ObjectGuid _pendingCopy;
    bool _collected = false;
};

struct npc_darnel_q26800 : public ScriptedAI
{
    npc_darnel_q26800(Creature* creature) : ScriptedAI(creature)
    {
        me->SetReactState(REACT_PASSIVE);
    }

    void IsSummonedBy(Unit* /*summoner*/) override
    {
        Talk(0);
        Player* player = GetDarnellOwner(me);
        if (!player)
            return;

        QuestStatus status = player->GetQuestStatus(RecruitmentQuest);
        if (status == QUEST_STATUS_INCOMPLETE || status == QUEST_STATUS_COMPLETE)
        {
            player->BindQuestObject(RecruitmentQuest, me);
            me->GetMotionMaster()->MoveFollow(player, me->GetFollowDistance(), me->GetFollowAngle());
        }
    }

    bool StartPickup(Creature& source, npc_scarlet_corpse_q26800& corpse, Player& player)
    {
        Vehicle* vehicle = me->GetVehicleKit();
        if (GetDarnellOwner(me) != &player || !me->HasQuestObjectScope() || me->IsQuestObjectExpired() ||
            !me->IsAlive() || me->IsDespawn() ||
            !source.IsWithinDistInMap(me, 10.0f) || !vehicle || !vehicle->GetAvailableSeatCount() ||
            PickupInFlight(*me, _pendingCopy))
            return false;

        TempSummon* copy = corpse.CreatePickup(player);
        if (!copy)
            return false;

        _pendingCopy = copy->GetGUID();
        if (copy->CastSpell(me, RideDarnell, true) != SPELL_CAST_OK)
            copy->DespawnOrUnsummon(1);
        return true;
    }

    void DoAction(int32 const action) override
    {
        if (action != ACTION_1)
            return;

        if (Vehicle* vehicle = me->GetVehicleKit())
            for (auto const& seat : vehicle->Seats)
                if (Unit* passenger = vehicle->GetPassenger(seat.first))
                    if (passenger->GetEntry() == ScarletCorpse && passenger->ToTempSummon())
                        passenger->ExitVehicle();
    }

    void PassengerBoarded(Unit* passenger, int8 /*seatId*/, bool apply) override
    {
        TempSummon* copy = passenger->ToTempSummon();
        if (!copy || passenger->GetEntry() != ScarletCorpse)
            return;

        if (!apply)
        {
            copy->DespawnOrUnsummon(2000);
            return;
        }

        if (_pendingCopy != copy->GetGUID())
            return;
        _pendingCopy.Clear();

        Player* player = GetDarnellOwner(me);
        Unit* source = copy->GetSummoner();
        Creature* original = source ? source->ToCreature() : nullptr;
        auto corpse = original ? dynamic_cast<npc_scarlet_corpse_q26800*>(original->AI()) : nullptr;
        if (!player || me->IsQuestObjectExpired() || copy->IsQuestObjectExpired() ||
            !corpse || !corpse->ConfirmPickup(*player, *copy))
            copy->DespawnOrUnsummon(1);
    }

private:
    ObjectGuid _pendingCopy;
};

void npc_scarlet_corpse_q26800::OnSpellClick(Unit* who)
{
    Player* player = who->ToPlayer();
    if (!player || !CanCollect(*player))
        return;

    for (ObjectGuid const& guid : *player->GetSummonList(Darnell))
        if (Creature* darnell = ObjectAccessor::GetCreature(*player, guid))
            if (auto carrier = dynamic_cast<npc_darnel_q26800*>(darnell->AI()))
                if (carrier->StartPickup(*me, *this, *player))
                    return;
}

// 1740
struct npc_deathguard_saltain : public ScriptedAI
{
    npc_deathguard_saltain(Creature* creature) : ScriptedAI(creature)
    {
        me->SetReactState(REACT_PASSIVE);
    }

    void OnQuestReward(Player* player, Quest const* quest) override
    {
        if (quest->GetQuestId() == RecruitmentQuest)
        {
            auto cList = player->GetSummonList(49337);
            for (GuidList::const_iterator iter = cList->begin(); iter != cList->end(); ++iter)
            {
                if (auto darnel = ObjectAccessor::GetCreature(*player, (*iter)))
                {
                    if (darnel->IsAIEnabled)
                        darnel->AI()->DoAction(ACTION_1);
                }
            }
        }
    }

};



// 91938
class spell_summon_darnel_q26800 : public AuraScript
{
    PrepareAuraScript(spell_summon_darnel_q26800);

    void OnRemove(AuraEffect const* /*effect*/, AuraEffectHandleModes /*mode*/)
    {
        Player* player = GetUnitOwner()->ToPlayer();
        if (!player)
            return;

        for (ObjectGuid const& guid : *player->GetSummonList(Darnell))
            if (Creature* darnell = ObjectAccessor::GetCreature(*player, guid))
                if (darnell->HasQuestObjectScope() && GetDarnellOwner(darnell) == player)
                    darnell->DespawnOrUnsummon(100);
    }

    void Register() override
    {
        AfterEffectRemove += AuraEffectRemoveFn(spell_summon_darnel_q26800::OnRemove, EFFECT_1, SPELL_AURA_DUMMY, AURA_EFFECT_HANDLE_REAL);
    }
};

void AddSC_tirisfal_glades()
{
    RegisterCreatureAI(npc_darnel_q26800);
    RegisterCreatureAI(npc_scarlet_corpse_q26800);
    RegisterCreatureAI(npc_deathguard_saltain);
    RegisterAuraScript(spell_summon_darnel_q26800);
}
