#include <cassert>
#include "JournalsFixture.inc"
#include "StormheimTorchTargets.h"
using Object=WorldObject;
constexpr uint32 TYPEMASK_UNIT=1,TYPEMASK_GAMEOBJECT=2;
#include "JournalRelations.inc"
#include "QuestMenu.inc"
#include "QuestSource.inc"
constexpr uint32 QUEST_SPECIAL_FLAGS_NOT_REMOVE_SOURCE=0x10;
#include "MurkyTakeSource.inc"
Item* Player::StoreNewItem(ItemPosCountVec const&,uint32 id,bool){bags[id]+=storeCount;static Item item;return &item;}
bool WorldObject::hasInvolvedQuest(uint32 id){auto bounds=store.enders.equal_range(GetEntry());for(auto i=bounds.first;i!=bounds.second;++i)if(i->second==id)return true;return false;}
namespace ObjectAccessor {Object* GetObjectByTypeMask(Player& p,ObjectGuid guid,uint32){return GetCreature(p,guid);}}
namespace WorldPackets {namespace Quest {struct QuestGiverRequestReward {ObjectGuid QuestGiverGUID;uint32 QuestID;};}}
struct WorldSession {Player* _player;void HandleQuestGiverRequestReward(WorldPackets::Quest::QuestGiverRequestReward&);};
#include "JournalRequest.inc"
#include "MurkyAuraStack.inc"
using int64=int64_t;using SMART_EVENT=uint32;using ObjectList=std::list<WorldObject*>;
enum {SMART_EVENT_LINK=61,SMART_EVENT_SPELLHIT=8,SMART_EVENT_SPELLHIT_TARGET=31,SMART_EVENT_RESET=25,SMART_EVENT_RESPAWN=11,SMART_EVENT_FLAG_NOT_REPEATABLE=1,SMART_EVENT_FLAG_EVENT_NON_COMBAT=0x40,SMART_EVENT_FLAG_DONT_RESET=0x100,
 SMART_ACTION_CALL_KILLEDMONSTER=33,SMART_ACTION_ADD_AURA=75,SMART_ACTION_FORCE_DESPAWN=41,SMART_SCRIPT_TYPE_EVENTOBJECT=12,SMART_ESCORT_NONE=0,CONDITION_AURA=1,CONDITION_QUESTTAKEN=9,CONDITION_OBJECT_ENTRY=31};
struct ConditionSourceInfo {WorldObject* mConditionTargets[2];ConditionSourceInfo(WorldObject* a,WorldObject* b):mConditionTargets{a,b}{}};
struct Condition {uint32 type=0,target=0,a=0,b=0;bool negative=false;bool Check(ConditionSourceInfo& info)const{auto object=info.mConditionTargets[target];if(!object)return false;uint32 ConditionValue1=a,ConditionValue2=b;bool condMeets=false;switch(type){
 #include "MurkyConditionAura.inc"
 #include "MurkyConditionQuest.inc"
 #include "MurkyConditionType.inc"
 }return negative?!condMeets:condMeets;}};
using ConditionList=std::vector<Condition>;using ConditionTypeContainer=std::map<uint32,ConditionList>;using SmartEventConditionContainer=std::map<std::pair<int64,uint32>,ConditionTypeContainer>;
struct ConditionMgr {SmartEventConditionContainer SmartEventConditionStore;bool IsObjectMeetToConditions(ConditionSourceInfo& info,ConditionList const& c)const{for(auto& x:c)if(!x.Check(info))return false;return true;}bool IsObjectMeetingSmartEventConditions(int64,uint32,uint32,Unit*,WorldObject*)const;ConditionList GetConditionsForSmartEvent(int64 entry,uint32 id,uint32 source){return SmartEventConditionStore[{entry,source}][id+1];}} conditions;auto sConditionMgr=&conditions;
#include "MurkyConditionDispatch.inc"
struct SmartScriptHolder {uint32 event_id=0,link=0,entryOrGuid=98577,source_type=0;bool active=true,runOnce=false;
 struct {uint32 type=8,event_flags=0,event_phase_mask=0,event_chance=100;struct {uint32 spell=194067,school=0,cooldownMin=0,cooldownMax=0;}spellHit;}event;
 struct {uint32 type=33;struct{uint32 creature=98577;}killedMonster;struct{uint32 spell=194069,stack=0;}addAura;struct{uint32 delay=10000;}forceDespawn;}action;
 uint32 target=7;uint32 GetEventType()const{return event.type;}uint32 GetActionType()const{return action.type;}uint32 GetScriptType()const{return source_type;}};
struct MurkyAI;
struct SmartScript {
 // This older scope has no linked-cast opt-in; unexpected use must fail.
 template<class... Args>void QueueLinkedCastRetry(Args const&...){assert(false);}
 template<class... Args>uint32 BeginLinkedCastAttempt(Args const&...){assert(false);return 0;}
 template<class... Args>bool FinishLinkedCastAttempt(Args const&...){assert(false);return false;}
 void UpdateLinkedCastRetries(uint32){}
 void ResetLinkedCastRetries(){}
Creature* me;MurkyAI* ai;std::vector<SmartScriptHolder> mEvents;ObjectGuid mLastInvoker;uint32 mScriptType=0;bool trigger=false;struct DummyEvent{void Remove(){}} eventValue;DummyEvent* event=&eventValue;
 WorldObject* GetBaseObject(){return me;}bool IsInPhase(uint32){return true;}void SetPhase(uint32){}void ResetBaseObject(){}void InitTimer(SmartScriptHolder& e){e.active=true;}bool IsUnit(WorldObject* o){return o&&o->ToUnit();}bool IsPlayer(WorldObject* o){return o&&o->ToPlayer();}bool IsSmart(){return true;}Unit* GetLastInvoker(){return nullptr;}
 ObjectList* GetTargets(SmartScriptHolder const& e,Unit* unit){return new ObjectList{e.target==1?static_cast<WorldObject*>(me):unit};}SmartScriptHolder FindLinkedEvent(uint32 id){for(auto e:mEvents)if(e.event_id==id)return e;return {};}
 void RecalcTimer(SmartScriptHolder&,uint32,uint32){}void OnReset();void ProcessEventsFor(SMART_EVENT,Unit* =nullptr,uint32=0,uint32=0,bool=false,SpellInfo const* =nullptr,GameObject* =nullptr);void ProcessEvent(SmartScriptHolder&,Unit* =nullptr,uint32=0,uint32=0,bool=false,SpellInfo const* =nullptr,GameObject* =nullptr);void ProcessAction(SmartScriptHolder&,Unit* =nullptr,uint32=0,uint32=0,bool=false,SpellInfo const* =nullptr,GameObject* =nullptr);
};
struct MurkyAI {Creature* me;SmartScript script;uint32 mDespawnState=0,mDespawnTime=0,mEscortState=0,mFollowCredit=0,mFollowCreditType=0,mFollowArrivedEntry=0,mFollowArrivedTimer=0;float mFollowDist=0,mFollowAngle=0;ObjectGuid mFollowGuid;
 explicit MurkyAI(Creature* c):me(c){script.me=c;script.ai=this;}SmartScript* GetScript(){return &script;}void Reset(){script.OnReset();}void UpdateDespawn(uint32);void SetDespawnTime(uint32);void StartDespawn();void JustRespawned();};
bool roll_chance_i(uint32){return true;}
#define CAST_AI(T,X) ai
#include "MurkyDespawn.inc"
#include "MurkyStartDespawn.inc"
#include "MurkyRespawn.inc"
#include "MurkyReset.inc"
#include "MurkyEvents.inc"
#include "MurkyEventGate.inc"
 switch(e.GetEventType()){
 case SMART_EVENT_LINK:ProcessAction(e,unit,var0,var1,bvar,spell,gob);break;
 #include "MurkySpellHit.inc"
 }}
#include "MurkyActionGate.inc"
 switch(e.GetActionType()){
 #include "MurkyCredit.inc"
 #include "MurkyAura.inc"
 #include "MurkyDespawnAction.inc"
 }
#include "MurkyLink.inc"
#undef CAST_AI
ConditionList area;
bool AreaTarget(Unit* target,Unit* caster){ConditionSourceInfo info(target,caster);return conditions.IsObjectMeetToConditions(info,area);}
void Targets(){using namespace TorchTargets;TorchTargets::SpellInfo info;info.Id=194067;info.attrs[13]=131072;TorchTargets::Player p;TorchTargets::Creature corpse;corpse.flags=536904448;corpse.entry=98577;
 assert(info.CheckExplicitTarget(&p,nullptr,nullptr)==SPELL_CAST_OK&&info.CheckTarget(&p,&corpse,true)==SPELL_CAST_OK);WorldObjectSpellAreaTargetCheck area;area._caster=&p;area._spellInfo=&info;assert(area(&corpse));corpse.near=false;assert(!area(&corpse));corpse.near=true;corpse.flight=true;assert(!area(&corpse));}
int main(int argc,char** argv){
 _set_abort_behavior(0,_WRITE_ABORT_MSG|_CALL_REPORTFAULT);_CrtSetReportMode(_CRT_ASSERT,_CRTDBG_MODE_FILE);_CrtSetReportFile(_CRT_ASSERT,_CRTDBG_FILE_STDERR);Targets();
 assert(argc==2);std::ifstream in(argv[1]);assert(in);std::string line;std::vector<SmartScriptHolder> events;
 while(std::getline(in,line)){std::istringstream s(line);std::string kind;s>>kind;
  if(kind=="quest"){uint32 id;s>>id;auto& q=store.quests[id];q.id=id;s>>q.flags>>q.FlagsEx>>q.AllowableRaces>>q.SourceItemId>>q.SourceItemIdCount>>q.NextQuestIdChain>>q.PrevQuestID>>q.NextQuestID>>q.special;}
  if(kind=="objective"){uint32 q,id;int index;QuestObjective o;s>>q>>id>>o.Type>>o.ObjectID>>o.Amount>>index>>o.Flags;o.StorageIndex=int8(index);store.quests[q].objectives.push_back(o);}
  if(kind=="starter"||kind=="npc-ender"){uint32 npc,q;s>>npc>>q;(kind=="starter"?store.starters:store.enders).emplace(npc,q);}
  if(kind=="smart"){SmartScriptHolder e;uint32 action,param;s>>e.event_id>>e.link>>e.event.type>>e.event.event_flags>>e.event.spellHit.spell>>e.event.spellHit.school>>e.event.spellHit.cooldownMin>>e.event.spellHit.cooldownMax>>e.event.event_chance>>action>>param>>e.target;e.action.type=action;if(action==33)e.action.killedMonster.creature=param;if(action==75)e.action.addAura.spell=param;if(action==41)e.action.forceDespawn.delay=param;events.push_back(e);}
  if(kind=="condition"){uint32 source,group,entry,unused,elseGroup;Condition c;s>>source>>group>>entry>>c.type>>c.target>>c.a>>c.b>>unused>>c.negative>>elseGroup;assert(!elseGroup);if(source==22)conditions.SmartEventConditionStore[{entry,0}][group].push_back(c);else area.push_back(c);}
 }
 auto& q=store.quests.at(40120);q.special|=QUEST_SPECIAL_FLAGS_KILL|QUEST_SPECIAL_FLAGS_CAST;q.ItemDrop[0]=129725;q.ItemDropQuantity[0]=1;assert(events.size()==3);
 Creature giver;giver.entry=98587;giver.guid={292435};SmartAI giverAI(&giver);creatures[giver.guid]=&giver;itemStore.items[129725].stack=1;
 SpellInfo torch;torch.Id=194067;SpellInfo wrong;wrong.Id=1;
 // Reproduce eight credits from one original repeatable corpse, using the native hit/action/counter paths.
 {Player p;p.Accept(40120);Creature c;c.entry=98577;MurkyAI ai(&c);ai.script.mEvents={events[0]};ai.script.mEvents[0].link=0;ai.script.mEvents[0].event.event_flags=0;auto saved=conditions.SmartEventConditionStore;conditions.SmartEventConditionStore.clear();for(int n=0;n<8;++n)ai.script.ProcessEventsFor(8,&p,0,0,false,&torch);assert(p.GetQuestObjectiveData(40120,98577)==8&&p.CanRewardQuest(&q,false));conditions.SmartEventConditionStore=saved;}
 for(auto team:{ALLIANCE,HORDE}){
  Player p;p.team=team;p.race=team==ALLIANCE?1:2;p.PrepareQuestMenu(giver.guid);assert(p.menu.quests.items.at(40120)==2);p.AddQuestAndCheckCompletion(&q,&giver);assert(p.GiveQuestSourceItem(&q)&&p.bags[129725]==1);
  std::array<Creature,8> corpses;std::vector<std::unique_ptr<MurkyAI>> ais;
  for(unsigned n=0;n<8;++n){auto& c=corpses[n];c.entry=98577;c.guid={292450+n};auto ai=std::make_unique<MurkyAI>(&c);ai->script.mEvents=events;
   assert(AreaTarget(&c,&p));Creature unrelated;unrelated.entry=1;assert(!AreaTarget(&unrelated,&p));Player noQuest;Unit pet;pet.ownerPlayer=&p;
   for(auto u:{static_cast<Unit*>(nullptr),static_cast<Unit*>(&noQuest),&pet})ai->script.ProcessEventsFor(8,u,0,0,false,&torch);
   ai->script.ProcessEventsFor(8,&p,0,0,false,&wrong);assert(!ai->script.mEvents[0].runOnce&&!c.HasAuraEffect(194069,0));
   ai->script.ProcessEventsFor(8,&p,0,0,false,&torch);assert(p.GetQuestObjectiveData(40120,98577)==int32(n+1)&&c.HasAuraEffect(194069,0)&&ai->mDespawnTime==10001&&!AreaTarget(&c,&p));
   for(unsigned repeat=0;repeat<12;++repeat)ai->script.ProcessEventsFor(8,&p,0,0,false,&torch);assert(p.GetQuestObjectiveData(40120,98577)==int32(n+1));
   ai->Reset();ai->script.ProcessEventsFor(8,&p,0,0,false,&torch);assert(p.GetQuestObjectiveData(40120,98577)==int32(n+1)); // Native reset rearms; base aura condition still rejects.
   ai->UpdateDespawn(10000);assert(c.visible&&c.world);ai->UpdateDespawn(2);assert(!c.visible&&c.world);ai->UpdateDespawn(5001);assert(!c.world);ais.push_back(std::move(ai));
  }
  assert(p.completions==1&&p.CanRewardQuest(&q,false));WorldSession session{&p};WorldPackets::Quest::QuestGiverRequestReward request{giver.guid,40120};session.HandleQuestGiverRequestReward(request);assert(p.menu.offers.size()==1);
  // The eighth hit still linked the native visual and cleanup after completing the quest.
  assert(ais.back()->mDespawnState==3&&corpses.back().HasAuraEffect(194069,0));
  assert(p.TakeQuestSourceItem(40120,true)&&!p.bags[129725]);p.quests.clear();assert(!p.CanRewardQuest(&q,false));p.Accept(40120);assert(p.GiveQuestSourceItem(&q)&&p.bags[129725]==1&&!p.GetQuestObjectiveData(40120,98577));auto& c=corpses[0];c.roots.clear();c.world=true;ais[0]->JustRespawned();assert(c.visible&&ais[0]->mDespawnState==0);ais[0]->script.ProcessEventsFor(8,&p,0,0,false,&torch);assert(p.GetQuestObjectiveData(40120,98577)==1);
 }
 {Player p;p.Accept(40120);Creature c;c.entry=98577;c.failAura=true;MurkyAI ai(&c);ai.script.mEvents=events;ai.script.ProcessEventsFor(8,&p,0,0,false,&torch);assert(p.GetQuestObjectiveData(40120,98577)==1&&!c.HasAuraEffect(194069,0)&&ai.mDespawnTime==10001);ai.script.ProcessEventsFor(8,&p,0,0,false,&torch);assert(p.GetQuestObjectiveData(40120,98577)==1);}
 {Player a,b,far;a.Accept(40120);b.Accept(40120);far.Accept(40120);far.rewardRange=false;GroupReference f{&far,nullptr},m{&b,&f},o{&a,&m};Group g;g.first=&o;a.group=&g;Creature c;c.entry=98577;MurkyAI ai(&c);ai.script.mEvents=events;ai.script.ProcessEventsFor(8,&a,0,0,false,&torch);assert(a.GetQuestObjectiveData(40120,98577)==1&&b.GetQuestObjectiveData(40120,98577)==1&&!far.GetQuestObjectiveData(40120,98577));}
 {Player p;p.Accept(40120);p.bank[129725]=1;assert(p.TakeQuestSourceItem(40120,true)&&!p.bank[129725]);p.bags[129725]=1;p.unequip=3;assert(!p.TakeQuestSourceItem(40120,true)&&p.bags[129725]==1);}
 std::cout<<"PASS: native repeated-hit exploit reproduced; production SmartAI condition dispatch, one-shot gates, SpellHit/action33/75/41/LINK, SetAuraStack, despawn/respawn, native target checks and quest handlers. Both factions, eight distinct corpses, eighth visual/cleanup, invalid invokers, group range, reset-while-burning, retry and aura failure. World/inventory/cast scheduling/client services remain adapters.\n";
}
