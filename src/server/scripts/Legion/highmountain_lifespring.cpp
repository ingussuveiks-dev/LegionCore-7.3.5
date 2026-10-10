#include "ScriptMgr.h"
#include "ScriptedCreature.h"
#include "SmartAI.h"
#include "SpellScript.h"
#include "Player.h"
#include "TemporarySummon.h"
#include "ObjectAccessor.h"
#include "QuestDef.h"
#include <map>
#include <mutex>

namespace Lifespring
{
bool Active(Player* player, uint32 quest)
{
    auto status = player->GetQuestStatus(quest);
    return status == QUEST_STATUS_INCOMPLETE || status == QUEST_STATUS_COMPLETE;
}
bool Wanted(Player* player)
{
    return player && player->IsAlive() && player->GetMapId() == 1220 && player->GetAreaId() == 7786 &&
        !player->IsBeingTeleported() && !player->isInFlight() && !player->GetVehicle() &&
        player->GetQuestRewardStatus(39661) && !Active(player,39498) && !player->GetQuestRewardStatus(39498) &&
        (Active(player,39488) || Active(player,39489) || Active(player,39487) ||
         player->GetQuestRewardStatus(39488) || player->GetQuestRewardStatus(39489) || player->GetQuestRewardStatus(39487));
}
Player* Owner(Creature* creature)
{
    TempSummon* summon = creature->ToTempSummon();
    Unit* owner = summon ? summon->GetSummoner() : nullptr;
    return owner ? owner->ToPlayer() : nullptr;
}
void Clear(Player* player)
{
    auto guids = *player->GetSummonList(96038);
    for (ObjectGuid const& guid : guids)
        if (Creature* npc = ObjectAccessor::GetCreature(*player,guid))
            if (Owner(npc) == player) npc->DespawnOrUnsummon();
}
Creature* Existing(Player* player)
{
    Creature* result = nullptr;
    auto guids = *player->GetSummonList(96038);
    for (ObjectGuid const& guid : guids)
        if (Creature* npc = ObjectAccessor::GetCreature(*player,guid))
            if (Owner(npc) == player)
            {
                if (!npc->IsAlive() || result) npc->DespawnOrUnsummon();
                else result = npc;
            }
    return result;
}
void Recover(Player* player)
{
    if (!Wanted(player)) { Clear(player); return; }
    if (!Existing(player)) player->CastSpell(player,190370,true);
}
}

class spell_lifespring_companion : public SpellScript
{
    PrepareSpellScript(spell_lifespring_companion);
    SpellCastResult Check()
    {
        Player* player = GetCaster()->ToPlayer();
        return Lifespring::Wanted(player) && !Lifespring::Existing(player) ? SPELL_CAST_OK : SPELL_FAILED_DONT_REPORT;
    }
    void Register() override { OnCheckCast += SpellCheckCastFn(spell_lifespring_companion::Check); }
};

struct npc_lifespring_companion : SmartAI
{
    npc_lifespring_companion(Creature* creature) : SmartAI(creature) { }
    uint32 poll = 0;
    void IsSummonedBy(Unit* summoner) override
    {
        Player* player = summoner->ToPlayer();
        if (!Lifespring::Wanted(player)) { me->DespawnOrUnsummon(); return; }
        me->AddPlayerInPersonnalVisibilityList(player->GetGUID());
        me->SetReactState(REACT_PASSIVE);
        me->SetWalk(false);
        Talk(0,player->GetGUID());
        me->GetMotionMaster()->MoveFollow(player,3.0f,3.14f);
    }
    void UpdateAI(uint32 diff) override
    {
        if (!me->ToTempSummon()) { SmartAI::UpdateAI(diff); return; }
        Player* player = Lifespring::Owner(me);
        if (!Lifespring::Wanted(player)) { me->DespawnOrUnsummon(); return; }
        if (poll > diff) { poll -= diff; return; }
        poll = 1000;
        // Recreate beside the owner after a lost path/map boundary, rather than
        // dragging an old creature through cave walls or another floor.
        if (player->GetDistance(me) > 60.0f || !player->InSamePhase(me))
        { me->DespawnOrUnsummon(); return; }
        if (me->GetMotionMaster()->GetCurrentMovementGeneratorType() != FOLLOW_MOTION_TYPE && !me->isInCombat())
            me->GetMotionMaster()->MoveFollow(player,3.0f,3.14f);
    }
    void sQuestAccept(Player* player, Quest const* quest) override
    {
        if (quest->GetQuestId() == 39498 && Lifespring::Owner(me) == player)
            Talk(1,player->GetGUID());
        SmartAI::sQuestAccept(player,quest);
    }
};

class player_lifespring_companion : public PlayerScript
{
    std::mutex mutex;
    std::map<ObjectGuid,uint32> timers;
public:
    player_lifespring_companion() : PlayerScript("player_lifespring_companion") { }
    void OnLogout(Player* player) override
    {
        Lifespring::Clear(player);
        std::lock_guard<std::mutex> lock(mutex);
        timers.erase(player->GetGUID());
    }
    void OnMapChanged(Player* player) override { OnLogout(player); }
    void OnUpdate(Player* player, uint32 diff) override
    {
        bool wanted = Lifespring::Wanted(player);
        {
            std::lock_guard<std::mutex> lock(mutex);
            if (!wanted)
            {
                // Only a player we have serviced can have our companion. Avoid
                // allocating summon-list entries for every unrelated player.
                if (!timers.erase(player->GetGUID())) return;
            }
            else
            {
                uint32& timer = timers[player->GetGUID()];
                if (timer > diff) { timer -= diff; return; }
                timer = 1000;
            }
        }
        Lifespring::Recover(player);
    }
};

void AddSC_highmountain_lifespring()
{
    RegisterSpellScript(spell_lifespring_companion);
    RegisterCreatureAI(npc_lifespring_companion);
    new player_lifespring_companion();
}
