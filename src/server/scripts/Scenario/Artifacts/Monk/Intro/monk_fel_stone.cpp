#include "ScriptMgr.h"
#include "SmartAI.h"
#include "Player.h"
#include "Group.h"
#include "QuestData.h"
#include "Scenario.h"
#include "ScenarioMgr.h"
#include "Spell.h"
#include "SpellMgr.h"
#include "SpellScript.h"
#include "SpellAuras.h"
#include "Timer.h"
#include "monk_fel_stone.h"

void AddSC_monk_fel_stone()
{
    RegisterCreatureAI(npc_monk_fel_stone);
    RegisterSpellAndAuraScriptPair(spell_monk_fel_stone, aura_monk_fel_stone_scene);
    RegisterSpellScript(spell_monk_fel_stone_event);
    RegisterSpellAndAuraScriptPair(spell_monk_fel_stone_pilgrimage, aura_monk_fel_stone_pilgrimage);
    new player_monk_fel_stone();
}
