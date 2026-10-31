#include <algorithm>
#include <cassert>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <iostream>
#include <map>
#include <sstream>
#include <vector>
#include <set>
#include <memory>
#include <functional>
#include <atomic>
#include <mutex>
#include <chrono>
// The retained conversation host compiles actual payload/lifetime/visibility/
// expiry functions. Its viewer GUID service uses synthetic32bit keys; the
// Erunak controller and its registry below use both full GUID components.
namespace NativeVoice {
#include "VoiceHost.inc"
}
#include "EntryPrelude.inc"
#include "TimerHost.inc"
struct ObjectAccessor{
 inline static std::vector<Player*> players;inline static std::vector<Creature*> creatures;
 static Player* GetPlayer(Unit const& source,ObjectGuid id){for(auto p:players)if(p->guid==id&&p->world&&p->GetMap()==source.GetMap())return p;return nullptr;}
};
Creature* Map::GetCreature(ObjectGuid const& id){for(auto c:ObjectAccessor::creatures)if(c->guid==id&&c->world&&c->map==this)return c;return nullptr;}
Creature* Player::FindNearestCreature(uint32 id,float,bool living){for(auto c:ObjectAccessor::creatures)if(c->entry==id&&c->world&&!c->preDelete&&c->map==map&&c->phase&&phase&&(!living||c->alive))return c;return nullptr;}
#include "RearmUpper.inc"
#include "Quest.inc"
#include "Objective.inc"
void Player::KilledMonsterCredit(uint32 entry){
 assert(entry==105805);++calls;credits.push_back(entry);uint32 real_entry=entry,addKillCount=1;ObjectGuid guid;
 Quest const* qInfo=&nativeQuest;QuestStatusData* q_status=getQuestStatus(40341);uint32 questid=40341;if(!q_status)return;
#include "Credit.inc"
 if(onCredit)onCredit();
}
struct SpellInfo{uint32 Id=209312;};
struct SpellManager{bool available=true;SpellInfo spell;SpellInfo const* GetSpellInfo(uint32 id){assert(id==209312);return available?&spell:nullptr;}}spellManager;auto sSpellMgr=&spellManager;
enum class HighGuid{Conversation};
struct Generator{uint64 value=0;uint64 Generate(){return ++value;}};
struct ObjectManager{Generator generator;template<HighGuid>Generator* GetGenerator(){return &generator;}}objectManager;auto sObjectMgr=&objectManager;
struct Conversation;
std::vector<std::unique_ptr<Conversation>> conversations;
uint32 factoryCalls=0;bool failFactory=false,wrongActor=false;std::function<void()> onCreate;
struct Conversation{
 NativeVoice::Conversation* native=new NativeVoice::Conversation;
 NativeVoice::WorldObject proxy;NativeVoice::Creature erunakProxy,adelProxy;
 Unit* creator=nullptr;uint32 spellId=0;float createdZ=0;bool registered=false,bound=false;
 std::weak_ptr<uint8> scope;
 ~Conversation(){if(!registered)delete native;}
 bool CreateConversation(uint64 id,uint32 entry,Unit* owner,SpellInfo const* info,Creature const& pos){
  assert(entry==1868);++factoryCalls;if(failFactory)return false;creator=owner;spellId=info?info->Id:0;createdZ=pos.GetPositionZ();
  proxy.player=owner->ToPlayer()!=nullptr;proxy.guid=uint32(owner->GetGUID().GetLowPart());
  auto ep=owner->FindNearestCreature(102826,owner->GetVisibilityRange(),true);auto ap=owner->FindNearestCreature(104853,owner->GetVisibilityRange(),true);
  if(ep){erunakProxy.identity=uint32(ep->GetGUID().GetLowPart());erunakProxy.alive=ep->IsAlive()&&ep->world;proxy.nearby[102826]=&erunakProxy;}
  if(ap){adelProxy.identity=uint32(ap->GetGUID().GetLowPart());adelProxy.alive=ap->IsAlive()&&ap->world;proxy.nearby[104853]=&adelProxy;}
  if(!native->CreateConversation(uint32(id),entry,&proxy,info,{pos.GetPositionX(),pos.GetPositionY(),pos.GetPositionZ()}))return false;
  registered=true;conversations.emplace_back(this);
  if(wrongActor)native->creatures.at(0).GuidHight^=1;
  if(onCreate)onCreate();return true;
 }
 NativeVoice::ConversationDynamicFieldCreatures const* GetCreature(uint8 index)const{auto it=native->creatures.find(index);return it==native->creatures.end()?nullptr:&it->second;}
 void Remove(){native->Remove();}
 bool Removed()const{return native->removed;}
 void Tick(){native->questExpired=bound&&scope.expired();native->Update(0);}
 void View(Player& player){NativeVoice::WorldObject viewer;viewer.guid=uint32(player.GetGUID().GetLowPart());assert(!native->CanNeverSee2(&viewer));}
};
void Player::BindQuestObject(uint32 id,Conversation* object){assert(id==40341&&object&&record&&quest.ScriptLifetime);++binds;object->scope=quest.ScriptLifetime;object->bound=true;}
void Creature::CreateConversation(uint32 id){conversations.push_back(id);auto object=new Conversation;if(!object->CreateConversation(sObjectMgr->generator.Generate(),id,this,nullptr,*this))delete object;if(onPresentation)onPresentation();}
namespace SharasdalGrash {
#include "Receipt.inc"
}
namespace SharasdalWall {
#include "Path.inc"
}
#include "Recovery.inc"
#include "Actor.inc"
using Erunak=npc_erunak_stonespeaker::npc_erunak_stonespeakerAI;
struct CountedErunak:Erunak{uint32 reentries=0;using Erunak::Erunak;void SetGUID(ObjectGuid const& id,int32 event=0)override{++reentries;Erunak::SetGUID(id,event);}};
static void LoadVoice(){
 std::ifstream input("../sharasdal-upper-voice-data/conversation.tsv");assert(input);std::string text;NativeVoice::store={};NativeVoice::store.identity=1868;
 while(std::getline(input,text)){std::istringstream row(text);std::string kind;row>>kind;
  if(kind=="line"){NativeVoice::ConversationData value{};assert(row>>value.entry>>value.id>>value.textId>>value.unk1>>value.unk2>>value.idx);NativeVoice::store.lines.push_back(value);}
  else{assert(kind=="creature");NativeVoice::ConversationCreature value{};assert(row>>value.entry>>value.id>>value.creatureId>>value.creatureGuid>>value.unk1>>value.unk2>>value.duration);NativeVoice::store.creatures.push_back(value);}
 }
 assert(NativeVoice::store.lines.size()==5&&NativeVoice::store.creatures.size()==2);
}
struct Run{
 Creature actor,grash,adel,brute;Player player,second;CreatureAI grashAI{&grash},adelAI{&adel},temporary{&actor},otherTemporary{&grash};
 Run(){conversations.clear();NativeVoice::live.clear();NativeVoice::currentTime=0;LoadVoice();scenario={};scenario.step=3;scenario.achievements.progress[47131].Counter=1;scenarioManager.exists=true;warnings=0;factoryCalls=0;failFactory=wrongActor=false;onCreate={};spellManager.available=true;
  actor.z=797;actor.entry=102826;actor.db=362093;actor.guid={1,102826};actor.i_AI=new CountedErunak(&actor);
  grash.entry=105298;grash.db=362092;grash.guid={1,105298};grash.i_AI=&grashAI;adel.entry=104853;adel.db=362094;adel.guid={1,104853};adel.i_AI=&adelAI;brute.entry=102792;brute.db=362064;brute.guid={1,102792};
  player.guid={1,1};second.guid={1,2};actor.grash=&grash;actor.adel=&adel;actor.brute=&brute;actor.instance.upperErunak=actor.guid;
  ObjectAccessor::players={&player,&second};ObjectAccessor::creatures={&actor,&grash,&adel,&brute};liveMap.players={{&player}};
 }
 ~Run(){conversations.clear();NativeVoice::live.clear();if(actor.i_disabledAI&&actor.i_disabledAI!=actor.i_AI)delete actor.i_disabledAI;if(actor.i_AI!=&temporary)delete actor.i_AI;ObjectAccessor::players.clear();ObjectAccessor::creatures.clear();liveMap.players.clear();}
 CountedErunak* Content(){return static_cast<CountedErunak*>(actor.i_disabledAI?actor.i_disabledAI:actor.i_AI);}
 void Accept(){Content()->MoveInLineOfSight_Safe(&player);}
 void Tick(uint32 diff,bool ai=true){NativeVoice::currentTime+=diff;actor.Lifecycle(diff);actor.functions.Update(diff);player.functions.Update(diff);second.functions.Update(diff);if(ai&&actor.i_AI)actor.i_AI->UpdateAI(diff);for(auto& object:conversations)object->Tick();}
 void Start(){Accept();assert(actor.motion.paths==1&&grash.motion.paths==1&&adel.motion.paths==1);}
 uint32 Active(){uint32 count=0;for(auto& object:conversations)if(!object->Removed())++count;return count;}
};
int main(){
 _set_abort_behavior(0,_CALL_REPORTFAULT);
#ifdef BASELINE
 {Run r;r.player.gm=true;r.player.record=false;r.Accept();assert(r.Content()->_introDone&&r.actor.motion.paths==1);r.Tick(5000);assert(factoryCalls==1&&r.Active()==1&&conversations.back()->creator==&r.actor&&!r.player.binds);conversations.back()->View(r.player);r.Tick(100000);assert(!conversations.back()->Removed());}
 {Run r;scenario.step=0;scenario.achievements.progress.clear();r.Accept();assert(r.Content()->_introDone&&r.actor.motion.paths==1);}
 {Run r;r.actor.grash=nullptr;r.Accept();assert(r.Content()->_introDone&&!r.grash.motion.paths&&r.actor.motion.paths==1);}
 {Run r;NativeVoice::store.lines.clear();r.Accept();r.Tick(5000);assert(factoryCalls==1&&r.Active()==0&&r.Content()->_introDone);LoadVoice();r.Accept();r.Tick(5000);assert(factoryCalls==1);}
 std::cout<<"PASS EXPECTED BROKEN b8f080c1f full Erunak: GM/no quest and early native stage consume/move; missing Grash consumes partial approach; failed native conversation loses retry; actual NPC creator has no Player playback key and remains allocated after per-view deadline. No actual client traversal or fabricated crash.\n";
#else
 unsigned cases=0;
 for(uint32 status:{1u,3u}){Run r;r.player.quest.Status=status;r.Start();assert(!r.Content()->_introDone&&!factoryCalls&&r.player.calls==0);r.Tick(4999);assert(!factoryCalls);r.Tick(1);assert(r.Active()==1&&factoryCalls==1&&r.Content()->_introDone&&r.player.binds==1);auto object=conversations.back().get();assert(object->creator==&r.player&&object->spellId==209312&&object->createdZ==797&&object->native->lines.size()==5&&object->native->lifetime==47816);object->View(r.player);r.Accept();r.Tick(47816);assert(!object->Removed()&&factoryCalls==1);r.Tick(1);assert(object->Removed());++cases;}
 for(uint32 deny=0;deny<24;++deny){Run r;switch(deny){case 0:r.player.world=false;break;case 1:r.player.contact=false;break;case 2:r.player.alive=false;break;case 3:r.player.gm=true;break;case 4:r.player.teleport=true;break;case 5:r.player.classId=1;break;case 6:r.player.map=&otherMap;break;case 7:r.player.instanceId=8;break;case 8:r.player.phase=false;break;case 9:r.player.detect=false;break;case 10:r.player.los=false;break;case 11:r.player.near=false;break;case 12:r.player.rewarded=true;break;case 13:r.player.record=false;break;case 14:r.player.quest.Status=0;break;case 15:r.player.quest.Status=6;break;case 16:r.actor.world=false;break;case 17:r.actor.preDelete=true;break;case 18:r.actor.alive=false;break;case 19:r.actor.entry=999;break;case 20:r.actor.mapId=1;break;case 21:scenario.id=99;break;case 22:scenario.step=2;break;case 23:scenarioManager.exists=false;break;}r.Accept();r.Tick(5000);assert(!r.Content()->_introDone&&!r.actor.motion.paths&&!factoryCalls&&r.player.calls==0);++cases;}
 {Run r;r.actor.db=999;r.Accept();r.Tick(5000);assert(!r.actor.motion.paths&&!factoryCalls);++cases;}
 {Run r;liveMap.players.clear();Unit pet;r.Content()->MoveInLineOfSight_Safe(&pet);r.Content()->MoveInLineOfSight(nullptr);r.Tick(5000);assert(!r.actor.motion.paths&&!factoryCalls);++cases;}
 {Run r;r.player.contact=false;r.player.teleport=true;r.Accept();r.Tick(1000);assert(!r.actor.motion.paths);r.player.contact=true;r.player.teleport=false;r.Tick(1000);assert(r.actor.motion.paths==1&&!factoryCalls);r.Tick(4999);assert(!factoryCalls);r.Tick(1);assert(r.Active()==1);++cases;}
 {Run r;scenario.achievements.progress.clear();r.Accept();r.Tick(1000);assert(r.Content()->_upperPending&&!r.actor.motion.paths);scenario.achievements.progress[47131].Counter=1;r.Tick(1000);assert(r.actor.motion.paths==1);r.Tick(5000);assert(r.Active()==1&&r.player.calls==0);++cases;}
 {Run r;scenario.achievements.progress.clear();r.Accept();for(int i=1;i<30;++i)r.Tick(1000);assert(warnings==1&&r.Content()->_upperExhausted&&!r.actor.motion.paths);scenario.achievements.progress[47131].Counter=1;r.Tick(0);r.Tick(5000);assert(r.Active()==1);++cases;}
 {Run r;r.actor.grash=nullptr;r.Accept();for(int i=1;i<30;++i)r.Tick(1000);assert(warnings==1&&!r.actor.motion.paths&&!r.adel.motion.paths);r.actor.grash=&r.grash;r.Tick(0);r.Tick(5000);assert(r.Active()==1&&r.grash.motion.paths==1);++cases;}
 for(uint32 who=0;who<3;++who){Run r;Creature* actors[]={&r.grash,&r.actor,&r.adel};actors[who]->motion.accept=false;r.Accept();assert(!r.Content()->_upperWaiting);for(int i=1;i<30;++i)r.Tick(1000);assert(warnings==1&&actors[who]->motion.paths==30&&!factoryCalls);actors[who]->motion.accept=true;r.actor.instance.RearmErunak(r.player.guid);r.Tick(0);r.Tick(5000);assert(r.Active()==1);for(uint32 n=0;n<3;++n)assert(actors[n]->motion.paths==(n==who?31u:1u));assert(r.grash.talks.size()==1&&r.adel.talks.size()==1);++cases;}
 for(uint32 who=0;who<3;++who){Run r;Creature* actors[]={&r.grash,&r.actor,&r.adel};actors[who]->states|=UNIT_STATE_STUNNED;r.Accept();r.Tick(1000);assert(!r.actor.motion.paths&&!factoryCalls);actors[who]->states=0;r.Tick(1000);r.Tick(5000);assert(r.Active()==1);++cases;}
 {Run r;r.grash.motion.path.id=r.actor.motion.path.id=r.adel.motion.path.id=10993811;r.Accept();r.Tick(5000);assert(r.Active()==1&&!r.grash.motion.paths&&!r.actor.motion.paths&&!r.adel.motion.paths&&r.grash.talks.empty());++cases;}
 for(uint32 mode=0;mode<5;++mode){Run r;r.Start();if(mode==0)r.actor.states|=UNIT_STATE_STUNNED;if(mode==1)r.actor.states|=UNIT_STATE_CASTING;if(mode>=2){r.actor.i_disabledAI=r.actor.i_AI;r.actor.i_AI=mode==3?nullptr:mode==4?r.actor.i_disabledAI:&r.temporary;}r.Tick(4999);assert(!factoryCalls);r.Tick(1);assert(r.Active()==1&&r.player.binds==1);++cases;}
 {Run r;r.Start();scenario.step=4;r.actor.grash=nullptr;r.Tick(5000);assert(r.Active()==1&&r.grash.motion.paths==1);++cases;}
 {Run r;r.actor.grash=nullptr;r.Accept();scenario.step=4;r.Tick(1000);assert(!factoryCalls&&!r.actor.motion.paths&&!r.Content()->_upperPending);++cases;}
 for(uint32 reason=0;reason<3;++reason){Run r;r.Start();if(reason==0)failFactory=true;if(reason==1)spellManager.available=false;if(reason==2)NativeVoice::store.lines.clear();r.Tick(5000);assert(!r.Content()->_introDone&&r.Active()==0);for(int i=1;i<30;++i)r.Tick(1000);assert(warnings==1&&r.Content()->_upperExhausted);auto calls=factoryCalls;r.Tick(10000);assert(factoryCalls==calls);failFactory=false;spellManager.available=true;LoadVoice();r.actor.instance.RearmErunak({2,r.player.guid.low});r.Tick(0);assert(r.Content()->_upperExhausted);r.actor.instance.RearmErunak(r.player.guid);r.Tick(0);assert(r.Active()==1&&r.player.binds==1&&r.actor.motion.paths==1&&r.grash.talks.size()==1);++cases;}
 {Run r;r.Start();wrongActor=true;r.Tick(5000);assert(r.Active()==0&&!r.Content()->_introDone&&!r.player.binds&&conversations.size()==1&&conversations.back()->Removed());wrongActor=false;r.Tick(1000);assert(r.Active()==1&&r.actor.motion.paths==1);++cases;}
 {Run r;r.Start();onCreate=[&]{r.Accept();assert(!r.actor.AIM_Destroy());};r.Tick(5000);assert(factoryCalls==1&&r.player.binds==1);++cases;}
 {Run r;r.Start();onCreate=[&]{r.player.quest.ScriptLifetime.reset();r.player.record=false;};r.Tick(5000);assert(!r.Content()->_introDone&&!r.player.binds&&r.Active()==0);r.Tick(1000);assert(!r.Content()->_upperPending);++cases;}
 {Run r;r.Start();r.player.quest.ScriptLifetime.reset();r.player.quest.ScriptLifetime=std::make_shared<uint8>();r.Tick(1000);r.Tick(5000);assert(r.Active()==1&&r.actor.motion.paths==1&&r.grash.talks.size()==1);++cases;}
 {Run r;r.Start();r.Tick(5000);assert(r.Active()==1);r.player.quest.ScriptLifetime.reset();r.Tick(0);assert(r.Active()==0&&factoryCalls==1);++cases;}
 {Run r;r.Start();r.Content()->Reset();r.Tick(5000);assert(r.Active()==1&&r.actor.motion.paths==1);++cases;}
 {Run r;r.Start();assert(r.actor.AIM_Destroy());r.actor.functions.Update(10000);assert(!factoryCalls&&r.actor.functions.Empty());++cases;}
 for(uint32 mode=0;mode<4;++mode){Run r;r.Start();failFactory=true;r.Tick(5000);for(int i=1;i<30;++i)r.Tick(1000);auto content=r.Content();assert(content->_upperExhausted);if(mode){r.actor.i_disabledAI=r.actor.i_AI;r.actor.i_AI=mode==2?nullptr:mode==3?r.actor.i_disabledAI:&r.temporary;}failFactory=false;r.actor.instance.RearmErunak(r.player.guid);assert(content->reentries==1);if(mode==1)assert(r.temporary.guidCalls==1);r.Tick(0);assert(r.Active()==1);++cases;}
 {Run r;scenario.step=0;r.actor.db=362063;r.actor.z=245;r.actor.instance.upperErunak={0,0};r.actor.instance.lowerErunak=r.actor.guid;r.player.reject=true;r.Accept();for(int i=1;i<30;++i)r.Tick(1000);assert(r.Content()->_lowerEntries.at(r.player.guid).exhausted&&r.player.calls==30);r.player.reject=false;r.actor.instance.RearmErunak(r.player.guid);r.Tick(0);assert(r.player.GetQuestObjectiveData(40341,105805)==1&&r.Content()->_introDone);++cases;}
 {Run r;scenario.step=0;r.actor.db=362063;r.actor.z=245;r.player.contact=false;r.Tick(1000);assert(!r.player.calls);r.player.contact=true;r.Tick(1000);assert(r.player.GetQuestObjectiveData(40341,105805)==1&&r.actor.talks.empty());r.Tick(2999);assert(r.actor.talks.empty());r.Tick(1);assert(r.actor.talks.size()==1);++cases;}
 for(uint32 who=0;who<2;++who)for(uint32 bad=0;bad<7;++bad){Run r;Creature* target=who?&r.adel:&r.grash;switch(bad){case 0:target->entry=999;break;case 1:target->db=999;break;case 2:target->phase=false;break;case 3:target->world=false;break;case 4:target->preDelete=true;break;case 5:target->alive=false;break;case 6:target->i_AI=nullptr;break;}r.Accept();r.Tick(1000);assert(!r.actor.motion.paths&&!factoryCalls);++cases;}
 for(uint32 flag:{uint32(UNIT_STATE_ROOT),uint32(UNIT_STATE_CASTING)}){Run r;r.grash.states|=flag;r.Accept();r.Tick(1000);assert(!r.actor.motion.paths);r.grash.states=0;r.Tick(1000);r.Tick(5000);assert(r.Active()==1);++cases;}
 {Run r;r.grash.i_disabledAI=r.grash.i_AI;r.grash.i_AI=nullptr;r.Accept();r.Tick(5000);assert(r.Active()==1&&r.grash.motion.paths==1);++cases;}
 {Run r;r.Start();r.player.near=false;liveMap.players={{&r.second}};r.Tick(5000);assert(!factoryCalls&&r.Content()->_upperRecipient==r.player.guid);r.player.near=true;r.Tick(1000);assert(r.Active()==1&&conversations.back()->creator==&r.player&&!r.second.binds);++cases;}
 {Run r;liveMap.players={{&r.player},{&r.second}};r.Tick(1000);assert(!r.actor.motion.paths&&!r.Content()->_upperPending);r.Accept();r.Tick(5000);assert(r.Active()==1&&conversations.back()->creator==&r.player);++cases;}
 {Run r;r.Start();scenario.step=5;r.Tick(5000);assert(!factoryCalls&&!r.Content()->_upperPending&&r.actor.motion.paths==1);++cases;}
 {Run r;r.Accept();uint32 damage=100;r.Content()->DamageTakenAfterScaling(&r.player,damage,0);assert(!damage&&r.Content()->wait_heal);r.actor.hp=41;r.Content()->OnHealApplied(&r.player,41,0);r.Tick(1000);assert(!r.Content()->wait_heal&&!r.actor.HasUnitState(UNIT_STATE_STUNNED));r.Tick(4000);assert(r.Active()==1);++cases;}
 {Run r;scenario.step=0;r.actor.db=362063;r.actor.z=245;r.player.near=false;Unit pet;r.Content()->MoveInLineOfSight_Safe(&pet);r.Tick(5000);assert(!r.player.calls);r.player.near=true;r.Tick(0);assert(r.player.GetQuestObjectiveData(40341,105805)==1);++cases;}
 std::cout<<"PASS "<<cases<<" full current Erunak/both headers, native quest/objective/lifecycle/FunctionProcessor/instance re-entry/path reader plus native Conversation payload/per-view/expiry ASAN contexts: exact upper role/native3+actualascent/original40341, installed paths once/failed-part retry/no replay, native5s clock, actual two speakers/209312 metadata/player creator/quest binding/47816 expiry, finite failures/original re-entry and shared lower no-away-tick/loading recovery. Registry/movement/creator/group/scope proxies remain controlled; no actual client route/voice/camera/full1066 claim.\n";
#endif
}
