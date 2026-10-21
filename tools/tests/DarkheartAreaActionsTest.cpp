#include <cassert>
#include <cstdint>
#include <fstream>
#include <iostream>
#include <map>
#include <sstream>
#include <string>
constexpr int AT_TARGET_FLAG_VALIDATTACK=2,AT_TARGET_FLAG_PLAYER=8,AT_TARGET_FLAG_CAST_AT_SRC=32;
constexpr int AT_TARGET_FLAG_CASTER_IS_TARGET=64,AT_TARGET_FLAG_TARGET_IS_CASTER_2=0x0200000;
constexpr int EFFECT_0=0,TARGET_UNIT_CASTER=1;
constexpr int AT_ACTION_TYPE_CAST_SPELL=0,AT_ACTION_TYPE_REMOVE_AURA_BY_CASTER=13;
constexpr int TRIGGERED_FULL_MASK=1,TRIGGERED_CASTED_BY_AREATRIGGER=2;
using TriggerCastFlags=int;
using int32=int32_t;
constexpr int DUMMY_TRIGGER_CAST_IGNORE_GCD=11;
struct Unit
{
    bool player=true,attackable=true,totem=false;
    int guid=1,hits=0,lastSpell=0,lastCaster=0;
    std::map<int,int> auras;
    bool IsPlayer(){return player;}
    bool isTotem(){return totem;}
    bool IsValidAttackTarget(Unit* u){return u->attackable;}
    int GetGUID(){return guid;}
    void RemoveAura(int spell,int caster){if(auras[spell]==caster)auras.erase(spell);}
    void CastSpell(Unit* unit,int spell,int flags,void*)
    {assert(flags&TRIGGERED_CASTED_BY_AREATRIGGER);++unit->hits;unit->lastSpell=spell;unit->lastCaster=guid;unit->auras[spell]=guid;}
    void CastSpell(float,float,float,int,int,void*){assert(false&&"Unexpected position cast");}
    void CastSpell(Unit* unit,int spell,bool triggered,void*)
    {assert(triggered);++unit->hits;unit->lastSpell=spell;unit->lastCaster=guid;}
};
struct Item{bool IsInWorld(){return true;}};
struct Data{int moment=0,actionType=0,targetFlags=0,spellId=0;};
struct ActionInfo{Data* action;};
struct Target{int value=6;int GetTarget(){return value;}};
struct Effect{Target TargetA;};
struct SpellInfo{int EffectMask=1;Effect effect;Effect* Effects[1]={&effect};};
bool LaunchDummyMissile(Unit* triggerCaster,Unit* triggerTarget,int damage,int overrideSpell)
{
    struct Row{int spell_trigger;}itr{overrideSpell};
    bool check=false;void* m_CastItem=nullptr;
#include "DummySpellId.inc"
    switch(DUMMY_TRIGGER_CAST_IGNORE_GCD)
    {
#include "DummyMissile.inc"
    }
    return check;
}
struct AreaTrigger
{
    Unit* caster;bool _on_remove=false;Item* m_CastItem=nullptr;
    float GetPositionX(){return 0;}float GetPositionY(){return 0;}float GetPositionZ(){return 0;}
    void Dispatch(Unit* unit,ActionInfo& action)
    {
        Unit* original=caster;
        SpellInfo info;SpellInfo const* spellInfo=&info;
        if(action.action->spellId==220855)info.effect.TargetA.value=TARGET_UNIT_CASTER;
#include "AreaAttack.inc"
#include "AreaPlayer.inc"
#include "AreaSelfCaster.inc"
        bool CallScriptAreaTriggerCast=false;
        switch(action.action->actionType)
        {
#include "AreaCast.inc"
#include "AreaRemove.inc"
        }
        (void)CallScriptAreaTriggerCast;
        caster=original;
    }
};
int main(int argc,char**argv)
{
    assert(argc==2);std::ifstream input(argv[1]);assert(input.good());
    std::string line;int tested=0;
    while(std::getline(input,line))
    {
        std::istringstream row(line);std::string marker;int entry,custom,id;Data data;
        row>>marker>>entry>>custom>>id>>data.moment>>data.actionType>>data.targetFlags>>data.spellId;
        if(marker!="AT"||entry==5445)continue;
        Unit caster,player,pet,friendly,totem;caster.guid=123;pet.player=false;friendly.attackable=false;totem.totem=true;
        AreaTrigger at{&caster};ActionInfo action{&data};
        if(data.actionType==0)
        {
            assert(data.moment==1);
            at.Dispatch(&pet,action);at.Dispatch(&friendly,action);at.Dispatch(&totem,action);
            assert(!pet.hits&&!friendly.hits&&!totem.hits);
            at.Dispatch(&player,action);assert(player.hits==1&&player.lastSpell==data.spellId&&player.lastCaster==(data.spellId==220855?player.guid:123));
            at._on_remove=true;at.Dispatch(&player,action);assert(player.hits==1);
        }
        else
        {
            assert(data.actionType==13&&data.moment==42);
            player.auras[data.spellId]=456;at.Dispatch(&player,action);assert(player.auras[data.spellId]==456);
            player.auras[data.spellId]=(data.spellId==220855?player.guid:123);at.Dispatch(&player,action);assert(!player.auras.count(data.spellId));
        }
        ++tested;
    }
    assert(tested==14);
    Unit keeper,target;keeper.guid=987;
    assert(LaunchDummyMissile(&keeper,&target,220369,0));
    assert(target.hits==1&&target.lastSpell==220369&&target.lastCaster==987);
    assert(!LaunchDummyMissile(nullptr,&target,220369,0));
    assert(!LaunchDummyMissile(&keeper,nullptr,220369,0));
    std::cout<<"PASS: fourteen actual SQL area actions executed through extracted native attack/player admission, native self-caster selection and cast/owner-only removal dispatch; friendly units, pets and totems rejected.\n";
}
