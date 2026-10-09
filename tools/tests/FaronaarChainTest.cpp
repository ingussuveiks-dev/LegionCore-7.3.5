#include <cassert>
#include <cmath>
#include <cstdint>
#include <iostream>
#include <map>
#include <memory>
#include <set>
#include <string>
#include <tuple>
#include <vector>
using uint32=uint32_t;using uint64=uint64_t;
struct ObjectGuid { unsigned value=0;void Clear(){value=0;}bool operator<(ObjectGuid b)const{return value<b.value;}bool operator==(ObjectGuid b)const{return value==b.value;} };
using GuidList=std::vector<ObjectGuid>;
enum { QUEST_STATUS_INCOMPLETE=1,QUEST_STATUS_COMPLETE=2,QUEST_OBJECTIVE_GAMEOBJECT=2,TEMPSUMMON_TIMED_DESPAWN=1,REACT_PASSIVE=0,FOLLOW_MOTION_TYPE=4 };
struct Position { float x=0,y=0,z=0,o=0;float GetPositionX()const{return x;}float GetPositionY()const{return y;}float GetPositionZ()const{return z;} };
struct Player;struct Creature;struct TempSummon;
struct Unit {
 ObjectGuid guid;Position pos;unsigned map=1220;bool alive=true;
 virtual ~Unit()=default;ObjectGuid GetGUID()const{return guid;}bool IsAlive()const{return alive;}
 float GetDistance(Position p){return std::sqrt((pos.x-p.x)*(pos.x-p.x)+(pos.y-p.y)*(pos.y-p.y)+(pos.z-p.z)*(pos.z-p.z));}
 bool IsWithinDistInMap(Unit* u,float r){return u && map==u->map && GetDistance(u->pos)<=r;}
 Position GetPosition(){return pos;}float GetPositionX(){return pos.x;}float GetPositionY(){return pos.y;}float GetPositionZ(){return pos.z;}
};
struct Motion { unsigned type=0,moves=0;void MovePoint(unsigned,float,float,float){++moves;}void MoveFollow(Player*,float,float){type=FOLLOW_MOTION_TYPE;++moves;}unsigned GetCurrentMovementGeneratorType(){return type;} };
struct CreatureAI { unsigned releases=0;void SetData(unsigned,unsigned){++releases;} };
struct Creature:Unit {
 unsigned entry=0,expiry=0;ObjectGuid owner,viewer;bool removed=false,flying=false,combat=false;Motion motion;CreatureAI ai;ObjectGuid beamTarget;
 virtual TempSummon* ToTempSummon(){return nullptr;}CreatureAI* AI(){return &ai;}
 void DespawnOrUnsummon(unsigned delay=0){if(delay)expiry=delay;else removed=true;}
 void SetReactState(int){}void SetDisableGravity(bool){}void SetCanFly(bool v){flying=v;}bool isInCombat(){return combat;}
 Motion* GetMotionMaster(){return &motion;}void AddPlayerInPersonnalVisibilityList(ObjectGuid id){viewer=id;}
 void CastSpell(Creature* c,unsigned id,bool){assert(id==65612);beamTarget=c->guid;}void RemoveAurasDueToSpell(unsigned){}
};
struct TempSummon:Creature { TempSummon* ToTempSummon()override{return this;}ObjectGuid GetSummonerGUID(){return owner;} };
static std::map<ObjectGuid,TempSummon> creatures;static unsigned serial=100;
struct GameObject:Unit { unsigned entry=0;uint64 spawn=0;unsigned GetEntry(){return entry;}uint64 GetDBTableGUIDLow(){return spawn;}
 Creature* FindNearestCreature(unsigned id,float range,bool){for(auto& pair:creatures)if(pair.second.entry==id && pair.second.alive && !pair.second.removed && IsWithinDistInMap(&pair.second,range))return &pair.second;return nullptr;}
};
struct Player:Unit {
 Player(){guid={++serial};}unsigned zone=7334;bool key=false,los=true,failedSpawn=false,flight=false,flying=false,boarded=false;
 std::map<unsigned,int> quests;std::map<std::pair<unsigned,unsigned>,unsigned> objectives;std::set<unsigned> rewarded;
 std::map<unsigned,GuidList> summons;std::vector<unsigned> credits;
 unsigned GetGUIDLow(){return guid.value;}unsigned GetMapId(){return map;}unsigned GetZoneId(){return zone;}
 int GetQuestStatus(unsigned q){return quests[q];}bool GetQuestRewardStatus(unsigned q){return rewarded.count(q)!=0;}
 unsigned GetQuestObjectiveData(unsigned q,unsigned obj){return objectives[{q,obj}];}
 bool HasItemCount(unsigned id,unsigned){return id==120359 && key;}bool IsWithinLOSInMap(Unit*){return los;}
 GuidList* GetSummonList(unsigned id){return &summons[id];}bool isInFlight(){return flight;}bool IsFlying(){return flying;}void* GetVehicle(){return boarded?this:nullptr;}
 void KilledMonsterCredit(unsigned id,ObjectGuid={}){credits.push_back(id);unsigned q=(id==90487?37656:(id==90546||id==105635?37450:37449));++objectives[{q,id}];}
 void QuestObjectiveSatisfy(unsigned id,unsigned,unsigned,ObjectGuid){credits.push_back(id);++objectives[{37450,id}];if(objectives[{37450,id}]==3)quests[37450]=QUEST_STATUS_COMPLETE;}
 TempSummon* SummonCreature(unsigned id,Position where,int,unsigned duration,int,ObjectGuid viewer){if(failedSpawn)return nullptr;ObjectGuid next{++serial};auto& c=creatures[next];c.guid=next;c.entry=id;c.pos=where;c.map=map;c.owner=guid;c.viewer=viewer;c.expiry=duration;summons[id].push_back(next);return &c;}
 void CastSpell(float x,float y,float z,unsigned id,bool){assert(id==178860);SummonCreature(90474,{x,y,z,0},1,0,0,guid);}
 Creature* FindNearestCreature(unsigned id,float r){for(auto& pair:creatures)if(!pair.second.removed && pair.second.entry==id && IsWithinDistInMap(&pair.second,r))return &pair.second;return nullptr;}
};
struct ObjectAccessor { static Creature* GetCreature(Player& p,ObjectGuid id){auto it=creatures.find(id);return it==creatures.end()||it->second.removed||it->second.map!=p.map?nullptr:&it->second;} };
struct ScriptedAI { Creature* me;ScriptedAI(Creature* c):me(c){} };
struct PlayerScript {PlayerScript(char const*){}virtual ~PlayerScript()=default;virtual void OnLogout(Player*){}virtual void OnMapChanged(Player*){}virtual void OnUpdate(Player*,unsigned){} };
struct GameObjectScript {GameObjectScript(char const*){}virtual ~GameObjectScript()=default;virtual bool OnGossipHello(Player*,GameObject*){return false;} };
#define RegisterCreatureAI(x) ((void)0)
struct Field { uint64 value=0;uint32 GetUInt32(){return uint32(value);}uint64 GetUInt64(){return value;} };
struct Result {std::vector<std::vector<Field>> rows;size_t cursor=0;Field* Fetch(){return rows[cursor].data();}bool NextRow(){return ++cursor<rows.size();} };
using QueryResult=std::shared_ptr<Result>;
struct Database {
 std::map<std::tuple<unsigned,unsigned,uint64>,unsigned> rows;
 QueryResult PQuery(char const*,unsigned guid){auto result=std::make_shared<Result>();for(auto& row:rows)if(std::get<0>(row.first)==guid)result->rows.push_back({{std::get<1>(row.first)},{std::get<2>(row.first)},{row.second}});return result->rows.empty()?nullptr:result;}
 void DirectPExecute(char const* sql,unsigned guid,unsigned quest,unsigned long long spawn,unsigned ordinal=0){auto key=std::make_tuple(guid,quest,uint64(spawn));if(std::string(sql).find("INSERT")==0){assert(!rows.count(key));rows[key]=ordinal;}else rows.erase(key);}
} CharacterDatabase;

#include "../../src/server/scripts/Legion/faronaar_chain.cpp"

int main()
{
 using namespace Faronaar;
 Player p,other;p.pos=BoundDragon;other.pos=BoundDragon;
 player_faronaar_chain lifecycle;go_faronaar_objective script;
 p.quests[Saving]=QUEST_STATUS_INCOMPLETE;other.quests[Saving]=QUEST_STATUS_INCOMPLETE;
 lifecycle.OnUpdate(&p,1000);lifecycle.OnUpdate(&other,1000);
 assert(Owned(&p,90546) && Owned(&other,90546) && Owned(&p,90546)!=Owned(&other,90546));
 assert(Done(&p,Saving,90546));
 for(auto& pair:creatures)if(pair.second.entry==90578)assert(creatures[pair.second.beamTarget].owner==pair.second.owner);
 GameObject lock;lock.entry=239455;lock.spawn=Chains[0].spawn;lock.pos=Chains[0].beam;p.pos=lock.pos;
 script.OnGossipHello(&p,&lock);assert(Count(&p,Saving)==0); // key missing
 p.key=true;p.los=false;script.OnGossipHello(&p,&lock);assert(Count(&p,Saving)==0);p.los=true;
 p.pos.x+=20;script.OnGossipHello(&p,&lock);assert(Count(&p,Saving)==0);p.pos=lock.pos;
 script.OnGossipHello(&p,&lock);assert(Count(&p,Saving)==1 && Done(&p,Saving,105635));
 script.OnGossipHello(&p,&lock);assert(Count(&p,Saving)==1 && Count(&other,Saving)==0);
 lifecycle.OnLogout(&p);lifecycle.OnUpdate(&p,1000);script.OnGossipHello(&p,&lock);assert(Count(&p,Saving)==1); // persisted distinct use
 for(unsigned i=1;i<3;++i){lock.spawn=Chains[i].spawn;lock.pos=Chains[i].beam;p.pos=lock.pos;script.OnGossipHello(&p,&lock);}
 assert(Count(&p,Saving)==3 && states[p.guid].released && Owned(&p,90546)->motion.moves==1);
 assert(Count(&other,Saving)==0 && Owned(&other,90546)); // no shared despawn or credit
 p.quests[Saving]=0;lifecycle.OnUpdate(&p,1000);assert(!Owned(&p,90546));
 p.quests[Saving]=QUEST_STATUS_INCOMPLETE;p.objectives[{Saving,239455}]=0;p.pos=BoundDragon;
 lifecycle.OnUpdate(&p,1000);lock.spawn=Chains[0].spawn;lock.pos=Chains[0].beam;p.pos=lock.pos;script.OnGossipHello(&p,&lock);assert(Count(&p,Saving)==1); // abandon/reaccept
 // Reconcile an interaction saved just before a character-progress rollback.
 p.objectives[{Saving,239455}]=0;lifecycle.OnLogout(&p);lifecycle.OnUpdate(&p,1000);script.OnGossipHello(&p,&lock);assert(Count(&p,Saving)==1);
 GameObject crystal;crystal.entry=240075;crystal.spawn=109119;crystal.pos=p.pos;
 p.quests[Rescue]=QUEST_STATUS_INCOMPLETE;script.OnGossipHello(&p,&crystal);assert(Count(&p,Rescue)==0); // captive absent
 Creature* captive=p.SummonCreature(90487,p.pos,1,10000,0,p.guid);
 script.OnGossipHello(&p,&crystal);script.OnGossipHello(&p,&crystal);assert(Count(&p,Rescue)==1 && captive->ai.releases==1);
 lifecycle.OnLogout(&p);lifecycle.OnUpdate(&p,1000);script.OnGossipHello(&p,&crystal);assert(Count(&p,Rescue)==1);
 p.rewarded.insert(36920);lifecycle.OnUpdate(&p,1000);Creature* follower=Owned(&p,90474);assert(follower && follower->motion.type==FOLLOW_MOTION_TYPE);
 unsigned moves=follower->motion.moves;lifecycle.OnUpdate(&p,1000);assert(follower->motion.moves==moves);
 lifecycle.OnLogout(&p);assert(!Owned(&p,90474));lifecycle.OnUpdate(&p,1000);assert(Owned(&p,90474));
 p.quests[Revelations]=QUEST_STATUS_INCOMPLETE;p.pos=Traitor;lifecycle.OnUpdate(&p,1000);assert(!Owned(&p,90982));
 p.objectives[{Revelations,90621}]=1;p.failedSpawn=true;lifecycle.OnUpdate(&p,1000);assert(!Owned(&p,90982));p.failedSpawn=false;
 lifecycle.OnUpdate(&p,1000);assert(Owned(&p,90982) && !p.boarded && !Done(&p,Revelations,90982) && !Done(&p,Revelations,112175));
 p.pos={-103.434f,6961.33f,12.5469f,0};Creature* leader=p.SummonCreature(89362,p.pos,1,30000,0,p.guid);assert(leader);
 p.boarded=true;lifecycle.OnUpdate(&p,1000);assert(!Done(&p,Revelations,112175));p.boarded=false;
 p.flying=true;lifecycle.OnUpdate(&p,1000);assert(!Done(&p,Revelations,112175));p.flying=false;
 p.los=false;lifecycle.OnUpdate(&p,1000);assert(!Done(&p,Revelations,112175));p.los=true;
 p.objectives[{Revelations,90621}]=0;lifecycle.OnUpdate(&p,1000);assert(!Done(&p,Revelations,112175));
 p.objectives[{Revelations,90621}]=1;lifecycle.OnUpdate(&p,1000);assert(Done(&p,Revelations,112175));
 p.quests[Revelations]=QUEST_STATUS_COMPLETE;lifecycle.OnUpdate(&p,1000);assert(!Owned(&p,90474) && !Owned(&p,90982));
 other.alive=false;lifecycle.OnUpdate(&other,1000);assert(!Owned(&other,90546));other.alive=true;lifecycle.OnUpdate(&other,1000);assert(Owned(&other,90546));
 other.map=1;lifecycle.OnMapChanged(&other);assert(!states.count(other.guid));other.map=1220;other.pos.x+=300;lifecycle.OnUpdate(&other,1000);assert(!Owned(&other,90546));
 std::cout<<"PASS: complete production Faronaar handlers: per-player beams/credits, distinct persistent uses, rollback/reaccept, key/range/LOS, reconnect, optional ride and real return.\n";
}
