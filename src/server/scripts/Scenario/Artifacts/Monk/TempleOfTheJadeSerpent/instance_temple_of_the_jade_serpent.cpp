/*
    Monk Specialization: Brewmaster
*/

#include "ScriptMgr.h"
#include "InstanceScript.h"
#include "SmartAI.h"
#include "ConditionMgr.h"
#include "Conversation.h"
#include "GameObject.h"
#include "TemporarySummon.h"
#include "Map.h"
#include "ObjectAccessor.h"
#include "QuestData.h"
#include "Scenario.h"
#include "ScenarioMgr.h"
#include "Spell.h"
#include "SpellScript.h"
#include "fuzan_temple_intro.h"
#include "fuzan_temple_sanctuary_door.h"
#include "fuzan_belphiar.h"
#include "fuzan_summerpetal.h"
#include "fuzan_wave_entry.h"
#include "fuzan_brewing.h"

class instance_temple_of_the_jade_serpent : public InstanceMapScript
{
public:
    instance_temple_of_the_jade_serpent() : InstanceMapScript("instance_temple_of_the_jade_serpent", 1625) {}

    InstanceScript* GetInstanceScript(InstanceMap* map) const override
    {
        // Native tree 49567 unlocks Yu'lon's first line/path. Korithis's
        // TEXT_OVER 0 then owns DATA 2/2 and Yu'lon's reply. A raw 32-death
        // callback consumed that one-shot reply before the confrontation.
        return new FuZanWaveEntry::Instance(map);
    }
};

void AddSC_instance_temple_of_the_jade_serpent()
{
    new instance_temple_of_the_jade_serpent();
    RegisterCreatureAI(npc_fuzan_temple_intro);
    RegisterSpellScript(spell_fuzan_temple_intro);
    RegisterSpellScript(spell_fuzan_temple_complete);
    RegisterGameObjectAI(go_fuzan_temple_sanctuary_door);
    RegisterCreatureAI(npc_fuzan_belphiar);
    RegisterCreatureAI(npc_fuzan_summerpetal);
    new condition_fuzan_summerpetal_continuation();
    new eventobject_fuzan_wave_entry();
    RegisterSpellScript(spell_fuzan_wave_attempt);
    RegisterAuraScript(aura_fuzan_wave_controller);
    RegisterSpellScript(spell_fuzan_wave_conversation);
    RegisterSpellScript(spell_fuzan_wave_retrieval);
    RegisterCreatureAI(npc_fuzan_brewing);
    RegisterCreatureAI(npc_fuzan_brewing_questgiver);
    RegisterGameObjectAI(go_fuzan_brewing_fire);
    RegisterGameObjectAI(go_fuzan_brewing_ingredient);
    RegisterSpellScript(spell_fuzan_brewing_summon);
    RegisterSpellScript(spell_fuzan_brewing_ingredient);
    RegisterSpellScript(spell_fuzan_brewing_delivery);
    RegisterSpellScript(spell_fuzan_brewing_receipt);
    RegisterAuraScript(aura_fuzan_brewing_controller);
    new map_fuzan_brewing();
    new player_fuzan_brewing();
}
