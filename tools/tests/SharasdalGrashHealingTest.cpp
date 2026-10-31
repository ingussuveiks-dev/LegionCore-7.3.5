#include <algorithm>
#include <cassert>
#include <atomic>
#include <cstdint>
#include <functional>
#include <iostream>
#include <map>
#include <memory>
#include <mutex>
#include <set>
#include <vector>
using uint8=uint8_t;using uint32=uint32_t;using uint64=uint64_t;using int32=int32_t;using int64=int64_t;using DamageEffectType=uint32;
enum{CLASS_SHAMAN=7,QUEST_STATUS_NONE=0,QUEST_STATUS_COMPLETE=1,QUEST_STATUS_INCOMPLETE=3,CRITERIA_TYPE_SCRIPT_EVENT_2=92,UNIT_FIELD_NPC_FLAGS=1,UNIT_STATE_STUNNED=8,UNIT_STATE_CASTING=16,UNIT_NPC_FLAG_GOSSIP=1,GOSSIP_SENDER_MAIN=1,GOSSIP_ACTION_INFO_DEF=1000,WAYPOINT_MOTION_TYPE=2,POINT_MOTION_TYPE=3,EVENT_1=1,EVENT_2=2};
constexpr float INTERACTION_DISTANCE=5;
#include "Functions.inc"
struct Unit;struct Player;struct Creature;
struct ObjectGuid{uint64 high=0,low=0;bool IsPlayer()const{return high==1;}bool IsEmpty()const{return !high&&!low;}void Clear(){high=low=0;}explicit operator bool()const{return !IsEmpty();}bool operator==(ObjectGuid other)const{return high==other.high&&low==other.low;}};
struct Map{}liveMap,otherMap;
struct CreatureAI{
 virtual uint32 GetData(uint32)const{return 0;}
 virtual ~CreatureAI()=default;virtual void HealReceived(Unit*,uint32&){}virtual void HealDone(Unit*,uint32&){}virtual void OnHealApplied(Unit*,uint32,int32){}virtual void UpdateLifecycle(uint32){}virtual void Reset(){}virtual void EnterCombat(Unit*){}virtual void DoAction(int32){}virtual void DamageTakenAfterScaling(Unit*,uint32&,DamageEffectType){}virtual void MovementInform(uint32,uint32){}virtual void UpdateAI(uint32){}virtual void sGossipSelect(Player*,uint32,uint32){}
 std::vector<uint32> speech;void Talk(uint32 line){speech.push_back(line);}void AttackStart(Unit*){}
};
struct InstanceScript{bool CheckDamageManager()const{return false;}void UpdateDamageManager(ObjectGuid,int32,bool){}void HandleGameObject(ObjectGuid,bool){}uint32 getScenarionStep()const;};
namespace WorldPackets::Combat{struct HealthUpdate{ObjectGuid Guid;uint64 Health=0;int Write(){return 0;}};}
struct Unit{
 virtual ~Unit()=default;uint64 health=1,maximum=1000;bool IsAIEnabled=false,world=true,alive=true,phase=true;Map* map=&liveMap;ObjectGuid guid{2,1},owner,charmer;CreatureAI* healAI=nullptr;FunctionProcessor functions;
 virtual Player const* ToPlayer()const{return nullptr;}Player* ToPlayer(){return const_cast<Player*>(static_cast<Unit const*>(this)->ToPlayer());}
 virtual Creature* ToCreature(){return nullptr;}virtual CreatureAI* GetAI(){return healAI;}
 ObjectGuid GetGUID()const{return guid;}ObjectGuid GetCharmerGUID()const{return charmer;}ObjectGuid GetOwnerGUID()const{return owner;}
#include "OwnerGuid.inc"
 Player* GetCharmerOrOwnerPlayerOrPlayerItself()const;
 bool IsInWorld()const{return world;}bool IsAlive()const{return alive;}Map* GetMap()const{return map;}uint32 GetInstanceId()const{return 7;}
 bool InSamePhase(Unit* target)const{return phase&&target->phase;}
 uint64 GetHealth(Unit* =nullptr)const{return health;}uint64 GetMaxHealth(Unit* =nullptr)const{return maximum;}float GetHealthPct()const{return maximum?100.f*float(health)/float(maximum):0;}
 void SetHealth(uint64 value,uint32){health=value;}void SetHealthScal(uint64 value,Unit*,uint32){health=value;}
 InstanceScript* GetInstanceScript(){static InstanceScript instance;return &instance;}
 void AddDelayedEvent(uint64 delay,std::function<void()>&& callback){functions.AddDelayedEvent(delay,std::move(callback));}
 int64 ModifyHealth(int64,Unit* =nullptr,uint32=0);int32 DealHeal(Unit*,uint32);
};
enum class GossipOptionNpc:uint8{None,Other};
struct GossipMenuItem{GossipOptionNpc OptionNpc=GossipOptionNpc::None;bool IsCoded=false;uint32 Sender=1,Action=1001;};
struct GossipMenu{uint32 id=0;std::vector<GossipMenuItem> items;void SetMenuId(uint32 value){id=value;}uint32 GetMenuId()const{return id;}GossipMenuItem const* GetItem(uint32 i)const{return i<items.size()?&items[i]:nullptr;}};
struct PlayerMenu{GossipMenu menu;void ClearMenus(){menu.items.clear();menu.id=0;}GossipMenu& GetGossipMenu(){return menu;}};
struct QuestStatusData{uint32 Status=3;std::shared_ptr<uint8> ScriptLifetime;};
struct Player:Unit{
 QuestStatusData quest;bool record=true,contact=true,gm=false,teleport=false,near=true,interact=true,los=true,detect=true,rewarded=false;uint32 classId=7,closed=0,sent=0,sentMenu=0,sentText=0;PlayerMenu talk;PlayerMenu* PlayerTalkClass=&talk;std::map<uint32,uint32> credits;std::function<void(uint32)> onCredit;
 Player(){guid={1,1};}Player const* ToPlayer()const override{return this;}
 bool CanContact()const{return contact;}bool isGameMaster()const{return gm;}bool IsBeingTeleported()const{return teleport;}uint32 getClass()const{return classId;}
 bool IsWithinDistInMap(Unit*,float range)const{assert(range==100||range==5);return range==5?interact:near;}
 bool canSeeOrDetect(Unit*)const{return detect;}bool IsWithinLOSInMap(Unit*)const{return los;}
 bool GetQuestRewardStatus(uint32 questId)const{assert(questId==40341);return rewarded;}uint32 GetQuestStatus(uint32 questId)const{assert(questId==40341);return record?quest.Status:0;}
 QuestStatusData* getQuestStatus(uint32 questId){assert(questId==40341);return record?&quest:nullptr;}
 void AddQuestDelayedEvent(uint32,uint64,std::function<void()>&&);void UpdateAchievementCriteria(uint32,uint32);
 void ADD_GOSSIP_ITEM_DB(uint32,uint32 option,uint32 sender,uint32 action){assert(!option);talk.menu.items.push_back({GossipOptionNpc::None,false,sender,action});}
 void ADD_GOSSIP_ITEM(GossipOptionNpc icon,char const*,uint32 sender,uint32 action){talk.menu.items.push_back({icon,false,sender,action});}
 void SEND_GOSSIP_MENU(uint32 text,ObjectGuid){++sent;sentMenu=talk.menu.id;sentText=text;}
 void CLOSE_GOSSIP_MENU(){++closed;}template<class Packet>void SendDirectMessage(Packet const&){}
};
struct Motion{std::vector<uint32> paths,points;void MovePath(uint32 id,bool,int=0,int=0){paths.push_back(id);}void MovePoint(uint32 id,float,float,float){points.push_back(id);}};
struct Creature:Unit{
 bool preDelete=false,m_AI_locked=false;uint32 entry=105298,mapId=1600,flags=1,states=0,removals=0;float z=245;CreatureAI* primary=nullptr;CreatureAI* i_disabledAI=nullptr;Creature* erunak=nullptr;Creature* adel=nullptr;Player* nearest=nullptr;Motion motion;
 Creature(){IsAIEnabled=true;}Creature* ToCreature()override{return this;}CreatureAI* AI(){return primary;}CreatureAI* GetAI()override{return primary;}
 bool IsPreDelete()const{return preDelete;}uint32 GetEntry()const{return entry;}uint32 GetMapId()const{return mapId;}
 bool HasFlag(uint32,uint32 flag)const{return (flags&flag)!=0;}void RemoveFlag(uint32,uint32 flag){flags&=~flag;}void SetUInt32Value(uint32,uint32 value){flags=value;}
 bool HasUnitState(uint32 state)const{return (states&state)!=0;}void AddUnitState(uint32 state){states|=state;}void ClearUnitState(uint32 state){states&=~state;}
 void AddAura(uint32 spell,Creature* target){assert(spell==208093&&target==this);}void RemoveAura(uint32 spell){assert(spell==208093);++removals;}
 float GetPositionX()const{return 0;}float GetPositionY()const{return 0;}float GetPositionZ()const{return z;}float GetOrientation()const{return 0;}void SetHomePosition(float,float,float,float){}
 Creature* FindNearestCreature(uint32 id,float,bool=true){return id==102826?erunak:id==104853?adel:nullptr;}
 Player* FindNearestPlayer(float){return nearest;}struct GO{ObjectGuid GetGUID()const{return {};}};GO* FindNearestGameObject(uint32,float){return nullptr;}
 Motion* GetMotionMaster(){return &motion;}void NotifyHealApplied(Unit*,uint32,int32);
 void Tick(uint32 diff=0){CreatureAI* i_AI=primary;
#include "Lifecycle.inc"
 }
};
using GameObject=Creature::GO;
struct ObjectAccessor{inline static std::vector<Player*> players;static Player* GetPlayer(Unit const& source,ObjectGuid id){for(auto player:players)if(player->guid==id&&player->world&&player->map==source.map)return player;return nullptr;}};
#include "Owner.inc"
#include "Quest.inc"
#include "ModifyHealth.inc"
#include "Notify.inc"
#include "DealHeal.inc"
struct CriteriaProgress{uint64 Counter=0;};
struct AchievementMgr{std::map<uint32,CriteriaProgress> progress;auto const* GetCriteriaProgressMap()const{return &progress;}};
struct Scenario{uint32 id=1066,step=0;AchievementMgr achievements;std::set<uint32> rejected;uint32 GetScenarioId()const{return id;}uint32 GetCurrentStep()const{return step;}AchievementMgr& GetAchievementMgr(){return achievements;}} scenario;
struct ScenarioMgr{Scenario* active=&scenario;Scenario* GetScenario(uint32 id){assert(id==7);return active;}} manager;
auto sScenarioMgr=&manager;
uint32 InstanceScript::getScenarionStep()const{return scenario.step;}
void Player::UpdateAchievementCriteria(uint32 type,uint32 asset){assert(type==92);++credits[asset];if(!scenario.rejected.count(asset)){uint32 leaf=asset==49815?47121:asset==49900?47180:asset==50161?47303:asset==49819?47133:asset==49817?47127:0;assert(leaf);++scenario.achievements.progress[leaf].Counter;}if(onCredit)onCredit(asset);}
using WaypointPath=std::vector<uint32>;
struct WaypointMgr{std::set<uint32> missing;WaypointPath nodes{1};WaypointPath const* GetPathScript(uint32 path){return missing.count(path)?nullptr:&nodes;}} paths;
auto sWaypointMgr=&paths;
uint32 warnings=0;
#define TC_LOG_WARN(...) (++warnings)
struct EventMap{void Reset(){}void RescheduleEvent(uint32,uint32){}void Update(uint32){}uint32 ExecuteEvent(){return 0;}};
int irand(int,int){return 0;}
struct ScriptedAI:CreatureAI{Creature* me;explicit ScriptedAI(Creature* actor):me(actor){actor->primary=this;}bool UpdateVictim(){return false;}void DoCast(uint32){}void DoMeleeAttackIfReady(){}};
struct CreatureScript{explicit CreatureScript(char const*){}virtual ~CreatureScript()=default;virtual bool OnGossipHello(Player*,Creature*){return false;}virtual bool OnGossipSelect(Player*,Creature*,uint32,uint32){return false;}virtual CreatureAI* GetAI(Creature*)const{return nullptr;}};
#include "Policy.inc"
#include "Actor.inc"
struct HealerAI:CreatureAI{bool cancel=false;void HealDone(Unit*,uint32& amount)override{if(cancel)amount=0;}};
struct Run{
 Player owner,neighbor;Unit pet,guardian,npc;Creature actor,erunak;CreatureAI erunakAI;HealerAI healer;std::unique_ptr<npc_grash::npc_grashAI> ai; npc_grash script;
 Run(){scenario=Scenario{};paths.missing.clear();warnings=0;owner.guid={1,1};neighbor.guid={1,2};ObjectAccessor::players={&owner,&neighbor};pet.owner=guardian.owner=owner.guid;pet.guid={3,1};guardian.guid={3,2};actor.nearest=&neighbor;actor.erunak=&erunak;erunak.primary=&erunakAI;erunak.entry=102826;owner.IsAIEnabled=true;owner.healAI=&healer;ai=std::make_unique<npc_grash::npc_grashAI>(&actor);}
 ~Run(){ObjectAccessor::players.clear();}
 void Hello(){script.OnGossipHello(&owner,&actor);}void Select(uint32 menu=19509,uint32 index=0){ai->sGossipSelect(&owner,menu,index);script.OnGossipSelect(&owner,&actor,1,1001);}
};
int main(){
#ifdef BASELINE
 {Run r;assert(r.owner.DealHeal(&r.actor,899)==899&&r.actor.health==900&&!r.ai->_introDone);r.actor.Tick();assert(!r.owner.credits[49815]);r.owner.DealHeal(&r.actor,1);assert(r.neighbor.credits[49815]==1&&r.neighbor.credits[49900]==1&&r.neighbor.credits[50161]==1&&!r.owner.credits[49815]);}
 {Run r;r.script.OnGossipSelect(&r.neighbor,&r.actor,0,0);assert(r.neighbor.credits[50161]==1&&!r.actor.flags);}
 std::cout<<"PASS COUNTERFACTUAL e17413eaa: actual90pct crossing requires another heal; nearest unrelated player receives heal AND recruitment; translated unoffered gossip starts route. Baseline avoids deliberately freeing the raw delayed AI capture.\n";
#else
 unsigned cases=0;
 for(uint32 status:{1u,3u})for(int source=0;source<4;++source)for(int mode=0;mode<4;++mode){Run r;r.owner.quest.Status=status;Unit* healer[]={&r.owner,&r.pet,&r.guardian,&r.pet};if(source==3){r.pet.owner={};r.pet.charmer=r.owner.guid;}
  CreatureAI temporary;if(mode==1){r.actor.primary=&temporary;r.actor.i_disabledAI=r.ai.get();}if(mode==2)r.actor.i_disabledAI=r.ai.get();if(mode==3){r.actor.primary=nullptr;r.actor.i_disabledAI=r.ai.get();r.actor.IsAIEnabled=false;}
  assert(healer[source]->DealHeal(&r.actor,899)==899&&r.actor.health==900&&!r.ai->_introDone);r.actor.Tick(5000);
  assert(r.ai->_introDone&&r.ai->_introDone1&&r.owner.credits[49815]==1&&r.owner.credits[49900]==1&&!r.owner.credits[50161]&&r.neighbor.credits.empty());
  assert(r.ai->speech==std::vector<uint32>{0}&&r.erunakAI.speech.empty());r.actor.Tick(2999);assert(r.erunakAI.speech.empty());r.actor.Tick(1);assert(r.erunakAI.speech==std::vector<uint32>{1});
  healer[source]->DealHeal(&r.actor,100);r.actor.Tick(5000);assert(r.owner.credits[49815]==1&&r.erunakAI.speech.size()==1);++cases;
 }
 for(int deny=0;deny<23;++deny){Run r;switch(deny){case 0:r.owner.world=false;break;case 1:r.owner.contact=false;break;case 2:r.owner.alive=false;break;case 3:r.owner.gm=true;break;case 4:r.owner.teleport=true;break;case 5:r.owner.classId=5;break;case 6:r.actor.world=false;break;case 7:r.actor.preDelete=true;break;case 8:r.actor.alive=false;break;case 9:r.actor.entry=999;break;case 10:r.actor.mapId=1;break;case 11:r.owner.map=&otherMap;break;case 12:r.owner.phase=false;break;case 13:r.owner.detect=false;break;case 14:r.owner.los=false;break;case 15:r.owner.near=false;break;case 16:r.owner.rewarded=true;break;case 17:r.owner.record=false;break;case 18:r.owner.quest.Status=0;break;case 19:scenario.id=99;break;case 20:scenario.step=2;break;case 21:r.actor.z=797;break;case 22:manager.active=nullptr;break;}
  r.owner.DealHeal(&r.actor,899);r.actor.Tick();assert(!r.ai->_introDone&&!r.ai->_introDone1&&r.owner.credits.empty()&&r.neighbor.credits.empty());manager.active=&scenario;++cases;
 }
 {Run r;r.actor.health=1000;r.healer.cancel=true;r.owner.DealHeal(&r.actor,100);r.actor.Tick();assert(!r.ai->_introDone&&!r.ai->_introDone1);r.healer.cancel=false;assert(!r.owner.DealHeal(&r.actor,1));r.actor.Tick();assert(r.ai->_introDone);}
 {Run r;r.owner.DealHeal(&r.actor,0);r.actor.Tick();assert(!r.ai->_introDone1);r.owner.DealHeal(&r.actor,498);r.actor.Tick();assert(!r.ai->_introDone1);r.owner.DealHeal(&r.actor,1);r.actor.Tick();assert(r.ai->_introDone1&&!r.ai->_introDone);r.owner.DealHeal(&r.actor,399);r.actor.Tick();assert(!r.ai->_introDone);r.owner.DealHeal(&r.actor,1);r.actor.Tick();assert(r.ai->_introDone&&!r.owner.credits[50161]);}
 {Run r;r.owner.DealHeal(&r.actor,499);r.npc.DealHeal(&r.actor,400);r.actor.Tick();assert(r.ai->_introDone1&&!r.ai->_introDone&&r.owner.credits.empty());r.owner.DealHeal(&r.actor,1);r.actor.Tick();assert(r.ai->_introDone);}
 for(int deny=0;deny<6;++deny){Run r;r.owner.DealHeal(&r.actor,899);auto held=r.owner.quest.ScriptLifetime;switch(deny){case 0:r.owner.quest.ScriptLifetime=std::make_shared<uint8>();break;case 1:r.owner.record=false;break;case 2:r.owner.map=&otherMap;break;case 3:r.actor.health=499;break;case 4:r.ai->Reset();break;case 5:ObjectAccessor::players={&r.neighbor};break;}r.actor.Tick();assert(!r.ai->_introDone&&r.owner.credits.empty());++cases;}
 {Run r;scenario.rejected.insert(49900);r.owner.DealHeal(&r.actor,899);r.actor.Tick();assert(!r.ai->_introDone&&r.owner.credits[49815]==1&&r.owner.credits[49900]==1);r.actor.Tick(999);assert(r.owner.credits[49900]==1);r.actor.Tick(1);assert(r.owner.credits[49815]==1&&r.owner.credits[49900]==2);scenario.rejected.clear();r.actor.Tick(1000);assert(r.ai->_introDone&&r.owner.credits[49815]==1&&r.owner.credits[49900]==3);}
 {Run r;scenario.rejected.insert(49900);r.owner.DealHeal(&r.actor,899);r.actor.Tick();for(int i=0;i<35;++i)r.actor.Tick(1000);assert(warnings==1&&r.owner.credits[49900]==30&&!r.ai->_introDone);scenario.rejected.clear();r.owner.DealHeal(&r.actor,1);r.actor.Tick();assert(r.ai->_introDone&&r.owner.credits[49815]==1);}
 {Run r;r.owner.onCredit=[&](uint32){r.ai->CompleteIntroHeal(1000);r.owner.DealHeal(&r.actor,1);};r.owner.DealHeal(&r.actor,899);r.actor.Tick();assert(r.owner.credits[49815]==1&&r.owner.credits[49900]==1);}
 {Run r;r.owner.onCredit=[&](uint32 asset){if(asset==49815)r.owner.quest.ScriptLifetime=std::make_shared<uint8>();};r.owner.DealHeal(&r.actor,899);r.actor.Tick();assert(!r.ai->_introDone&&r.owner.credits[49815]==1&&!r.owner.credits[49900]);r.owner.onCredit={};r.owner.DealHeal(&r.actor,1);r.actor.Tick();assert(r.ai->_introDone&&r.owner.credits[49815]==1&&r.owner.credits[49900]==1);}
 for(int changed=0;changed<4;++changed){Run r;r.owner.DealHeal(&r.actor,899);r.actor.Tick();switch(changed){case 0:r.owner.quest.ScriptLifetime=std::make_shared<uint8>();break;case 1:r.erunak.primary=nullptr;break;case 2:r.actor.erunak=nullptr;break;case 3:r.ai->Reset();break;}r.actor.Tick(3000);assert(r.erunakAI.speech.empty());++cases;}
 {Run r;r.owner.DealHeal(&r.actor,899);r.actor.Tick();r.ai.reset();r.actor.primary=nullptr;r.actor.IsAIEnabled=false;r.actor.functions.Update(5000);assert(r.erunakAI.speech.empty());}
 for(int denied=0;denied<14;++denied){Run r;r.owner.DealHeal(&r.actor,899);r.actor.Tick();r.Hello();uint32 menu=19509,index=0;switch(denied){case 0:menu=0;break;case 1:index=1;break;case 2:r.owner.talk.menu.id=0;break;case 3:r.owner.talk.menu.items.clear();break;case 4:r.owner.talk.menu.items[0].Sender=0;break;case 5:r.owner.talk.menu.items[0].Action=0;break;case 6:r.owner.talk.menu.items[0].IsCoded=true;break;case 7:r.owner.talk.menu.items[0].OptionNpc=GossipOptionNpc::Other;break;case 8:r.actor.flags=0;break;case 9:r.actor.states=8;break;case 10:r.owner.interact=false;break;case 11:scenario.achievements.progress[47121].Counter=0;break;case 12:paths.missing.insert(10993808);break;case 13:paths.missing.insert(10993809);break;}r.Select(menu,index);r.ai->UpdateAI(1000);assert(!r.owner.credits[50161]&&r.actor.motion.paths.empty());++cases;}
 for(uint32 stage:{0u,1u,2u,4u}){
  Run r;r.owner.DealHeal(&r.actor,899);r.actor.Tick();scenario.step=stage;
  if(stage==1)scenario.achievements.progress[47303].Counter=1; // Legacy heal already recruited; do not emit it again.
  r.actor.flags=0x11;r.Hello();uint32 menu=SharasdalGrash::Menu(stage);
  assert(r.owner.sentMenu==menu&&r.owner.talk.menu.items.size()==1);
  if(stage==0){scenario.rejected.insert(50161);r.owner.onCredit=[](uint32 asset){if(asset==50161&&!scenario.rejected.count(asset))scenario.step=1;};}
  if(stage==4)scenario.rejected.insert(49819);
  r.Select(menu);
  if(!scenario.rejected.empty()){assert(r.actor.flags==0x11&&r.actor.motion.points.empty());scenario.rejected.clear();r.Select(menu);}
  assert(r.actor.flags==0x10);r.actor.flags|=1;uint32 sentBefore=r.owner.sent;r.Hello();
  assert(r.owner.sent==sentBefore&&r.owner.talk.menu.items.empty());r.Select(menu);
  r.ai->UpdateAI(999);assert(r.actor.motion.paths.empty());r.ai->UpdateAI(1);
  if(stage<=2)assert(r.actor.motion.paths==std::vector<uint32>{stage<=1?10993808u:10993810u});
  else assert(r.actor.motion.points==std::vector<uint32>{1});
  assert(r.owner.credits[50161]==(stage==0?2u:0u));++cases;
 }
 for(int source=0;source<4;++source)for(int mode=0;mode<4;++mode){
  Run r;r.actor.health=400;r.ai->wait_heal=true;r.actor.states=8;Unit* healer[]={&r.owner,&r.pet,&r.guardian,&r.npc};
  healer[source]->IsAIEnabled=true;healer[source]->healAI=&r.healer;
  CreatureAI temporary;if(mode==1){r.actor.primary=&temporary;r.actor.i_disabledAI=r.ai.get();}
  if(mode==2)r.actor.i_disabledAI=r.ai.get();
  if(mode==3){r.actor.primary=nullptr;r.actor.i_disabledAI=r.ai.get();r.actor.IsAIEnabled=false;}
  r.healer.cancel=true;healer[source]->DealHeal(&r.actor,1);r.actor.Tick();assert(r.ai->wait_heal&&r.actor.removals==0);
  r.healer.cancel=false;healer[source]->DealHeal(&r.actor,1);r.actor.Tick();assert(!r.ai->wait_heal&&r.actor.removals==1);++cases;
 }
 {Run r;auto transient=std::make_unique<Unit>();transient->owner=r.owner.guid;transient->DealHeal(&r.actor,899);transient.reset();r.actor.Tick();assert(r.ai->_introDone&&r.owner.credits[49815]==1);}
 {Run r;r.owner.DealHeal(&r.actor,899);r.actor.Tick();r.Hello();r.owner.onCredit=[&](uint32 asset){if(asset==50161)r.Select();};r.Select();r.ai->UpdateAI(1000);assert(r.owner.credits[50161]==1&&r.actor.motion.paths.size()==1);}
 {Run r;r.owner.DealHeal(&r.actor,899);r.actor.Tick();r.Hello();r.Select();uint32 lethal=1000;r.ai->DamageTakenAfterScaling(&r.pet,lethal,0);assert(!lethal&&r.ai->wait_heal);r.ai->UpdateAI(5000);assert(r.actor.motion.paths.empty());r.pet.DealHeal(&r.actor,1);r.actor.Tick();r.ai->UpdateAI(999);assert(r.actor.motion.paths.empty());r.ai->UpdateAI(1);assert(r.actor.motion.paths.size()==1);}
 std::cout<<"PASS "<<cases<<" Grash heal/gossip context matrices plus boundary/retry/reentry/lifetime cases: full production actor, native owner/quest lease/DealHeal/ModifyHealth/Notify/current+suspended lifecycle/FunctionProcessor. Actual90pct owner heal gives49815+49900 once;50161 only validated gossip, original1s routes/3s reply and other flags retained. Criteria/registry/geometry/motion services controlled; no client/full scenario proof.\n";
#endif
}
