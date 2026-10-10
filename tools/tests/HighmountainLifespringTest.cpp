#include <cassert>
#include <cstdint>
#include <iostream>
#include <map>
#include <mutex>
#include <set>
#include <vector>
using uint32=uint32_t; using ObjectGuid=uint64_t;
enum {QUEST_STATUS_NONE=0,QUEST_STATUS_COMPLETE=1,QUEST_STATUS_INCOMPLETE=3,REACT_PASSIVE=0,FOLLOW_MOTION_TYPE=1};
enum SpellCastResult {SPELL_CAST_OK,SPELL_FAILED_DONT_REPORT};
struct Player; struct Creature; struct TempSummon;
struct Unit { virtual ~Unit()=default; virtual Player* ToPlayer(){return nullptr;} };
struct Motion {int type=0;unsigned calls=0;Player* target=nullptr;void MoveFollow(Player* p,float,float){type=FOLLOW_MOTION_TYPE;target=p;++calls;}int GetCurrentMovementGeneratorType(){return type;}};
struct Creature : Unit {
 ObjectGuid guid; bool alive=true,gone=false,combat=false,walk=true;unsigned smartUpdates=0,smartAccepts=0;int react=1;std::set<ObjectGuid> viewers;Motion motion;
 explicit Creature(ObjectGuid id):guid(id){}
 virtual TempSummon* ToTempSummon(){return nullptr;}
 void DespawnOrUnsummon(){gone=true;} bool IsAlive(){return alive;} bool isInCombat(){return combat;}
 void AddPlayerInPersonnalVisibilityList(ObjectGuid p){viewers.insert(p);} void SetReactState(int r){react=r;}void SetWalk(bool w){walk=w;}Motion* GetMotionMaster(){return &motion;}
};
struct TempSummon : Creature {Unit* owner;TempSummon(ObjectGuid id,Unit* p):Creature(id),owner(p){} TempSummon* ToTempSummon()override{return this;} Unit* GetSummoner(){return owner;}};
std::map<ObjectGuid,Creature*> objects;ObjectGuid nextGuid=100;
struct Player : Unit {
 ObjectGuid guid;bool alive=true,teleport=false,flight=false,vehicle=false,samePhase=true;uint32 map=1220,area=7786;float distance=5;unsigned casts=0;
 std::map<uint32,int> quests;std::set<uint32> rewards{39661};std::map<uint32,std::vector<ObjectGuid>> summons;
 explicit Player(ObjectGuid id):guid(id){} Player* ToPlayer()override{return this;}
 bool IsAlive(){return alive;}uint32 GetMapId(){return map;}uint32 GetAreaId(){return area;}bool IsBeingTeleported(){return teleport;}bool isInFlight(){return flight;}bool GetVehicle(){return vehicle;}
 int GetQuestStatus(uint32 q){return quests[q];}bool GetQuestRewardStatus(uint32 q){return rewards.count(q)!=0;}
 ObjectGuid GetGUID(){return guid;}auto GetSummonList(uint32 entry){return &summons[entry];}
 float GetDistance(Creature*){return distance;}bool InSamePhase(Creature*){return samePhase;}
 TempSummon* Add(){auto c=new TempSummon(++nextGuid,this);objects[c->guid]=c;summons[96038].push_back(c->guid);return c;}
 void CastSpell(Player* p,uint32 spell,bool triggered){assert(p==this&&spell==190370&&triggered);++casts;Add();}
};
struct ObjectAccessor {static Creature* GetCreature(Player&,ObjectGuid id){auto i=objects.find(id);return i!=objects.end()&&!i->second->gone?i->second:nullptr;}};
struct Quest {uint32 id;uint32 GetQuestId()const{return id;}};
struct SmartAI {
 Creature* me;unsigned talks=0;explicit SmartAI(Creature* c):me(c){}virtual ~SmartAI()=default;
 virtual void IsSummonedBy(Unit*){}virtual void UpdateAI(uint32){++me->smartUpdates;}virtual void sQuestAccept(Player*,Quest const*){++me->smartAccepts;}void Talk(int,ObjectGuid){++talks;}
};
struct PlayerScript {explicit PlayerScript(char const*){}virtual ~PlayerScript()=default;virtual void OnLogout(Player*){}virtual void OnMapChanged(Player*){}virtual void OnUpdate(Player*,uint32){}};
struct Hook {void operator+=(int){}};
struct SpellScript {Unit* caster=nullptr;Hook OnCheckCast;virtual ~SpellScript()=default;Unit* GetCaster(){return caster;}virtual void Register(){}};
#define PrepareSpellScript(...) public:
#define SpellCheckCastFn(...) 0
#define RegisterSpellScript(...)
#define RegisterCreatureAI(...)
#include "LifespringHandlers.inc"
int main()
{
 player_lifespring_companion service;Player a(1),b(2);assert(!Lifespring::Wanted(&a));
 service.OnUpdate(&a,1);assert(a.summons.empty()); // no per-player allocations outside this content
 a.quests[39489]=QUEST_STATUS_INCOMPLETE;assert(Lifespring::Wanted(&a)); // either parallel quest first
 service.OnUpdate(&a,1);assert(a.casts==1);auto first=Lifespring::Existing(&a);assert(first);
 for(int i=0;i<100;++i)service.OnUpdate(&a,10);assert(a.casts==1);
 spell_lifespring_companion spell;spell.caster=&a;assert(spell.Check()==SPELL_FAILED_DONT_REPORT);
 auto duplicate=a.Add();assert(Lifespring::Existing(&a)==first&&duplicate->gone);
 b.quests[39488]=QUEST_STATUS_INCOMPLETE;service.OnUpdate(&b,1000);auto other=Lifespring::Existing(&b);
 a.summons[96038].push_back(other->guid); // defensive foreign GUID: must never despawn another player's summon
 a.quests[39489]=QUEST_STATUS_NONE;service.OnUpdate(&a,1);assert(first->gone&&!other->gone);
 assert(spell.Check()==SPELL_FAILED_DONT_REPORT);
 a.quests[39488]=QUEST_STATUS_INCOMPLETE;assert(spell.Check()==SPELL_CAST_OK);service.OnUpdate(&a,1000);
 a.quests[39489]=QUEST_STATUS_INCOMPLETE;a.quests[39488]=QUEST_STATUS_NONE;service.OnUpdate(&a,1000);assert(Lifespring::Existing(&a));
 a.quests[39489]=QUEST_STATUS_COMPLETE;assert(Lifespring::Wanted(&a));
 a.quests.clear();a.rewards.insert(39489);assert(Lifespring::Wanted(&a)); // reward gap, both reward orders
 a.rewards.erase(39489);a.rewards.insert(39488);assert(Lifespring::Wanted(&a));
 a.rewards.insert(39489);a.quests[39487]=QUEST_STATUS_INCOMPLETE;assert(Lifespring::Wanted(&a));
 a.quests.clear();assert(Lifespring::Wanted(&a)); // boss abandon/reaccept retains legitimate reward history
 a.rewards.insert(39487);assert(Lifespring::Wanted(&a));
 a.quests[39498]=QUEST_STATUS_COMPLETE;service.OnUpdate(&a,1000);assert(!Lifespring::Existing(&a));
 a.quests.clear();service.OnUpdate(&a,1000);assert(Lifespring::Existing(&a)); // abandon High Water
 a.rewards.insert(39498);service.OnUpdate(&a,1000);assert(!Lifespring::Wanted(&a)&&!Lifespring::Existing(&a));a.rewards.erase(39498);
 for(bool* flag:{&a.teleport,&a.flight,&a.vehicle}){*flag=true;assert(!Lifespring::Wanted(&a));*flag=false;}
 a.alive=false;assert(!Lifespring::Wanted(&a));a.alive=true;
 a.map=0;assert(!Lifespring::Wanted(&a));a.map=1220;a.area=0;assert(!Lifespring::Wanted(&a));a.area=7786;
 service.OnUpdate(&a,1000);auto follower=Lifespring::Existing(&a);npc_lifespring_companion ai(follower);ai.IsSummonedBy(&a);
 assert(follower->viewers==std::set<ObjectGuid>{a.guid}&&!follower->walk&&follower->react==REACT_PASSIVE&&follower->motion.target==&a);
 follower->motion.type=0;ai.UpdateAI(1000);assert(follower->motion.calls==2);
 a.distance=61;ai.UpdateAI(1000);assert(follower->gone);service.OnUpdate(&a,1000);follower=Lifespring::Existing(&a);assert(follower);
 npc_lifespring_companion ai2(follower);a.distance=5;a.samePhase=false;ai2.UpdateAI(1000);assert(follower->gone);a.samePhase=true;
 service.OnUpdate(&a,1000);follower=Lifespring::Existing(&a);follower->alive=false;service.OnUpdate(&a,1000);assert(follower->gone&&Lifespring::Existing(&a)!=follower);
 follower=Lifespring::Existing(&a);a.alive=false;service.OnUpdate(&a,1);assert(follower->gone);a.alive=true;service.OnUpdate(&a,1);assert(Lifespring::Existing(&a));
 service.OnMapChanged(&a);assert(!Lifespring::Existing(&a));service.OnUpdate(&a,1);assert(Lifespring::Existing(&a));service.OnLogout(&a);assert(!Lifespring::Existing(&a));
 Creature stationary(800);npc_lifespring_companion staticAI(&stationary);staticAI.UpdateAI(1000);Quest highWater{39498};staticAI.sQuestAccept(&a,&highWater);assert(stationary.smartUpdates==1&&stationary.smartAccepts==1&&!stationary.gone);
 auto invalid=a.Add();a.quests[39498]=QUEST_STATUS_COMPLETE;npc_lifespring_companion invalidAI(invalid);invalidAI.IsSummonedBy(&a);assert(invalid->gone);
 assert(!other->gone);
 for(auto item:objects)delete item.second;
 std::cout<<"PASS: extracted production summon gate, both quest orders, reward gaps, abandon/reaccept, duplicates, owner isolation, death/recovery, map/logout, follow/phase/distance and stationary AI.\n";
}
