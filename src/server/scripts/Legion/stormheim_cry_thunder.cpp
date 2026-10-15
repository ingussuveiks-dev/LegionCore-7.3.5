#include "ScriptMgr.h"
#include "SmartAI.h"
#include "ScriptedCreature.h"
#include "SpellScript.h"
#include "Player.h"
#include "ObjectAccessor.h"
#include "ObjectMgr.h"
#include "QuestData.h"
#include "TemporarySummon.h"
#include "Vehicle.h"
#include "MoveSpline.h"
#include "MotionMaster.h"
#include "Map.h"
#include "ModelIgnoreFlags.h"
#include "stormheim_cry_thunder_route.h"
#include <array>
#include <map>
#include <set>
#include <mutex>
#include <cmath>
#include <algorithm>

namespace StormheimCryThunder
{
constexpr uint32 QuestId=38624;
enum class Phase { Boarding, Assault, Returning, Exiting, Recovering };
struct Slot {ObjectGuid guid;uint32 retry=0;bool counted=false;};
struct Ride {
 ObjectGuid dragon;Map* scene=nullptr;std::set<ObjectGuid> hidden;std::weak_ptr<uint8> lifetime;Phase phase=Phase::Boarding;
 uint32 elapsed=0;size_t node=0;bool requested=false,success=false;
 Position climb;std::array<Slot,SeedCount> targets;std::set<ObjectGuid> storms;
};
std::map<ObjectGuid,Ride> rides;
std::recursive_mutex mutex;
bool Definition()
{
 auto q=sQuestDataStore->GetQuestTemplate(QuestId);if(!q||q->StartScript)return false;
 bool begin=false,kills=false,end=false;
 for(auto const& o:q->GetObjectives())if(!(o.Flags&QUEST_OBJECTIVE_FLAG_OPTIONAL)){
  begin|=o.Type==QUEST_OBJECTIVE_TALKTO&&o.ObjectID==91767&&o.StorageIndex==0&&o.Amount==1;
  kills|=o.Type==QUEST_OBJECTIVE_MONSTER&&o.ObjectID==92360&&o.StorageIndex==3&&o.Amount==150&&(o.Flags&QUEST_OBJECTIVE_FLAG_SEQUENCED);
  end|=o.Type==QUEST_OBJECTIVE_MONSTER&&o.ObjectID==91768&&o.StorageIndex==1&&o.Amount==1&&(o.Flags&QUEST_OBJECTIVE_FLAG_SEQUENCED);
 }
 return begin&&kills&&end;
}
bool SameMap(Player* p){return p&&p->IsInWorld()&&p->GetMapId()==1220&&!p->GetInstanceId();}
bool World(Player* p){return SameMap(p)&&p->GetZoneId()==7541;}
bool Ground(Player* p){if(!World(p)||!p->IsAlive()||p->IsBeingTeleported()||p->GetVehicle()||p->GetTransport()||p->isInFlight()||p->IsFlying()||p->HasUnitMovementFlag(MOVEMENTFLAG_FALLING|MOVEMENTFLAG_FALLING_FAR)||!p->movespline->Finalized())return false;
 float floor=p->GetMap()->GetHeight(p->GetPositionX(),p->GetPositionY(),p->GetPositionZ()+2,true,5);return std::isfinite(floor)&&std::fabs(p->GetPositionZ()-floor)<1.0f;}
bool Scope(Creature* c){return c&&c->GetEntry()==91767&&c->GetDBTableGUIDLow()==388111&&c->GetMapId()==1220&&c->GetZoneId()==7541&&!c->GetInstanceId();}
bool Active(Player* p){return World(p)&&p->IsAlive()&&!p->IsBeingTeleported()&&Definition()&&p->GetQuestStatus(QuestId)==QUEST_STATUS_INCOMPLETE;}
bool Current(Player* p,Ride const& r){auto q=p->getQuestStatus(QuestId);return Active(p)&&q&&!r.lifetime.expired()&&r.lifetime.lock()==q->ScriptLifetime;}
bool CanStart(Player* p,Creature* source){return Active(p)&&Ground(p)&&Scope(source)&&source->IsAlive()&&source->IsInWorld()&&source->HasFlag(UNIT_FIELD_NPC_FLAGS,UNIT_NPC_FLAG_SPELLCLICK)&&!p->isInCombat()&&p->GetExactDist(&Source)<8&&p->GetExactDist(source)<8&&p->InSamePhase(source)&&p->canSeeOrDetect(source)&&p->IsWithinLOSInMap(source)&&!rides.count(p->GetGUID());}
void Credit(Player* p,uint32 entry,uint32 amount=1)
{
 if(!Active(p))return;
 auto q=sQuestDataStore->GetQuestTemplate(QuestId);
 for(auto const& o:q->GetObjectives())if(o.ObjectID==entry){
  uint32 before=std::max<int32>(0,p->GetQuestObjectiveData(QuestId,entry));
  uint32 after=std::min<uint32>(o.Amount,before+amount);if(after==before)return;
  p->SetQuestObjectiveData(q,&o,after);p->SendQuestUpdateAddCredit(q,ObjectGuid::Empty,o,after);
  if(p->CanCompleteQuest(QuestId))p->CompleteQuest(QuestId);return;
 }
}
Creature* Dragon(Player* p,Ride const& r){return ObjectAccessor::GetCreature(*p,r.dragon);}
Player* Pilot(Unit* caster)
{
 Creature* c=caster?caster->ToCreature():nullptr;if(!c||c->GetEntry()!=92302)return nullptr;
 for(auto const& item:rides)if(item.second.dragon==c->GetGUID())return ObjectAccessor::GetPlayer(*c,item.first);
 return nullptr;
}
bool Assault(Player* p,Ride const& r){return Current(p,r)&&r.phase==Phase::Assault&&p->GetVehicleBase()&&p->GetVehicleBase()->GetGUID()==r.dragon;}
bool Target(Ride const& r,WorldObject* o){if(!o||!o->ToCreature()||!o->ToCreature()->IsAlive())return false;for(auto const& slot:r.targets)if(slot.guid==o->GetGUID()&&!slot.counted)return true;return false;}
void Restore(Player* p,ObjectGuid dragon){if(p->GetUnitBeingMoved()->GetGUID()==dragon)p->SetClientControl(p,true);else if(auto v=p->GetViewpoint())if(v->GetGUID()==dragon)p->SetViewpoint(v,false);}
void Hide(Player* p,Ride& r,Creature* c)
{
 if(!c)return;r.hidden.insert(c->GetGUID());if(!c->ShouldHideFor(p->GetGUID())){c->AddToHideList(p->GetGUID());c->DestroyForPlayer(p);}
}
void ClearActors(Player* p,Ride const& r)
{
 if(!r.scene)return;
 for(auto const& slot:r.targets)if(auto c=r.scene->GetCreature(slot.guid))c->DespawnOrUnsummon();
 for(auto guid:r.storms)if(auto c=r.scene->GetCreature(guid))c->DespawnOrUnsummon();
 for(auto guid:r.hidden)if(auto c=r.scene->GetCreature(guid))c->RemoveFromHideList(p->GetGUID());
 if(p->IsInWorld()&&p->GetMap()==r.scene)p->UpdateVisibilityForPlayer();
}
void Exit(Player* p,Ride& r)
{
 r.phase=Phase::Exiting;r.elapsed=0;Restore(p,r.dragon);
 if(auto c=Dragon(p,r))c->StopMoving();
 if(p->GetVehicleBase()&&p->GetVehicleBase()->GetGUID()==r.dragon)p->ExitVehicle(r.success?&Landing:nullptr);
 ClearActors(p,r);
}
void Finish(Player* p,bool recover,bool logout=false)
{
 auto it=rides.find(p->GetGUID());if(it==rides.end())return;Ride r=it->second;
 Restore(p,r.dragon);
 if(p->GetVehicleBase()&&p->GetVehicleBase()->GetGUID()==r.dragon)p->ExitVehicle(r.success?&Landing:nullptr);
 Creature* dragon=r.scene?r.scene->GetCreature(r.dragon):nullptr;
 ClearActors(p,r);
 bool landed=Ground(p)&&p->GetExactDist(&Landing)<3;
 if(recover&&SameMap(p)&&!p->IsBeingTeleported()&&!p->GetVehicle()){
  if(r.success&&!logout&&!landed){it->second.phase=Phase::Recovering;it->second.elapsed=0;p->NearTeleportTo(Landing);if(dragon)dragon->DespawnOrUnsummon(1000);return;}
  if(!landed)p->NearTeleportTo(Source);
 }
 if(r.success&&landed&&Current(p,r)&&p->GetQuestObjectiveData(QuestId,91767)&&p->GetQuestObjectiveData(QuestId,92360)>=150)Credit(p,91768);
 rides.erase(it);if(dragon)dragon->DespawnOrUnsummon(1000);
}
void Populate(Player* p,Ride& r,uint32 diff)
{
 if(p->GetQuestObjectiveData(QuestId,92360)>=150)return;
 for(size_t i=0;i<SeedCount;++i){auto& slot=r.targets[i];
  auto const& original=Seeds[i];uint32 entry=original.entry==92367?91204:original.entry==92312?91205:91417;
  Hide(p,r,r.scene->GetCreature(ObjectGuid::Create<HighGuid::Creature>(1220,entry,original.source)));
  if(auto c=ObjectAccessor::GetCreature(*p,slot.guid))if(c->IsAlive())continue;
  if(slot.retry>diff){slot.retry-=diff;continue;}slot.retry=1000;
  auto const& seed=Seeds[i];float floor=p->GetMap()->GetHeight(seed.pos.GetPositionX(),seed.pos.GetPositionY(),seed.pos.GetPositionZ()+2,true,5);
  if(!std::isfinite(floor)||std::fabs(floor-seed.pos.GetPositionZ())>2)continue;
  auto info=sObjectMgr->GetCreatureTemplate(seed.entry);if(!info||sObjectMgr->GetScriptName(info->ScriptID)!="npc_stormheim_cry_thunder_target")continue;
  if(auto c=p->SummonCreature(seed.entry,seed.pos,TEMPSUMMON_MANUAL_DESPAWN,0)){
   slot.guid=c->GetGUID();slot.counted=false;c->AddPlayerInPersonnalVisibilityList(p->GetGUID());c->SetReactState(REACT_PASSIVE);c->setActive(true);
  }
 }
}
void Died(Creature* target,Unit* killer)
{
 std::lock_guard<std::recursive_mutex> lock(mutex);
 Player* p=killer?killer->GetCharmerOrOwnerPlayerOrPlayerItself():nullptr;if(!p)return;
 auto it=rides.find(p->GetGUID());if(it==rides.end()||!Assault(p,it->second)||target->IsAlive())return;
 for(auto& slot:it->second.targets)if(slot.guid==target->GetGUID()&&!slot.counted){slot.counted=true;slot.retry=8000;Credit(p,92360);target->DespawnOrUnsummon(2000);return;}
}
bool ReturnReady(Player* p,Ride const& r,Creature* c)
{
 if(!Assault(p,r)||p->GetQuestObjectiveData(QuestId,92360)<150||!c)return false;
 float x=c->GetPositionX(),y=c->GetPositionY(),z=c->GetPositionZ();
 return x>=2350&&x<=2720&&y>=2250&&y<=2680&&z>=200&&z<820&&
 c->GetMap()->isInLineOfSight(x,y,z+2,x,y,850,p->GetPhases(),VMAP::ModelIgnoreFlags::Nothing);
}
void Return(Player* p,Ride& r,Creature* c)
{
 r.phase=Phase::Returning;r.node=0;r.elapsed=0;r.climb.Relocate(c->GetPositionX(),c->GetPositionY(),850,0);
 p->SetClientControl(c,false);c->StopMoving();c->GetMotionMaster()->MovePoint(100,r.climb,false,24);
}
void Update(Player* p,uint32 diff)
{
 auto it=rides.find(p->GetGUID());if(it==rides.end())return;auto& r=it->second;r.elapsed+=diff;
 if(r.phase==Phase::Recovering){if(!SameMap(p)){r.success=false;Finish(p,false);}else if(Ground(p)&&p->GetExactDist(&Landing)<3)Finish(p,false);else if(r.elapsed>=10000){r.success=false;Finish(p,false);}return;}
 if(!SameMap(p)||p->IsBeingTeleported()){Finish(p,false);return;}
 if(r.phase==Phase::Exiting){if(!p->GetVehicle()&&(p->movespline->Finalized()||r.elapsed>=10000))Finish(p,true);else if(r.elapsed>=10000)Finish(p,true);return;}
 auto c=Dragon(p,r);
 if(!Current(p,r)||!c||!c->IsAlive()||c->IsDespawn()||!p->InSamePhase(c)||!p->canSeeOrDetect(c)||p->GetDistance(c)>25){Exit(p,r);return;}
 if(r.phase==Phase::Boarding){
  if(!r.requested){r.requested=true;p->RemoveAurasByType(SPELL_AURA_MOUNTED);p->StopMoving();p->EnterVehicle(c,0,true);}
  if(p->GetVehicleBase()==c&&c->GetVehicleKit()->GetPassenger(0)==p&&p->movespline->Finalized()){
   r.phase=Phase::Assault;r.elapsed=0;Credit(p,91767);Populate(p,r,0);
  }else if(r.elapsed>=10000)Exit(p,r);return;
 }
 if(p->GetVehicleBase()!=c){r.success=false;Exit(p,r);return;}
 if(r.phase==Phase::Assault){
  if(r.elapsed>=1800000||c->GetPositionX()<1800||c->GetPositionX()>3300||c->GetPositionY()<1800||c->GetPositionY()>3000||c->GetPositionZ()>1000){Exit(p,r);return;}
  Populate(p,r,diff);return;
 }
 Position const& point=r.node?ReturnPath[r.node-1]:r.climb;
 if(c->movespline->Finalized()&&c->GetExactDist(&point)<1){
  if(++r.node==4){r.success=true;Exit(p,r);}else{r.elapsed=0;c->GetMotionMaster()->MovePoint(100+uint32(r.node),ReturnPath[r.node-1],false,24);}
 }else if(r.elapsed>=60000){Exit(p,r);}
}
void Start(Player* p,Creature* source)
{
 if(!CanStart(p,source))return;
 auto info=sObjectMgr->GetCreatureTemplate(92302);if(!info||sObjectMgr->GetScriptName(info->ScriptID)!="npc_stormheim_dragons")return;
 auto c=p->SummonCreature(92302,Source,TEMPSUMMON_MANUAL_DESPAWN,0,4133);
 if(!c)return;if(!c->GetVehicleKit()||c->GetVehicleKit()->GetVehicleInfo()->ID!=4133){c->DespawnOrUnsummon();return;}
 p->AddQuestDelayedEvent(QuestId,0,[]{});Ride r;r.dragon=c->GetGUID();r.scene=c->GetMap();Hide(p,r,source);r.lifetime=p->getQuestStatus(QuestId)->ScriptLifetime;rides.emplace(p->GetGUID(),r);
 c->AddPlayerInPersonnalVisibilityList(p->GetGUID());c->SetCanFly(true);c->SetDisableGravity(true);c->SetReactState(REACT_PASSIVE);c->setActive(true);
 c->m_templateSpells[0]=183058;c->m_templateSpells[1]=183065;c->m_templateSpells[2]=183042;c->m_templateSpells[3]=183471;
 c->SetSpeed(MOVE_FLIGHT,2.5f); // Native driver seat hands movement and the vehicle bar to the player.
}
}

class spell_stormheim_cry_thunder_board : public SpellScript
{
 PrepareSpellScript(spell_stormheim_cry_thunder_board);
 ObjectGuid source;std::weak_ptr<uint8> lifetime;bool scoped=false,hit=false;
 Creature* SourceActor(){auto p=GetCaster()->ToPlayer();return p?ObjectAccessor::GetCreature(*p,source):nullptr;}
 bool Valid(){auto p=GetCaster()->ToPlayer();return scoped&&!lifetime.expired()&&p&&p->getQuestStatus(38624)&&lifetime.lock()==p->getQuestStatus(38624)->ScriptLifetime&&StormheimCryThunder::CanStart(p,SourceActor());}
 SpellCastResult Check(){std::lock_guard<std::recursive_mutex> lock(StormheimCryThunder::mutex);auto p=GetCaster()->ToPlayer();
  if(!scoped){auto c=GetOriginalTarget()?GetOriginalTarget()->ToCreature():nullptr;if(!StormheimCryThunder::Scope(c))return SPELL_CAST_OK;
   scoped=true;source=c->GetGUID();if(!StormheimCryThunder::CanStart(p,c))return SPELL_FAILED_BAD_TARGETS;p->AddQuestDelayedEvent(38624,0,[]{});lifetime=p->getQuestStatus(38624)->ScriptLifetime;}
  return Valid()?SPELL_CAST_OK:SPELL_FAILED_BAD_TARGETS;}
 void Summon(SpellEffIndex i){if(!scoped)return;PreventHitDefaultEffect(i);hit=Valid();}
 void Complete(){std::lock_guard<std::recursive_mutex> lock(StormheimCryThunder::mutex);if(hit&&Valid())StormheimCryThunder::Start(GetCaster()->ToPlayer(),SourceActor());}
 void Register() override{OnCheckCast+=SpellCheckCastFn(spell_stormheim_cry_thunder_board::Check);OnEffectHit+=SpellEffectFn(spell_stormheim_cry_thunder_board::Summon,EFFECT_0,SPELL_EFFECT_SUMMON);AfterCast+=SpellCastFn(spell_stormheim_cry_thunder_board::Complete);}
};
struct npc_stormheim_cry_thunder_target : SmartAI
{
 explicit npc_stormheim_cry_thunder_target(Creature* c):SmartAI(c){}
 void JustDied(Unit* killer) override{StormheimCryThunder::Died(me,killer);SmartAI::JustDied(killer);}
};
class spell_stormheim_cry_thunder_attack : public SpellScript
{
 PrepareSpellScript(spell_stormheim_cry_thunder_attack);
 SpellCastResult Check(){std::lock_guard<std::recursive_mutex> lock(StormheimCryThunder::mutex);auto p=StormheimCryThunder::Pilot(GetCaster());if(!p)return SPELL_CAST_OK;return StormheimCryThunder::Assault(p,StormheimCryThunder::rides.at(p->GetGUID()))?SPELL_CAST_OK:SPELL_FAILED_BAD_TARGETS;}
 void Filter(std::list<WorldObject*>& targets){std::lock_guard<std::recursive_mutex> lock(StormheimCryThunder::mutex);auto p=StormheimCryThunder::Pilot(GetCaster());if(!p)return;
  auto const& r=StormheimCryThunder::rides.at(p->GetGUID());if(!StormheimCryThunder::Assault(p,r)){targets.clear();return;}
  // Native zero-radius area selection already expands to visibility range; retain its candidates.
  targets.remove_if([&](WorldObject* o){return !StormheimCryThunder::Target(r,o)||(m_scriptSpellId==183042&&!GetCaster()->IsWithinDistInMap(o,150));});
 }
 void Register()override{OnCheckCast+=SpellCheckCastFn(spell_stormheim_cry_thunder_attack::Check);OnObjectAreaTargetSelect+=SpellObjectAreaTargetSelectFn(spell_stormheim_cry_thunder_attack::Filter,EFFECT_0,TARGET_UNIT_DEST_AREA_ENTRY);
  if(m_scriptSpellId==183065)OnObjectAreaTargetSelect+=SpellObjectAreaTargetSelectFn(spell_stormheim_cry_thunder_attack::Filter,EFFECT_0,TARGET_UNIT_DEST_AREA_ENEMY);}
};
class spell_stormheim_cry_thunder_end : public SpellScript
{
 PrepareSpellScript(spell_stormheim_cry_thunder_end);
 SpellCastResult Check(){std::lock_guard<std::recursive_mutex> lock(StormheimCryThunder::mutex);auto p=StormheimCryThunder::Pilot(GetCaster());if(!p)return SPELL_CAST_OK;return StormheimCryThunder::ReturnReady(p,StormheimCryThunder::rides.at(p->GetGUID()),GetCaster()->ToCreature())?SPELL_CAST_OK:SPELL_FAILED_BAD_TARGETS;}
 void Suppress(SpellEffIndex i){std::lock_guard<std::recursive_mutex> lock(StormheimCryThunder::mutex);if(StormheimCryThunder::Pilot(GetCaster()))PreventHitDefaultEffect(i);}
 void Complete(){std::lock_guard<std::recursive_mutex> lock(StormheimCryThunder::mutex);auto p=StormheimCryThunder::Pilot(GetCaster());if(p){auto& r=StormheimCryThunder::rides.at(p->GetGUID());if(StormheimCryThunder::ReturnReady(p,r,GetCaster()->ToCreature()))StormheimCryThunder::Return(p,r,GetCaster()->ToCreature());}}
 void Register()override{OnCheckCast+=SpellCheckCastFn(spell_stormheim_cry_thunder_end::Check);OnEffectHitTarget+=SpellEffectFn(spell_stormheim_cry_thunder_end::Suppress,EFFECT_0,SPELL_EFFECT_DUMMY);OnEffectLaunch+=SpellEffectFn(spell_stormheim_cry_thunder_end::Suppress,EFFECT_1,SPELL_EFFECT_TRIGGER_SPELL);OnEffectLaunchTarget+=SpellEffectFn(spell_stormheim_cry_thunder_end::Suppress,EFFECT_1,SPELL_EFFECT_TRIGGER_SPELL);AfterCast+=SpellCastFn(spell_stormheim_cry_thunder_end::Complete);}
};
struct npc_stormheim_cry_thunder_tempest : SmartAI
{
 explicit npc_stormheim_cry_thunder_tempest(Creature* c):SmartAI(c){}
 ObjectGuid owner;uint32 tick=1000,age=0;
 void IsSummonedBy(Unit* summoner)override{std::lock_guard<std::recursive_mutex> lock(StormheimCryThunder::mutex);SmartAI::IsSummonedBy(summoner);if(auto p=StormheimCryThunder::Pilot(summoner)){auto& r=StormheimCryThunder::rides.at(p->GetGUID());if(!StormheimCryThunder::Assault(p,r))return;owner=p->GetGUID();r.storms.insert(me->GetGUID());me->SetOwnerGUID(owner);me->AddPlayerInPersonnalVisibilityList(owner);me->CastSpell(me,183073,true);}}
 void UpdateAI(uint32 diff)override{if(owner.IsEmpty()){SmartAI::UpdateAI(diff);return;}std::lock_guard<std::recursive_mutex> lock(StormheimCryThunder::mutex);
  auto p=ObjectAccessor::GetPlayer(*me,owner);auto it=StormheimCryThunder::rides.find(owner);age+=diff;if(!p||it==StormheimCryThunder::rides.end()||!StormheimCryThunder::Assault(p,it->second)||age>=10000){me->DespawnOrUnsummon();return;}
  if(tick>diff){tick-=diff;return;}tick=1000;
  Creature* nearest=nullptr;float distance=12;for(auto const& slot:it->second.targets)if(auto c=ObjectAccessor::GetCreature(*p,slot.guid))if(c->IsAlive()&&me->GetExactDist(c)<distance){nearest=c;distance=me->GetExactDist(c);}
  if(nearest)me->CastSpell(nearest,183072,true);
 }
};
class spell_stormheim_cry_thunder_tempest : public SpellScript
{
 PrepareSpellScript(spell_stormheim_cry_thunder_tempest);
 void Select(WorldObject*& target){auto caster=GetCaster();if(!caster||caster->GetEntry()!=92342)return;std::lock_guard<std::recursive_mutex> lock(StormheimCryThunder::mutex);
  auto p=caster->GetCharmerOrOwnerPlayerOrPlayerItself();auto it=p?StormheimCryThunder::rides.find(p->GetGUID()):StormheimCryThunder::rides.end();if(it==StormheimCryThunder::rides.end()||!it->second.storms.count(caster->GetGUID()))return;
  target=nullptr;if(!StormheimCryThunder::Assault(p,it->second))return;float distance=12;for(auto const& slot:it->second.targets)if(auto c=ObjectAccessor::GetCreature(*p,slot.guid))if(c->IsAlive()&&caster->GetExactDist(c)<distance){target=c;distance=caster->GetExactDist(c);}
 }
 void Register()override{OnObjectTargetSelect+=SpellObjectTargetSelectFn(spell_stormheim_cry_thunder_tempest::Select,EFFECT_0,TARGET_UNIT_NEARBY_ENTRY);OnObjectTargetSelect+=SpellObjectTargetSelectFn(spell_stormheim_cry_thunder_tempest::Select,EFFECT_1,TARGET_UNIT_NEARBY_ENTRY);}
};
class player_stormheim_cry_thunder : public PlayerScript
{
 std::map<ObjectGuid,bool> clicks;
 void Dirty(Player* p){std::list<Creature*> sources;GetCreatureListWithEntryInGrid(sources,p,91767,150);for(auto c:sources)if(StormheimCryThunder::Scope(c))c->ForceValuesUpdateAtIndex(UNIT_FIELD_NPC_FLAGS);}
public:
 player_stormheim_cry_thunder():PlayerScript("player_stormheim_cry_thunder"){}
 void OnUpdate(Player* p,uint32 diff)override{std::lock_guard<std::recursive_mutex> lock(StormheimCryThunder::mutex);StormheimCryThunder::Update(p,diff);if(StormheimCryThunder::World(p)){bool active=p->GetQuestStatus(38624)==QUEST_STATUS_INCOMPLETE;auto it=clicks.find(p->GetGUID());if(it==clicks.end()||it->second!=active){clicks[p->GetGUID()]=active;Dirty(p);}}}
 void OnMapChanged(Player* p)override{std::lock_guard<std::recursive_mutex> lock(StormheimCryThunder::mutex);StormheimCryThunder::Finish(p,false);clicks.erase(p->GetGUID());}
 void OnLogout(Player* p)override{std::lock_guard<std::recursive_mutex> lock(StormheimCryThunder::mutex);bool recover=StormheimCryThunder::SameMap(p)&&!p->IsBeingTeleported();bool had=StormheimCryThunder::rides.count(p->GetGUID());StormheimCryThunder::Finish(p,recover,true);clicks.erase(p->GetGUID());if(had&&recover)p->SaveToDB();}
 void OnLogin(Player* p)override{if(!StormheimCryThunder::Definition())return;if(p->GetQuestStatus(38624)==QUEST_STATUS_COMPLETE&&(!p->GetQuestObjectiveData(38624,91767)||p->GetQuestObjectiveData(38624,92360)<150||!p->GetQuestObjectiveData(38624,91768)))p->SetQuestStatus(38624,QUEST_STATUS_INCOMPLETE);Dirty(p);}
};
void AddSC_stormheim_cry_thunder()
{
 RegisterSpellScript(spell_stormheim_cry_thunder_board);RegisterSpellScript(spell_stormheim_cry_thunder_attack);RegisterSpellScript(spell_stormheim_cry_thunder_end);RegisterSpellScript(spell_stormheim_cry_thunder_tempest);
 RegisterCreatureAI(npc_stormheim_cry_thunder_target);RegisterCreatureAI(npc_stormheim_cry_thunder_tempest);new player_stormheim_cry_thunder();
}
