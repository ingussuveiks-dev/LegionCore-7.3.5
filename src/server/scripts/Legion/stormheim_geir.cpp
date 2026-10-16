#include "ScriptMgr.h"
#include "SmartAI.h"
#include "Player.h"
#include "ObjectMgr.h"

// Keep combat, loot and ordinary kill/group credit in their native handlers.
// Action206 accepts only Player targets, whereas Unit::Kill passes the actual
// killing pet/guardian to JustDied before the delayed50ms quest credit runs.
struct npc_stormheim_geir : SmartAI
{
    explicit npc_stormheim_geir(Creature* c) : SmartAI(c) { }
    void JustDied(Unit* killer) override
    {
        Position const point = {209.854f,587.158f,-4.0533f,1.39828f};
        if (killer && !killer->ToPlayer() && me->GetEntry() == 91948 && me->GetDBTableGUIDLow() == 11546719 &&
            me->GetMapId() == 1463 && !me->GetInstanceId() && me->GetZoneId() == 7588 && me->GetHomePosition().GetExactDist(&point) < 1 &&
            sObjectMgr->GetScriptName(me->GetCreatureTemplate()->ScriptID) == "npc_stormheim_geir")
            if (auto owner = killer->GetCharmerOrOwnerPlayerOrPlayerItself())
            {
                auto status = owner->GetQuestStatus(39853);
                if (owner->IsInWorld() && owner->GetMapId() == 1463 && !owner->GetInstanceId() &&
                    owner->GetZoneId() == 7588 && !owner->HasPhaseId(8306) && owner->InSamePhase(me) &&
                    owner->IsAtGroupRewardDistance(me) && (status == QUEST_STATUS_INCOMPLETE || status == QUEST_STATUS_COMPLETE))
                {
                    SmartAI::JustDied(owner);
                    return;
                }
            }
        SmartAI::JustDied(killer);
    }
};

void AddSC_stormheim_geir()
{
    RegisterCreatureAI(npc_stormheim_geir);
}
