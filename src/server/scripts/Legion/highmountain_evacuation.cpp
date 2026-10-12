#include "ScriptMgr.h"
#include "ScriptedCreature.h"
#include "Player.h"
#include "TemporarySummon.h"
#include "ObjectAccessor.h"
#include "CreatureTextMgr.h"
#include "MoveSplineInit.h"
#include "MoveSpline.h"
#include <map>

namespace SnowmaneEvacuation
{
constexpr uint32 QuestId = 42088;
// Reconstructed ground routes from the installed 7.3.5 terrain 1532 navmesh.
// PathGenerator uses map 1220, whose destroyed village has different heights.
// Use linear splines through the verified polygon crossings, without smoothing.
Movement::PointsArray const RedPath = {
    {3820.420f,4011.600f,898.530f},
    {3820.515f,4011.822f,898.530f},
    {3820.705f,4012.266f,898.530f},
    {3822.604f,4016.709f,898.530f},
    {3822.794f,4017.154f,898.562f},
    {3823.743f,4019.375f,899.428f},
    {3823.933f,4019.820f,899.547f},
    {3824.693f,4021.597f,898.612f},
    {3824.883f,4022.041f,898.530f},
    {3825.073f,4022.486f,898.530f},
    {3825.251f,4022.901f,898.530f},
    {3825.606f,4023.733f,898.530f},
    {3825.790f,4024.164f,898.530f},
    {3828.000f,4029.333f,898.530f},
    {3828.229f,4029.714f,898.530f},
    {3829.600f,4032.000f,898.530f},
    {3829.866f,4032.355f,898.530f},
    {3830.400f,4033.066f,898.530f},
    {3830.771f,4033.204f,898.008f},
    {3831.513f,4033.480f,896.964f},
    {3831.960f,4033.646f,896.827f},
    {3840.000f,4036.632f,894.195f},
    {3840.453f,4036.801f,893.902f},
    {3845.893f,4038.821f,890.386f},
    {3846.347f,4038.990f,890.148f},
    {3847.707f,4039.495f,888.921f},
    {3849.067f,4040.000f,887.910f}
};
Movement::PointsArray const TarvimPath = {
    {3799.070f,3919.260f,916.320f},
    {3800.848f,3920.947f,916.458f},
    {3801.204f,3921.284f,916.422f},
    {3801.915f,3921.959f,916.128f},
    {3802.271f,3922.297f,916.046f},
    {3805.471f,3925.333f,916.130f},
    {3805.697f,3925.548f,916.130f},
    {3805.923f,3925.762f,916.130f},
    {3806.269f,3926.091f,916.130f},
    {3808.000f,3927.733f,916.130f},
    {3808.480f,3927.707f,917.062f},
    {3808.960f,3927.680f,917.508f},
    {3812.800f,3927.467f,915.597f},
    {3813.240f,3927.432f,915.463f},
    {3815.441f,3927.259f,914.793f},
    {3815.902f,3927.223f,914.549f},
    {3816.823f,3927.151f,914.055f},
    {3818.667f,3927.006f,913.254f},
    {3819.163f,3926.968f,913.078f},
    {3819.659f,3926.928f,912.901f},
    {3829.085f,3926.189f,908.221f},
    {3829.581f,3926.150f,908.056f},
    {3840.000f,3925.333f,905.997f},
    {3840.464f,3925.156f,905.949f},
    {3861.333f,3917.156f,903.806f},
    {3861.796f,3916.978f,903.591f},
    {3868.270f,3914.496f,900.588f},
    {3879.832f,3910.064f,896.397f},
    {3880.000f,3910.000f,896.397f}
};
struct Resident { uint32 entry, menu, spell; Movement::PointsArray const* path; };
Resident const Residents[] = {
    {106115,19560,210246,&RedPath}, {106153,19561,210304,&TarvimPath},
    {106191,19565,0,nullptr} // Nefu refuses to leave; his reply is the objective.
};
bool Active(Player* player)
{
    return player && player->IsInWorld() && player->IsAlive() && player->GetMapId() == 1220 &&
        player->GetQuestStatus(QuestId) == QUEST_STATUS_INCOMPLETE && !player->IsBeingTeleported() &&
        !player->GetVehicle() && !player->GetTransport() && !player->isInFlight() && !player->IsFlying();
}
Player* Owner(Creature* npc)
{
    auto summon = npc->ToTempSummon();
    Unit* owner = summon ? summon->GetSummoner() : nullptr;
    return owner ? owner->ToPlayer() : nullptr;
}
}

struct npc_highmountain_evacuation : ScriptedAI
{
    npc_highmountain_evacuation(Creature* npc) : ScriptedAI(npc)
    {
        for (auto const& row : SnowmaneEvacuation::Residents)
            if (row.entry == me->GetEntry()) resident = &row;
    }
    SnowmaneEvacuation::Resident const* resident = nullptr;
    struct Visit { ObjectGuid actor; std::weak_ptr<uint8> lifetime; };
    std::map<ObjectGuid,Visit> visitors;
    std::weak_ptr<uint8> lifetime;
    uint32 timer = 2000, elapsed = 0;
    bool departed = false;
    void AttackStart(Unit*) override { }
    void MoveInLineOfSight(Unit*) override { }

    void sGossipSelect(Player* player, uint32 menu, uint32 option) override
    {
        using namespace SnowmaneEvacuation;
        if (!resident || me->ToTempSummon() || menu != resident->menu || option != 0 ||
            !Active(player) || player->GetQuestObjectiveData(QuestId,resident->entry) ||
            visitors.count(player->GetGUID()) || !me->IsAlive() || me->IsDespawn() ||
            !player->InSamePhase(me) || !player->canSeeOrDetect(me) ||
            !player->IsWithinDistInMap(me,6.0f) || !player->IsWithinLOSInMap(me)) return;

        if (!resident->path)
        {
            player->PlayerTalkClass->SendCloseGossip();
            sCreatureTextMgr->SendChat(me,0,player->GetGUID(),CHAT_MSG_MONSTER_WHISPER);
            player->KilledMonsterCredit(resident->entry,me->GetGUID());
            return;
        }
        auto const& start = resident->path->front();
        // Do not run a relocated/custom spawn through the native village route.
        if (me->GetDistance(start.x,start.y,start.z) > 2.0f) return;
        player->AddQuestDelayedEvent(QuestId,0,[] {});
        if (Creature* actor = player->SummonCreature(resident->entry,me->GetPosition(),
            TEMPSUMMON_TIMED_DESPAWN,45000,0,player->GetGUID()))
        {
            if (!actor->IsAIEnabled || actor->IsDespawn()) { actor->DespawnOrUnsummon(); return; }
            visitors.emplace(player->GetGUID(),Visit{actor->GetGUID(),player->getQuestStatus(QuestId)->ScriptLifetime});
            me->AddToHideList(player->GetGUID());
            player->UpdateVisibilityOf(me);
            player->PlayerTalkClass->SendCloseGossip();
        }
    }
    void IsSummonedBy(Unit* summoner) override
    {
        using namespace SnowmaneEvacuation;
        Player* player = summoner->ToPlayer();
        if (!resident || !resident->path || !Active(player)) { me->DespawnOrUnsummon(); return; }
        player->AddQuestDelayedEvent(QuestId,0,[] {});
        lifetime = player->getQuestStatus(QuestId)->ScriptLifetime;
        me->AddPlayerInPersonnalVisibilityList(player->GetGUID());
        me->RemoveFlag(UNIT_FIELD_NPC_FLAGS,UNIT_NPC_FLAG_GOSSIP);
        me->SetFlag(UNIT_FIELD_FLAGS,UNIT_FLAG_NON_ATTACKABLE | UNIT_FLAG_IMMUNE_TO_PC | UNIT_FLAG_IMMUNE_TO_NPC);
        me->SetReactState(REACT_PASSIVE);
        sCreatureTextMgr->SendChat(me,0,player->GetGUID(),CHAT_MSG_MONSTER_WHISPER);
    }
    void UpdateAI(uint32 diff) override
    {
        using namespace SnowmaneEvacuation;
        if (!resident) return;
        if (!me->ToTempSummon())
        {
            if (timer > diff) { timer -= diff; return; }
            timer = 1000;
            for (auto it = visitors.begin(); it != visitors.end();)
            {
                Player* player = ObjectAccessor::GetPlayer(*me,it->first);
                Creature* actor = ObjectAccessor::GetCreature(*me,it->second.actor);
                bool restore = it->second.lifetime.expired() || !player || !player->IsInWorld() ||
                    !player->IsAlive() || !player->InSamePhase(me) || !player->IsWithinDistInMap(me,200.0f) ||
                    player->GetQuestStatus(QuestId) == QUEST_STATUS_REWARDED ||
                    ((!actor || actor->IsDespawn()) && !player->GetQuestObjectiveData(QuestId,resident->entry));
                if (!restore) { ++it; continue; }
                if (actor) actor->DespawnOrUnsummon();
                me->RemoveFromHideList(it->first);
                if (player && player->IsInWorld()) player->UpdateVisibilityOf(me);
                it = visitors.erase(it);
            }
            return;
        }
        Player* player = Owner(me);
        elapsed += diff;
        // The third credit changes INCOMPLETE to COMPLETE. Let its departure finish.
        if (lifetime.expired() || !player || !player->IsInWorld() || !player->IsAlive() ||
            !player->InSamePhase(me) || !player->IsWithinDistInMap(me,200.0f) ||
            player->IsBeingTeleported() || elapsed >= 44000 ||
            (player->GetQuestStatus(QuestId) != QUEST_STATUS_INCOMPLETE && player->GetQuestStatus(QuestId) != QUEST_STATUS_COMPLETE))
        { me->DespawnOrUnsummon(); return; }
        if (departed)
        {
            if (me->movespline->Finalized()) me->DespawnOrUnsummon();
            return;
        }
        if (timer > diff) { timer -= diff; return; }
        if (!Active(player) || player->GetQuestObjectiveData(QuestId,resident->entry))
        { me->DespawnOrUnsummon(); return; }
        me->CastSpell(me,resident->spell,true);
        if (!me->HasAura(resident->spell)) { me->DespawnOrUnsummon(); return; }
        me->SetWalk(false);
        Movement::MoveSplineInit init(*me);
        init.MovebyPath(*resident->path);
        init.SetWalk(false);
        if (init.Launch() <= 0) { me->DespawnOrUnsummon(); return; }
        departed = true;
        player->KilledMonsterCredit(resident->entry,me->GetGUID());
    }
};

void AddSC_highmountain_evacuation()
{
    RegisterCreatureAI(npc_highmountain_evacuation);
}
