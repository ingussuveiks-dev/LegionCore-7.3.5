#include "ScriptMgr.h"
#include "GameObjectAI.h"
#include "SpellScript.h"
#include "Player.h"
#include "Creature.h"
#include "GameObject.h"
#include "ObjectMgr.h"
#include "ObjectAccessor.h"
#include "QuestData.h"
#include "Map.h"
#include "MoveSplineInit.h"
#include "MoveSpline.h"
#include "stormheim_wings_departure.h"
#include <array>
#include <map>
#include <mutex>

namespace StormheimWings
{
constexpr uint32 QuestId=38413;
struct Site {uint32 circle;ObjectGuid::LowType spawn;Position point;uint32 actor;ObjectGuid::LowType actorSpawn;Position actorPoint;};
Site const Sites[] = {
 {240650,115480,{2033.69f,2186.11f,465.141f,0},97028,146706136,{2033.7f,2186.07f,465.226f,.13262f}},
 {244335,115466,{2133.51f,2373.01f,490.76f,0},97031,146706113,{2132.36f,2372.49f,490.706f,3.76372f}},
 {244336,115473,{2058.21f,2475.53f,484.165f,0},97032,146706114,{2057.1f,2473.28f,483.957f,4.23507f}},
 {244337,115476,{1998.f,2383.88f,482.131f,0},97033,146706125,{1998.81f,2385.1f,482.035f,.961753f}}
};
struct Slot {ObjectGuid original,circle,freed;uint32 age=0;bool flying=false;};
struct Run {Map* map=nullptr;std::weak_ptr<uint8> lifetime;std::array<Slot,4> slots;uint32 visibilityTimer=0;};
std::map<ObjectGuid,Run> runs;
std::recursive_mutex mutex;
bool Scope(Player* p){return p&&p->IsInWorld()&&p->GetMapId()==1220&&p->GetZoneId()==7541&&!p->GetInstanceId();}
int Identity(GameObject* go)
{
 if(go&&go->IsInWorld()&&go->GetMapId()==1220&&go->GetZoneId()==7541&&!go->GetInstanceId())
  for(int i=0;i<4;++i)if(go->GetEntry()==Sites[i].circle&&go->GetDBTableGUIDLow()==Sites[i].spawn)return i;
 return -1;
}
bool Definition()
{
 auto q=sQuestDataStore->GetQuestTemplate(QuestId);if(!q||q->StartScript)return false;
 uint32 mask=0;for(auto const& o:q->GetObjectives()){
  bool found=false;for(int i=0;i<4;++i)if(o.Type==QUEST_OBJECTIVE_GAMEOBJECT&&o.ObjectID==Sites[i].circle&&o.Amount==1&&o.StorageIndex==i+1&&!o.Flags&&!(mask&(1u<<i))){mask|=1u<<i;found=true;}
  if(!found)return false;
 }
 if(mask!=15)return false;
 for(auto const& s:Sites){auto t=sObjectMgr->GetGameObjectTemplate(s.circle);
  if(!t||t->type!=GAMEOBJECT_TYPE_GOOBER||t->goober.open!=1690||t->goober.questID||t->goober.spell!=193637||sObjectMgr->GetScriptName(t->ScriptId)!="go_stormheim_wings")return false;
 }
 return true;
}
bool Active(Player* p){return Scope(p)&&p->IsAlive()&&!p->IsBeingTeleported()&&p->GetQuestStatus(QuestId)==QUEST_STATUS_INCOMPLETE&&Definition();}
bool Ready(Player* p,GameObject* go)
{
 int i=Identity(go);return i>=0&&Active(p)&&go->GetExactDist(&Sites[i].point)<1&&go->isSpawned()&&!go->IsQuestObjectExpired()&&
  !go->HasFlag(GAMEOBJECT_FIELD_FLAGS,GO_FLAG_IN_USE|GO_FLAG_NOT_SELECTABLE)&&!p->GetVehicle()&&!p->GetTransport()&&!p->isInFlight()&&!p->isInCombat()&&
  p->InSamePhase(go)&&p->canSeeOrDetect(go)&&!go->ShouldHideFor(p->GetGUID())&&p->IsWithinDistInMap(go,go->GetInteractionDistance())&&p->IsWithinLOSInMap(go)&&!p->GetQuestObjectiveData(QuestId,Sites[i].circle);
}
Creature* Original(Player* p,int i)
{
 std::list<Creature*> actors;GetCreatureListWithEntryInGrid(actors,p,Sites[i].actor,100);
 for(auto c:actors)if(c->GetDBTableGUIDLow()==Sites[i].actorSpawn&&c->GetExactDist(&Sites[i].actorPoint)<1&&c->IsAlive()&&p->InSamePhase(c))return c;
 return nullptr;
}
bool FlightClear(Creature* actor,int i)
{
 // Recheck both surveyed legs in the actor's phase, including dynamic objects.
 auto const& via=Departures[i];
 for(float dx:{-.5f,0.f,.5f})if(!actor->GetMap()->isInLineOfSight(actor->GetPositionX()+dx,actor->GetPositionY(),actor->GetPositionZ()+2,
  via.x+dx,via.y,via.z+2,actor->GetPhases(),VMAP::ModelIgnoreFlags::Nothing)||
  !actor->GetMap()->isInLineOfSight(via.x+dx,via.y,via.z+2,via.x+dx,via.y,actor->GetPositionZ()+42,actor->GetPhases(),VMAP::ModelIgnoreFlags::Nothing))return false;
 return true;
}
Run& Ensure(Player* p)
{
 auto it=runs.find(p->GetGUID());if(it!=runs.end())return it->second;
 Run r;r.map=p->GetMap();if(p->getQuestStatus(QuestId)){p->AddQuestDelayedEvent(QuestId,0,[]{});r.lifetime=p->getQuestStatus(QuestId)->ScriptLifetime;}
 return runs.emplace(p->GetGUID(),r).first->second;
}
template<class T>void Hide(Player* p,T* object)
{
 object->AddToHideList(p->GetGUID());object->DestroyForPlayer(p);
}
void Clear(Player* p)
{
 auto it=runs.find(p->GetGUID());if(it==runs.end())return;Run r=it->second;runs.erase(it);
 for(auto const& s:r.slots){if(auto c=r.map->GetCreature(s.freed))c->DespawnOrUnsummon();
  if(auto c=r.map->GetCreature(s.original))c->RemoveFromHideList(p->GetGUID());
  if(auto go=r.map->GetGameObject(s.circle))go->RemoveFromHideList(p->GetGUID());
 }
 p->UpdateVisibilityForPlayer();
}
void Release(Player* p,GameObject* go)
{
 if(!Ready(p,go))return;int i=Identity(go);auto actor=Original(p,i);if(!actor||!FlightClear(actor,i))return;
 auto& r=Ensure(p);if(r.map!=p->GetMap()||r.lifetime.expired()||r.lifetime.lock()!=p->getQuestStatus(QuestId)->ScriptLifetime){Clear(p);return;}
 // Allocate the private actor before consuming the site. Failed construction
 // leaves the circle and objective available for a genuine retry.
 auto clone=p->SummonCreature(Sites[i].actor,Sites[i].actorPoint,TEMPSUMMON_MANUAL_DESPAWN,0,0,p->GetGUID());if(!clone)return;
 p->BindQuestObject(QuestId,clone);clone->SetStandState(UNIT_STAND_STATE_STAND);clone->SetReactState(REACT_PASSIVE);
 clone->SetFlag(UNIT_FIELD_FLAGS,UNIT_FLAG_NON_ATTACKABLE|UNIT_FLAG_IMMUNE_TO_PC|UNIT_FLAG_IMMUNE_TO_NPC);clone->setActive(true);
 auto& slot=r.slots[i];slot.original=actor->GetGUID();slot.circle=go->GetGUID();slot.freed=clone->GetGUID();slot.age=0;slot.flying=false;
 Hide(p,actor);Hide(p,go);
 auto q=sQuestDataStore->GetQuestTemplate(QuestId);for(auto const& o:q->GetObjectives())if(o.ObjectID==Sites[i].circle){
  p->SetQuestObjectiveData(q,&o,1);p->SendQuestUpdateAddCredit(q,go->GetGUID(),o,1);if(p->CanCompleteQuest(QuestId))p->CompleteQuest(QuestId);break;
 }
 TC_LOG_DEBUG("scripts", "Wings of Liberty: private release after completed opening, site %u", Sites[i].circle);
}
void Update(Player* p,uint32 diff)
{
 auto status=p->GetQuestStatus(QuestId);bool rewarded=p->GetQuestRewardStatus(QuestId);
 if(!Scope(p)||(!rewarded&&status!=QUEST_STATUS_INCOMPLETE&&status!=QUEST_STATUS_COMPLETE)||!Definition()||!p->IsAlive()||p->IsBeingTeleported()){Clear(p);return;}
 auto it=runs.find(p->GetGUID());
 if(it!=runs.end()&&(it->second.map!=p->GetMap()||(!rewarded&&(it->second.lifetime.expired()||it->second.lifetime.lock()!=p->getQuestStatus(QuestId)->ScriptLifetime)))){Clear(p);return;}
 // Rehydrate only visibility from durable objective/reward state. Login and
 // proximity must never award credit or replay a completed departure.
 bool visibility=it==runs.end()||it->second.visibilityTimer<=diff;
 if(it!=runs.end())it->second.visibilityTimer=visibility?500:it->second.visibilityTimer-diff;
 if(visibility)for(int i=0;i<4;++i)if((rewarded||p->GetQuestObjectiveData(QuestId,Sites[i].circle))&&p->GetExactDist(&Sites[i].point)<80){
  auto& slot=Ensure(p).slots[i];if(auto c=Original(p,i)){slot.original=c->GetGUID();if(!c->ShouldHideFor(p->GetGUID()))Hide(p,c);}
  std::list<GameObject*> objects;GetGameObjectListWithEntryInGrid(objects,p,Sites[i].circle,100);
  for(auto go:objects)if(Identity(go)==i){slot.circle=go->GetGUID();if(!go->ShouldHideFor(p->GetGUID()))Hide(p,go);}
 }
 it=runs.find(p->GetGUID());if(it==runs.end())return;
 for(int i=0;i<4;++i){auto& s=it->second.slots[i];auto clone=it->second.map->GetCreature(s.freed);if(!clone)continue;
  if(!p->InSamePhase(clone)||!p->canSeeOrDetect(clone)||clone->IsQuestObjectExpired()){clone->DespawnOrUnsummon();s.freed.Clear();continue;}
  s.age+=diff;if(!s.flying&&s.age>=2000){if(!FlightClear(clone,i)){clone->DespawnOrUnsummon();s.freed.Clear();continue;}
   clone->SetCanFly(true);clone->SetDisableGravity(true);Movement::MoveSplineInit init(*clone);
   auto const& via=Departures[i];Movement::PointsArray path={G3D::Vector3(clone->GetPositionX(),clone->GetPositionY(),clone->GetPositionZ()),G3D::Vector3(via.x,via.y,via.z),G3D::Vector3(via.x,via.y,Sites[i].actorPoint.GetPositionZ()+40)};
   init.MovebyPath(path);init.SetFly();init.SetVelocity(8);init.Launch();s.flying=true;s.age=0;
  }else if(s.flying&&((s.age>=1000&&clone->movespline->Finalized())||s.age>=30000)){clone->DespawnOrUnsummon();s.freed.Clear();}
 }
}
}

struct go_stormheim_wings_ai:GameObjectAI
{
 explicit go_stormheim_wings_ai(GameObject* go):GameObjectAI(go){}
 bool GossipHello(Player* p,bool opened)override{std::lock_guard<std::recursive_mutex> lock(StormheimWings::mutex);if(StormheimWings::Identity(go)<0)return false;if(opened)StormheimWings::Release(p,go);return true;}
};
class go_stormheim_wings:public GameObjectScript
{
public:go_stormheim_wings():GameObjectScript("go_stormheim_wings"){}
 bool OnGossipHello(Player*,GameObject* go)override{return StormheimWings::Identity(go)>=0;}
 GameObjectAI* GetAI(GameObject* go)const override{return new go_stormheim_wings_ai(go);}
};
class spell_stormheim_wings_opening:public SpellScript
{
 PrepareSpellScript(spell_stormheim_wings_opening);
 ObjectGuid circle;std::weak_ptr<uint8> lifetime;bool captured=false,scoped=false;
 bool Valid(){auto p=GetCaster()->ToPlayer();auto go=p?ObjectAccessor::GetGameObject(*p,circle):nullptr;
  return captured&&StormheimWings::Ready(p,go)&&!lifetime.expired()&&p->getQuestStatus(38413)&&lifetime.lock()==p->getQuestStatus(38413)->ScriptLifetime;
 }
 SpellCastResult Check(){std::lock_guard<std::recursive_mutex> lock(StormheimWings::mutex);
  if(!captured){auto go=GetExplTargetGObj();scoped=StormheimWings::Identity(go)>=0;if(!scoped)return SPELL_CAST_OK;
   auto p=GetCaster()->ToPlayer();if(!StormheimWings::Ready(p,go))return SPELL_FAILED_BAD_TARGETS;
   p->AddQuestDelayedEvent(38413,0,[]{});circle=go->GetGUID();lifetime=p->getQuestStatus(38413)->ScriptLifetime;captured=true;
  }return Valid()?SPELL_CAST_OK:SPELL_FAILED_BAD_TARGETS;
 }
 void Hit(SpellEffIndex){std::lock_guard<std::recursive_mutex> lock(StormheimWings::mutex);if(scoped&&(!Valid()||!GetHitGObj()||GetHitGObj()->GetGUID()!=circle))PreventHitDefaultEffect(EFFECT_0);}
 void Register()override{OnCheckCast+=SpellCheckCastFn(spell_stormheim_wings_opening::Check);OnEffectHitTarget+=SpellEffectFn(spell_stormheim_wings_opening::Hit,EFFECT_0,SPELL_EFFECT_OPEN_LOCK);}
};
class player_stormheim_wings:public PlayerScript
{
public:player_stormheim_wings():PlayerScript("player_stormheim_wings"){}
 void OnUpdate(Player* p,uint32 diff)override{std::lock_guard<std::recursive_mutex> lock(StormheimWings::mutex);StormheimWings::Update(p,diff);}
 void OnLogout(Player* p)override{std::lock_guard<std::recursive_mutex> lock(StormheimWings::mutex);StormheimWings::Clear(p);}
 void OnMapChanged(Player* p)override{std::lock_guard<std::recursive_mutex> lock(StormheimWings::mutex);StormheimWings::Clear(p);}
};
void AddSC_stormheim_wings(){new go_stormheim_wings();RegisterSpellScript(spell_stormheim_wings_opening);new player_stormheim_wings();}
