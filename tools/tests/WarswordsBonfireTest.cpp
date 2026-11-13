#include <cassert>
#include <cstdint>
#include <functional>
#include <list>
#include <iostream>
#include <string>
using uint32=uint32_t;enum{CLASS_WARRIOR=1,QUEST_STATUS_INCOMPLETE=3,QUEST_STATUS_COMPLETE=1,CRITERIA_TYPE_SCRIPT_EVENT_2=92,GAMEOBJECT_FIELD_FLAGS=0,GO_FLAG_IN_USE=1,GO_FLAG_NOT_SELECTABLE=16,GAMEOBJECT_TYPE_GOOBER=10,GO_STATE_ACTIVE=0,GO_STATE_READY=1,SPELL_ATTR2_CAN_TARGET_NOT_IN_LOS=4};
struct Map{};struct InstanceScript{uint32 getScenarionStep()const{return 0;}};struct GameObject;
struct Scenario{uint32 id=944,step=0;uint32 GetScenarioId()const{return id;}uint32 GetCurrentStep()const{return step;}};
struct ScenarioMgr{Scenario state;Scenario* value=&state;Scenario*GetScenario(uint32){return value;}}manager;auto*sScenarioMgr=&manager;
struct Player{
 bool world=true,contact=true,alive=true,teleport=false,phase=true,visible=true,los=true,range=true,reward=false,casting=false,failCast=false;uint32 cls=1,instance=42,status=3,credits=0,requests=0;Map*map=nullptr;InstanceScript*script=nullptr;std::function<void()>callback;
 bool IsInWorld()const{return world;}bool CanContact()const{return contact;}bool IsAlive()const{return alive;}bool IsBeingTeleported()const{return teleport;}uint32 getClass()const{return cls;}Map*GetMap()const{return map;}uint32 GetInstanceId()const{return instance;}InstanceScript*GetInstanceScript()const{return script;}
 bool InSamePhase(GameObject*)const{return phase;}bool canSeeOrDetect(GameObject*)const{return visible;}bool IsWithinLOSInMap(GameObject*)const{return los;}bool IsWithinDistInMap(GameObject*,float)const{return range;}bool GetQuestRewardStatus(uint32 q)const{assert(q==40043);return reward;}uint32 GetQuestStatus(uint32 q)const{assert(q==40043);return status;}
 void UpdateAchievementCriteria(uint32 type,uint32 id){assert(type==92&&id==47272);++credits;if(callback)callback();if(manager.value)manager.value->step=1;}
 bool IsNonMeleeSpellCast(bool delayed)const{assert(!delayed);return casting;}int CastSpell(GameObject* target,uint32 id,bool triggered){assert(target&&id==198513&&!triggered);++requests;casting=!failCast;return failCast?1:0;}
};
struct GameObjectAI{GameObject*go;explicit GameObjectAI(GameObject*g):go(g){}virtual~GameObjectAI()=default;virtual bool GossipHello(Player*,bool){return false;}virtual bool GossipUse(Player*){return false;}};
struct GameObject{
 bool world=true,spawn=true;uint32 entry=245188,mapId=1511,instance=42,flags=0,db=134117,type=10,mask=1,phases=0,state=GO_STATE_READY,lights=0;Map*map=nullptr;InstanceScript*script=nullptr;GameObjectAI*ai=nullptr;std::list<GameObject*>fog;
 bool IsInWorld()const{return world;}bool isSpawned()const{return spawn;}uint32 GetEntry()const{return entry;}uint32 GetMapId()const{return mapId;}bool HasFlag(uint32,uint32 bits)const{return flags&bits;}Map*GetMap()const{return map;}uint32 GetInstanceId()const{return instance;}InstanceScript*GetInstanceScript()const{return script;}float GetInteractionDistance()const{return 5;}uint32 GetDBTableGUIDLow()const{return db;}uint32 GetGoType()const{return type;}uint32 GetScriptId()const{return 1;}GameObjectAI*AI(){return ai;}
 void GetGameObjectListWithEntryInGrid(std::list<GameObject*>&out,uint32 id,float range){assert(id==244751&&range==45);out=fog;}void SetPhaseMask(uint32 value,bool update){assert(value==4&&update);mask=value;++phases;}
 void SetGoState(uint32 value){assert(value==GO_STATE_ACTIVE);state=value;++lights;}
};
struct GameObjectScript{explicit GameObjectScript(char const*){}virtual GameObjectAI*GetAI(GameObject*)const{return nullptr;}};
struct SpellInfo{uint32 Id=198513;bool nativeNoLos=true;bool HasAttribute(uint32 bit)const{assert(bit==SPELL_ATTR2_CAN_TARGET_NOT_IN_LOS);return nativeNoLos;}};
struct SpellMgr{SpellInfo ignition;bool missing=false;SpellInfo const* GetSpellInfo(uint32 id)const{assert(id==198513);return missing?nullptr:&ignition;}}spellManager;auto*sSpellMgr=&spellManager;
// Logging is an observed engine endpoint in the native regression; this model
// controls that endpoint while exercising the complete production AI.
#define TC_LOG_INFO(...) ((void)0)
#include "Bonfire.inc"
struct ObjectMgr{std::string binding="go_mystic_bonfire";std::string GetScriptName(uint32)const{return binding;}}objectManager;auto*sObjectMgr=&objectManager;
struct Dispatch{GameObject*gameObjTarget;SpellInfo*m_spellInfo;uint32 fallback=0;void Open(Player*player,uint32 lockId=2474,bool succeeded=true){if(!succeeded)return;
#include "Dispatch.inc"
 ++fallback;
}};
struct Fixture{Map map,other;InstanceScript script,otherScript;GameObject fire,fog;Player player;go_mystic_bonfire::go_mystic_bonfireAI ai{&fire};SpellInfo info;Dispatch dispatch{&fire,&info};Fixture(){manager={};manager.value=&manager.state;objectManager={};spellManager={};player.map=fire.map=&map;player.script=fire.script=&script;fire.ai=&ai;fog.entry=244751;fire.fog={&fog};}void Open(){dispatch.Open(&player);}};
int main(){uint32 cases=0;
 for(uint32 status:{1u,3u}){Fixture f;f.player.status=status;f.ai.GossipHello(&f.player,false);assert(!f.player.credits);f.Open();assert(f.player.credits==1&&manager.state.step==1&&f.fog.phases==1&&!f.dispatch.fallback);for(uint32 n=0;n<3;++n){f.Open();f.ai.GossipHello(&f.player,false);}assert(f.player.credits==1&&f.fog.phases==1);++cases;}
 for(uint32 mode=0;mode<27;++mode){Fixture f;switch(mode){case 0:f.player.world=false;break;case 1:f.player.contact=false;break;case 2:f.player.alive=false;break;case 3:f.player.teleport=true;break;case 4:f.player.cls=2;break;case 5:f.fire.world=false;break;case 6:f.fire.spawn=false;break;case 7:f.fire.entry=1;break;case 8:f.fire.mapId=1;break;case 9:f.fire.flags=GO_FLAG_IN_USE;break;case 10:f.fire.flags=GO_FLAG_NOT_SELECTABLE;break;case 11:f.player.map=nullptr;break;case 12:f.player.map=&f.other;break;case 13:f.player.instance=43;break;case 14:f.player.phase=false;break;case 15:f.player.visible=false;break;case 16:f.player.los=false;spellManager.ignition.nativeNoLos=false;break;case 17:f.player.range=false;break;case 18:f.fire.script=nullptr;break;case 19:f.player.script=&f.otherScript;break;case 20:f.player.reward=true;break;case 21:f.player.status=0;break;case 22:manager.value=nullptr;break;case 23:manager.state.id=909;break;case 24:manager.state.step=1;break;case 25:manager.state.step=7;break;case 26:f.player.script=nullptr;break;}
 // AI admission separately exercises all guards, including dispatch mismatches.
 f.ai.GossipUse(&f.player);assert(!f.player.requests);if(mode!=9){f.ai.GossipHello(&f.player,true);assert(!f.player.credits&&!f.fog.phases);}++cases;}
 for(uint32 mode=0;mode<7;++mode){Fixture f;uint32 lock=2474;switch(mode){case 0:f.info.Id=3365;break;case 1:f.fire.mapId=1;break;case 2:f.fire.entry=1;break;case 3:f.fire.db=1;break;case 4:f.fire.type=3;break;case 5:lock=1;break;case 6:objectManager.binding="other";break;}f.dispatch.Open(&f.player,lock);assert(!f.player.credits&&f.dispatch.fallback==1);++cases;}
 {Fixture f;f.dispatch.Open(&f.player,2474,false);assert(!f.player.credits&&!f.fog.phases&&!f.dispatch.fallback);++cases;}
 {Fixture f;f.player.callback=[&]{f.Open();};f.Open();assert(f.player.credits==1&&f.fog.phases==1);++cases;}
 {Fixture f;Scenario replacement;f.player.callback=[&]{manager.value=&replacement;};f.Open();assert(f.player.credits==1&&!f.fog.phases);++cases;}
 {Fixture f;assert(f.ai.GossipUse(&f.player));assert(f.player.requests==1&&!f.player.credits&&!f.fire.lights&&!f.fog.phases);f.ai.GossipUse(&f.player);f.ai.GossipHello(&f.player,false);assert(f.player.requests==1&&!f.player.credits);f.Open();assert(f.player.credits==1&&f.fire.lights==1&&f.fog.phases==1);f.ai.GossipUse(&f.player);assert(f.player.requests==1);++cases;}
 {Fixture f;f.fire.flags=GO_FLAG_IN_USE;f.Open();assert(f.player.credits==1&&f.fire.lights==1&&f.fog.phases==1);++cases;}
 {Fixture f;f.player.casting=true;assert(f.ai.GossipUse(&f.player));assert(!f.player.requests&&!f.player.credits&&f.player.casting);++cases;}
 {Fixture f;f.player.failCast=true;f.ai.GossipUse(&f.player);assert(f.player.requests==1&&!f.player.casting&&!f.player.credits&&!f.fire.lights);f.player.failCast=false;f.ai.GossipUse(&f.player);assert(f.player.requests==2&&!f.player.credits);f.Open();assert(f.player.credits==1);++cases;}
 {Fixture f;f.fire.db=134118;assert(!f.ai.GossipUse(&f.player));f.Open();assert(!f.player.requests&&!f.player.credits&&!f.fire.lights);++cases;}
 {Fixture f;f.player.los=false;f.ai.GossipUse(&f.player);assert(f.player.requests==1&&!f.player.credits&&!f.fire.lights);f.Open();assert(f.player.credits==1&&f.fire.lights==1&&f.fog.phases==1);++cases;}
 {Fixture f;spellManager.ignition.nativeNoLos=false;f.ai.GossipUse(&f.player);assert(f.player.requests==1&&!f.player.credits);f.Open();assert(f.player.credits==1);++cases;}
 {Fixture f;spellManager.missing=true;f.ai.GossipUse(&f.player);f.Open();assert(!f.player.requests&&!f.player.credits&&!f.fire.lights);++cases;}
 {Fixture f;f.player.los=false;f.player.failCast=true;f.ai.GossipUse(&f.player);assert(f.player.requests==1&&!f.player.credits&&!f.fire.lights);++cases;}
 std::cout<<"PASS "<<cases<<" production bonfire/actual post-lock dispatch ASAN contexts: completed198513, active Warrior admission, report-use/failed-lock/wrong dispatch, repeat/reentrant once-only credit, foreign scenario cleanup guard. Engine admission/cast/criteria/phase endpoints controlled; real native traversal tested separately\n";
}
