#include "ScriptMgr.h"
#include "ScriptedCreature.h"
#include "Player.h"
#include "ObjectAccessor.h"
#include "Map.h"
#include "MoveSpline.h"
#include "QuestData.h"

namespace HighmountainRiver
{
bool Ready(Player* player)
{
    return player && player->IsAlive() && player->GetMapId() == 1220 &&
        !player->IsBeingTeleported() && player->GetQuestStatus(39614) == QUEST_STATUS_INCOMPLETE;
}

void CreditRescue(Player* player, ObjectGuid fishGuid)
{
    // Entry 95148 also occurs in quest 41144. Keep this river rescue scoped.
    if (Quest const* quest = sQuestDataStore->GetQuestTemplate(39614))
        for (QuestObjective const& objective : quest->GetObjectives())
            if (objective.Type == QUEST_OBJECTIVE_MONSTER && objective.ObjectID == 95148)
            {
                uint32 count = player->GetQuestObjectiveData(39614,95148) + 1;
                if (count > uint32(objective.Amount)) return;
                player->SetQuestObjectiveData(quest,&objective,count);
                player->SendQuestUpdateAddCredit(quest,fishGuid,objective,count);
                if (player->CanCompleteQuest(39614)) player->CompleteQuest(39614);
                break;
            }
}

void WatchLanding(Player* player, ObjectGuid fishGuid, std::weak_ptr<uint8> kick, uint8 remaining)
{
    if (kick.expired() || !Ready(player)) return;
    Creature* fish = ObjectAccessor::GetCreature(*player,fishGuid);
    if (!fish || !fish->IsAlive() || player->GetDistance(fish) > 100.0f || !player->canSeeOrDetect(fish)) return;

    if (fish->movespline->Finalized())
    {
        // Unit::IsInWater is not updated for ordinary creatures in this core.
        // Test the fish's actual liquid depth, not the player's state or the
        // presence of a water surface somewhere below a dry landing.
        auto liquid = fish->GetMap()->getLiquidStatus(fish->GetPositionX(),fish->GetPositionY(),
            fish->GetPositionZ(),MAP_ALL_LIQUIDS,nullptr);
        if (liquid & (LIQUID_MAP_IN_WATER | LIQUID_MAP_UNDER_WATER))
        {
            CreditRescue(player,fishGuid);
            fish->DespawnOrUnsummon();
            return;
        }
        return; // A dry landing is not a rescue; allow another kick.
    }
    if (remaining)
        player->AddQuestDelayedEvent(39614,500,[player,fishGuid,kick,remaining]()
        {
            WatchLanding(player,fishGuid,kick,remaining-1);
        });
}
}

struct npc_highmountain_whitewater_carp : ScriptedAI
{
    npc_highmountain_whitewater_carp(Creature* creature) : ScriptedAI(creature) { }
    std::shared_ptr<uint8> _kick;
    void Reset() override { _kick.reset(); }
    void SpellHit(Unit* caster, SpellInfo const* spell) override
    {
        if (spell->Id != 188447) return;
        _kick.reset(); // Even an ineligible player's kick supersedes the old one.
        Player* player = caster->ToPlayer();
        if (!me->IsAlive() || !HighmountainRiver::Ready(player) ||
            player->GetDistance(me) > 5.0f || !player->canSeeOrDetect(me)) return;
        // A later kick supersedes the earlier player's observation. A dry
        // landing leaves the fish available for another kick.
        _kick = std::make_shared<uint8>(uint8(0));
        ObjectGuid guid = me->GetGUID();
        std::weak_ptr<uint8> kick = _kick;
        player->AddQuestDelayedEvent(39614,500,[player,guid,kick]()
        {
            HighmountainRiver::WatchLanding(player,guid,kick,19);
        });
    }
};

void AddSC_highmountain_river()
{
    RegisterCreatureAI(npc_highmountain_whitewater_carp);
}
