#include <cassert>
#include <array>
#include <cstdint>
#include <functional>
#include <iostream>
#include <list>
#include <map>
#include <memory>
#include <set>
#include <vector>
using uint32=uint32_t;using uint64=uint64_t;using uint8=uint8_t;
using ObjectGuid=int;using GuidSet=std::set<ObjectGuid>;
template<std::size_t N>
#include "ConquestConditionLogic.inc"
constexpr float M_PI=3.14159265f;
enum {QUEST_STATUS_NONE=0,QUEST_STATUS_COMPLETE=1,QUEST_STATUS_INCOMPLETE=3};
struct Player;struct EventObject;struct Creature;
struct Map{int GetDifficultyID(){return 0;}};
struct Group{bool raid=false;bool isRaidGroup(){return raid;}};
struct Quest{bool IsAllowedInRaid(int){return false;}};
struct QuestStore{Quest q;Quest* GetQuestTemplate(uint32 id){assert(id==44721);return &q;}} store;auto sQuestDataStore=&store;
struct QuestStatusData{int Status=QUEST_STATUS_NONE;std::shared_ptr<uint8> ScriptLifetime;};
struct Menu{int closes=0;void SendCloseGossip(){++closes;}};
struct Player{
 int guid=1;uint32 map=1220,zone=7541;bool alive=true,phase=true,visible=true,castWorks=true;float distance=0;
 QuestStatusData status;std::map<uint32,int> objectives;Group* group=nullptr;Menu menu;Menu* PlayerTalkClass=&menu;
 int credits=0,casts=0;struct Pending{uint64 delay;std::function<void()> action;};std::vector<Pending> pending;
 bool IsAlive(){return alive;}uint32 GetMapId(){return map;}uint32 GetZoneId(){return zone;}Map* GetMap(){static Map m;return &m;}
 Group* GetGroup(){return group;}int GetQuestStatus(uint32 q){assert(q==44721);return status.Status;}
 QuestStatusData* getQuestStatus(uint32 q){assert(q==44721);return &status;}
 int GetQuestObjectiveData(uint32 q,uint32 id){assert(q==44721);return objectives[id];}
 int GetGUID(){return guid;}int GetPosition(){return 0;}
 bool InSamePhase(EventObject*){return phase;}template<class T>bool IsWithinDistInMap(T*,float range){return distance<=range;}
 bool canSeeOrDetect(Creature*){return phase&&visible;}float GetDistance(EventObject*){return distance;}
 void KilledMonsterCredit(uint32 id){assert(id==114964||id==114972);if(status.Status!=3)return;++credits;objectives[id]=1;
  if(objectives[114964]>=1&&objectives[115291]>=4&&objectives[114957]>=1&&objectives[114972]>=1)status.Status=1;}
 void CastSpell(Player* p,uint32 id,bool trigger){assert(p==this&&trigger);++casts;if(castWorks)KilledMonsterCredit(id==228873?114964:114972);}
 void AddDelayedEvent(uint64 delay,std::function<void()>&& action){pending.push_back({delay,std::move(action)});}
 void AddQuestDelayedEvent(uint32,uint64,std::function<void()>&&);
 void Tick(uint64 diff){auto events=std::move(pending);pending.clear();for(auto& e:events){if(e.delay<=diff)e.action();else{e.delay-=diff;pending.push_back(std::move(e));}}}
 void Accept(){status={};status.Status=3;objectives.clear();}void Abandon(){status={};objectives.clear();}
 void TeleportTo(uint32,float,float,float,float){assert(false);}
};
#include "ConquestQuestDelay.inc"
struct Creature{uint32 map=1220,area=7803;int guid=92539;bool exists=true;uint32 GetMapId(){return map;}uint32 GetAreaId(){return area;}int GetGUID(){return guid;}};
std::map<int,Player*> players;Creature* actor=nullptr;
namespace ObjectAccessor{Player* GetPlayer(EventObject&,int guid){auto i=players.find(guid);return i==players.end()?nullptr:i->second;}
 Creature* GetCreature(Player& p,int guid){return actor&&actor->exists&&actor->guid==guid&&actor->map==p.map?actor:nullptr;}}
struct SmartAI{Creature* me;int selections=0,resets=0;explicit SmartAI(Creature* c):me(c){}virtual ~SmartAI()=default;
 virtual void Reset(){++resets;}virtual void UpdateAI(uint32){}virtual void sGossipSelect(Player*,uint32,uint32){++selections;}};
struct EventObjectTemplate{float radius=100;uint32 SpellID=0,WorldSafeLocID=0,Flags=0;};
struct EventObject{EventObjectTemplate data;EventObjectTemplate const* eventTemplate=&data;bool m_isUpdate=false;GuidSet affectedPlayers,affectedPermamentPlayers;
 uint32 entry=373,map=1220;uint64 db=410124;uint32 GetEntry(){return entry;}uint32 GetMapId(){return map;}uint64 GetDBTableGUIDLow(){return db;}
 bool ActivatedForPlayer(Player* p){return affectedPlayers.count(p->guid)||affectedPermamentPlayers.count(p->guid);}
 void Update(uint32);
};
struct EventObjectScript{explicit EventObjectScript(char const*){}virtual bool IsTriggerMeets(Player*,EventObject*){return true;}virtual bool OnTrigger(Player*,EventObject*,bool){return false;}};
enum class HighGuid{Conversation};struct Generator{int Generate(){return 1;}};
struct ObjectMgr{template<HighGuid>Generator* GetGenerator(){static Generator g;return &g;}} objectMgr;auto sObjectMgr=&objectMgr;
struct Conversation{static inline int calls=0;static inline bool works=true;bool CreateConversation(int,int id,Player*,void*,int){assert(id==3880);++calls;return works;}};
#include "ConquestProduction.inc"
eventobject_stormheim_conquest arrival;
struct ScriptMgr{bool IsEOMeetConditions(Player* p,EventObject* e){return arrival.IsTriggerMeets(p,e);}void OnEventObject(Player* p,EventObject* e,bool enter){arrival.OnTrigger(p,e,enter);}} scripts;auto sScriptMgr=&scripts;
void GetPlayerListInGrid(std::list<Player*>& list,float radius){for(auto& pair:players)if(pair.second->distance<=radius)list.push_back(pair.second);}
struct WorldSafeLocsEntry{uint32 MapID;struct{float X,Y,Z,O;}Loc;};struct SafeStore{WorldSafeLocsEntry const* LookupEntry(uint32){return nullptr;}} sWorldSafeLocsStore;
constexpr int EO_FLAG_ONE_ACTION=1;
#include "ConquestEventUpdate.inc"
void Ready(Player& p){p.Accept();p.objectives={{114964,1},{115291,4},{114957,1}};}
int main(){
 for(bool stormheim:{false,true})for(bool conquest:{false,true}){std::array<bool,4> rewarded{stormheim,conquest,true,true};assert(PlayerConditionLogic(1,rewarded)==(stormheim&&conquest));}
 Player p;EventObject eo;players={{p.guid,&p}};eo.Update(1);assert(eo.affectedPlayers.empty()&&!p.credits);
 // Native affected-player admission must allow stationary acceptance and retry.
 p.Accept();p.castWorks=false;Conversation::works=false;eo.Update(1);assert(p.objectives[114964]==1&&p.credits==1&&eo.ActivatedForPlayer(&p));
 for(int i=0;i<10;++i)eo.Update(1);assert(p.credits==1&&Conversation::calls==1);
 p.Abandon();p.Accept();eo.Update(1);assert(!p.credits||!p.objectives[114964]);
 // Reaccepting while already affected needs leaving/reentering, as in native EO.
 p.distance=101;eo.Update(1);p.distance=0;eo.Update(1);assert(p.objectives[114964]==1);
 for(int blocked=0;blocked<6;++blocked){Player other;other.guid=blocked+2;other.Accept();EventObject e;
  if(blocked==0)other.alive=false;if(blocked==1)other.phase=false;if(blocked==2)other.map=0;
  if(blocked==3)other.zone=7502;if(blocked==4)other.status.Status=1;Group raid;if(blocked==5){raid.raid=true;other.group=&raid;}
  players={{other.guid,&other}};e.Update(1);assert(!other.credits&&!e.ActivatedForPlayer(&other));
 }
 Creature havi;actor=&havi;npc_havi_conquest ai(&havi);Player owner,other;Ready(owner);Ready(other);other.guid=22;
 // All three prerequisites required even for a direct/stale gossip selection.
 for(uint32 missing:{114964u,115291u,114957u}){Player wrong;Ready(wrong);wrong.objectives[missing]=0;ai.sGossipSelect(&wrong,20539,1);assert(!ai.busy&&wrong.pending.empty());}
 ai.sGossipSelect(&owner,20539,1);assert(ai.selections==1&&owner.pending.size()==1);
 ai.sGossipSelect(&other,20539,1);assert(other.pending.empty()); // no overwrite/credit theft
 owner.Tick(15999);assert(!owner.objectives[114972]);owner.Tick(3000);assert(!owner.objectives[114972]);
 owner.castWorks=false;owner.Tick(1);assert(owner.objectives[114972]==1&&owner.status.Status==1&&!other.objectives[114972]);
 owner.Tick(10000);assert(owner.objectives[114972]==1);ai.UpdateAI(22000);ai.sGossipSelect(&other,20539,1);other.Tick(19000);assert(other.status.Status==1);
 // Safe cancellation: abandon/reaccept, reset, actor unload, death, map and range.
 for(int fail=0;fail<7;++fail){ai.Reset();Player retry;Ready(retry);ai.sGossipSelect(&retry,20539,1);
  if(fail==0){retry.Abandon();Ready(retry);}if(fail==1)ai.Reset();if(fail==2)havi.exists=false;
  if(fail==3)retry.alive=false;if(fail==4)retry.map=0;if(fail==5)retry.distance=101;if(fail==6)retry.phase=false;
  retry.Tick(19000);assert(!retry.objectives[114972]);havi.exists=true;
 }
 std::cout<<"PASS: extracted arrival/gossip code, native EventObject::Update and quest-owned delay; early entry, failed optional casts/conversation, per-player credit,19s completion, competing invokers, invalid prerequisites, abandonment/reaccept, reset, unloaded actor, death/map/phase/range. World/credit writes are adapters.\n";
}
