/*
    Monk Specialization: Brewmaster
*/

#include "ScriptMgr.h"
#include "InstanceScript.h"
#include "SmartAI.h"
#include "ConditionMgr.h"
#include "Conversation.h"
#include "Map.h"
#include "ObjectAccessor.h"
#include "QuestData.h"
#include "Scenario.h"
#include "ScenarioMgr.h"
#include "Spell.h"
#include "SpellScript.h"
#include "fuzan_temple_intro.h"

class instance_temple_of_the_jade_serpent : public InstanceMapScript
{
public:
    instance_temple_of_the_jade_serpent() : InstanceMapScript("instance_temple_of_the_jade_serpent", 1625) {}

    InstanceScript* GetInstanceScript(InstanceMap* map) const override
    {
        // Native tree 49567 unlocks Yu'lon's first line/path. Korithis's
        // TEXT_OVER 0 then owns DATA 2/2 and Yu'lon's reply. A raw 32-death
        // callback consumed that one-shot reply before the confrontation.
        return new InstanceScript(map);
    }
};

void AddSC_instance_temple_of_the_jade_serpent()
{
    new instance_temple_of_the_jade_serpent();
    RegisterCreatureAI(npc_fuzan_temple_intro);
    RegisterSpellScript(spell_fuzan_temple_intro);
    RegisterSpellScript(spell_fuzan_temple_complete);
}
