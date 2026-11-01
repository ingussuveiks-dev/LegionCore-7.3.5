#pragma once

namespace FuZanTempleDoor
{
bool Opened(Scenario* scenario)
{
    if (!scenario) return false;
    auto progress = scenario->GetAchievementMgr().GetCriteriaProgressMap();
    auto found = progress->find(49141);
    return found != progress->end() && found->second.Counter;
}
}

struct go_fuzan_temple_sanctuary_door : SmartGameObjectAI
{
    explicit go_fuzan_temple_sanctuary_door(GameObject* object) : SmartGameObjectAI(object) { }

    bool Scoped() const
    {
        return go->GetEntry() == 251591 && go->GetDBTableGUIDLow() == 138378 && go->GetMapId() == 1625;
    }

    Scenario* World() const
    {
        if (!go->IsInWorld() || go->IsPreDelete()) return nullptr;
        Scenario* scenario = sScenarioMgr->GetScenario(go->GetInstanceId());
        return scenario && scenario->GetScenarioId() == 1137 ? scenario : nullptr;
    }

    bool Contact(Player* player) const
    {
        return !_using && !_restoring && World() &&
            go->getLootState() == GO_READY &&
            go->isSpawned() && !go->HasFlag(GAMEOBJECT_FIELD_FLAGS, GO_FLAG_IN_USE | GO_FLAG_NOT_SELECTABLE) &&
            player && player->IsInWorld() && player->CanContact() && player->IsAlive() &&
            !player->isGameMaster() && !player->IsBeingTeleported() && player->getClass() == CLASS_MONK &&
            player->GetMap() == go->GetMap() && player->InSamePhase(go) &&
            player->IsWithinDistInMap(go, go->GetInteractionDistance()) &&
            !player->GetQuestRewardStatus(42765) &&
            (player->GetQuestStatus(42765) == QUEST_STATUS_INCOMPLETE || player->GetQuestStatus(42765) == QUEST_STATUS_COMPLETE);
    }

    bool Eligible(Player* player) const
    {
        Scenario* scenario = World();
        return scenario && scenario->GetCurrentStep() == 1 && !FuZanTempleDoor::Opened(scenario) && Contact(player);
    }

    bool GossipUse(Player* player) override
    {
        if (!Scoped()) return SmartGameObjectAI::GossipUse(player);
        // Returning true also stops GameObject::Use's native door toggle.
        // A source22 rejection alone would still latch this no-close door.
        // Recreated doors may reopen from the actual persisted receipt,
        // without replaying the one-shot SmartAI credit or changing stages.
        if (Contact(player) && FuZanTempleDoor::Opened(World())) return false;
        if (!Eligible(player)) return true;
        return SmartGameObjectAI::GossipUse(player);
    }

    bool GossipHello(Player* player, bool isUse) override
    {
        if (!Scoped()) return SmartGameObjectAI::GossipHello(player, isUse);
        if (!Eligible(player)) return true;
        _using = true;
        SmartGameObjectAI::GossipHello(player, isUse);
        _using = false;
        if (FuZanTempleDoor::Opened(World())) return false;
        // The pinned graph contains only this one native credit action.
        // Keep the original usable state and permit an actual later retry;
        // never manufacture the missing criterion or hide its diagnostic.
        SmartGameObjectAI::Reset();
        TC_LOG_ERROR("scripts.ai", "Fu Zan: native sanctuary door event52452 did not acknowledge49141; original object remains available for retry");
        return true;
    }

    void OnStateChanged(uint32 state, Unit* unit) override
    {
        // Native GameObject::Use admits non-Players without GossipUse. Such
        // an activation cannot replace the actual eligible player event.
        // Roll back this rejected activation through the native door reset,
        // then rearm before generic GO_JUST_DEACTIVATED can despawn a static
        // door or save a respawn. No criterion or other flag is changed.
        if (Scoped() && !_restoring && state == GO_ACTIVATED && go->getLootState() == GO_ACTIVATED &&
            !FuZanTempleDoor::Opened(World()))
        {
            _restoring = true;
            go->ResetDoorOrButton();
            go->SetLootState(GO_READY);
            _restoring = false;
            return;
        }
        SmartGameObjectAI::OnStateChanged(state, unit);
    }

private:
    bool _using = false;
    bool _restoring = false;
};
