////////////////////////////////////////////////////////////////////////////////
//
//  MILLENIUM-STUDIO
//  Copyright 2016 Millenium-studio SARL
//  All Rights Reserved.
//
////////////////////////////////////////////////////////////////////////////////

#include "ScriptMgr.h"
#include "PetBattle.h"
#include "Common.h"
#include "PetBattleSystem.h"

class PlayerScriptPetBattle : public PlayerScript
{
public:
    PlayerScriptPetBattle() : PlayerScript("PlayerScriptPetBattle") { }

    void OnMovementInform(Player* player, uint32 mveType, uint32 id) override
    {
        if (player && mveType == POINT_MOTION_TYPE && id == PETBATTLE_ENTER_MOVE_SPLINE_ID &&
            IsPendingParticipant(player, sPetBattleSystem->GetBattle(player->_petBattleId)))
        {
            std::lock_guard<std::mutex> lock(m_Mutex);
            auto pending = m_DelayedPetBattleStart.find(player->GetGUID());
            // Duplicate movement completion must not postpone this start.
            if (pending == m_DelayedPetBattleStart.end() || pending->second.BattleID != player->_petBattleId)
                m_DelayedPetBattleStart[player->GetGUID()] = {player->_petBattleId, getMSTime()};
        }
    }

    void OnUpdate(Player* player, uint32 /*diff*/) override
    {
        if (!player)
            return;

        ObjectGuid battleID;
        {
            std::lock_guard<std::mutex> lock(m_Mutex);
            auto pending = m_DelayedPetBattleStart.find(player->GetGUID());
            if (pending == m_DelayedPetBattleStart.end() || uint32(getMSTime() - pending->second.StartedAt) < 1000)
                return;
            battleID = pending->second.BattleID;
            m_DelayedPetBattleStart.erase(pending);
        }

        // Consume once before callbacks, and never start a replacement battle
        // from a previous arrival or reset an already running battle.
        PetBattle* battle = sPetBattleSystem->GetBattle(battleID);
        if (player->_petBattleId == battleID && IsPendingParticipant(player, battle))
        {
            player->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_PACIFIED | UNIT_FLAG_IMMUNE_TO_NPC);
            player->SetControlled(true, UNIT_STATE_ROOT);
            battle->Begin();
        }
    }

    void OnBeforeLogout(Player* player) override
    {
        if (player)
        {
            std::lock_guard<std::mutex> lock(m_Mutex);
            m_DelayedPetBattleStart.erase(player->GetGUID());
        }
    }

    static bool IsPendingParticipant(Player* player, PetBattle* battle)
    {
        if (!battle || battle->BattleStatus != PETBATTLE_STATUS_CREATION || !player->IsInWorld() || player->IsDelete() || player->isDead())
            return false;
        for (auto team : battle->Teams)
            if (team && team->PlayerGuid == player->GetGUID() && team->OwnerGuid == player->GetGUID())
                return true;
        return false;
    }

    struct PendingStart
    {
        ObjectGuid BattleID;
        uint32 StartedAt;
    };
    std::map<ObjectGuid, PendingStart> m_DelayedPetBattleStart;
    std::mutex m_Mutex;
};

void AddSC_PetBattlePlayerScript()
{
    new PlayerScriptPetBattle;
}
