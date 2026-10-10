#include "ScriptMgr.h"
#include "ScriptedCreature.h"
#include "SpellScript.h"
#include "Player.h"
#include "ObjectAccessor.h"
#include "QuestData.h"

namespace HighmountainIntro
{
// Native TaxiNodes 1719, not a scripted teleport destination.
Position const Landing = {4156.5098f,4374.4502f,768.16f,0};
bool Ready(Player* player)
{
    return player && player->IsAlive() && player->GetMapId() == 1220 && !player->IsBeingTeleported();
}
void CreditFlight(Player* player)
{
    // 96813 is also used by Broken Shore quest 45571. Only update this quest.
    if (Quest const* quest = sQuestDataStore->GetQuestTemplate(38907))
        for (QuestObjective const& objective : quest->GetObjectives())
            if (objective.Type == QUEST_OBJECTIVE_MONSTER && objective.ObjectID == 96813)
            {
                player->SetQuestObjectiveData(quest,&objective,1);
                player->SendQuestUpdateAddCredit(quest,ObjectGuid::Empty,objective,1);
                if (player->CanCompleteQuest(38907)) player->CompleteQuest(38907);
                else player->SetQuestStatus(38907,QUEST_STATUS_INCOMPLETE);
                break;
            }
}
void WatchFlight(Player* player, bool approaching = false)
{
    if (player->GetQuestStatus(38907) != QUEST_STATUS_INCOMPLETE || player->GetQuestObjectiveData(38907,96813)) return;
    if (!Ready(player)) approaching = false;
    else
    {
        if (player->isInFlight() && player->m_taxi.GetTaxiDestination() == 1719) approaching = true;
        if (approaching && !player->isInFlight())
        {
            if (!player->GetVehicle() && player->GetDistance(Landing) < 20.0f)
            {
                CreditFlight(player);
                return;
            }
            approaching = false; // canceled flight or a different landing
        }
    }
    // The quest-owned guard cancels this observer on abandon/reaccept. Login
    // restarts observation, including an ordinary resumed taxi flight.
    player->AddQuestDelayedEvent(38907,500,[player,approaching]() { WatchFlight(player,approaching); });
}
}

class spell_highmountain_discover_thunder_totem : public SpellScript
{
    PrepareSpellScript(spell_highmountain_discover_thunder_totem);
    void Start()
    {
        if (Player* player = GetCaster()->ToPlayer()) HighmountainIntro::WatchFlight(player);
    }
    void Register() override { AfterCast += SpellCastFn(spell_highmountain_discover_thunder_totem::Start); }
};

class player_highmountain_intro : public PlayerScript
{
public:
    player_highmountain_intro() : PlayerScript("player_highmountain_intro") { }
    void OnLogin(Player* player) override { HighmountainIntro::WatchFlight(player); }
};

struct npc_highmountain_arrival_oro : ScriptedAI
{
    npc_highmountain_arrival_oro(Creature* creature) : ScriptedAI(creature) { }
    void MoveInLineOfSight(Unit* who) override
    {
        Player* player = who->ToPlayer();
        if (!HighmountainIntro::Ready(player) || player->isInFlight() || player->GetVehicle() ||
            player->GetQuestStatus(38907) != QUEST_STATUS_INCOMPLETE ||
            player->GetQuestObjectiveData(38907,106244) || player->GetDistance(me) > 8.0f ||
            !player->canSeeOrDetect(me)) return;
        Talk(0,player->GetGUID());
        player->KilledMonsterCredit(106244);
        ObjectGuid guid = me->GetGUID();
        player->AddQuestDelayedEvent(38907,3000,[player,guid]()
        {
            if (!HighmountainIntro::Ready(player)) return;
            if (Creature* oro = ObjectAccessor::GetCreature(*player,guid))
                if (player->GetDistance(oro) < 20.0f && player->canSeeOrDetect(oro))
                    oro->AI()->Talk(1,player->GetGUID());
        });
    }
};

struct npc_highmountain_poison_idol : ScriptedAI
{
    npc_highmountain_poison_idol(Creature* creature) : ScriptedAI(creature) { }
    void SpellHit(Unit* caster, SpellInfo const* spell) override
    {
        Player* player = caster->ToPlayer();
        uint32 entry = me->GetEntry();
        if (spell->Id != 195481 || entry < 99433 || entry > 99436 || !me->IsAlive() ||
            !HighmountainIntro::Ready(player) || player->GetQuestStatus(39272) != QUEST_STATUS_INCOMPLETE ||
            player->GetDistance(me) > 5.0f || !player->canSeeOrDetect(me) ||
            player->GetQuestObjectiveData(39272,entry)) return;
        // Native hidden objectives persist each distinct idol across logout.
        // Do not disable or despawn a shared idol for other players.
        player->KilledMonsterCredit(entry);
        player->KilledMonsterCredit(94965);
    }
};

void AddSC_highmountain_intro()
{
    RegisterSpellScript(spell_highmountain_discover_thunder_totem);
    RegisterCreatureAI(npc_highmountain_arrival_oro);
    RegisterCreatureAI(npc_highmountain_poison_idol);
    new player_highmountain_intro();
}
