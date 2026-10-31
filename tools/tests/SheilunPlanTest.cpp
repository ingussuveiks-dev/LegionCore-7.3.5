#include <cassert>
#include <cstdint>
#include <map>
#include <memory>
#include <vector>
#include <functional>
#include <algorithm>
#include <iostream>
using uint8=uint8_t;using uint32=uint32_t;using uint64=uint64_t;using int32=int32_t;
constexpr uint32 CLASS_MONK=10,CRITERIA_TYPE_SCRIPT_EVENT_2=92,UNIT_FIELD_NPC_FLAGS=1,UNIT_NPC_FLAG_GOSSIP=1;
constexpr float INTERACTION_DISTANCE=5;
enum QuestStatus{QUEST_STATUS_NONE,QUEST_STATUS_COMPLETE,QUEST_STATUS_INCOMPLETE};
struct ObjectGuid{uint64 value=0;bool operator==(ObjectGuid const&other)const{return value==other.value;}};
struct QuestStatusData{std::shared_ptr<uint8>ScriptLifetime;};
enum class GossipOptionNpc{None,Vendor};
struct GossipMenuItem{GossipOptionNpc OptionNpc=GossipOptionNpc::None;uint32 Sender=0,Action=0,BoxMoney=0;bool IsCoded=false;};
using GossipMenuItemContainer=std::map<uint32,GossipMenuItem>;
struct GossipMenu{uint32 _menuId=0;uint32&id=_menuId;GossipMenuItemContainer _menuItems;GossipMenuItemContainer&items=_menuItems;std::map<uint32,uint32>_menuItemData;
 uint32 GetMenuId()const;GossipMenuItemContainer const&GetMenuItems()const;void ClearMenu();uint32 GetMenuItemSender(uint32)const;uint32 GetMenuItemAction(uint32)const;};
struct QuestMenu{void ClearMenu(){}};
struct PlayerMenu{GossipMenu _gossipMenu;GossipMenu&menu=_gossipMenu;QuestMenu _questMenu;GossipMenu const&GetGossipMenu()const{return _gossipMenu;}void ClearMenus();uint32 GetGossipOptionSender(uint32)const;uint32 GetGossipOptionAction(uint32)const;};
#include "Gossip.inc"
struct Player;struct Creature;struct CreatureAI;
struct Map{std::map<uint64,Creature*>actors;std::map<uint64,Player*>players;Creature*GetCreature(ObjectGuid guid){auto i=actors.find(guid.value);return i==actors.end()?nullptr:i->second;}};
struct Unit{virtual~Unit()=default;};
struct Progress{uint64 Counter=0;};struct Achievements{std::map<uint32,Progress>progress;auto const*GetCriteriaProgressMap()const{return &progress;}};
struct Scenario{uint32 id=1007,step=2;Achievements achievements;uint32 GetScenarioId()const{return id;}uint32 GetCurrentStep()const{return step;}auto&GetAchievementMgr(){return achievements;}};
struct Manager{Scenario*scenario=nullptr;Scenario*GetScenario(uint32 instance){assert(instance==7);return scenario;}}manager;auto*sScenarioMgr=&manager;
struct Player:Unit{
 ObjectGuid guid{42};Map*map=nullptr;bool world=true,alive=true,contact=true,gm=false,teleport=false,phase=true,visible=true,los=true,range=true,rewarded=false,questPresent=true,accept=true;
 uint32 klass=10,closes=0,requests=0;QuestStatus status=QUEST_STATUS_INCOMPLETE;QuestStatusData quest;PlayerMenu talk;PlayerMenu*PlayerTalkClass=&talk;std::function<void()>onCredit;std::vector<uint32>events;
 bool IsInWorld()const{return world;}bool IsAlive()const{return alive;}bool CanContact()const{return contact;}bool isGameMaster()const{return gm;}bool HasPendingTeleport()const{return teleport;}
 uint32 getClass()const{return klass;}Map*GetMap()const{return map;}ObjectGuid GetGUID()const{return guid;}bool InSamePhase(Creature*)const{return phase;}bool canSeeOrDetect(Creature*)const{return visible;}
 bool IsWithinLOSInMap(Creature*)const{return los;}bool IsWithinDistInMap(Creature*,float distance)const{assert(distance==5);return range;}bool GetQuestRewardStatus(uint32 id)const{assert(id==41003);return rewarded;}
 QuestStatus GetQuestStatus(uint32 id)const{assert(id==41003);return status;}QuestStatusData*getQuestStatus(uint32 id){assert(id==41003);return questPresent?&quest:nullptr;}
 void AddQuestDelayedEvent(uint32 id,uint64 delay,std::function<void()>&&fn){assert(id==41003&&!delay);if(questPresent&&!quest.ScriptLifetime)quest.ScriptLifetime=std::make_shared<uint8>();fn();}
 void CLOSE_GOSSIP_MENU(){++closes;}
 void UpdateAchievementCriteria(uint32 type,uint32 event){assert(type==92);++requests;events.push_back(event);if(onCredit)onCredit();if(!accept)return;auto s=manager.scenario;if(!s)return;
  uint32 tree=event==48446?46636:event==48447?46637:event==48448?46638:event==48494?46662:0;assert(tree);s->achievements.progress[tree].Counter=1;
  if(s->step==2&&s->achievements.progress[46636].Counter&&s->achievements.progress[46637].Counter&&s->achievements.progress[46638].Counter)s->step=3;
 }
 void OnGossipSelect(Creature*,uint32,uint32){assert(false);}
};
struct Motion{uint32 moves=0;void MovePoint(uint32 point,float x,float y,float z){assert(!point&&x==-1019.15f&&y==-2795.95f&&z==38.91f);++moves;}};
struct GameObject{uint32 phases=0;void SetPhaseMask(uint32 mask,bool force){assert(mask==2&&force);++phases;}};
struct Creature:Unit{
 Map*map=nullptr;ObjectGuid guid;uint32 entry=101881,spawn=362837,mapId=1541,flags=32,casts=0;bool world=true,alive=true,predelete=false,scoped=false;CreatureAI*ai=nullptr;Motion motion;GameObject*wall=nullptr;
 bool IsInWorld()const{return world;}bool IsAlive()const{return alive;}bool IsPreDelete()const{return predelete;}bool HasQuestObjectScope()const{return scoped;}
 uint32 GetMapId()const{return mapId;}uint32 GetInstanceId()const{return 7;}uint32 GetEntry()const{return entry;}uint32 GetDBTableGUIDLow()const{return spawn;}Map*GetMap()const{return map;}ObjectGuid GetGUID()const{return guid;}
 bool HasFlag(uint32 field,uint32 flag){assert(field==UNIT_FIELD_NPC_FLAGS);return (flags&flag)!=0;}void SetFlag(uint32 field,uint32 flag){assert(field==1);flags|=flag;}void RemoveFlag(uint32 field,uint32 flag){assert(field==1);flags&=~flag;}void SetUInt32Value(uint32 field,uint32 value){assert(field==1);flags=value;}
 CreatureAI*AI(){return ai;}Motion*GetMotionMaster(){return &motion;}void CastSpell(Creature*target,uint32 spell){assert(target==this&&spell==60958);++casts;}
 Creature*FindNearestCreature(uint32 wanted,float distance,bool living=true){assert(distance==100.f||distance==107.f);for(auto const&row:map->actors)if(row.second->entry==wanted&&(!living||row.second->alive))return row.second;return nullptr;}
 GameObject*FindNearestGameObject(uint32 wanted,float distance){assert(wanted==246464&&distance==100.f);return wall;}
};
namespace ObjectAccessor{Player*FindPlayer(Map*map,ObjectGuid id){auto i=map->players.find(id.value);return i==map->players.end()?nullptr:i->second;}}
struct CreatureAI{Creature*me;uint32 talks=0;explicit CreatureAI(Creature*actor):me(actor){}virtual~CreatureAI()=default;virtual void sGossipSelect(Player*,uint32,uint32){}virtual void DoAction(int32){}virtual void Talk(uint32 group){assert(group==4);++talks;}};
struct CreatureScript{virtual~CreatureScript()=default;virtual bool OnGossipSelect(Player*,Creature*,uint32,uint32)=0;};
#include "Plan.inc"
struct Taran:CreatureAI{using CreatureAI::CreatureAI;uint8 countgossip=0;
#include "TaranAI.inc"
#include "Action.inc"
};
struct Small:CreatureAI{using CreatureAI::CreatureAI;
#include "SmallAI.inc"
};
struct TaranScript:CreatureScript{
#include "TaranScript.inc"
}taranScript;
struct SmallScript:CreatureScript{
#include "SmallScript.inc"
}smallScript;
struct Scripts{bool OnGossipSelect(Player*player,Creature*actor,uint32 sender,uint32 action){return (actor->entry==101881?static_cast<CreatureScript&>(taranScript):static_cast<CreatureScript&>(smallScript)).OnGossipSelect(player,actor,sender,action);}}scripts;auto*sScriptMgr=&scripts;
struct Packet{uint32 GossipID=0,GossipIndex=0;};
void Dispatch(Player*player,Creature*unit,Packet packet){
#include "Dispatch.inc"
}
struct Run{
 Map map;Scenario scenario;Player player;GameObject wall;Creature taran,fei,nurong,taoshi;Taran taranAI{&taran};Small feiAI{&fei},nurongAI{&nurong},taoshiAI{&taoshi};
 Run(){manager.scenario=&scenario;player.map=&map;map.players[42]=&player;taran.map=fei.map=nurong.map=taoshi.map=&map;taran.guid={1};fei.guid={2};nurong.guid={3};taoshi.guid={4};fei.entry=101882;fei.spawn=362865;nurong.entry=101883;nurong.spawn=362869;taoshi.entry=101885;taoshi.spawn=362867;taran.ai=&taranAI;fei.ai=&feiAI;nurong.ai=&nurongAI;taoshi.ai=&taoshiAI;map.actors={{1,&taran},{2,&fei},{3,&nurong},{4,&taoshi}};fei.flags=nurong.flags=taoshi.flags=33;taran.wall=&wall;}
 Creature*Role(uint32 i){return i==0?&fei:i==1?&nurong:&taoshi;}uint32 Menu(Creature*actor){return actor==&taran?19235:actor==&fei?19225:actor==&nurong?19226:19227;}
 void Offer(Creature*actor){player.talk.menu.id=Menu(actor);player.talk.menu.items={{0,{}},{1,{}}};if(actor==&taran)player.talk.menu.items.erase(1);}
 void Click(Creature*actor,uint32 option=0,bool offer=true){if(offer)Offer(actor);Dispatch(&player,actor,{Menu(actor),option});}
 void Ready(){scenario.step=3;for(uint32 tree:{46636,46637,46638})scenario.achievements.progress[tree].Counter=1;SheilunPlan::Refresh(&taran);}
 bool Has(uint32 tree){return SheilunPlan::Receipt(&scenario,tree);}
};
int main(){
#ifdef BASELINE
 {Run r;r.player.accept=false;for(uint32 i=0;i<3;++i)r.Click(&r.fei);assert(r.taranAI.countgossip==3&&(r.taran.flags&1)&&!r.Has(46636));}
 {Run r;r.scenario.step=3;r.player.accept=false;r.Click(&r.taran);assert(r.taran.motion.moves==1&&r.fei.motion.moves==1&&r.fei.casts==1&&r.wall.phases==1&&!r.Has(46662));}
 std::cout<<"PASS EXPECTED BROKEN04e5ba3b5: three rejected/repeated Fei Li selections enable Taran with no plan receipts; rejected start still consumes flags and moves companions/hides wall. Native script dispatch compiled; no client or ordinary-success claim.\n";
#else
 unsigned cases=0;
 std::vector<uint32>order{0,1,2};do{for(uint32 choices=0;choices<64;++choices){Run r;if(choices&8)r.fei.spawn=362835;if(choices&16)r.nurong.spawn=362834;if(choices&32)r.taoshi.spawn=362831;for(uint32 i=0;i<3;++i){r.Click(r.Role(order[i]),(choices>>order[i])&1);assert(r.player.requests==i+1);assert(bool(r.taran.flags&1)==(i==2));r.Click(r.Role(order[i]));assert(r.player.requests==i+1);}assert(r.scenario.step==3);r.Click(&r.taran);assert(r.player.requests==4&&r.Has(46662)&&r.taran.motion.moves==1&&r.fei.motion.moves==1&&r.fei.casts==1&&r.nurong.motion.moves==1&&r.taoshi.motion.moves==1&&r.wall.phases==1);r.Click(&r.taran);assert(r.player.requests==4&&r.taran.motion.moves==1&&r.taran.flags==32);++cases;}}while(std::next_permutation(order.begin(),order.end()));
 for(uint32 role=0;role<3;++role){Run r;auto actor=r.Role(role);r.Click(actor);uint32 original=actor->spawn;actor->spawn=role==0?362835:role==1?362834:362831;actor->flags=33;r.Click(actor,1);assert(r.player.requests==1&&r.scenario.step==2&&!(r.taran.flags&1));actor->spawn=original;++cases;}
 for(uint32 role=0;role<4;++role){for(uint32 denial=0;denial<31;++denial){Run r;Creature*actor=role==3?&r.taran:r.Role(role);if(role==3)r.Ready();r.Offer(actor);Packet packet{r.Menu(actor),0};switch(denial){
  case 0:r.player.world=false;break;case 1:r.player.alive=false;break;case 2:r.player.contact=false;break;case 3:r.player.gm=true;break;case 4:r.player.teleport=true;break;case 5:r.player.klass=1;break;case 6:r.player.map=nullptr;break;case 7:r.player.phase=false;break;case 8:r.player.visible=false;break;case 9:r.player.los=false;break;case 10:r.player.range=false;break;case 11:r.player.rewarded=true;break;case 12:r.player.status=QUEST_STATUS_NONE;break;case 13:r.player.questPresent=false;break;case 14:actor->world=false;break;case 15:actor->alive=false;break;case 16:actor->predelete=true;break;case 17:actor->scoped=true;break;case 18:actor->mapId=870;break;case 19:++actor->entry;break;case 20:++actor->spawn;break;case 21:actor->flags=32;break;case 22:r.scenario.id=42;break;case 23:manager.scenario=nullptr;break;case 24:r.scenario.step=role==3?2:1;break;case 25:++packet.GossipID;break;case 26:r.player.talk.menu.items.clear();break;case 27:r.player.talk.menu.id=1;break;case 28:packet.GossipIndex=role==3?1:2;break;case 29:r.player.talk.menu.items[0].Action=1;break;case 30:r.player.talk.menu.items[0].IsCoded=true;break;}
  Dispatch(&r.player,actor,packet);assert(r.player.requests==0&&!actor->motion.moves&&!r.wall.phases);++cases;
 }}
 for(uint32 role=0;role<4;++role){Run r;auto actor=role==3?&r.taran:r.Role(role);if(role==3)r.Ready();r.player.onCredit=[&]{r.Click(actor,0,false);};r.Click(actor);assert(r.player.requests==1&&!(actor->flags&1));++cases;}
 for(uint32 role=0;role<4;++role){Run r;auto actor=role==3?&r.taran:r.Role(role);if(role==3)r.Ready();r.player.accept=false;r.Click(actor);assert(r.player.requests==1&&(actor->flags&1)&&!actor->motion.moves&&!r.wall.phases);r.player.accept=true;r.Click(actor,role==3?0:1);assert(r.player.requests==2&&!(actor->flags&1));++cases;}
 for(uint32 role=0;role<4;++role)for(uint32 revoke=0;revoke<14;++revoke){Run r;auto actor=role==3?&r.taran:r.Role(role);if(role==3)r.Ready();r.player.onCredit=[&]{switch(revoke){case 0:r.player.quest.ScriptLifetime.reset();break;case 1:r.player.quest.ScriptLifetime=std::make_shared<uint8>();break;case 2:r.player.map=nullptr;break;case 3:r.map.actors.erase(actor->guid.value);break;case 4:r.map.players.clear();break;case 5:r.player.status=QUEST_STATUS_NONE;break;case 6:actor->world=false;break;case 7:++actor->entry;break;case 8:++actor->spawn;break;case 9:actor->predelete=true;break;case 10:++r.scenario.id;break;case 11:r.scenario.step=4;break;case 12:r.player.phase=false;break;case 13:r.player.rewarded=true;break;}};r.Click(actor);assert(r.player.requests==1&&(actor->flags&1)&&!actor->motion.moves&&!r.wall.phases);++cases;}
 {Run r;for(uint32 i=0;i<3;++i){r.taranAI.DoAction(1);assert(!(r.taran.flags&1));}r.scenario.step=3;for(uint32 tree:{46636,46637})r.scenario.achievements.progress[tree].Counter=1;r.taranAI.DoAction(1);assert(!(r.taran.flags&1));r.scenario.achievements.progress[46638].Counter=1;r.taranAI.DoAction(1);assert(r.taran.flags==33);++cases;}
 {Run r;r.map.actors.erase(1);for(uint32 i=0;i<3;++i)r.Click(r.Role(i));assert(r.scenario.step==3&&r.taran.flags==32);r.map.actors[1]=&r.taran;SheilunPlan::Refresh(&r.taran);assert(r.taran.flags==33);++cases;}
 for(uint32 missing=0;missing<4;++missing){Run r;r.Ready();if(missing<3)r.map.actors.erase(missing+2);else r.taran.wall=nullptr;if(missing==0)r.fei.ai=nullptr;r.Click(&r.taran);assert(r.Has(46662)&&r.taran.motion.moves==1);++cases;}
 {Run r;r.Ready();r.fei.ai=nullptr;r.Click(&r.taran);assert(r.fei.motion.moves==1&&r.fei.casts==1&&r.Has(46662));++cases;}
 {Run r;r.player.status=QUEST_STATUS_COMPLETE;r.Click(&r.fei,1);assert(r.Has(46636)&&r.fei.flags==32);++cases;}
 std::cout<<"PASS "<<cases<<" actual Sheilun plan helpers, both native AI menu/index delegates, inert legacy scripts, native ordinary packet dispatch and real DoAction under ASAN: all384 role orders/tactical-index/original rescued-or-upper-spawn combinations, duplicate/rejected credit,124 invalid player/actor/quest/menu contexts, reentrant attempt/map/actor revocations, exact native receipt before flags/motion, missing actors, readiness recovery and null Fei AI. Criteria/world/visibility/motion/cast endpoints controlled; native DB2 verified separately; tactical combat effects, client and later pickup/return remain OPEN.\n";
#endif
}
