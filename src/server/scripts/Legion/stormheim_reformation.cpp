#include "ScriptMgr.h"
#include "SmartAI.h"
#include "Player.h"

// Native 192069 supplies the objective credit and a short visual helper.
// Ashildir 93234 is the persistent phase-6776 questgiver. Do not summon a
// second Ashildir or award another credit from this optional effect.
struct npc_stormheim_reformation_bones : SmartAI
{
    explicit npc_stormheim_reformation_bones(Creature* creature) : SmartAI(creature) { }
    bool started = false;

    void IsSummonedBy(Unit* summoner) override
    {
        SmartAI::IsSummonedBy(summoner);
        Player* player = summoner ? summoner->ToPlayer() : nullptr;
        if (started || !player || !player->IsInWorld() || !player->IsAlive() ||
            player->GetMapId() != 1220 || player->GetInstanceId() || player->GetZoneId() != 7541 ||
            me->GetEntry() != 97281 || !me->isSummon() ||
            me->GetUInt32Value(UNIT_FIELD_CREATED_BY_SPELL) != 192069 ||
            me->GetExactDist(3532.88f, 887.547f, 53.4053f) > 1.0f)
            return;

        QuestStatus status = player->GetQuestStatus(38815);
        if (status != QUEST_STATUS_INCOMPLETE && status != QUEST_STATUS_COMPLETE)
            return;

        started = true;
        player->BindQuestObject(38815, me);
        me->SetReactState(REACT_PASSIVE);
        // Native 192062 has a 2-second cast and 2-second aura/visual 49745.
        // Its optional failure must not gate the rune's native spell credit.
        me->CastSpell(me, 192062, false);
    }
};

void AddSC_stormheim_reformation()
{
    RegisterCreatureAI(npc_stormheim_reformation_bones);
}
