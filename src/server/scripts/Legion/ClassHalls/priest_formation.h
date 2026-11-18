#pragma once
#include "EventObject.h"
#include "EventObjectData.h"
#include "MotionMaster.h"
#include "SpellAuraEffects.h"
#include "SpellAuras.h"

namespace PriestFormation
{
constexpr uint32 QuestId=40938, SceneId=1191, SceneSpell=202457;
Position const Entry={1249.25f,1344.14f,185.0f,.00925024f};
// Exact matching26972 Path/PathNode/Location points, not later map coordinates.
Position const Ritual={1335.02783f,1344.22229f,177.137268f,0}; // Path16138
Position const Target={1417.67712f,1343.95483f,184.22673f,3.13932f}; // Path16137
Position const Grip={1384.08679f,1344.2135f,176.737564f,.202962f}; // Path16239 final
enum class Stage:uint8{None,Intro,Ready,Casting,RitualDone,Ceremony,Finished};
struct State
{
 std::weak_ptr<QuestCancellationReceipt const> receipt;
 Stage stage=Stage::None;
 ObjectGuid anchor,cast,cast2;
 uint32 elapsed=0,castAt=0,ritualAt=0,gripAt=0,ceremonyAt=0,spell=0,wait=1000,entryWait=0;
 uint8 attempts=0,leadAttempts=0;
 bool ready=false,tick=false,grip=false,gripArrived=false,entryPending=false,finish=false;
};
std::mutex stateMutex;
std::unordered_map<ObjectGuid,State> states;
bool Active(Player* p)
{
 return p&&p->IsInWorld()&&p->CanContact()&&p->IsAlive()&&p->getClass()==CLASS_PRIEST&&!p->GetQuestRewardStatus(QuestId)&&
  (p->GetQuestStatus(QuestId)==QUEST_STATUS_INCOMPLETE||p->GetQuestStatus(QuestId)==QUEST_STATUS_COMPLETE);
}
bool Prior(Player* p,uint32 last)
{
 for(uint32 credit:{102466u,102465u,102652u,102653u,102654u}){
  if(p->GetQuestObjectiveData(QuestId,credit)<1)return false;if(credit==last)return true;
 }
 return false;
}
State Load(Player* p)
{
 if(!p)return {};std::lock_guard<std::mutex> lock(stateMutex);auto it=states.find(p->GetGUID());return it==states.end()?State():it->second;
}
void Save(Player* p,State const& state){std::lock_guard<std::mutex> lock(stateMutex);states[p->GetGUID()]=state;}
void Forget(Player* p){if(p){std::lock_guard<std::mutex> lock(stateMutex);states.erase(p->GetGUID());}}
bool Accepted(Player* p,State const& state)
{
 auto receipt=state.receipt.lock();return Active(p)&&receipt&&!receipt->load(std::memory_order_acquire)&&receipt==p->GetQuestCancellationReceipt(QuestId);
}
bool Current(WorldObject const* object,Player* p)
{
 if(!object||!Active(p)||object->GetMap()!=p->GetMap()||!object->HasQuestObjectScope()||object->IsQuestObjectExpired())return false;
 auto scope=object->GetQuestObjectScope().lock();return scope&&scope->Owner==p->GetGUID()&&scope->CancellationReceipt()==p->GetQuestCancellationReceipt(QuestId);
}
EventObject* Event(Player* p,uint32 entry)
{
 if(!p||p->GetMapId()!=1512||(entry!=119&&entry!=120))return nullptr;
 auto event=p->GetMap()->GetEventObject(ObjectGuid::Create<HighGuid::EventObject>(1512,entry,entry==119?400319:400320));
 return event&&event->IsInWorld()&&event->GetEntry()==entry&&event->InSamePhase(p)?event:nullptr;
}
bool AtEvent(Player* p,uint32 id)
{
 auto event=Event(p,id);return event&&event->eventTemplate&&p->IsWithinDistInMap(event,event->eventTemplate->radius)&&p->IsWithinLOSInMap(event);
}
uint32 Bar(Player* p)
{
 // Same priority as native scene PlayerConditions38059/60/61. Inventory counts
 // include equipped and bag items, exclude bank, and do not require this spec.
 if(p->HasItemCount(128827,1,false))return 202749;
 if(p->HasItemCount(128868,1,false))return 216416;
 if(p->HasItemCount(128825,1,false))return 216419;
 return 0;
}
uint32 Channel(uint32 bar){return bar==202749?202747:bar==216416?216409:bar==216419?216418:0;}
Creature* Anchor(Player* p,State const& state)
{
 auto actor=p&&p->GetMapId()==1512?p->GetMap()->GetCreature(state.anchor):nullptr;
 return actor&&actor->GetEntry()==102528&&Current(actor,p)&&actor->IsAlive()?actor:nullptr;
}
Creature* EnsureAnchor(Player* p)
{
 auto state=Load(p);if(!Accepted(p,state)||p->GetMapId()!=1512)return nullptr;if(auto actor=Anchor(p,state))return actor;
 auto properties=sSummonPropertiesStore.LookupEntry(2912);if(!properties)return nullptr;
 auto actor=p->SummonCreature(102528,Target,p->GetGUID(),TEMPSUMMON_TIMED_DESPAWN,600000,0,properties);
 if(!actor||!Current(actor,p)){if(actor)actor->DespawnOrUnsummon();return nullptr;}
 actor->SetReactState(REACT_PASSIVE);actor->SetFlag(UNIT_FIELD_FLAGS,UNIT_FLAG_IMMUNE_TO_PC|UNIT_FLAG_IMMUNE_TO_NPC);
 state.anchor=actor->GetGUID();Save(p,state);return actor;
}
void ClearBars(Player* p){for(uint32 spell:{202835u,202749u,216416u,216419u})p->RemoveAurasDueToSpell(spell,p->GetGUID());}
void Cleanup(Player* p,bool cancelScene=true)
{
 if(!p)return;auto state=Load(p);Forget(p); // invalidate before aura removal callbacks
 if(auto actor=p->GetMap()?p->GetMap()->GetCreature(state.anchor):nullptr)if(actor->GetEntry()==102528&&actor->ToTempSummon()&&actor->ToTempSummon()->GetSummonerGUID()==p->GetGUID())actor->DespawnOrUnsummon();
 for(uint32 spell:{202747u,216409u,216418u})p->RemoveAurasDueToSpell(spell,p->GetGUID());
 ClearBars(p);if(cancelScene)p->RemoveAurasDueToSpell(SceneSpell,p->GetGUID());
}
bool SceneLive(Player* p,State const& state)
{
 return Accepted(p,state)&&p->GetMapId()==1512&&p->HasAura(SceneSpell,p->GetGUID())&&p->HasSceneStatus(SceneId,SCENE_LAUNCH);
}
bool CanStart(Player* p)
{
 return Active(p)&&p->GetMapId()==1512&&!p->HasPendingTeleport()&&Prior(p,102465)&&!p->GetQuestObjectiveData(QuestId,102654)&&
  !p->GetVehicle()&&!p->isInFlight()&&!p->HasAura(SceneSpell)&&Bar(p)&&(AtEvent(p,119)||AtEvent(p,120));
}
void Started(Player* p)
{
 if(!Active(p)||p->GetMapId()!=1512||!Prior(p,102465)||!p->HasAura(SceneSpell,p->GetGUID())||!p->HasSceneStatus(SceneId,SCENE_LAUNCH))return;
 State state;state.receipt=p->GetQuestCancellationReceipt(QuestId);state.stage=Stage::Intro;Save(p,state);
}
bool CanReady(Player* p)
{
 auto state=Load(p);return SceneLive(p,state)&&state.ready&&state.stage==Stage::Intro&&AtEvent(p,120)&&Bar(p)&&EnsureAnchor(p);
}
void Ready(Player* p)
{
 auto state=Load(p);if(!SceneLive(p,state)||!state.ready||!p->HasAura(202835,p->GetGUID())||p->GetQuestObjectiveData(QuestId,102652)<1)return;
 uint32 bar=Bar(p);if(!bar)return;state.stage=Stage::Ready;state.spell=Channel(bar);Save(p,state);
 p->CastSpell(p,bar,true);
}
bool CanChannel(Player* p,uint32 spell)
{
 auto state=Load(p);return SceneLive(p,state)&&state.stage==Stage::Ready&&state.spell==spell&&Prior(p,102652)&&
  p->HasAura(202835,p->GetGUID())&&p->HasAura(Bar(p),p->GetGUID())&&p->GetExactDist(&Ritual)<15&&Anchor(p,state);
}
GameObject* Portal(Player* p)
{
 if(!p||p->GetMapId()!=1220)return nullptr;std::list<GameObject*> objects;p->GetGameObjectListWithEntryInGrid(objects,246663,25);
 for(auto object:objects)if(object->GetDBTableGUIDLow()==25164530&&object->InSamePhase(p)&&object->IsWithinDistInMap(p,object->GetInteractionDistance())&&p->IsWithinLOSInMap(object))return object;
 return nullptr;
}
bool Returning(Player* p){return p&&p->getClass()==CLASS_PRIEST&&p->GetQuestRewardStatus(QuestId)&&Portal(p);}
bool EntryCast(Player* p){auto state=Load(p);return Active(p)&&Portal(p)&&Prior(p,102466)&&Accepted(p,state)&&state.entryPending;}
bool FinishCast(Player* p,uint32 spell)
{
 auto state=Load(p);if(!SceneLive(p,state))return false;
 if(spell==202945)return state.stage==Stage::RitualDone&&state.gripArrived&&state.grip&&state.elapsed-state.gripAt>=28000&&Prior(p,102653);
 if(spell==202491)return state.stage==Stage::Ceremony&&state.finish&&state.elapsed-state.ceremonyAt>=4000&&Prior(p,102654);
 return spell==202982&&state.stage==Stage::RitualDone&&state.grip&&state.elapsed-state.ritualAt>=12000&&Prior(p,102653);
}
void Tick(Player* p,uint32 diff)
{
 auto state=Load(p);if(!Active(p)){if(state.stage!=Stage::None||state.entryPending)Cleanup(p);return;}
 if(p->GetQuestStatus(QuestId)==QUEST_STATUS_COMPLETE&&!Prior(p,102654))p->SetQuestStatus(QuestId,QUEST_STATUS_INCOMPLETE);
 if((state.stage!=Stage::None||state.entryPending)&&!Accepted(p,state)){Cleanup(p);return;}
 if(state.entryPending){
  if(p->GetMapId()==1512&&!p->HasPendingTeleport()&&p->GetExactDist(&Entry)<8&&Prior(p,102466)){
   state.entryPending=false;Save(p,state);if(!p->GetQuestObjectiveData(QuestId,102465))p->KilledMonsterCredit(102465);state=Load(p);
  }else if(state.entryWait<=diff){state.entryPending=false;Save(p,state);}else{state.entryWait-=diff;Save(p,state);}
 }
 if(state.stage!=Stage::None){
  if(!SceneLive(p,state)){Cleanup(p);return;}state.elapsed+=diff;
  // Missing targets and canceled/failed channels never imply a completed
  // ritual. Restore its real selected bar and target for a bounded retry.
  if(state.stage==Stage::Casting&&(!Anchor(p,state)||state.elapsed-state.castAt>16000)){
   p->InterruptNonMeleeSpells(true,state.spell);p->RemoveAurasDueToSpell(state.spell,p->GetGUID());
   if(auto actor=Anchor(p,state))actor->RemoveAurasDueToSpell(state.spell,p->GetGUID());
   state.stage=Stage::Ready;state.cast.Clear();state.cast2.Clear();state.tick=false;
  }
  if(state.grip&&!state.gripArrived&&p->GetExactDist(&Grip)<4&&p->movespline->Finalized())state.gripArrived=true;
  if(state.elapsed>600000){Cleanup(p);return;}Save(p,state);
  if(state.stage==Stage::Ready&&!Anchor(p,state))EnsureAnchor(p);
  return;
 }
 // Replay the real scene from its actual entrance/central checkpoint after a
 // partial retry. Starting presentation grants no milestone or ritual result.
 if(state.wait>diff){state.wait-=diff;Save(p,state);return;}state.wait=1000;
 if(state.attempts<3&&CanStart(p)){++state.attempts;Save(p,state);p->CastSpell(p,SceneSpell,true);}else Save(p,state);
}
}

class spell_priest_formation_scene : public SpellScript
{
 PrepareSpellScript(spell_priest_formation_scene);
 SpellCastResult Check(){return PriestFormation::CanStart(GetCaster()->ToPlayer())?SPELL_CAST_OK:SPELL_FAILED_BAD_TARGETS;}
 void Register()override{OnCheckCast+=SpellCheckCastFn(spell_priest_formation_scene::Check);}
};
class aura_priest_formation_scene : public AuraScript
{
 PrepareAuraScript(aura_priest_formation_scene);
 void Applied(AuraEffect const*,AuraEffectHandleModes){PriestFormation::Started(GetTarget()->ToPlayer());}
 void Removed(AuraEffect const*,AuraEffectHandleModes){auto p=GetTarget()->ToPlayer();if(p&&PriestFormation::Load(p).stage!=PriestFormation::Stage::None)PriestFormation::Cleanup(p,false);}
 void Register()override{
  AfterEffectApply+=AuraEffectApplyFn(aura_priest_formation_scene::Applied,EFFECT_0,SPELL_AURA_ACTIVATE_SCENE,AURA_EFFECT_HANDLE_REAL);
  AfterEffectRemove+=AuraEffectRemoveFn(aura_priest_formation_scene::Removed,EFFECT_0,SPELL_AURA_ACTIVATE_SCENE,AURA_EFFECT_HANDLE_REAL);
 }
};
class spell_priest_formation_ready : public SpellScript
{
 PrepareSpellScript(spell_priest_formation_ready);
 SpellCastResult Check(){return PriestFormation::CanReady(GetCaster()->ToPlayer())?SPELL_CAST_OK:SPELL_FAILED_BAD_TARGETS;}
 void Finished(){PriestFormation::Ready(GetCaster()->ToPlayer());}
 void Register()override{OnCheckCast+=SpellCheckCastFn(spell_priest_formation_ready::Check);OnFinishCast+=SpellCastFn(spell_priest_formation_ready::Finished);}
};
class spell_priest_formation_channel : public SpellScript
{
 PrepareSpellScript(spell_priest_formation_channel);
 SpellCastResult Check(){return PriestFormation::CanChannel(GetCaster()->ToPlayer(),GetSpellInfo()->Id)?SPELL_CAST_OK:SPELL_FAILED_BAD_TARGETS;}
 void Select(WorldObject*& target){auto p=GetCaster()->ToPlayer();target=PriestFormation::Anchor(p,PriestFormation::Load(p));}
 void Cast(){auto p=GetCaster()->ToPlayer();auto state=PriestFormation::Load(p);if(!PriestFormation::CanChannel(p,GetSpellInfo()->Id))return;state.stage=PriestFormation::Stage::Casting;state.castAt=state.elapsed;state.cast=GetSpell()->m_castGuid[0];state.cast2=GetSpell()->m_castGuid[1];state.tick=false;PriestFormation::Save(p,state);}
 void Hit(){auto p=GetCaster()->ToPlayer();auto state=PriestFormation::Load(p);if(GetHitUnit()==PriestFormation::Anchor(p,state))if(auto aura=GetHitAura()){aura->SetScriptGuid(409380,GetSpell()->m_castGuid[0]);aura->SetScriptGuid(409381,GetSpell()->m_castGuid[1]);}}
 void Register()override{
  OnCheckCast+=SpellCheckCastFn(spell_priest_formation_channel::Check);OnCast+=SpellCastFn(spell_priest_formation_channel::Cast);
  OnObjectTargetSelect+=SpellObjectTargetSelectFn(spell_priest_formation_channel::Select,EFFECT_0,TARGET_UNIT_NEARBY_ENTRY);AfterHit+=SpellHitFn(spell_priest_formation_channel::Hit);
 }
};
class aura_priest_formation_channel : public AuraScript
{
 PrepareAuraScript(aura_priest_formation_channel);
 ObjectGuid cast,cast2;
 void SetGuid(uint32 type,ObjectGuid const& guid)override{if(type==409380)cast=guid;else if(type==409381)cast2=guid;}
 void Periodic(AuraEffect const*)
 {
  PreventDefaultAction();auto p=GetCaster()?GetCaster()->ToPlayer():nullptr;if(!p)return;auto state=PriestFormation::Load(p);
  if(!PriestFormation::SceneLive(p,state)||state.stage!=PriestFormation::Stage::Casting||state.cast!=cast||state.cast2!=cast2||(cast.IsEmpty()&&cast2.IsEmpty())||state.tick||
   state.spell!=GetId()||GetUnitOwner()!=PriestFormation::Anchor(p,state)||p->GetExactDist(&PriestFormation::Ritual)>=15)return;
  // The actual native14s periodic tick is the receipt, not aura presence,
  // client scene timing, cast acceptance, a death or a scripted delay.
  state.tick=true;PriestFormation::Save(p,state);auto result=p->CastSpell(p,202881,true);state=PriestFormation::Load(p);
  if(result==SPELL_CAST_OK&&p->GetQuestObjectiveData(40938,102653)>0){state.stage=PriestFormation::Stage::RitualDone;state.ritualAt=state.elapsed;}
  else{state.stage=PriestFormation::Stage::Ready;state.tick=false;}PriestFormation::Save(p,state);
 }
 void Removed(AuraEffect const*,AuraEffectHandleModes)
 {
  auto p=GetCaster()?GetCaster()->ToPlayer():nullptr;if(!p||GetTargetApplication()->GetRemoveMode()==AURA_REMOVE_BY_EXPIRE)return;
  auto state=PriestFormation::Load(p);if(state.cast==cast&&state.cast2==cast2&&state.stage==PriestFormation::Stage::Casting){state.stage=PriestFormation::Stage::Ready;state.cast.Clear();state.cast2.Clear();state.tick=false;PriestFormation::Save(p,state);}
 }
 void Register()override{
  OnEffectPeriodic+=AuraEffectPeriodicFn(aura_priest_formation_channel::Periodic,EFFECT_0,SPELL_AURA_PERIODIC_TRIGGER_SPELL);
  AfterEffectRemove+=AuraEffectRemoveFn(aura_priest_formation_channel::Removed,EFFECT_0,SPELL_AURA_PERIODIC_TRIGGER_SPELL,AURA_EFFECT_HANDLE_REAL);
 }
};
class spell_priest_formation_ritual : public SpellScript
{
 PrepareSpellScript(spell_priest_formation_ritual);
 SpellCastResult Check(){auto p=GetCaster()->ToPlayer();auto state=PriestFormation::Load(p);return PriestFormation::SceneLive(p,state)&&state.stage==PriestFormation::Stage::Casting&&state.tick&&(!state.cast.IsEmpty()||!state.cast2.IsEmpty())&&PriestFormation::Prior(p,102652)?SPELL_CAST_OK:SPELL_FAILED_BAD_TARGETS;}
 void Register()override{OnCheckCast+=SpellCheckCastFn(spell_priest_formation_ritual::Check);}
};
class spell_priest_formation_finish : public SpellScript
{
 PrepareSpellScript(spell_priest_formation_finish);
 SpellCastResult Check(){return PriestFormation::FinishCast(GetCaster()->ToPlayer(),GetSpellInfo()->Id)?SPELL_CAST_OK:SPELL_FAILED_BAD_TARGETS;}
 void Register()override{OnCheckCast+=SpellCheckCastFn(spell_priest_formation_finish::Check);}
};
class scene_priest_formation : public SceneTriggerScript
{
public:
 scene_priest_formation():SceneTriggerScript("scene_priest_formation"){}
 bool OnTrigger(Player* p,SpellScene const* scene,std::string event)override
 {
  using namespace PriestFormation;if(!scene||scene->MiscValue!=SceneId)return false;auto state=Load(p);
  if(!SceneLive(p,state))return true;
  if(event=="ReadyForSpell"){
   // Native main's fixed waits alone total35s, plus real client actor movement.
   if(state.stage==Stage::Intro&&state.elapsed>=35000){state.ready=true;Save(p,state);}return true;
  }
  if(event=="LifeGrip"){
   if(state.stage==Stage::RitualDone&&!state.grip&&state.elapsed-state.ritualAt>=12000){state.grip=true;state.gripAt=state.elapsed;Save(p,state);if(p->CastSpell(p,202982,true)!=SPELL_CAST_OK){state.grip=false;Save(p,state);}}return true;
  }
  if(event=="CeremonyComplete"){
   if(FinishCast(p,202945)){p->CastSpell(p,202945,true);if(Prior(p,102654)){state=Load(p);state.stage=Stage::Ceremony;state.ceremonyAt=state.elapsed;Save(p,state);}}return true;
  }
  if(event=="PortPlayer"){
   if(state.stage==Stage::Ceremony&&!state.finish&&state.elapsed-state.ceremonyAt>=4000){state.finish=true;Save(p,state);if(p->CastSpell(p,202491,true)!=SPELL_CAST_OK){state.finish=false;Save(p,state);}}return true;
  }
  if(event=="complete"){Cleanup(p,false);return true;}
  return false;
 }
};
class eventobject_priest_formation : public EventObjectScript
{
public:
 eventobject_priest_formation():EventObjectScript("eventobject_priest_formation"){}
 bool IsTriggerMeets(Player* p,EventObject* event)override
 {
  if(!event||event->GetMapId()!=1512||(event->GetEntry()!=119&&event->GetEntry()!=120))return true;
  if(!PriestFormation::Active(p)||event!=PriestFormation::Event(p,event->GetEntry()))return false;
  return event->GetEntry()==119?PriestFormation::CanStart(p):PriestFormation::CanReady(p);
 }
 bool OnTrigger(Player* p,EventObject* event,bool enter)override
 {
  if(!p||!p->IsAlive())return false;if(enter&&!IsTriggerMeets(p,event))return false;
  SmartScript script;script.OnInitialize(event,nullptr);script.ProcessEventsFor(enter?SMART_EVENT_EVENTOBJECT_ONTRIGGER:SMART_EVENT_EVENTOBJECT_OFFTRIGGER,p,event->GetEntry());return true;
 }
};
