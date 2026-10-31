#include <cstdint>
#include <map>
#include <vector>
#include <memory>
#include <functional>
#include <string>
#include <set>
using int8=int8_t;using int64=int64_t;using uint8=uint8_t;using uint32=uint32_t;using uint64=uint64_t;using int32=int32_t;
enum{CLASS_SHAMAN=7,QUEST_STATUS_NONE=0,QUEST_STATUS_COMPLETE=1,QUEST_STATUS_INCOMPLETE=3,QUEST_SPECIAL_FLAGS_KILL=1,QUEST_SPECIAL_FLAGS_CAST=2,QUEST_OBJECTIVE_MONSTER=0,QUEST_OBJECTIVE_TALKTO=9};
struct ObjectGuid{uint64 high=1,low=1;bool operator<(ObjectGuid b)const{return high!=b.high?high<b.high:low<b.low;}bool operator==(ObjectGuid b)const{return high==b.high&&low==b.low;}};
struct Map{uint32 GetDifficultyID()const{return 0;}}liveMap,otherMap;
struct QuestObjective{int32 ObjectID;int8 StorageIndex;uint32 Type,Amount;};
struct Quest{bool kill=true,raidAllowed=false;std::vector<QuestObjective> objectives{{105805,0,0,1},{105787,1,0,1},{105806,2,0,1},{111286,3,0,1}};uint32 GetQuestId()const{return 40341;}auto const& GetObjectives()const{return objectives;}bool HasSpecialFlag(uint32)const{return kill;}bool IsAllowedInRaid(uint32)const{return raidAllowed;}}nativeQuest;
struct QuestStore{Quest const* GetQuestTemplate(uint32 id){return id==40341?&nativeQuest:nullptr;}}questStore;auto sQuestDataStore=&questStore;
struct QuestStatusData{uint32 Status=3;std::shared_ptr<uint8> ScriptLifetime;std::map<int8,int32> ObjectiveData;};
struct Group{bool raid=false;bool isRaidGroup()const{return raid;}};
struct Scenario{uint32 id=1066,step=0;uint32 GetScenarioId()const{return id;}uint32 GetCurrentStep()const{return step;}}scenario;
struct ScenarioManager{bool exists=true;Scenario* GetScenario(uint32){return exists?&scenario:nullptr;}}scenarioManager;auto sScenarioMgr=&scenarioManager;
inline uint32 warnings=0;
#define TC_LOG_WARN(...) (++warnings)
#include "TimerHost.inc"
struct ObjectAccessor{inline static std::vector<Player*> players;static Player* GetPlayer(Creature const& source,ObjectGuid id){for(auto player:players)if(player->guid==id&&player->world&&player->GetMap()==source.GetMap())return player;return nullptr;}};
#include "Quest.inc"
#include "Objective.inc"
void Player::KilledMonsterCredit(uint32 entry){
 assert(entry==105805);++calls;credits.push_back(entry);uint32 real_entry=entry,addKillCount=1;ObjectGuid guid;
 Quest const* qInfo=&nativeQuest;QuestStatusData* q_status=getQuestStatus(40341);uint32 questid=40341;if(!q_status)return;
#include "Credit.inc"
 if(onCredit)onCredit();
}
#include "Recovery.inc"
#include "Actor.inc"
using Erunak=npc_erunak_stonespeaker::npc_erunak_stonespeakerAI;
struct Run{
 Creature actor,brute;Player player,second;CreatureAI temporary{&actor};
 Run(){scenario={};scenarioManager.exists=true;warnings=0;nativeQuest.kill=true;nativeQuest.raidAllowed=false;actor.i_AI=new Erunak(&actor);actor.db=362063;actor.entry=102826;actor.brute=&brute;brute.db=362064;brute.entry=102792;second.guid={1,2};ObjectAccessor::players={&player,&second};}
 ~Run(){if(actor.i_disabledAI&&actor.i_disabledAI!=actor.i_AI)delete actor.i_disabledAI;if(actor.i_AI!=&temporary)delete actor.i_AI;ObjectAccessor::players.clear();}
 Erunak* Content(){return static_cast<Erunak*>(actor.i_disabledAI?actor.i_disabledAI:actor.i_AI);}
 void Accept(Player* p=nullptr){Content()->MoveInLineOfSight_Safe(p?p:&player);}
 void Tick(uint32 diff){actor.Lifecycle(diff);actor.functions.Update(diff);player.functions.Update(diff);second.functions.Update(diff);if(actor.i_AI)actor.i_AI->UpdateAI(diff);}
 int32 Count(Player& p){return p.GetQuestObjectiveData(40341,105805);}
};
int main(){
#ifdef BASELINE
 {Run r;r.player.gm=true;r.Accept();assert(r.Count(r.player)==1&&r.Content()->_introDone);}
 {Run r;r.player.record=false;r.Accept();assert(!r.Count(r.player)&&r.Content()->_introDone);}
 {Run r;r.Accept();r.Accept(&r.second);assert(r.Count(r.player)==1&&!r.Count(r.second));}
 {Run r;r.actor.db=999;r.Accept();assert(r.Count(r.player)==1);}
 {Run r;scenario.step=5;r.Accept();assert(r.Count(r.player)==1);}
 std::cout<<"PASS EXPECTED BROKEN bf9292b2f: GM/native quest write, no-quest latch, second eligible visitor loses105805, foreign spawn and late native stage award entry. Native objective lookup/write branch compiled; no client claim.\n";
#else
 unsigned cases=0;
 for(uint32 status:{1u,3u}){Run r;r.player.quest.Status=status;if(status==1)r.player.quest.ObjectiveData[0]=1;r.Accept();assert(r.Count(r.player)==1&&r.Content()->_introDone&&r.player.calls==(status==1?0u:1u));r.Tick(2999);assert(r.actor.talks.empty());r.Tick(1);assert(r.actor.talks==std::vector<uint32>{0}&&r.actor.attacks==1);r.Accept();r.Tick(5000);assert(r.actor.talks.size()==1&&r.player.calls==(status==1?0u:1u));++cases;}
 for(uint32 deny=0;deny<24;++deny){Run r;switch(deny){case 0:r.player.world=false;break;case 1:r.player.contact=false;break;case 2:r.player.alive=false;break;case 3:r.player.gm=true;break;case 4:r.player.teleport=true;break;case 5:r.player.classId=1;break;case 6:r.player.map=&otherMap;break;case 7:r.player.instanceId=8;break;case 8:r.player.phase=false;break;case 9:r.player.detect=false;break;case 10:r.player.los=false;break;case 11:r.player.near=false;break;case 12:r.player.rewarded=true;break;case 13:r.player.record=false;break;case 14:r.player.quest.Status=0;break;case 15:r.player.quest.Status=6;break;case 16:r.actor.world=false;break;case 17:r.actor.preDelete=true;break;case 18:r.actor.alive=false;break;case 19:r.actor.entry=999;break;case 20:r.actor.mapId=1;break;case 21:scenario.id=99;break;case 22:scenario.step=2;break;case 23:scenarioManager.exists=false;break;}r.Accept();r.Tick(5000);assert(!r.Count(r.player)&&!r.Content()->_introDone&&r.actor.talks.empty()&&!r.player.calls);++cases;}
 {Run r;r.actor.db=999;r.Accept();assert(!r.Count(r.player)&&!r.Content()->_introDone);++cases;}
 {Run r;Unit pet;r.Content()->MoveInLineOfSight_Safe(&pet);r.Content()->MoveInLineOfSight(nullptr);assert(!r.player.calls&&!r.Content()->_introDone);++cases;}
 {Run r;r.Accept();r.Accept(&r.second);assert(r.Count(r.player)==1&&r.Count(r.second)==1);r.Tick(3000);assert(r.actor.talks.size()==1&&r.player.calls==1&&r.second.calls==1);++cases;}
 for(uint32 reason=0;reason<3;++reason){Run r;if(reason==0)r.player.reject=true;if(reason==1){r.player.group=&r.player.groupValue;r.player.groupValue.raid=true;}if(reason==2)nativeQuest.kill=false;r.Accept();assert(!r.Count(r.player)&&!r.Content()->_introDone);for(int i=1;i<30;++i)r.Tick(1000);uint32 calls=reason==0?30u:0u;assert(r.player.calls==calls&&warnings==1&&!r.Content()->_introDone);r.Tick(10000);r.Accept();assert(r.player.calls==calls);r.player.reject=false;r.player.group=nullptr;nativeQuest.kill=true;r.player.near=false;r.Tick(0);r.player.near=true;r.Accept();assert(r.Count(r.player)==1&&r.Content()->_introDone);r.Tick(3000);assert(r.actor.talks.size()==1);++cases;}
 for(uint32 reason=0;reason<4;++reason){Run r;r.player.reject=true;r.Accept();r.player.quest.ScriptLifetime.reset();if(reason==0)r.player.record=false;if(reason==1)r.player.quest.Status=0;if(reason==2)r.player.rewarded=true;if(reason==3)r.player.quest.ScriptLifetime=std::make_shared<uint8>();r.Tick(1000);assert(r.Content()->_lowerEntries.empty()&&!r.Content()->_introDone&&r.player.calls==1);++cases;}
 {Run r;r.player.onCredit=[&]{r.Accept();};r.Accept();assert(r.player.calls==1&&r.Content()->_introDone);++cases;}
 {Run r;r.player.onCredit=[&]{r.player.quest.ScriptLifetime.reset();r.player.quest.ScriptLifetime=std::make_shared<uint8>();};r.Accept();r.Tick(1000);assert(!r.Content()->_introDone&&r.Content()->_lowerEntries.empty());++cases;}
 for(uint32 cancel=0;cancel<4;++cancel){Run r;r.Accept();if(cancel==0)r.player.quest.ScriptLifetime.reset();if(cancel==1)r.player.record=false;if(cancel==2)r.player.near=false;if(cancel==3)scenario.step=2;r.Tick(3000);assert(r.actor.talks.empty()&&!r.Content()->_introDone&&!r.actor.attacks);++cases;}
 {Run r;r.player.reject=true;r.Accept();r.player.near=false;r.Tick(1000);r.second.quest.ObjectiveData[0]=1;r.Accept(&r.second);r.Tick(3000);assert(!r.Count(r.player)&&r.Count(r.second)==1&&r.actor.talks.size()==1);++cases;}
 for(uint32 mode=0;mode<5;++mode){Run r;r.Accept();auto content=r.Content();if(mode==0)r.actor.states|=UNIT_STATE_STUNNED;if(mode==1)r.actor.states|=UNIT_STATE_CASTING;if(mode>=2){r.actor.i_disabledAI=r.actor.i_AI;r.actor.i_AI=mode==3?nullptr:mode==4?r.actor.i_disabledAI:&r.temporary;}r.Tick(2999);assert(r.actor.talks.empty());r.Tick(1);assert(r.actor.talks.size()==1&&r.Count(r.player)==1&&content->_introDone);r.Tick(10000);assert(r.actor.talks.size()==1);++cases;}
 for(uint32 bad=0;bad<5;++bad){Run r;if(bad==0)r.actor.brute=nullptr;if(bad==1)r.brute.db=999;if(bad==2)r.brute.map=&otherMap;if(bad==3)r.brute.phase=false;if(bad==4)r.brute.preDelete=true;r.Accept();r.Tick(3000);assert(r.actor.talks.size()==1&&!r.actor.attacks&&r.Count(r.player)==1);++cases;}
 {Run r;r.Accept();assert(r.actor.functions.SizeQueue()==0);assert(r.actor.AIM_Destroy());r.actor.functions.Update(10000);assert(r.actor.talks.empty());++cases;}
 {Run r;r.player.reject=true;r.Accept();scenario.step=2;r.Tick(1000);assert(r.Content()->_lowerEntries.empty()&&r.player.calls==1&&!r.Count(r.player));++cases;}
 {Run r;r.player.reject=true;r.Accept();r.player.quest.ObjectiveData[0]=1;r.Accept();assert(r.Content()->_lowerEntries.empty()&&r.Content()->_introDone&&r.player.calls==1);++cases;}
 {Run r;r.player.group=&r.player.groupValue;r.second.group=r.player.group;r.Accept();r.Accept(&r.second);assert(r.Count(r.player)==1&&r.Count(r.second)==1);r.Tick(3000);assert(r.actor.talks.size()==1);++cases;}
 {Run r;r.player.group=&r.player.groupValue;r.player.groupValue.raid=true;nativeQuest.raidAllowed=true;r.Accept();assert(r.Count(r.player)==1);++cases;}
 {Run r;std::vector<std::unique_ptr<Player>> visitors;for(uint64 i=0;i<40;++i){auto p=std::make_unique<Player>();p->guid={1,100+i};p->reject=true;ObjectAccessor::players.push_back(p.get());r.Accept(p.get());visitors.push_back(std::move(p));}assert(r.Content()->_lowerEntries.size()==40);r.Accept();r.Accept();assert(warnings==1&&!r.player.calls);visitors[0]->quest.ScriptLifetime.reset();r.Tick(0);r.Accept();assert(r.Count(r.player)==1&&r.Content()->_lowerEntries.size()==39);++cases;}
 {Run r;r.Accept();r.Content()->Reset();r.Tick(3000);assert(r.actor.talks.size()==1&&r.Count(r.player)==1);++cases;}
 {Run r;r.Accept();r.player.quest.ScriptLifetime.reset();r.Tick(3000);assert(!r.Content()->_introDone&&r.actor.talks.empty());r.Accept();r.Tick(3000);assert(r.actor.talks.size()==1&&r.player.calls==1);++cases;}
 {Run r;r.player.reject=true;r.Accept();r.actor.i_disabledAI=r.actor.i_AI;r.actor.i_AI=nullptr;r.player.reject=false;r.Tick(1000);assert(r.Count(r.player)==1&&r.Content()->_introDone);r.Tick(3000);assert(r.actor.talks.size()==1&&!r.actor.attacks);++cases;}
 std::cout<<"PASS "<<cases<<" actual full lower Erunak/owned timer/native quest lease/objective read and native quest-credit branch ASAN cases: exact role/map/1066/0..1/active40341/player gates, acknowledged105805 per visitor, no pet/unrelated/late admission, finite30 refusal/raid/missing kill flag and genuine re-entry, abandonment/replacement/reentrant/repeated/other visitor, missing actor/current+suspended/alias and destruction. Quest-log enumeration/achievement/network/registry/geometry services controlled; upper intro/waypoints/client/full1066 remain separate.\n";
#endif
}
