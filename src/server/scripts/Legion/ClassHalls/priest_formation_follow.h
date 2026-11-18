#pragma once
namespace PriestFormation
{
Position const VelenStart={-698.346f,4494.7f,728.399f,4.18694f};
Position const Upper={-844.597f,4467.76f,736.042f,0};
Position const Lower={-784.91f,4419.19f,602.48f,2.45236f};
Position const PortalPoint={-881.08f,4496.95f,580.311f,5.63959f};
Creature* Leader(Player* p)
{
 if(!Active(p)||p->GetMapId()!=1220)return nullptr;
 for(auto const& guid:*p->GetSummonList(101313))if(auto actor=p->GetMap()->GetCreature(guid))
  if(actor->IsAlive()&&actor->ToTempSummon()&&actor->ToTempSummon()->GetSummonerGUID()==p->GetGUID()&&actor->GetUInt32Value(UNIT_FIELD_CREATED_BY_SPELL)==0&&Current(actor,p))return actor;
 return nullptr;
}
Creature* EnsureLeader(Player* p)
{
 if(!Active(p)||p->GetMapId()!=1220||p->GetQuestObjectiveData(QuestId,102466)||p->GetVehicle()||p->isInFlight())return nullptr;
 auto state=Load(p);auto receipt=p->GetQuestCancellationReceipt(QuestId);if(state.receipt.lock()!=receipt){state=State();state.receipt=receipt;}
 if(auto actor=Leader(p)){state.leadAttempts=0;Save(p,state);return actor;}if(state.leadAttempts>=3)return nullptr;
 std::list<Creature*> actors;p->GetCreatureListWithEntryInGrid(actors,101313,30);bool source=false;
 for(auto actor:actors)if(!actor->ToTempSummon()&&actor->GetDBTableGUIDLow()==371123&&actor->InSamePhase(p)&&p->IsWithinLOSInMap(actor)){source=true;break;}
 if(!source||p->GetExactDist(&VelenStart)>30)return nullptr;
 ++state.leadAttempts;Save(p,state);auto properties=sSummonPropertiesStore.LookupEntry(2912);auto actor=properties?p->SummonCreature(101313,VelenStart,p->GetGUID(),TEMPSUMMON_TIMED_DESPAWN,600000,0,properties):nullptr;
 if(actor&&!Current(actor,p)){actor->DespawnOrUnsummon();return nullptr;}return actor;
}
struct LeaderAI:ScriptedAI
{
 explicit LeaderAI(Creature* actor):ScriptedAI(actor){}
 enum Step:uint8{Street,Pad,Transfer,Hall,AtPortal,Done};
 Step step=Street;uint32 elapsed=0,life=600000,deadline=0;uint8 speech=0,tries=0;bool initialized=false;
 bool Personal()const{return me->GetMapId()==1220&&me->ToTempSummon()&&me->HasQuestObjectScope();}
 Player* Owner()const{auto summon=me->ToTempSummon();auto p=summon?ObjectAccessor::GetPlayer(*me,summon->GetSummonerGUID()):nullptr;return Current(me,p)?p:nullptr;}
 void Move(uint32 id,Position const& pos){deadline=90000;me->SetWalk(true);me->GetMotionMaster()->MovePoint(id,pos,true);}
 void IsSummonedBy(Unit* unit)override
 {
  auto p=unit?unit->ToPlayer():nullptr;if(!Personal())return;if(!Current(me,p)){me->DespawnOrUnsummon();return;}
  initialized=true;me->SetReactState(REACT_PASSIVE);me->SetFlag(UNIT_FIELD_FLAGS,UNIT_FLAG_IMMUNE_TO_PC|UNIT_FLAG_IMMUNE_TO_NPC);
  me->RemoveFlag(UNIT_FIELD_NPC_FLAGS,UNIT_NPC_FLAG_GOSSIP|UNIT_NPC_FLAG_QUESTGIVER);Talk(12,p->GetGUID());Move(1,Upper);
 }
 void MovementInform(uint32 type,uint32 id)override
 {
  if(!Personal()||type!=POINT_MOTION_TYPE)return;
  if(id==1&&step==Street&&me->GetExactDist(&Upper)<3){step=Pad;deadline=0;}
  if(id==2&&step==Hall&&me->GetExactDist(&PortalPoint)<3){step=AtPortal;deadline=0;}
 }
 void sQuestAccept(Player* p,Quest const* quest)override{if(!Personal()&&me->GetMapId()==1220&&me->GetDBTableGUIDLow()==371123&&quest->GetQuestId()==QuestId)EnsureLeader(p);}
 bool CanBeVisibleFor(WorldObject const* seer)const override
 {
  if(!Personal()&&me->GetMapId()==1220&&me->GetDBTableGUIDLow()==371123&&seer&&seer->IsPlayer()&&Leader(const_cast<Player*>(seer->ToPlayer())))return false;return true;
 }
 void UpdateAI(uint32 diff)override
 {
  if(!Personal()||!initialized)return;auto p=Owner();if(!p||me->GetDistance(p)>250||life<=diff){me->DespawnOrUnsummon();return;}life-=diff;elapsed+=diff;
  // Speech cadence/ground navigation are reconstructed from native anchors and
  // the reviewed full footage. Only real route/transfer arrivals award credit.
  if(speech==0&&elapsed>=14000){speech=1;Talk(13,p->GetGUID());}
  if(speech==1&&elapsed>=34000){speech=2;Talk(14,p->GetGUID());}
  if(deadline){if(deadline<=diff){deadline=0;if(step!=Transfer&&++tries<3){if(step==Street)Move(1,Upper);else if(step==Hall)Move(2,PortalPoint);}
    else{TC_LOG_WARN("scripts.ai","Priest40938: Velen movement/transfer unacknowledged; no follow credit");me->DespawnOrUnsummon();}return;}deadline-=diff;}
  if(step==Pad&&p->GetExactDist(&Upper)<8&&!p->HasPendingTeleport()){
   step=Transfer;deadline=60000;me->CastSpell(me,192293,true);
  }
  if(step==Transfer&&me->GetExactDist(&Lower)<8&&p->GetExactDist(&Lower)<15&&!p->HasPendingTeleport()){
   step=Hall;tries=0;Talk(15,p->GetGUID());Move(2,PortalPoint);
  }
  if(step==AtPortal&&me->GetExactDist(&PortalPoint)<3&&Portal(p)&&p->IsWithinLOSInMap(me)){
   if(!p->GetQuestObjectiveData(QuestId,102466))p->KilledMonsterCredit(102466);
   if(p->GetQuestObjectiveData(QuestId,102466)){step=Done;me->DespawnOrUnsummon(5000);}
  }
 }
};
}
class npc_priest_formation_velen:public CreatureScript
{
public:npc_priest_formation_velen():CreatureScript("npc_priest_formation_velen"){}
 CreatureAI* GetAI(Creature* actor)const override{return new PriestFormation::LeaderAI(actor);}
};
class go_priest_formation_portal:public GameObjectScript
{
public:go_priest_formation_portal():GameObjectScript("go_priest_formation_portal"){}
 struct AI:GameObjectAI
 {
  explicit AI(GameObject* object):GameObjectAI(object){}
  bool GossipUse(Player* p)override
  {
   using namespace PriestFormation;if(go->GetMapId()!=1220||go->GetDBTableGUIDLow()!=25164530||!Active(p))return false;
   if(Portal(p)!=go||!Prior(p,102466)||p->HasPendingTeleport())return true;
   auto state=Load(p);state.receipt=p->GetQuestCancellationReceipt(QuestId);state.entryPending=true;state.entryWait=15000;Save(p,state);
   if(p->CastSpell(p,202324,true)!=SPELL_CAST_OK){state.entryPending=false;Save(p,state);}return true;
  }
  bool GossipHello(Player* p,bool)override{return GossipUse(p);}
 };
 GameObjectAI* GetAI(GameObject* object)const override{return new AI(object);}
};
class spell_priest_formation_entry:public SpellScript
{
 PrepareSpellScript(spell_priest_formation_entry);
 SpellCastResult Check(){auto p=GetCaster()->ToPlayer();return PriestFormation::EntryCast(p)||PriestFormation::Returning(p)?SPELL_CAST_OK:SPELL_FAILED_BAD_TARGETS;}
 void Credit(SpellEffIndex effect){if(!PriestFormation::Returning(GetCaster()->ToPlayer()))PreventHitDefaultEffect(effect);}
 void Register()override{OnCheckCast+=SpellCheckCastFn(spell_priest_formation_entry::Check);OnEffectHitTarget+=SpellEffectFn(spell_priest_formation_entry::Credit,EFFECT_1,SPELL_EFFECT_KILL_CREDIT);}
};
class player_priest_formation:public PlayerScript
{
 std::mutex mutex;std::unordered_map<ObjectGuid,uint32> wait;
 void Clear(Player* p){if(p){if(auto actor=PriestFormation::Leader(p))actor->DespawnOrUnsummon();PriestFormation::Cleanup(p);std::lock_guard<std::mutex> lock(mutex);wait.erase(p->GetGUID());}}
public:
 player_priest_formation():PlayerScript("player_priest_formation"){}
 void OnBeforeLogout(Player* p)override{Clear(p);}
 void OnLogout(Player* p)override{Clear(p);}
 void OnMapChanged(Player* p)override{PriestFormation::Tick(p,0);}
 void OnLogin(Player* p)override
 {
  // A crash can persist the infinite scene/override auras before orderly
  // logout cleanup. They have no surviving client instance/cast receipt.
  if(p&&p->getClass()==CLASS_PRIEST&&!p->GetQuestRewardStatus(40938)&&
   (p->GetQuestStatus(40938)==QUEST_STATUS_INCOMPLETE||p->GetQuestStatus(40938)==QUEST_STATUS_COMPLETE))Clear(p);
  // Persisted real follow milestone plus actual native entry receiver repairs
  // a login between completed map transfer and its ordinary acknowledgement.
  if(p&&p->getClass()==CLASS_PRIEST&&p->IsAlive()&&!p->GetQuestRewardStatus(40938)&&
   (p->GetQuestStatus(40938)==QUEST_STATUS_INCOMPLETE||p->GetQuestStatus(40938)==QUEST_STATUS_COMPLETE)&&
   p->GetMapId()==1512&&p->GetExactDist(&PriestFormation::Entry)<8&&
   PriestFormation::Prior(p,102466)&&!p->GetQuestObjectiveData(40938,102465)){
   auto state=PriestFormation::Load(p);state.receipt=p->GetQuestCancellationReceipt(40938);
   state.entryPending=true;state.entryWait=15000;PriestFormation::Save(p,state);
  }
 }
 void OnUpdate(Player* p,uint32 diff)override
 {
  PriestFormation::Tick(p,diff);if(!PriestFormation::Active(p)){std::lock_guard<std::mutex> lock(mutex);wait.erase(p->GetGUID());return;}
  uint32 timer=0;{std::lock_guard<std::mutex> lock(mutex);timer=wait[p->GetGUID()];wait[p->GetGUID()]=timer>diff?timer-diff:1000;}
  if(timer<=diff)PriestFormation::EnsureLeader(p);
 }
};
