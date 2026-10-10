#include <cassert>
#include <cstdint>
#include <functional>
#include <iostream>
#include <map>
#include <memory>
#include <vector>
using uint8=uint8_t;using uint32=uint32_t;using uint64=uint64_t;using int32=int32_t;using QuestStatus=int;
enum {QUEST_STATUS_NONE=0,QUEST_STATUS_INCOMPLETE=3,QUEST_OBJECTIVE_MONSTER=0,MAP_ALL_LIQUIDS=15,
    LIQUID_MAP_ABOVE_WATER=1,LIQUID_MAP_IN_WATER=4,LIQUID_MAP_UNDER_WATER=8};
struct ObjectGuid {unsigned value=0;bool operator<(ObjectGuid const& r)const{return value<r.value;}};
#include "QuestStatusData.inc"
struct Player;struct Creature;
struct Unit {virtual ~Unit()=default;virtual Player* ToPlayer(){return nullptr;}};
struct QuestObjective {int Type=0,ObjectID=95148,Amount=8;};
struct Quest {std::vector<QuestObjective> objectives{{}};auto const& GetObjectives()const{return objectives;}};
struct QuestStore {Quest quest;Quest const* GetQuestTemplate(unsigned id){assert(id==39614);return &quest;}} questStore;
auto sQuestDataStore=&questStore;
struct Player:Unit {
 bool alive=true,teleport=false,visible=true;unsigned map=1220;float distance=1;
 std::map<unsigned,QuestStatusData> quests;unsigned count=0,otherQuestCount=0,packets=0;
 std::vector<std::function<void()>> events;
 Player* ToPlayer()override{return this;}
 bool IsAlive(){return alive;}unsigned GetMapId(){return map;}bool IsBeingTeleported(){return teleport;}
 float GetDistance(Creature*){return distance;}bool canSeeOrDetect(Creature*){return visible;}
 QuestStatusData* getQuestStatus(unsigned q){auto i=quests.find(q);return i==quests.end()?nullptr:&i->second;}
 int GetQuestStatus(unsigned q){auto s=getQuestStatus(q);return s?s->Status:0;}
 void Accept(){quests.erase(39614);quests[39614].Status=3;count=0;}
 void Abandon(){quests.erase(39614);count=0;}
 unsigned GetQuestObjectiveData(unsigned q,unsigned e){assert(q==39614&&e==95148);return count;}
 void SetQuestObjectiveData(Quest const*,QuestObjective const*,unsigned value){count=value;}
 void SendQuestUpdateAddCredit(Quest const*,ObjectGuid guid,QuestObjective const&,unsigned value){assert(guid.value&&value==count);++packets;}
 bool CanCompleteQuest(unsigned q){assert(q==39614);return count==8;}
 void CompleteQuest(unsigned q){quests[q].Status=1;}
 void AddDelayedEvent(uint64,std::function<void()> fn){events.push_back(std::move(fn));}
 void AddQuestDelayedEvent(uint32,uint64,std::function<void()>&&);
 void Next(){assert(!events.empty());auto fn=std::move(events.front());events.erase(events.begin());fn();}
 void Drain(){unsigned calls=0;while(!events.empty()){assert(++calls<100);Next();}}
};
#include "QuestDelayedEvent.inc"
struct Map {unsigned liquid=0,calls=0;unsigned getLiquidStatus(float x,float y,float z,unsigned mask,void*){assert(x==20&&y==30&&z==40&&mask==MAP_ALL_LIQUIDS);++calls;return liquid;}};
struct Spline {bool finalized=false;bool Finalized(){return finalized;}};
struct Creature:Unit {
 ObjectGuid guid;bool alive=true,despawned=false;Map map;Spline spline;Spline* movespline=&spline;
 ObjectGuid GetGUID(){return guid;}bool IsAlive(){return alive;}Map* GetMap(){return &map;}
 float GetPositionX(){return 20;}float GetPositionY(){return 30;}float GetPositionZ(){return 40;}
 void DespawnOrUnsummon(){assert(!despawned);despawned=true;}
};
std::map<ObjectGuid,Creature*> world;
struct ObjectAccessor {static Creature* GetCreature(Player&,ObjectGuid guid){auto i=world.find(guid);return i==world.end()||i->second->despawned?nullptr:i->second;}};
struct SpellInfo {unsigned Id;};
struct ScriptedAI {Creature* me;ScriptedAI(Creature* c):me(c){}virtual void Reset(){}virtual void SpellHit(Unit*,SpellInfo const*){}};
#include "HighmountainRiver.inc"
int main()
{
 SpellInfo kick{188447},wrong{1};Player p,q;p.Accept();q.Accept();Creature fish;fish.guid={1};world[fish.guid]=&fish;npc_highmountain_whitewater_carp ai(&fish);
 ai.SpellHit(&p,&wrong);assert(p.events.empty());p.alive=false;ai.SpellHit(&p,&kick);p.alive=true;p.distance=10;ai.SpellHit(&p,&kick);p.distance=1;p.visible=false;ai.SpellHit(&p,&kick);p.visible=true;assert(p.events.empty());
 // Water under a fish that is still airborne, or under a dry landing, is not rescue.
 fish.map.liquid=LIQUID_MAP_IN_WATER;ai.SpellHit(&p,&kick);p.Next();assert(!p.count&&!fish.map.calls);fish.spline.finalized=true;fish.map.liquid=LIQUID_MAP_ABOVE_WATER;p.Drain();assert(!p.count&&!fish.despawned);
 // A second kick after the dry landing can rescue the same fish. Client gets total count.
 ai.SpellHit(&p,&kick);fish.map.liquid=LIQUID_MAP_IN_WATER;p.Drain();assert(p.count==1&&p.packets==1&&fish.despawned);
 fish.despawned=false;ai.Reset();fish.spline.finalized=false;
 // Last kicker owns the action, including an ineligible later kicker.
 ai.SpellHit(&p,&kick);ai.SpellHit(&q,&kick);fish.spline.finalized=true;p.Drain();assert(p.count==1);q.Drain();assert(q.count==1&&fish.despawned);
 fish.despawned=false;ai.SpellHit(&p,&kick);Player stranger;ai.SpellHit(&stranger,&kick);p.Drain();assert(p.count==1&&!fish.despawned);
 // Same-tick abandon/reaccept cannot inherit delayed credit, nor can a dead player.
 ai.SpellHit(&p,&kick);p.Abandon();p.Accept();p.Drain();assert(!p.count);ai.SpellHit(&p,&kick);p.alive=false;p.Drain();p.alive=true;assert(!p.count);
 ai.SpellHit(&p,&kick);p.map=1;p.Drain();p.map=1220;assert(!p.count);
 ai.SpellHit(&p,&kick);p.teleport=true;p.Drain();p.teleport=false;assert(!p.count);
 ai.SpellHit(&p,&kick);p.visible=false;p.Drain();p.visible=true;assert(!p.count);
 ai.SpellHit(&p,&kick);p.distance=101;p.Drain();p.distance=1;assert(!p.count);
 ai.SpellHit(&p,&kick);world.clear();p.Drain();world[fish.guid]=&fish;assert(!p.count);
 ai.SpellHit(&p,&kick);ai.Reset();p.Drain();assert(!p.count);
 fish.spline.finalized=false;ai.SpellHit(&p,&kick);p.Drain();assert(!p.count&&p.events.empty());fish.spline.finalized=true;
 // Eight actual rescues complete only 39614; no unrelated 41144 credit or overcount.
 for(unsigned i=0;i<8;++i){fish.despawned=false;ai.SpellHit(&p,&kick);p.Drain();assert(p.count==i+1&&fish.despawned);}
 assert(p.GetQuestStatus(39614)==1&&!p.otherQuestCount);fish.despawned=false;ai.SpellHit(&p,&kick);assert(p.events.empty()&&!fish.despawned);
 std::cout<<"PASS: actual production landing/credit handlers and quest scheduler: airborne/dry/wet landings, repeat kicks, last-kicker ownership, abandon/reaccept, death/map/teleport/phase/range/despawn/reset/timeout, eight rescues and total-count packets.\n";
}
