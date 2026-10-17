#include <cassert>
#include <cstdint>
#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <crtdbg.h>
using uint32=uint32_t;using int32=int32_t;using SpellEffIndex=int;
#define TC_LOG_DEBUG(...) ((void)0)
#define TC_LOG_ERROR(...) ((void)0)
enum {SPELL_EFFECT_HANDLE_HIT_TARGET=1,SPELL_EFFECT_HANDLE_HIT=2,TARGET_FLAG_UNIT_MASK=1,TARGET_FLAG_GAMEOBJECT_MASK=2,
 CRITERIA_TYPE_SCRIPT_EVENT=1,CRITERIA_TYPE_SCRIPT_EVENT_2=2,SCRIPT_COMMAND_TEMP_SUMMON_CREATURE=10,
 DEAD=2,CORPSE=1,TEMPSUMMON_MANUAL_DESPAWN=0,TEMPSUMMON_TIMED_DESPAWN=1,TEMPSUMMON_TIMED_DESPAWN_OUT_OF_COMBAT=2,
 TEMPSUMMON_CORPSE_TIMED_DESPAWN=3,TEMPSUMMON_CORPSE_DESPAWN=4,TEMPSUMMON_DEAD_DESPAWN=5,TEMPSUMMON_TIMED_OR_CORPSE_DESPAWN=6,TEMPSUMMON_TIMED_OR_DEAD_DESPAWN=7};
struct Player;struct Map;struct TempSummon;
struct WorldObject {
 virtual ~WorldObject()=default;virtual Player* ToPlayer(){return nullptr;}Map* map=nullptr;Map* GetMap(){return map;}
 TempSummon* SummonCreature(uint32,float,float,float,float,int,uint32);
 uint32 GetGUID(){return 1;}
};
struct ZoneScript {void ProcessEvent(WorldObject*,uint32){}};using InstanceScript=ZoneScript;
struct Unit:WorldObject {ZoneScript* GetZoneScript(){return nullptr;}InstanceScript* GetInstanceScript(){return nullptr;}};
struct Player:Unit {Player* ToPlayer()override{return this;}uint32 credits=0;void UpdateAchievementCriteria(int,uint32 id,int){assert(id==46479);++credits;}};
struct Creature:Unit {void Update(uint32){}};
struct TempSummon:Creature {
 uint32 m_type=TEMPSUMMON_TIMED_OR_DEAD_DESPAWN,m_timer=30000,m_lifetime=30000;int32 m_timerCheckLocation=3000;
 uint32 m_deathState=0;bool combat=false,expired=false,removed=false;uint32 locations=0;
 bool IsQuestObjectExpired(){return expired;}void UnSummon(){removed=true;}bool isInCombat(){return combat;}bool IsAlive(){return !m_deathState;}uint32 GetEntry(){return 97124;}void CheckLocation(){++locations;}
 void AddPlayerInPersonnalVisibilityList(uint32){}template<class F>void AddDelayedEvent(int,F f){f();}void SetVisible(bool){}
 void Update(uint32);
};
#include "SummonUpdate.inc"
struct ScriptInfo {
 struct {uint32 CreatureEntry=97124,DespawnDelay=30000;float PosX=3907.56f,PosY=2230.56f,PosZ=324.377f,Orientation=4.5006f;bool PersonalVisible=false;} TempSummonCreature;
 std::string GetDebugInfo(){return "wager";}
} info;
int sEventScripts=1;bool failSummon=false;TempSummon bird;uint32 summons=0;WorldObject* summoner=nullptr;
WorldObject* _GetScriptWorldObject(WorldObject* source,bool,ScriptInfo*){return source;}
struct Map {
 uint32 starts=0;WorldObject* lastTarget=nullptr;void ScriptsStart(int scripts,uint32 id,WorldObject* source,WorldObject* target){assert(scripts==sEventScripts&&id==46479);++starts;lastTarget=target;struct {ScriptInfo* script;} step{&info};
  switch(SCRIPT_COMMAND_TEMP_SUMMON_CREATURE){
   #include "EventSummon.inc"
  }
 }
};
TempSummon* WorldObject::SummonCreature(uint32 id,float x,float y,float z,float o,int type,uint32 duration){
 assert(id==97124&&x==3907.56f&&y==2230.56f&&z==324.377f&&o==4.5006f&&type==TEMPSUMMON_TIMED_OR_DEAD_DESPAWN&&duration==30000);
 if(failSummon)return nullptr;++summons;summoner=this;bird=TempSummon{};return &bird;
}
struct Effect {uint32 MiscValue=46479,provided=0;uint32 GetProvidedTargetMask(){return provided;}};
struct SpellInfo {uint32 Id=192045;Effect effect;Effect* GetEffect(int,int){return &effect;}};
struct Spell {Unit* m_caster;WorldObject* unitTarget=nullptr;WorldObject* gameObjTarget=nullptr;WorldObject* focusObject=nullptr;SpellInfo info;SpellInfo* m_spellInfo=&info;int m_diffMode=0,effectHandleMode=SPELL_EFFECT_HANDLE_HIT;void EffectSendEvent(SpellEffIndex);};
#include "SendEvent.inc"
int main(int argc,char** argv){_set_abort_behavior(0,_WRITE_ABORT_MSG|_CALL_REPORTFAULT);_CrtSetReportMode(_CRT_ASSERT,_CRTDBG_MODE_FILE);_CrtSetReportFile(_CRT_ASSERT,_CRTDBG_FILE_STDERR);assert(argc==2);std::ifstream input(argv[1]);std::string line;unsigned events=0;while(std::getline(input,line)){std::istringstream in(line);std::string kind;in>>kind;if(kind=="summon"){uint32 id,delay,command,personal;in>>id>>delay>>command>>info.TempSummonCreature.CreatureEntry>>info.TempSummonCreature.DespawnDelay>>personal>>info.TempSummonCreature.PosX>>info.TempSummonCreature.PosY>>info.TempSummonCreature.PosZ>>info.TempSummonCreature.Orientation;assert(id==46479&&!delay&&command==10&&!personal);++events;}}assert(events==1);
 Map map;Unit trigger;trigger.map=&map;Spell spell{&trigger};spell.EffectSendEvent(0);assert(summons==1&&summoner==&trigger&&!map.lastTarget);
 // Spell192045 has no unit/GO target, so only native HIT runs. A second
 // HIT_TARGET invocation is not scheduled by its zero provided-target mask.
 spell.effectHandleMode=0;spell.EffectSendEvent(0);assert(summons==1);spell.effectHandleMode=SPELL_EFFECT_HANDLE_HIT;
 spell.info.effect.provided=TARGET_FLAG_UNIT_MASK;spell.EffectSendEvent(0);assert(summons==1);spell.info.effect.provided=0;
 // No direct item grant is performed by the event or command. Failed native
 // summon leaves no consumed per-player progression and the event can retry.
 failSummon=true;spell.EffectSendEvent(0);assert(summons==1);failSummon=false;spell.EffectSendEvent(0);assert(summons==2);
 bird.Update(29999);assert(!bird.removed&&bird.m_timer==1);bird.combat=true;bird.Update(60000);assert(!bird.removed&&bird.m_timer==30000);
 bird.combat=false;bird.m_deathState=CORPSE;bird.Update(60000);assert(!bird.removed);bird.m_deathState=DEAD;bird.Update(1);assert(bird.removed);
 spell.EffectSendEvent(0);bird.Update(30000);assert(bird.removed);spell.EffectSendEvent(0);bird.expired=true;bird.Update(1);assert(bird.removed);
 Player player;player.map=&map;Spell playerCast{&player};playerCast.EffectSendEvent(0);assert(player.credits==2);
 std::cout<<"PASS: actual EffectSendEvent and MapScripts command10, live46479 summon fields, zero-target HIT, wrong dispatch, failed summon/retry, native TempSummon::Update:30s alive/OOC expiry, combat reset, corpse retention, dead and expired cleanup. GO cast/target scheduling, world allocation/phase, AI/combat and corpse loot remain outside this harness; client pending.\n";
}
