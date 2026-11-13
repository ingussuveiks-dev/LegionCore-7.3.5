// Complete production AI and exact post-lock dispatch; engine endpoints are
// controlled here. Real native casts/presentation/criteria run separately.
#include <algorithm>
#include <cassert>
#include <cstdint>
#include <functional>
#include <iostream>
#include <list>
#include <map>
#include <string>
using uint32=uint32_t;using uint64=uint64_t;
enum{CLASS_WARRIOR=1,QUEST_STATUS_INCOMPLETE=3,QUEST_STATUS_COMPLETE=1,CRITERIA_TYPE_SCRIPT_EVENT_2=92,GAMEOBJECT_FIELD_FLAGS=0,GO_FLAG_IN_USE=1,GO_FLAG_NOT_SELECTABLE=16,GAMEOBJECT_TYPE_GOOBER=10,SPELL_ATTR2_CAN_TARGET_NOT_IN_LOS=4,UNIT_FIELD_EMOTE_STATE=0};
struct Map{};struct InstanceScript{std::map<uint32,uint32> receipts;bool supported=true;uint32 GetData(uint32 key)const{auto i=receipts.find(key);return supported&&i!=receipts.end()?i->second:0;}void SetData(uint32 key,uint32 value){if(supported)receipts[key]=value;}};struct GameObject;struct Player;
struct Record{uint64 Counter=0;};struct Achievements{std::map<uint32,Record> records;auto const* GetCriteriaProgressMap()const{return &records;}};
struct Scenario{uint32 id=944,step=3;Achievements achievements;uint32 GetScenarioId()const{return id;}uint32 GetCurrentStep()const{return step;}Achievements&GetAchievementMgr(){return achievements;}};
struct ScenarioMgr{Scenario state,other;Scenario*value=&state;Scenario*GetScenario(uint32){return value;}}manager;auto*sScenarioMgr=&manager;
struct SpellInfo{uint32 Id=8386;bool noLos=false;bool HasAttribute(uint32 bit)const{assert(bit==4);return noLos;}};
struct SpellMgr{SpellInfo spell;bool missing=false;SpellInfo const*GetSpellInfo(uint32 id)const{assert(id==8386);return missing?nullptr:&spell;}}spellMgr;auto*sSpellMgr=&spellMgr;
struct Creature{uint32 changes=0;void SetUInt32Value(uint32 field,uint32 value){assert(field==0&&value==10);++changes;}};
struct Player{
 bool world=true,contact=true,alive=true,teleport=false,phase=true,visible=true,los=true,range=true,reward=false,casting=false,failCast=false,ignoreCredit=false,replace=false;
 uint32 cls=1,instance=42,status=3,requests=0,credits=0;Map*map=nullptr;InstanceScript*script=nullptr;std::list<Creature*>spirits;std::function<void()> reenter;
 bool IsInWorld()const{return world;}bool CanContact()const{return contact;}bool IsAlive()const{return alive;}bool IsBeingTeleported()const{return teleport;}uint32 getClass()const{return cls;}Map*GetMap()const{return map;}uint32 GetInstanceId()const{return instance;}
 bool InSamePhase(GameObject*)const{return phase;}bool canSeeOrDetect(GameObject*)const{return visible;}bool IsWithinLOSInMap(GameObject*)const{return los;}bool IsWithinDistInMap(GameObject*,float)const{return range;}
 InstanceScript*GetInstanceScript()const{return script;}bool GetQuestRewardStatus(uint32 id)const{assert(id==40043);return reward;}uint32 GetQuestStatus(uint32 id)const{assert(id==40043);return status;}bool IsNonMeleeSpellCast(bool b)const{assert(!b);return casting;}
 void CastSpell(GameObject*,uint32 id,bool triggered){assert(id==8386&&!triggered);++requests;if(!failCast)casting=true;}
 void UpdateAchievementCriteria(uint32 type,uint32 event){assert(type==92&&event==46970);++credits;if(reenter)reenter();if(ignoreCredit)return;auto&counter=manager.state.achievements.records[45863].Counter;++counter;if(counter==4)manager.state.step=4;if(replace)manager.value=&manager.other;}
};
void GetCreatureListWithEntryInGrid(std::list<Creature*>&out,Player*p,uint32 entry,float distance){assert(entry==98524&&distance==10);out=p->spirits;}
struct GameObjectAI{explicit GameObjectAI(GameObject*p):go(p){}virtual bool GossipUse(Player*){return false;}virtual bool GossipHello(Player*,bool){return false;}GameObject*go;};
struct GameObject{
 uint32 entry=244940,mapId=1511,db=134130,flags=0,type=10,scriptId=1,presentations=0,state=1;bool world=true,spawn=true;Map*map=nullptr;InstanceScript*script=nullptr;GameObjectAI*ai=nullptr;
 uint32 GetEntry()const{return entry;}uint32 GetMapId()const{return mapId;}uint32 GetDBTableGUIDLow()const{return db;}bool IsInWorld()const{return world;}bool isSpawned()const{return spawn;}bool HasFlag(uint32 field,uint32 mask)const{assert(!field);return flags&mask;}
 Map*GetMap()const{return map;}uint32 GetInstanceId()const{return 42;}float GetInteractionDistance()const{return 5;}InstanceScript*GetInstanceScript()const{return script;}uint32 GetGoType()const{return type;}uint32 GetScriptId()const{return scriptId;}GameObjectAI*AI()const{return ai;}
 void Use(Player*p){if(ai->GossipUse(p))return;if(flags&(1|16))return;++presentations;state=0;flags|=1;}
};
struct GameObjectScript{explicit GameObjectScript(char const*){}virtual GameObjectAI*GetAI(GameObject*)const{return nullptr;}};
#include "RunestoneAI.inc"
struct ObjectMgr{std::string binding="go_prison_runestone";std::string GetScriptName(uint32)const{return binding;}}objects;auto*sObjectMgr=&objects;
struct Dispatch{GameObject*gameObjTarget;SpellInfo*m_spellInfo;uint32 fallback=0;void Open(Player*player,bool passed=true,uint32 lockId=99){if(!passed)return;
#include "RunestoneDispatch.inc"
 ++fallback;
}};
struct Fixture{Map map,other;InstanceScript script,otherScript;GameObject rune;Player player;Creature spirit;go_prison_runestone::go_prison_runestoneAI ai{&rune};SpellInfo info;Dispatch dispatch{&rune,&info};Fixture(){manager={};manager.value=&manager.state;spellMgr={};objects={};player.map=rune.map=&map;player.script=rune.script=&script;rune.ai=&ai;player.spirits={&spirit};}void Open(){player.casting=false;dispatch.Open(&player);}};
int main(){uint32 cases=0;
 for(uint32 status:{1u,3u}){Fixture f;f.player.status=status;f.ai.GossipHello(&f.player,false);assert(!f.player.credits);f.rune.Use(&f.player);assert(f.player.requests==1&&!f.player.credits&&!f.rune.presentations);f.Open();assert(f.player.credits==1&&manager.state.achievements.records[45863].Counter==1&&f.rune.presentations==1&&f.spirit.changes==1);f.Open();f.rune.Use(&f.player);assert(f.player.credits==1&&f.player.requests==1&&f.rune.presentations==1);++cases;}
 for(uint32 mode=0;mode<26;++mode){Fixture f;switch(mode){case 0:f.player.world=false;break;case 1:f.player.contact=false;break;case 2:f.player.alive=false;break;case 3:f.player.teleport=true;break;case 4:f.player.cls=2;break;case 5:f.rune.world=false;break;case 6:f.rune.spawn=false;break;case 7:f.rune.entry=1;break;case 8:f.rune.mapId=1;break;case 9:f.rune.db=134134;break;case 10:f.rune.flags=16;break;case 11:f.player.map=nullptr;break;case 12:f.player.map=&f.other;break;case 13:f.player.instance=43;break;case 14:f.player.phase=false;break;case 15:f.player.visible=false;break;case 16:f.player.script=nullptr;break;case 17:f.player.range=false;break;case 18:f.rune.script=nullptr;break;case 19:f.player.script=&f.otherScript;break;case 20:f.player.reward=true;break;case 21:f.player.status=0;break;case 22:manager.value=nullptr;break;case 23:manager.state.id=909;break;case 24:manager.state.step=4;break;case 25:spellMgr.missing=true;break;}f.ai.GossipUse(&f.player);f.ai.GossipHello(&f.player,true);assert(!f.player.credits&&!f.player.requests&&!f.rune.presentations);++cases;}
 {Fixture f;f.ai.GossipUse(nullptr);f.ai.GossipHello(nullptr,true);assert(!f.rune.presentations);++cases;}
 {Fixture f;f.rune.flags=1;f.ai.GossipUse(&f.player);assert(!f.player.requests);f.Open();assert(f.player.credits==1&&!f.rune.presentations);++cases;}
 {Fixture f;f.player.casting=true;f.rune.Use(&f.player);assert(!f.player.requests&&f.player.casting&&!f.rune.presentations);++cases;}
 {Fixture f;f.player.failCast=true;f.rune.Use(&f.player);assert(f.player.requests==1&&!f.rune.presentations&&!f.player.credits);f.player.failCast=false;f.rune.Use(&f.player);f.Open();assert(f.player.credits==1&&f.rune.presentations==1);++cases;}
 {Fixture f;f.player.ignoreCredit=true;f.Open();assert(!f.ai.claimed&&!f.rune.presentations);f.player.ignoreCredit=false;f.Open();assert(f.ai.claimed&&f.rune.presentations==1);++cases;}
 {Fixture f;f.player.reenter=[&]{f.ai.GossipHello(&f.player,true);};f.Open();assert(f.player.credits==1&&f.rune.presentations==1);++cases;}
 {Fixture f;manager.state.achievements.records[45863].Counter=3;f.Open();assert(manager.state.step==4&&f.player.credits==1&&f.rune.presentations==1);++cases;}
 {Fixture f;f.player.replace=true;f.Open();assert(!f.rune.presentations&&!f.ai.claimed);++cases;}
 {Fixture f;f.player.spirits.clear();f.Open();assert(f.rune.presentations==1&&f.player.credits==1);++cases;}
 {Fixture f;f.player.los=false;f.Open();assert(f.player.credits==1&&f.rune.presentations==1);++cases;}
 for(uint32 mode=0;mode<7;++mode){Fixture f;uint32 lock=99;switch(mode){case 0:f.info.Id=3365;break;case 1:f.rune.entry=1;break;case 2:f.rune.mapId=1;break;case 3:f.rune.db=134134;break;case 4:f.rune.type=3;break;case 5:lock=1;break;case 6:objects.binding="other";break;}f.dispatch.Open(&f.player,true,lock);assert(!f.player.credits&&f.dispatch.fallback==1);++cases;}
 {Fixture f;f.dispatch.Open(&f.player,false);assert(!f.player.credits&&!f.rune.presentations);++cases;}
 {Fixture f;f.Open();go_prison_runestone::go_prison_runestoneAI reloaded(&f.rune);f.rune.ai=&reloaded;f.Open();assert(f.player.credits==1&&f.rune.presentations==1);++cases;}
 {Fixture f;f.script.supported=false;f.Open();assert(!f.player.credits&&!f.rune.presentations&&!f.ai.claimed);++cases;}
 std::cout<<"PASS "<<cases<<" complete production runestone AI/post-lock controlled ASAN contexts;native engine,geometry,presentation and four-object sequence checked separately\n";
}
