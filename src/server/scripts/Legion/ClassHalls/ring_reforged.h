#pragma once
#include "ScriptedGossip.h"
#include "MotionMaster.h"

namespace RingReforged
{
constexpr uint32 QuestId=40225, ActorEntry=96528, RockEntry=245122, OathPath=473698;
// The native circle bunny/quest POI supplies X/Y; matching1469 terrain supplies
// floor Z. The missing path and presentation cadence are reconstructed.
Position const Circle={824.349f,1093.92f,49.3583f,4.70418f};
Position const OutsideNobundo={824.283f,1090.97f,49.4341f,4.70418f};
Position const RockPose={887.451f,1106.41f,49.4598f,.237051f};
Position const Approach={873.0f,1109.7f,47.0f,.237051f};
bool Active(Player* p)
{
 return p&&p->IsInWorld()&&p->CanContact()&&p->IsAlive()&&!p->HasPendingTeleport()&&
  p->getClass()==CLASS_SHAMAN&&p->GetMapId()==1469&&!p->GetQuestRewardStatus(QuestId)&&
  (p->GetQuestStatus(QuestId)==QUEST_STATUS_INCOMPLETE||p->GetQuestStatus(QuestId)==QUEST_STATUS_COMPLETE);
}
bool Current(WorldObject const* object,Player* p)
{
 if(!object||!Active(p)||object->GetMap()!=p->GetMap()||!object->HasQuestObjectScope()||object->IsQuestObjectExpired())return false;
 auto scope=object->GetQuestObjectScope().lock();
 return scope&&scope->Owner==p->GetGUID()&&scope->CancellationReceipt()==p->GetQuestCancellationReceipt(QuestId);
}
Player* Owner(Creature* actor)
{
 auto scope=actor&&actor->HasQuestObjectScope()?actor->GetQuestObjectScope().lock():nullptr;
 auto player=scope?ObjectAccessor::GetPlayer(*actor,scope->Owner):nullptr;
 return Current(actor,player)?player:nullptr;
}
Creature* OwnedActor(Player* p)
{
 if(!Active(p))return nullptr;
 for(auto const& id:*p->GetSummonList(ActorEntry))if(auto actor=p->GetMap()->GetCreature(id))
  if(actor->IsAlive()&&actor->ToTempSummon()&&actor->ToTempSummon()->GetSummonerGUID()==p->GetGUID()&&
    actor->GetUInt32Value(UNIT_FIELD_CREATED_BY_SPELL)==0&&Current(actor,p))return actor;
 return nullptr;
}
struct AI;
AI* Controller(Player* p);
Creature* EnsureActor(Player* p);

enum class Stage:uint8{Ready,Parent,Child,Moving,Oath,Speech,Following,Rock,Done};
struct AI:SmartAI
{
 explicit AI(Creature* actor):SmartAI(actor){}
 Stage stage=Stage::Ready;
 ObjectGuid door,offered;
 uint32 life=300000,wait=2000,deadline=0,openNonce=0,nextNonce=0;
 uint8 doorAttempts=0,walkAttempts=0;
 bool movement=false,expired=false,opening=false,introduced=false;
 bool Personal()const{return me->GetMapId()==1469&&me->HasQuestObjectScope()&&me->ToTempSummon();}
 GameObject* Door()const{return me->GetMap()->GetGameObject(door);}
 void RemoveDoor(){if(auto object=Door())object->Delete();door.Clear();}
 void Reset()override{if(!Personal())SmartAI::Reset();}
 void IsSummonedBy(Unit* summoner)override
 {
  if(!Personal()){SmartAI::IsSummonedBy(summoner);return;}
  auto p=summoner?summoner->ToPlayer():nullptr;
  if(!Current(me,p)){me->DespawnOrUnsummon();return;}
  me->SetReactState(REACT_PASSIVE);me->SetFlag(UNIT_FIELD_FLAGS,UNIT_FLAG_IMMUNE_TO_PC|UNIT_FLAG_IMMUNE_TO_NPC);
  me->SetFlag(UNIT_FIELD_NPC_FLAGS,UNIT_NPC_FLAG_GOSSIP|UNIT_NPC_FLAG_QUESTGIVER);
  stage=p->GetQuestObjectiveData(QuestId,97048)?Stage::Done:p->GetQuestObjectiveData(QuestId,97047)?Stage::Rock:Stage::Ready;
  if(stage!=Stage::Ready)introduced=true;
 }
 bool CreateDoor()
 {
  if(door)return Door()!=nullptr;
  auto p=Owner(me);if(!p||p->GetQuestObjectiveData(QuestId,97048))return false;
  auto object=new GameObject;
  if(!object->Create(sObjectMgr->GetGenerator<HighGuid::GameObject>()->Generate(),RockEntry,me->GetMap(),me->GetPhaseMask(),RockPose,
       G3D::Quat(0,0,.118248f,.992984f),255,GO_STATE_READY)) {delete object;return false;}
  object->SetPhaseId(me->GetPhases(),false);
  if(!object->InheritQuestObjectScope(me)||!object->EnableQuestScopedCollision()){object->ResetMap();delete object;return false;}
  object->SetSummonerGUID(me->GetGUID());object->SetSpawnedByDefault(false);object->SetRespawnTime(300);
  object->RemoveFlag(GAMEOBJECT_FIELD_FLAGS,GO_FLAG_IN_USE);
  if(stage!=Stage::Rock)object->SetFlag(GAMEOBJECT_FIELD_FLAGS,GO_FLAG_NOT_SELECTABLE);
  if(!me->GetMap()->AddToMap(object)){object->CleanupsBeforeDelete();delete object;return false;}
  door=object->GetGUID();return true;
 }
 bool Hello(Player* p)
 {
  if(!Current(me,p)||!me->IsWithinDistInMap(p,INTERACTION_DISTANCE))return false;
  if(stage==Stage::Done){me->RemoveFlag(UNIT_FIELD_NPC_FLAGS,UNIT_NPC_FLAG_GOSSIP);return false;}
  if(stage!=Stage::Ready){p->PlayerTalkClass->ClearMenus();p->PrepareQuestMenu(me->GetGUID());p->SendPreparedGossip(me);return true;}
  playerMenu(p);offered=p->GetGUID();return true;
 }
 void playerMenu(Player* p)
 {
  p->PlayerTalkClass->ClearMenus();p->PlayerTalkClass->GetGossipMenu().SetMenuId(18940);
  p->ADD_GOSSIP_ITEM_DB(18940,0,GOSSIP_SENDER_MAIN,GOSSIP_ACTION_INFO_DEF);
  p->SEND_GOSSIP_MENU(27633,me->GetGUID());
 }
 void sGossipHello(Player* p)override{if(!Personal())SmartAI::sGossipHello(p);}
 void sGossipSelect(Player* p,uint32 menu,uint32 option)override
 {
  if(!Personal()){SmartAI::sGossipSelect(p,menu,option);return;}
  if(menu!=18940||option!=0||offered!=p->GetGUID()||!Current(me,p)||stage!=Stage::Ready||
    !me->IsWithinDistInMap(p,INTERACTION_DISTANCE)||!p->IsWithinLOSInMap(me)||p->GetVehicle()||p->isInFlight()||p->IsMounted())return;
  offered.Clear();p->PlayerTalkClass->SendCloseGossip();
  stage=p->GetQuestObjectiveData(QuestId,97047)?Stage::Child:Stage::Parent;
  wait=0;deadline=5000;movement=expired=false;
  auto result=p->CastSpell(p,stage==Stage::Parent?195070:195359,true);
  if(result!=SPELL_CAST_OK)RetryOath(p);
 }
 bool ParentAllowed(Player* p)const{return Current(me,p)&&stage==Stage::Parent&&p->GetQuestObjectiveData(QuestId,97047)==0;}
 void ParentFinished(Player* p)
 {
  if(!Current(me,p)||stage!=Stage::Parent)return;
  if(p->GetQuestObjectiveData(QuestId,97047)){stage=Stage::Child;deadline=5000;Talk(4,p->GetGUID());}
  else RetryOath(p);
 }
 bool ChildAllowed(Player* p)const{return Current(me,p)&&stage==Stage::Child&&p->GetQuestObjectiveData(QuestId,97047)>0&&p->GetQuestObjectiveData(QuestId,97048)==0;}
 bool BeginMovement(Player* p)
 {
  if(!ChildAllowed(p))return false;stage=Stage::Moving;deadline=20000;return true;
 }
 void Arrived(Player* p){if(Current(me,p)&&stage==Stage::Moving&&p->GetExactDist(&Circle)<2.0f)movement=true;}
 void Expired(Player* p){if(Current(me,p)&&(stage==Stage::Moving||stage==Stage::Oath))expired=true;}
 void RetryOath(Player* p)
 {
  if(Current(me,p)){p->RemoveAurasDueToSpell(195359);stage=Stage::Ready;movement=expired=false;deadline=0;wait=1000;}
 }
 void Walk()
 {
  auto p=Owner(me);if(!p)return;
  Position pos=Approach;float floor=me->GetHeight(pos.GetPositionX(),pos.GetPositionY(),50.0f,true,10.0f);
  if(!std::isfinite(floor)||floor<=INVALID_HEIGHT||floor<40||floor>52){TC_LOG_WARN("scripts.ai","A Ring Reforged: missing native1469 approach floor; no rock unlock");return;}
  pos.m_positionZ=floor;stage=Stage::Following;deadline=30000;++walkAttempts;me->SetWalk(true);
  me->GetMotionMaster()->MovePoint(QuestId,pos,true);
 }
 void MovementInform(uint32 type,uint32 id)override
 {
  if(!Personal()){SmartAI::MovementInform(type,id);return;}
  Position pos=Approach;
  if(type==POINT_MOTION_TYPE&&id==QuestId&&stage==Stage::Following&&me->GetExactDist2d(pos.GetPositionX(),pos.GetPositionY())<2.0f){
   stage=Stage::Rock;deadline=0;if(auto object=Door())object->RemoveFlag(GAMEOBJECT_FIELD_FLAGS,GO_FLAG_NOT_SELECTABLE);
  }
 }
 uint32 BeginOpen(Player* p,GameObject* object)
 {
  if(!Current(me,p)||stage!=Stage::Rock||!object||object->GetGUID()!=door||!Current(object,p)||
    !object->IsInWorld()||object->GetGoState()!=GO_STATE_READY||object->HasFlag(GAMEOBJECT_FIELD_FLAGS,GO_FLAG_NOT_SELECTABLE)||
    !object->IsWithinDistInMap(p,object->GetInteractionDistance())||!p->IsWithinLOSInMap(me)||
    p->GetQuestObjectiveData(QuestId,97047)<1||p->GetQuestObjectiveData(QuestId,97048)>0||openNonce)return 0;
  openNonce=++nextNonce;opening=false;return openNonce;
 }
 bool ArmOpen(Player* p,ObjectGuid const& source,uint32 nonce)
 {
  if(!Current(me,p)||stage!=Stage::Rock||source!=door||!nonce||nonce!=openNonce)return false;
  auto object=Door();if(!object||!Current(object,p)||object->GetGoState()!=GO_STATE_READY)return false;
  opening=true;return true;
 }
 bool ValidatedOpen(Player* p,GameObject* object)
 {
  if(!Current(me,p)||stage!=Stage::Rock||!opening||!openNonce||!object||object->GetGUID()!=door||!Current(object,p)||object->GetGoState()!=GO_STATE_ACTIVE)return false;
  opening=false;openNonce=0;
  // Native195132's self90 runs before its GO33 target in handle_immediate.
  // Defer the same native personal90 dispatcher until SendLoot has acknowledged
  // the real OPEN_LOCK hit after CanOpenLock. Report-use never arms this receipt.
  p->KilledMonsterCredit(97048);
  if(p->GetQuestObjectiveData(QuestId,97048)){stage=Stage::Done;wait=5000;me->RemoveFlag(UNIT_FIELD_NPC_FLAGS,UNIT_NPC_FLAG_GOSSIP);return true;}
  object->ResetDoorOrButton();return false;
 }
 void EndOpen(uint32 nonce){if(openNonce==nonce){opening=false;openNonce=0;}}
 void sQuestReward(Player* p,Quest const* quest,uint32 opt)override
 {
  if(me->GetMapId()==1469&&quest->GetQuestId()==QuestId){if(Personal()){RemoveDoor();me->DespawnOrUnsummon(1000);}return;}
  SmartAI::sQuestReward(p,quest,opt);
 }
 bool CanBeVisibleFor(WorldObject const* seer)const override
 {
  if(me->GetMapId()==1469&&!Personal()&&seer&&seer->IsPlayer())
   if(OwnedActor(const_cast<Player*>(seer->ToPlayer())))return false;
  return true;
 }
 void UpdateAI(uint32 diff)override
 {
  if(!Personal()){SmartAI::UpdateAI(diff);return;}
  auto p=Owner(me);
  if(!p||!me->IsWithinDistInMap(p,120.0f)||life<=diff){RemoveDoor();me->DespawnOrUnsummon();return;}life-=diff;
  if(p->GetQuestObjectiveData(QuestId,97048)&&stage!=Stage::Done){stage=Stage::Done;wait=5000;me->RemoveFlag(UNIT_FIELD_NPC_FLAGS,UNIT_NPC_FLAG_GOSSIP);}
  if(stage==Stage::Done){if(wait>diff)wait-=diff;else{RemoveDoor();me->RemoveFlag(UNIT_FIELD_NPC_FLAGS,UNIT_NPC_FLAG_GOSSIP);}return;}
  if(!Door()){
   if(door){door.Clear();doorAttempts=0;}
   if(wait>diff){wait-=diff;return;}wait=2000;
   if(doorAttempts>=3)return;++doorAttempts;
   if(!CreateDoor()){TC_LOG_WARN("scripts.ai","A Ring Reforged: private rock publication failed (%u/3); no progression",uint32(doorAttempts));return;}
  }
  if(!introduced){introduced=true;Talk(3,p->GetGUID());}
  if(deadline){if(deadline<=diff){deadline=0;
    if(stage==Stage::Following&&walkAttempts<3){Walk();return;}
    TC_LOG_WARN("scripts.ai","A Ring Reforged: missing native movement/oath acknowledgement; no rock unlock");RetryOath(p);return;
   }deadline-=diff;}
  if(stage==Stage::Moving&&movement&&expired){stage=Stage::Speech;deadline=0;wait=14000;Talk(5,p->GetGUID());}
  else if(stage==Stage::Speech){if(wait>diff)wait-=diff;else{Talk(6,p->GetGUID());Walk();}}
 }
};
AI* Controller(Player* p){auto actor=OwnedActor(p);return actor?dynamic_cast<AI*>(actor->AI()):nullptr;}
Creature* EnsureActor(Player* p)
{
 if(!Active(p))return nullptr;if(auto actor=OwnedActor(p))return actor;
 if(p->GetQuestObjectiveData(QuestId,97048))return nullptr;
 bool resumed=p->GetQuestObjectiveData(QuestId,97047)>0;Position spawn=OutsideNobundo;
 bool nearSource=false;
 std::list<Creature*> actors;p->GetCreatureListWithEntryInGrid(actors,ActorEntry,40.0f);
 for(auto actor:actors)if(actor->IsAlive()&&!actor->ToTempSummon()&&actor->GetDBTableGUIDLow()==11377878&&
   actor->InSamePhase(p)&&p->IsWithinLOSInMap(actor)){nearSource=true;break;}
 if(!nearSource&&(!resumed||p->GetExactDist2d(Approach.GetPositionX(),Approach.GetPositionY())>35.0f))return nullptr;
 if(!resumed&&p->GetExactDist2d(Circle.GetPositionX(),Circle.GetPositionY())>30.0f)return nullptr;
 if(resumed){spawn=Approach;float floor=p->GetHeight(spawn.GetPositionX(),spawn.GetPositionY(),50.0f,true,10.0f);if(!std::isfinite(floor)||floor<40||floor>52)return nullptr;spawn.m_positionZ=floor;}
 auto properties=sSummonPropertiesStore.LookupEntry(2912);if(!properties)return nullptr;
 return p->SummonCreature(ActorEntry,spawn,p->GetGUID(),TEMPSUMMON_TIMED_DESPAWN,300000,0,properties);
}
}
class npc_ring_reforged_nobundo:public CreatureScript
{
public:
 npc_ring_reforged_nobundo():CreatureScript("npc_ring_reforged_nobundo"){}
 CreatureAI* GetAI(Creature* actor)const override{return new RingReforged::AI(actor);}
 bool OnGossipHello(Player* p,Creature* actor)override
 {
  if(actor->GetMapId()!=1469||!RingReforged::Active(p))return false;
  auto chosen=RingReforged::EnsureActor(p);
  auto ai=chosen?dynamic_cast<RingReforged::AI*>(chosen->AI()):nullptr;
  return ai&&ai->Hello(p);
 }
 bool OnGossipSelect(Player*,Creature* actor,uint32,uint32)override{return actor->GetMapId()==1469&&actor->HasQuestObjectScope()&&actor->ToTempSummon();}
};
class go_ring_reforged_rock:public GameObjectScript
{
public:
 go_ring_reforged_rock():GameObjectScript("go_ring_reforged_rock"){}
 struct AI:GameObjectAI
 {
  explicit AI(GameObject* object):GameObjectAI(object){}
  void Request(Player* p)
  {
   if(!RingReforged::Current(go,p))return;
   auto ai=RingReforged::Controller(p);
   if(ai&&ai->stage==RingReforged::Stage::Rock&&ai->door==go->GetGUID()&&!ai->openNonce)
    p->CastSpell(go,195132,false);
  }
  bool GossipUse(Player* p)override{Request(p);return true;}
  bool GossipHello(Player* p,bool nativeHit)override
  {
   if(nativeHit){if(auto ai=RingReforged::Controller(p))ai->ValidatedOpen(p,go);}
   else Request(p);
   return true;
  }
 };
 GameObjectAI* GetAI(GameObject* object)const override{return new AI(object);}
};
class spell_ring_reforged_assume:public SpellScript
{
 PrepareSpellScript(spell_ring_reforged_assume);
 SpellCastResult Check(){auto p=GetCaster()->ToPlayer();auto ai=RingReforged::Controller(p);return ai&&ai->ParentAllowed(p)?SPELL_CAST_OK:SPELL_FAILED_BAD_TARGETS;}
 void Hit(SpellEffIndex effect)
 {
  auto p=GetCaster()->ToPlayer();auto ai=RingReforged::Controller(p);
  if(!ai||!ai->ParentAllowed(p)||GetHitPlayer()!=p)PreventHitDefaultEffect(effect);
 }
 void Trigger(SpellEffIndex effect)
 {
  auto p=GetCaster()->ToPlayer();auto ai=RingReforged::Controller(p);
  if(!ai||!ai->ParentAllowed(p))PreventHitDefaultEffect(effect);
 }
 void Finished(){auto p=GetCaster()->ToPlayer();if(auto ai=RingReforged::Controller(p))ai->ParentFinished(p);}
 void Register()override
 {
  OnCheckCast+=SpellCheckCastFn(spell_ring_reforged_assume::Check);
  OnEffectHitTarget+=SpellEffectFn(spell_ring_reforged_assume::Hit,EFFECT_0,SPELL_EFFECT_KILL_CREDIT);
  OnEffectLaunch+=SpellEffectFn(spell_ring_reforged_assume::Trigger,EFFECT_1,SPELL_EFFECT_TRIGGER_SPELL_4);
  OnEffectLaunchTarget+=SpellEffectFn(spell_ring_reforged_assume::Trigger,EFFECT_1,SPELL_EFFECT_TRIGGER_SPELL_4);
  OnFinishCast+=SpellCastFn(spell_ring_reforged_assume::Finished);
 }
};
class spell_ring_reforged_oath:public SpellScript
{
 PrepareSpellScript(spell_ring_reforged_oath);
 SpellCastResult Check(){auto p=GetCaster()->ToPlayer();auto ai=RingReforged::Controller(p);return ai&&ai->ChildAllowed(p)?SPELL_CAST_OK:SPELL_FAILED_BAD_TARGETS;}
 void Hit(SpellEffIndex effect)
 {
  auto p=GetCaster()->ToPlayer();auto ai=RingReforged::Controller(p);
  bool valid=ai&&GetHitPlayer()==p;
  if(effect==EFFECT_0)valid=valid&&ai->BeginMovement(p);
  else valid=valid&&ai->stage==RingReforged::Stage::Moving;
  if(!valid)PreventHitDefaultEffect(effect);
 }
 void Register()override
 {
  OnCheckCast+=SpellCheckCastFn(spell_ring_reforged_oath::Check);
  OnEffectHitTarget+=SpellEffectFn(spell_ring_reforged_oath::Hit,EFFECT_0,SPELL_EFFECT_PLAYER_MOVE_WAYPOINTS);
  OnEffectHitTarget+=SpellEffectFn(spell_ring_reforged_oath::Hit,EFFECT_1,SPELL_EFFECT_APPLY_AURA);
 }
};
class aura_ring_reforged_oath:public AuraScript
{
 PrepareAuraScript(aura_ring_reforged_oath);
 void Removed(AuraEffect const*,AuraEffectHandleModes)
 {
  if(GetTargetApplication()->GetRemoveMode()!=AURA_REMOVE_BY_EXPIRE)return;
  auto p=GetTarget()->ToPlayer();if(auto ai=RingReforged::Controller(p))ai->Expired(p);
 }
 void Register()override{AfterEffectRemove+=AuraEffectRemoveFn(aura_ring_reforged_oath::Removed,EFFECT_1,SPELL_AURA_DUMMY,AURA_EFFECT_HANDLE_REAL);}
};
class spell_ring_reforged_rock:public SpellScript
{
 PrepareSpellScript(spell_ring_reforged_rock);
 ObjectGuid source;uint32 nonce=0;
 SpellCastResult Check()
 {
  auto p=GetCaster()->ToPlayer();auto ai=RingReforged::Controller(p);
  // GetOriginalTargetGUID deliberately captures the original Unit target for
  // unit/self normalization. This spell has a real explicit GO target: retain
  // that full identity on the cast before its self effects are processed.
  if(source.IsEmpty()){auto target=GetExplTargetGObj();if(!target)return SPELL_FAILED_BAD_TARGETS;source=target->GetGUID();}
  auto object=p?p->GetMap()->GetGameObject(source):nullptr;
  if(!ai||!object||object->GetEntry()!=RingReforged::RockEntry)return SPELL_FAILED_BAD_TARGETS;
  if(nonce)return ai->openNonce==nonce&&ai->door==source?SPELL_CAST_OK:SPELL_FAILED_BAD_TARGETS;
  nonce=ai->BeginOpen(p,object);return nonce?SPELL_CAST_OK:SPELL_FAILED_BAD_TARGETS;
 }
 void Hit(SpellEffIndex effect)
 {
  auto p=GetCaster()->ToPlayer();auto ai=RingReforged::Controller(p);
  // Its self-target is processed before GO33. The completed native lock's
  // callback uses the same native personal KilledMonsterCredit endpoint instead.
  if(effect==EFFECT_3||!ai||!nonce||ai->openNonce!=nonce){PreventHitDefaultEffect(effect);return;}
  if(effect==EFFECT_0&&(!GetHitGObj()||GetHitGObj()->GetGUID()!=source||!ai->ArmOpen(p,source,nonce)))PreventHitDefaultEffect(effect);
 }
 void Trigger(SpellEffIndex effect)
 {
  auto p=GetCaster()->ToPlayer();auto ai=RingReforged::Controller(p);
  if(!ai||!nonce||ai->openNonce!=nonce||ai->door!=source)PreventHitDefaultEffect(effect);
 }
 void Finished(){auto p=GetCaster()->ToPlayer();if(auto ai=RingReforged::Controller(p))ai->EndOpen(nonce);}
 void Register()override
 {
  OnCheckCast+=SpellCheckCastFn(spell_ring_reforged_rock::Check);
  OnEffectHitTarget+=SpellEffectFn(spell_ring_reforged_rock::Hit,EFFECT_0,SPELL_EFFECT_OPEN_LOCK);
  OnEffectHitTarget+=SpellEffectFn(spell_ring_reforged_rock::Hit,EFFECT_1,SPELL_EFFECT_SEND_EVENT);
  OnEffectHitTarget+=SpellEffectFn(spell_ring_reforged_rock::Hit,EFFECT_3,SPELL_EFFECT_KILL_CREDIT);
  OnEffectLaunch+=SpellEffectFn(spell_ring_reforged_rock::Trigger,EFFECT_2,SPELL_EFFECT_TRIGGER_SPELL_4);
  OnEffectLaunchTarget+=SpellEffectFn(spell_ring_reforged_rock::Trigger,EFFECT_2,SPELL_EFFECT_TRIGGER_SPELL_4);
  OnFinishCast+=SpellCastFn(spell_ring_reforged_rock::Finished);
 }
};
class player_ring_reforged:public PlayerScript
{
 struct State{std::weak_ptr<QuestCancellationReceipt const> receipt;uint32 wait=1000;uint8 attempts=0;};
 std::mutex mutex;std::unordered_map<ObjectGuid,State> states;
 void Forget(Player* p){if(p){std::lock_guard<std::mutex> lock(mutex);states.erase(p->GetGUID());}}
public:
 player_ring_reforged():PlayerScript("player_ring_reforged"){}
 void OnBeforeLogout(Player* p)override{Forget(p);}
 void OnLogout(Player* p)override{Forget(p);}
 void OnMapChanged(Player* p)override{Forget(p);}
 void OnMovementInform(Player* p,uint32 type,uint32 id)override
 {
  if(type==WAYPOINT_MOTION_TYPE&&id==RingReforged::OathPath)
   if(auto ai=RingReforged::Controller(p))ai->Arrived(p);
 }
 void OnUpdate(Player* p,uint32 diff)override
 {
  if(!RingReforged::Active(p)||p->GetQuestObjectiveData(RingReforged::QuestId,97048)){Forget(p);return;}
  if(p->GetQuestStatus(RingReforged::QuestId)==QUEST_STATUS_COMPLETE)p->SetQuestStatus(RingReforged::QuestId,QUEST_STATUS_INCOMPLETE);
  auto receipt=p->GetQuestCancellationReceipt(RingReforged::QuestId);if(!receipt||receipt->load(std::memory_order_acquire))return;
  State state;{std::lock_guard<std::mutex> lock(mutex);state=states[p->GetGUID()];}
  if(state.receipt.lock()!=receipt){state=State();state.receipt=receipt;}
  if(state.wait>diff)state.wait-=diff;
  else{
   state.wait=1000;
   if(RingReforged::OwnedActor(p))state.attempts=0;
   else if(state.attempts<3&&!p->GetVehicle()&&!p->isInFlight()&&!p->IsMounted()){
    // Being far from either real checkpoint is ordinary travel, not a failed summon.
    bool nearCheckpoint=p->GetExactDist2d(RingReforged::Circle.GetPositionX(),RingReforged::Circle.GetPositionY())<=30||
      (p->GetQuestObjectiveData(RingReforged::QuestId,97047)>0&&p->GetExactDist2d(RingReforged::Approach.GetPositionX(),RingReforged::Approach.GetPositionY())<=35);
    if(nearCheckpoint){++state.attempts;if(!RingReforged::EnsureActor(p))TC_LOG_WARN("scripts.ai","A Ring Reforged: personal Nobundo publication not acknowledged (%u/3); no progression",uint32(state.attempts));}
   }
  }
  {std::lock_guard<std::mutex> lock(mutex);states[p->GetGUID()]=state;}
 }
};
