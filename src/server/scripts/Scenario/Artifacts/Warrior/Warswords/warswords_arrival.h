#pragma once

#include "MoveSpline.h"
#include "QuestObjectScope.h"
#include "Spell.h"
#include "SpellMgr.h"
#include <map>
#include <mutex>
#include <cmath>

namespace WarswordsArrivalRecovery
{
    uint32 const Ready = 0x574152;
    bool Context(Player* player)
    {
        if (!player || !player->GetMap() || player->GetMapId() != 1511 || player->GetMap()->IsMapUnload() ||
            !player->GetInstanceScript() || player->getClass() != CLASS_WARRIOR || player->GetQuestRewardStatus(40043) ||
            (player->GetQuestStatus(40043) != QUEST_STATUS_INCOMPLETE && player->GetQuestStatus(40043) != QUEST_STATUS_COMPLETE))
            return false;
        Scenario* scenario = sScenarioMgr->GetScenario(player->GetInstanceId());
        return scenario && scenario->GetScenarioId() == 944;
    }

    bool AirEntrance(Player* player)
    {
        // Native193335 entrance and the authored descent corridor. Grounded
        // visitors/re-entry never get pulled back to the landing point.
        float x=player->GetPositionX(), y=player->GetPositionY(), z=player->GetPositionZ();
        if (x < 3310 || x > 3427 || y < 1316 || y > 1447 || z < 74 || z > 199 ||
            player->GetTransport() || player->GetVehicle() || player->CanFly() || player->IsLevitating())
            return false;
        float floor=player->GetMap()->GetHeight(player->GetPhases(), x,y,z+2,true,200);
        return !std::isfinite(floor) || floor <= INVALID_HEIGHT || std::fabs(z-floor)>3;
    }

    struct State
    {
        uint32 instance, delay=3000, finish=5000, retry=0, spline=0, acknowledgement=0;
        bool started=false, held=false, warned=false, warnedFog=false, warnedStart=false, finishing=false;
        std::shared_ptr<QuestCancellationReceipt const> attempt;
        explicit State(Player* player):instance(player->GetInstanceId()),attempt(player->GetQuestCancellationReceipt(40043)) {}
        bool Valid(Player* player) const
        {
            return Context(player) && player->GetInstanceId()==instance && attempt && !attempt->load() &&
                attempt==player->GetQuestCancellationReceipt(40043);
        }
        void Hold(Player* player)
        {
            if (!held && player->IsAlive() && AirEntrance(player))
            {
                // Before first self-create on login, use the native bounded
                // root/gravity/load-ACK lifecycle; no immunity or teleport.
                player->BeginMapObjectArrival(player->IsInWorld());
                held=true;
            }
        }
        bool Tick(Player* player,uint32 diff)
        {
            if (!Valid(player)) return false;
            delay-=std::min(delay,diff);
            if (!player->IsAlive()) { started=false; finishing=false; held=false; finish=5000; return true; }
            Scenario* scenario=sScenarioMgr->GetScenario(instance);
            if (player->IsInWorld() && player->CanContact() && !player->IsBeingTeleported())
                player->GetInstanceScript()->SetData(Ready,1);
            if (!scenario || scenario->GetCurrentStep()!=0) return false;
            if (player->IsBeingTeleported()) return true;
            Hold(player);
            if (delay || !player->IsInWorld() || !player->CanContact() || player->IsBeingTeleported() ||
                player->IsMapObjectArrivalProtected() || player->HasUnitState(UNIT_STATE_ROOT) || player->GetVehicle() || player->GetTransport())
                return true;
            if (finishing)
            {
                auto destination=player->movespline->FinalDestination();
                bool arrivedOnOriginal=player->movespline->GetId()==spline && player->movespline->Finalized() &&
                    std::fabs(player->GetPositionX()-3386.46f)<1 && std::fabs(player->GetPositionY()-1406.94f)<1 && std::fabs(player->GetPositionZ()-68.94f)<1;
                if ((player->movespline->GetId()!=spline || arrivedOnOriginal) && !player->HasAura(189695) &&
                    std::fabs(destination.x-3386.46f)<0.1f && std::fabs(destination.y-1406.94f)<0.1f && std::fabs(destination.z-68.94f)<0.1f)
                { player->CreateConversation(838); return false; }
                acknowledgement-=std::min(acknowledgement,diff);
                if (acknowledgement) return true;
                if (!warned) { TC_LOG_WARN("scripts.ai","Warswords arrival: accepted finish has no native movement/aura acknowledgement; retaining retry"); warned=true; }
                if (player->movespline->GetId()!=spline &&
                    (std::fabs(destination.x-3386.46f)>=0.1f || std::fabs(destination.y-1406.94f)>=0.1f || std::fabs(destination.z-68.94f)>=0.1f)) return false;
                spline=player->movespline->GetId(); finishing=false;
                return true;
            }
            if (retry>diff) { retry-=diff; return true; }
            retry=1000;
            if (!started)
            {
                if (!AirEntrance(player)) return false;
                for (uint32 spell : {205534u,189695u})
                    if (!player->HasAura(spell))
                    {
                        SpellCastResult result=player->CastSpell(player,spell,true);
                        bool& reported=spell==205534 ? warnedFog : warnedStart;
                        if ((result!=SPELL_CAST_OK || !player->HasAura(spell)) && !reported)
                        { TC_LOG_WARN("scripts.ai","Warswords arrival: native presentation %u not acknowledged (result %u); descent remains independent",spell,uint32(result)); reported=true; }
                    }
                uint32 previous=player->movespline->GetId();
                player->GetMotionMaster()->MoveJump(3386.46f,1406.94f,68.94f,player->GetOrientation(),3.0f,3.0f);
                auto destination=player->movespline->FinalDestination();
                started=player->movespline->GetId()!=previous && !player->movespline->Finalized() &&
                    std::fabs(destination.x-3386.46f)<0.1f && std::fabs(destination.y-1406.94f)<0.1f && std::fabs(destination.z-68.94f)<0.1f;
                if (started) { spline=player->movespline->GetId(); finish=5000; retry=0; warned=false; }
                else if (!warned) { TC_LOG_WARN("scripts.ai","Warswords arrival: native descent was not accepted; retaining this entry for retry"); warned=true; }
                return true;
            }
            if (player->movespline->GetId()!=spline) return false; // Another real movement owns the player now.
            finish-=std::min(finish,diff);
            if (finish) { retry=0; return true; }
            uint32 previous=player->movespline->GetId();
            SpellCastTargets targets;
            targets.SetCaster(player);
            targets.SetUnitTarget(player); // Native203 removals target this Player; keep that recipient with the destination.
            targets.SetDst(3386.46f,1406.94f,68.94f,player->GetOrientation());
            SpellCastResult result=player->CastSpell(targets,sSpellMgr->GetSpellInfo(205744),nullptr,
                TriggerCastFlags(TRIGGERED_FULL_MASK & ~TRIGGERED_DONT_REPORT_CAST_ERROR));
            if (result==SPELL_CAST_OK)
            {
                finishing=true; acknowledgement=2000;
                // Native spell events can execute after prepare returned OK.
                // Observe their effects on a subsequent update, never grant an ACK.
                return true;
            }
            // A failed finish may have consumed its native aura/started its
            // spline. Track that actual motion for the next bounded retry.
            spline=player->movespline->GetId();
            if (!warned) { TC_LOG_WARN("scripts.ai","Warswords arrival: native finish not acknowledged (result %u, spline %u->%u, start aura %u); retrying without conversation or credit",uint32(result),previous,spline,uint32(player->HasAura(189695))); warned=true; }
            return true;
        }
    };

    std::mutex mutex;
    std::map<ObjectGuid,std::shared_ptr<State>> entries;
    void Forget(Player* player)
    {
        if (!player) return;
        std::lock_guard<std::mutex> guard(mutex);
        entries.erase(player->GetGUID());
    }
    void Enter(Player* player)
    {
        if (!Context(player)) return;
        auto state=std::make_shared<State>(player);
        if (sScenarioMgr->GetScenario(player->GetInstanceId())->GetCurrentStep()==0) state->Hold(player);
        std::lock_guard<std::mutex> guard(mutex);
        entries[player->GetGUID()]=std::move(state);
    }
}

class player_warswords_arrival_recovery : public PlayerScript
{
public:
    player_warswords_arrival_recovery():PlayerScript("player_warswords_arrival_recovery") {}
    void OnBeforeLogout(Player* player) override { WarswordsArrivalRecovery::Forget(player); }
    void OnLogout(Player* player) override { WarswordsArrivalRecovery::Forget(player); }
    void OnMapChanged(Player* player) override
    {
        // This notification follows destination OnPlayerEnter. Keep its new
        // entry; only retire an old context if no destination entry replaced it.
        std::lock_guard<std::mutex> guard(WarswordsArrivalRecovery::mutex);
        auto found=WarswordsArrivalRecovery::entries.find(player->GetGUID());
        if(found!=WarswordsArrivalRecovery::entries.end() && !found->second->Valid(player))
            WarswordsArrivalRecovery::entries.erase(found);
    }
    void OnUpdate(Player* player,uint32 diff) override
    {
        std::shared_ptr<WarswordsArrivalRecovery::State> state;
        { std::lock_guard<std::mutex> guard(WarswordsArrivalRecovery::mutex); auto found=WarswordsArrivalRecovery::entries.find(player->GetGUID()); if(found!=WarswordsArrivalRecovery::entries.end()) state=found->second; }
        if (state && !state->Tick(player,diff))
        {
            std::lock_guard<std::mutex> guard(WarswordsArrivalRecovery::mutex);
            auto found=WarswordsArrivalRecovery::entries.find(player->GetGUID());
            if(found!=WarswordsArrivalRecovery::entries.end() && found->second==state) WarswordsArrivalRecovery::entries.erase(found);
        }
    }
};
