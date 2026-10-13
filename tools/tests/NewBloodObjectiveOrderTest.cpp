#include <cassert>
#include <cstdint>
#include <string>
#include <vector>
#include <iostream>
using uint8=uint8_t; using int8=int8_t; using uint32=uint32_t; using int32=int32_t;
struct Field { int value=0; uint32 GetUInt32(){return value;} uint8 GetUInt8(){return static_cast<uint8>(value);} int8 GetInt8(){return static_cast<int8>(value);} int32 GetInt32(){return value;} float GetFloat(){return float(value);} bool GetBool(){return value!=0;} std::string GetString(){return {};} };
struct QuestObjective {uint32 ID=0,QuestID=0;uint8 Type=0;int8 StorageIndex=0;int32 ObjectID=0,Amount=0;uint32 Flags=0,Flags2=0;float TaskStep=0;std::string Description;bool Bugged=false;};
struct Quest { std::vector<QuestObjective> Objectives; void LoadQuestObjective(Field*); };
#include "QuestObjectiveLoad.inc"
void add(Quest& q,int quest,int id,int slot,int flags=0){Field f[11];f[0].value=id;f[1].value=quest;f[3].value=slot;f[6].value=flags;q.LoadQuestObjective(f);}
struct Unit {uint32 map;uint32 GetMapId(){return map;}};
struct SpellInfo {uint32 Id;};
struct Position {float x=1,y=2,z=3;void Relocate(float a,float b,float c){x=a;y=b;z=c;}};
void landing(Unit* m_caster,SpellInfo* m_spellInfo,Position* destTarget){
#include "JumpDestination.inc"
}
struct Store {Quest value;Quest const* GetQuestTemplate(uint32){return &value;}};
Quest const* reward(uint32 scenarioId){Store store;auto sQuestDataStore=&store;struct Step{uint32 RewardQuestID;};Step step{40729};Step* steps[]={&step};int rewardStep=0;
#include "RewardSelection.inc"
static Quest result;return quest?&result:nullptr;
}
int main(){
 for(uint32 scenario:{0u,998u,1011u})assert(reward(scenario));assert(!reward(962));
 for(uint32 map:{1220u,1502u,1522u})for(uint32 id:{199166u,199162u,199189u,199191u,192085u}){
  Unit unit{map};SpellInfo info{id};Position pos;landing(&unit,&info,&pos);
  if(map==1522&&id!=192085)assert(pos.x==3090.2f&&pos.z==257.026f);else assert(pos.x==1&&pos.z==3);
 }

 Quest q;add(q,40729,282105,0,2);add(q,40729,282106,1,28);add(q,40729,281139,2);
 assert(q.Objectives[0].ID==281139&&q.Objectives[0].StorageIndex==2);
 assert(q.Objectives[1].ID==282105&&q.Objectives[1].StorageIndex==0&&q.Objectives[1].Flags==2);
 assert(q.Objectives[2].ID==282106&&q.Objectives[2].StorageIndex==1);
 Quest other;add(other,40731,100,0);add(other,40731,101,1);assert(other.Objectives[0].ID==100);
 std::cout<<"PASS: production objective loader preserves saved slots and orders the ritual before the sequenced escape; other quests unchanged.\n";
}
