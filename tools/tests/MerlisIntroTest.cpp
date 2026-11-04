// ASAN executes the exact production helper block. Native API services below
// are modeled; the separate native suite exercises their real implementations.
#include <cassert>
#include <cmath>
#include <cstdint>
#include <functional>
#include <iostream>
#include <list>
#include <map>
#include <vector>
using uint8=uint8_t;using uint32=uint32_t;using uint64=uint64_t;
struct ObjectGuid{uint64 value=0;void Clear(){value=0;}explicit operator bool()const{return value!=0;}bool operator==(ObjectGuid b)const{return value==b.value;}bool operator!=(ObjectGuid b)const{return value!=b.value;}};
enum{CLASS_ROGUE=4,QUEST_STATUS_INCOMPLETE=3,QUEST_STATUS_COMPLETE=1,CRITERIA_TYPE_SCRIPT_EVENT_2=92,SPELL_CAST_OK=0};
struct Location{uint32 mapId=1620;float x=-8321.93f,y=287.852f,z=156.916f;uint32 GetMapId()const{return mapId;}float GetExactDist(float a,float b,float c)const{return std::sqrt((x-a)*(x-a)+(y-b)*(y-b)+(z-c)*(z-c));}};
struct Player;struct Creature;struct Map{std::list<Player*> players;std::list<Creature*> actors;};
struct Progress{uint64 Counter=0;};
struct Scenario{uint32 id=1123,stage=6;bool grant=true;std::map<uint32,Progress> progress;auto& GetAchievementMgr(){return *this;}auto GetCriteriaProgressMap()const{return &progress;}uint32 GetScenarioId()const{return id;}bool CanUpdateCriteria(uint32 leaf)const{return (stage==6&&leaf==48587)||(stage==7&&leaf==48589);}};
struct Manager{Scenario* value=nullptr;Scenario* GetScenario(uint32){return value;}}manager;auto sScenarioMgr=&manager;
struct World:Location{Map* map=nullptr;ObjectGuid guid;uint32 entry=107831;bool world=true,preDelete=false,alive=true,expired=false,scope=true,phase=true;bool IsInWorld()const{return world;}bool IsPreDelete()const{return preDelete;}bool IsAlive()const{return alive;}bool IsQuestObjectExpired()const{return expired;}bool CanTargetQuestObject(World const* other)const{return scope&&other->scope;}bool InSamePhase(World const* other)const{return phase==other->phase;}Map* GetMap()const{return map;}uint32 GetEntry()const{return entry;}uint32 GetInstanceId()const{return 1;}ObjectGuid GetGUID()const{return guid;}using Location::GetExactDist;float GetExactDist(World const* other)const{return Location::GetExactDist(other->x,other->y,other->z);}};
struct Player:World{
 bool contact=true,gm=false,pending=false,rewarded=false,aura=false,castOk=true,teleportOk=true,queueOk=true;uint32 klass=CLASS_ROGUE,quest=42504,status=QUEST_STATUS_INCOMPLETE,casts=0,teleports=0,awards=0;Location destination;std::function<void()> castHook,teleportHook,awardHook;
 bool CanContact()const{return contact;}bool isGameMaster()const{return gm;}uint32 getClass()const{return klass;}bool HasPendingTeleport()const{return pending;}bool GetQuestRewardStatus(uint32 id)const{return id==quest&&rewarded;}uint32 GetQuestStatus(uint32 id)const{return id==quest?status:0;}bool HasAura(uint32 id)const{assert(id==214942);return aura;}
 uint32 CastSpell(Player* target,uint32 id){assert(target==this&&id==214942);++casts;if(castHook)castHook();if(castOk)aura=true;return castOk?SPELL_CAST_OK:1;}
 Location& GetTeleportDest(){return destination;}
 bool TeleportTo(uint32 id,float a,float b,float c,float o){assert(id==1620&&o==0);++teleports;if(teleportOk&&queueOk){destination={id,a,b,c};pending=true;}if(teleportHook)teleportHook();return teleportOk;}
 void UpdateAchievementCriteria(uint32 type,uint32 event){assert(type==92&&event==51614);++awards;auto s=manager.value;if(s&&s->id==1123&&s->stage==6&&s->grant){s->progress[48587].Counter=1;s->stage=7;}if(awardHook)awardHook();}
 Creature* FindNearestCreature(uint32 entry,float range,bool alive);
};
struct Creature:World{uint32 despawns=0;void GetPlayerListInGrid(std::list<Player*>& result,float range){assert(range==70);result=map->players;}void DespawnOrUnsummon(){++despawns;alive=false;}};
Creature* Player::FindNearestCreature(uint32 id,float range,bool living){assert(living);for(auto actor:map->actors)if(actor->entry==id&&actor->world&&actor->alive&&actor->InSamePhase(this)&&GetExactDist(actor)<=range)return actor;return nullptr;}
namespace ObjectAccessor{Player* GetPlayer(Creature& source,ObjectGuid guid){for(auto p:source.map->players)if(p->guid==guid&&p->world&&p->map==source.map)return p;return nullptr;}Creature* GetCreature(Creature& source,ObjectGuid guid){for(auto actor:source.map->actors)if(actor->guid==guid&&actor->world&&actor->map==source.map)return actor;return nullptr;}}
enum class HighGuid{Conversation};struct Generator{uint32 Generate(){return 1;}}generator;struct ObjectMgr{template<HighGuid>Generator* GetGenerator(){return &generator;}}objects;auto sObjectMgr=&objects;
struct Conversation;
static bool voiceOk=true;static uint32 voices=0,warnings=0;static std::function<void()> voiceHook;static std::vector<Conversation*> stories;
struct Conversation{bool CreateConversation(uint32,uint32 id,Player*,void*,Player const&){assert(id==2484);++voices;if(voiceHook)voiceHook();if(voiceOk)stories.push_back(this);return voiceOk;}};
#define TC_LOG_WARN(...) (++warnings)
struct Merlis{Creature* me;bool introDone=false;uint32 timer=1000;
#include "MerlisHelpers.inc"
};
struct Fixture{
 Map map;Scenario scenario;Player player,visitor;Creature source,garona,boat;Merlis ai{&source};
 Fixture(){manager.value=&scenario;voiceOk=true;voices=warnings=0;voiceHook={};player.map=visitor.map=source.map=garona.map=boat.map=&map;player.guid={1};visitor.guid={2};source.guid={3};garona.guid={4};boat.guid={5};player.x+=10;visitor.x+=5;visitor.quest=0;garona.entry=108333;boat.entry=64367;garona.x=-8343.12f;garona.y=270.484f;garona.z=155.43f;boat.x=-8350.7f;boat.y=264.816f;boat.z=155.43f;map.players={&visitor,&player};map.actors={&source,&garona,&boat};}
 ~Fixture(){for(auto story:stories)delete story;stories.clear();voiceHook={};manager.value=nullptr;}
 void Begin(){ai.UpdateIntro(1000);assert(ai.introRecipient==player.guid&&!ai.introDone&&ai.introDelay==5000);}
 void Queue(){ai.UpdateIntro(4999);assert(player.teleports==0);ai.UpdateIntro(1);assert(player.pending);if(scenario.grant)assert(ai.introDone&&scenario.progress[48587].Counter==1&&scenario.CanUpdateCriteria(48589));else assert(ai.introArriving&&!ai.introDone&&scenario.progress.empty());}
 void Ack(){player.pending=false;player.x=player.destination.x;player.y=player.destination.y;player.z=player.destination.z;}
 void Finish(){Ack();ai.UpdateIntro(1000);assert(ai.introDone&&scenario.progress[48587].Counter==1&&scenario.stage==7);auto ports=player.teleports,awards=player.awards;ai.UpdateIntro(50000);assert(player.teleports==ports&&player.awards==awards&&scenario.progress.size()==1);}
};
int main(){uint32 cases=0;
 std::vector<std::function<void(Fixture&)>> denied={
 [](auto& f){f.player.gm=true;},[](auto& f){f.player.klass=1;},[](auto& f){f.player.status=0;},[](auto& f){f.player.rewarded=true;},[](auto& f){f.player.alive=false;},[](auto& f){f.player.contact=false;},[](auto& f){f.player.world=false;},[](auto& f){f.player.preDelete=true;},[](auto& f){f.player.pending=true;},[](auto& f){f.player.expired=true;},[](auto& f){f.player.scope=false;},[](auto& f){f.player.phase=false;},[](auto& f){f.player.mapId=99;f.player.map=nullptr;},[](auto& f){f.player.x+=61;},[](auto& f){f.player.z+=71;},[](auto& f){f.source.alive=false;},[](auto& f){f.source.world=false;},[](auto& f){f.source.preDelete=true;},[](auto& f){f.source.expired=true;},[](auto& f){f.source.scope=false;},[](auto& f){f.source.entry=1;},[](auto& f){f.source.mapId=1;},[](auto& f){f.scenario.stage=5;},[](auto& f){f.scenario.id=99;}
 };
 for(uint32 quest:{42504u,42627u}){
  for(auto const& deny:denied)for(bool delayed:{false,true}){Fixture f;f.player.quest=quest;auto p=f.player;auto s=f.source;auto scenario=f.scenario;if(delayed)f.Begin();deny(f);f.ai.UpdateIntro(delayed?5000:1000);assert(!f.ai.introRecipient&&!f.ai.introDone&&f.player.teleports==0&&f.player.awards==0&&f.garona.despawns==0&&f.boat.despawns==0);f.player=p;f.source=s;f.scenario=scenario;f.Begin();f.Queue();f.Finish();++cases;}
  for(uint32 mode=0;mode<21;++mode){Fixture f;f.player.quest=quest;
   if(mode==1)voiceOk=false;
   if(mode==2)f.player.castOk=false;
   if(mode==3)f.player.teleportOk=false;
   if(mode==4)f.player.queueOk=false;
   if(mode==7)f.player.aura=true;
   if(mode==9)voiceHook=[&]{f.ai.UpdateIntro(50000);};
   if(mode==10)f.player.castHook=[&]{f.ai.UpdateIntro(50000);};
   if(mode==11)f.player.teleportHook=[&]{f.ai.UpdateIntro(50000);};
   if(mode==12)voiceHook=[&]{f.player.status=0;};
   if(mode==13)f.player.castHook=[&]{f.player.status=0;};
   if(mode==14)f.player.teleportHook=[&]{f.player.status=0;};
   if(mode==16)f.player.awardHook=[&]{f.player.phase=false;};
   if(mode==20){f.scenario.progress[48587].Counter=1;f.scenario.stage=7;f.ai.UpdateIntro(1000);assert(f.ai.introDone&&voices==0&&f.player.teleports==0);++cases;continue;}
   if(mode==5||mode==15)f.scenario.grant=false;
   if(mode==17)f.player.teleportHook=[&]{f.garona.guid={99};};
   if(mode==18)f.player.teleportHook=[&]{f.boat.entry=999;};
   if(mode==19)f.player.teleportHook=[&]{f.boat.expired=true;};
   f.Begin();
   if(mode==1)voiceOk=true;
   if(mode==12){f.ai.UpdateIntro(5000);assert(!f.ai.introRecipient&&f.player.teleports==0&&f.player.awards==0);++cases;continue;}
   if(mode==13||mode==14){f.ai.UpdateIntro(5000);assert(!f.ai.introRecipient&&f.player.awards==0&&f.garona.despawns==0);assert(f.player.teleports==(mode==14?1u:0u));++cases;continue;}
   if(mode==3||mode==4){f.ai.UpdateIntro(5000);assert(!f.ai.introArriving&&!f.ai.introDone&&f.player.teleports==1&&f.player.awards==0&&f.garona.despawns==0&&(f.ai.introWarnings&2));f.ai.UpdateIntro(1);assert(f.player.teleports==1);f.player.teleportOk=f.player.queueOk=true;f.ai.UpdateIntro(999);assert(f.ai.introDone&&f.player.pending&&f.player.teleports==2&&f.scenario.progress[48587].Counter==1);}
   else f.Queue();
   if(mode==1)assert(warnings==1&&voices==2&&f.ai.introDone);
   if(mode==2)assert(warnings==1&&f.player.casts==1&&f.ai.introDone&&f.scenario.progress[48587].Counter==1);
   if(mode==5){assert(!f.ai.introDone&&f.player.awards==1&&f.player.teleports==1&&f.garona.despawns==0);f.ai.UpdateIntro(1);assert(f.player.awards==1);f.scenario.grant=true;f.ai.UpdateIntro(999);assert(f.ai.introDone&&f.player.pending&&f.player.awards==2&&f.player.teleports==1);f.Finish();++cases;continue;}
   if(mode==6){f.player.destination.x+=20;f.ai.UpdateIntro(1000);assert(f.ai.introDone&&f.player.awards==1&&f.scenario.progress[48587].Counter==1);++cases;continue;}
   if(mode==8)for(uint32 i=0;i<50;++i){f.ai.UpdateIntro(1000);assert(f.player.pending&&f.player.teleports==1&&f.player.awards==1&&f.scenario.CanUpdateCriteria(48589));}
   if(mode==15){f.ai.introScene=false;f.ai.introSceneRetry=0;f.player.aura=false;f.player.castHook=[&]{f.player.status=0;};f.ai.UpdateIntro(1000);assert(!f.ai.introRecipient&&f.player.awards==1&&f.garona.despawns==0);++cases;continue;}
   f.Finish();assert(f.garona.despawns==(mode==16||mode==17?0u:1u));assert(f.boat.despawns==(mode==16||mode==18||mode==19?0u:1u));if(mode==7)assert(f.player.casts==0);++cases;
  }
 }
 std::cout<<"PASS "<<cases<<" actual Merlis helper ASAN contexts: initial/delayed qualification and recovery, five-second delay, reentrant/failing factories/casts/travel/native receipt, actual queued destination before native event and independent ACK, optional actors/full GUID/entry/phase/scope cleanup and exactly-once shared event. Native API services modeled; no client/full-core ASAN claim\n";
}
