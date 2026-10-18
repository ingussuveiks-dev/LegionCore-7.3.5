#include "EventMap.h"
#include <algorithm>
#include <array>
#include <cassert>
#include <cmath>
#include <iostream>
#include <list>
#include <map>
#include <memory>
#include <set>
#include <vector>
#include "RockawayEvents.inc"

struct ObjectGuid { uint32 value=0; void Clear(){value=0;} bool operator<(ObjectGuid b)const{return value<b.value;} bool operator==(ObjectGuid b)const{return value==b.value;} };
using GuidSet=std::set<ObjectGuid>;
struct Position {
 float x=0,y=0,z=0,o=0; float GetPositionX()const{return x;}float GetPositionY()const{return y;}float GetPositionZ()const{return z;}
};
struct Player;struct Creature;struct ScriptedAI;struct Group{bool raid=false;bool isRaidGroup()const{return raid;}};
struct Map {uint32 GetDifficultyID()const{return 0;}} worldMap;
using DamageEffectType=int;
constexpr int REACT_PASSIVE=0,UNIT_STATE_CASTING=1,TEMPSUMMON_MANUAL_DESPAWN=0,QUEST_STATUS_INCOMPLETE=1;
constexpr int LootTemplates_Spell=12;
uint32 clockMs=0,nextGuid=1,failSummons=0;
std::set<uint32> failedSpells;
struct Unit:Position {
 ObjectGuid guid{nextGuid++}; uint32 entry=0,map=1220,phase=1,hp=100,healthRelative=100;bool alive=true,inWorld=true,visible=true,IsAIEnabled=true;
 Unit* owner=nullptr;std::set<uint32> auras;std::vector<uint32> casts;
 virtual ~Unit()=default;void Dispatch(Creature*creature);
 ObjectGuid GetGUID()const{return guid;}uint32 GetEntry()const{return entry;}uint32 GetMapId()const{return map;}
 Map*GetMap(){return &worldMap;}
 bool IsAlive()const{return alive;}bool IsCreature()const;Creature* ToCreature();
 Player* GetCharmerOrOwnerPlayerOrPlayerItself();
 bool InSamePhase(Unit*p)const{return map==p->map&&phase==p->phase;}
 bool IsWithinDistInMap(Unit*p,float d,bool=true,bool=false)const{return InSamePhase(p)&&std::hypot(x-p->x,y-p->y)<=d;}
 uint32 GetHealth(Unit*)const{return healthRelative;}
 void SetFullHealth(){hp=healthRelative=100;alive=true;}
 void SetVisible(bool v){visible=v;}void setActive(bool){}void SetReactState(int){}void SetDisableGravity(bool){}
 void SetFacingToObject(Unit*){}void CombatStop(bool){}void CastStop(){}
 bool HasUnitState(int)const{return false;}bool HasAura(uint32 s)const{return auras.count(s);}
 void RemoveAurasDueToSpell(uint32 id,ObjectGuid){auras.erase(id);}
 void CastSpell(Unit*target,uint32 id,bool);void CastSpell(float x,float y,float z,uint32 id,bool);
 Position GetNearPosition(float d,float a){return {x+d*std::cos(a),y+d*std::sin(a),z,0};}
};
std::map<ObjectGuid,std::unique_ptr<Unit>> units;
struct Player:Unit {
 uint32 status=1,credits=0,fragments=0,errors=0;bool lostArmy=true,fullBags=false;Group*group=nullptr;
 uint32 GetQuestStatus(uint32)const{return status;}Group*GetGroup()const{return group;}
 bool HasQuestForItem(uint32)const{return lostArmy&&fragments<1000;}
 void AutoStoreLoot(uint32 id,int){assert(id==229113);if(fullBags)++errors;else fragments+=15;}
 void KilledMonsterCredit(uint32 id){assert(id==115168);++credits;status=2;}
};
struct TempSummon{Unit*owner;Unit*GetSummoner(){return owner;}};
struct Creature:Unit {
 TempSummon temp{};TempSummon*ToTempSummon(){temp.owner=owner;return owner?&temp:nullptr;}void KillAllDelayedCombats(){}
 ScriptedAI*ai=nullptr;Unit*victim=nullptr;uint32 despawnTime=0,melees=0;std::vector<uint32>talks;
 ScriptedAI*AI(){return ai;}Creature*SummonCreature(uint32,Position const&,int);
 void GetPlayerListInGrid(std::list<Player*>&list,float d){for(auto&v:units)if(auto p=dynamic_cast<Player*>(v.second.get()))if(p->inWorld&&IsWithinDistInMap(p,d))list.push_back(p);}
 void DespawnOrUnsummon(uint32 delay=0);
};
struct ScriptedAI {
 Creature*me;explicit ScriptedAI(Creature*c):me(c){c->ai=this;}virtual~ScriptedAI()=default;
 virtual void AttackStart(Unit*u){me->victim=u;}virtual void MoveInLineOfSight(Unit*){}virtual uint32 GetData(uint32)const{return 0;}
 virtual void Reset(){}virtual void SetGUID(ObjectGuid const&,int32){}virtual void UpdateAI(uint32){}
 virtual void JustDied(Unit*){}virtual void JustSummoned(Creature*){}virtual void SummonedCreatureDies(Creature*,Unit*){}virtual void SummonedCreatureDespawn(Creature*){}
 virtual void IsSummonedBy(Unit*){}virtual void DamageTaken(Unit*,uint32&,int){}virtual void DamageTakenAfterScaling(Unit*,uint32&,int){}
 void Talk(uint32 group){me->talks.push_back(group);}bool UpdateVictim(){return me->victim&&me->victim->IsAlive();}void DoMeleeAttackIfReady(){++me->melees;}
};
struct SummonList:std::set<ObjectGuid> {
 Creature*me;explicit SummonList(Creature*c):me(c){}void Summon(Creature*c){insert(c->guid);}void Despawn(Creature*c){erase(c->guid);}
 void DespawnAll(){auto copy=*this;clear();for(auto g:copy){auto it=units.find(g);if(it!=units.end())static_cast<Creature*>(it->second.get())->DespawnOrUnsummon();}}
};
namespace ObjectAccessor {
 Creature*GetCreature(Unit&,ObjectGuid g){auto i=units.find(g);return i!=units.end()&&i->second->inWorld?dynamic_cast<Creature*>(i->second.get()):nullptr;}
 Player*GetPlayer(Unit&,ObjectGuid g){auto i=units.find(g);return i!=units.end()&&i->second->inWorld?dynamic_cast<Player*>(i->second.get()):nullptr;}
}
struct Quest{bool IsAllowedInRaid(uint32)const{return false;}};struct Rotation{uint32 ResetTime=10000;}rotation;
struct QuestStore {bool active=true;Quest quest;Quest*GetQuestTemplate(uint32){return &quest;}Rotation*GetWorldQuest(Quest*){return active?&rotation:nullptr;}}store;
auto sQuestDataStore=&store;
namespace GameTime{uint32 GetGameTime(){return clockMs/1000;}}
bool Unit::IsCreature()const{return dynamic_cast<Creature const*>(this)!=nullptr;}Creature*Unit::ToCreature(){return dynamic_cast<Creature*>(this);}
Player*Unit::GetCharmerOrOwnerPlayerOrPlayerItself(){if(auto p=dynamic_cast<Player*>(this))return p;return owner?owner->GetCharmerOrOwnerPlayerOrPlayerItself():nullptr;}

#include "RockawayActors.inc"

ScriptedAI*MakeAI(Creature*c){switch(c->entry){case 115133:return new npc_rockaway_vision(c);case 115132:return new npc_rockaway_ritualist(c);case 116187:return new npc_rockaway_tentacle(c);default:return new npc_rockaway_slime(c);}}
Creature*Spawn(uint32 entry,Unit*owner,Position pos={}){auto c=std::make_unique<Creature>();c->entry=entry;c->owner=owner;static_cast<Position&>(*c)=pos;auto ptr=c.get();units.emplace(c->guid,std::move(c));MakeAI(ptr);if(owner){ptr->AI()->IsSummonedBy(owner);owner->ToCreature()->AI()->JustSummoned(ptr);}return ptr;}
Creature*Creature::SummonCreature(uint32 e,Position const&p,int){if(failSummons){--failSummons;return nullptr;}return Spawn(e,this,p);}
void Creature::DespawnOrUnsummon(uint32 delay){if(delay){despawnTime=clockMs+delay;return;}if(!inWorld)return;inWorld=false;if(owner&&owner->IsCreature())owner->ToCreature()->AI()->SummonedCreatureDespawn(this);}
void Unit::CastSpell(Unit*target,uint32 id,bool){casts.push_back(id);if(failedSpells.count(id))return;if(id==231328||id==231333)auras.insert(id);if(id==231329)target->auras.insert(id);if(id==231296)Spawn(116187,this,*target);}
void Unit::CastSpell(float px,float py,float pz,uint32 id,bool){casts.push_back(id);if(!failedSpells.count(id)&&id==231354)Spawn(116195,this,{px,py,pz,0});}
void Unit::Dispatch(Creature*creature){
#include "RockawayDeath.inc"
}
void Tick(uint32 ms){for(uint32 t=0;t<ms;t+=100){auto dt=std::min(100u,ms-t);clockMs+=dt;std::vector<Creature*>copy;for(auto&v:units)if(auto c=dynamic_cast<Creature*>(v.second.get()))if(c->inWorld)copy.push_back(c);for(auto c:copy)if(c->inWorld&&c->alive)c->AI()->UpdateAI(dt);for(auto c:copy)if(c->inWorld&&c->despawnTime&&clockMs>=c->despawnTime)c->DespawnOrUnsummon();}}
Player*NewPlayer(Position p){auto v=std::make_unique<Player>();static_cast<Position&>(*v)=p;auto result=v.get();units.emplace(v->guid,std::move(v));return result;}
struct Run {
 Creature*vision; npc_rockaway_vision*ai;Player*player;Unit pet;
 Run(){units.clear();clockMs=0;store.active=true;rotation.ResetTime=10000;failSummons=0;failedSpells.clear();vision=Spawn(115133,nullptr,{6229.235f,4951.045f,0,0});ai=static_cast<npc_rockaway_vision*>(vision->AI());ai->Reset();player=NewPlayer(*vision);pet.owner=player;Tick(100);assert(ai->ready);}
 void Hit(uint32 slot,Unit*killer=nullptr,bool kill=false){auto c=ObjectAccessor::GetCreature(*vision,ai->slots[slot]);assert(c&&c->alive);uint32 damage=kill?100:1;if(c->IsAIEnabled)c->AI()->DamageTaken(killer?killer:player,damage,0);if(kill&&damage){c->alive=false;(killer?killer:player)->Dispatch(c);}}
};

struct AuraEffect{};using AuraEffectHandleModes=int;
constexpr int AURA_REMOVE_BY_EXPIRE=1,AURA_REMOVE_BY_ENEMY_SPELL=2;
struct App{int mode=1;int GetRemoveMode(){return mode;}};
struct TaintContext{
 Unit*caster;Unit*target;App app;Unit*GetCaster(){return caster;}Unit*GetTarget(){return target;}App*GetTargetApplication(){return &app;}
#include "RockawayTaint.inc"
};
int main(){
 // Every death order, including a pet's first-hit killing blow; no premature or duplicate credit.
 std::array<uint32,4>order{0,1,2,3};uint32 orders=0;
 do{Run r;for(uint32 j=0;j<4;++j){r.Hit(order[j],&r.pet,true);assert(r.player->credits==0);if(j<3)assert(!r.ai->outro);}assert(r.ai->outro);Tick(5900);assert(!r.player->credits);Tick(100);assert(r.player->credits==1&&r.player->fragments==15&&!r.vision->visible);r.ai->Finish();Tick(1000);assert(r.player->credits==1);++orders;}while(std::next_permutation(order.begin(),order.end()));assert(orders==24);
 {Run r;uint32 damage=1000000;r.vision->healthRelative=900;r.ai->DamageTakenAfterScaling(&r.pet,damage,0);assert(damage==899&&r.ai->started&&!r.ai->outro);Tick(6000);assert(!r.player->credits);}
 {Run r;store.active=false;Tick(100);assert(!r.ai->enabled&&!r.vision->visible);for(auto g:r.ai->slots)assert(!g.value);store.active=true;Tick(100);assert(r.ai->ready&&r.vision->visible);}
 {Run r;rotation.ResetTime=1;Tick(1000);assert(!r.ai->enabled&&!r.vision->visible);}
 {Run r;Group group;r.player->group=&group;auto ally=NewPlayer(*r.vision);ally->group=&group;auto observer=NewPlayer(*r.vision);auto wrong=NewPlayer(*r.vision);wrong->phase=2;wrong->group=&group;auto done=NewPlayer(*r.vision);done->status=2;done->group=&group;auto dead=NewPlayer(*r.vision);dead->alive=false;dead->group=&group;for(uint32 i=0;i<4;++i)r.Hit(i,nullptr,true);Tick(6000);assert(ally->credits==1&&observer->credits==0&&wrong->credits==0&&done->credits==0&&dead->credits==0);}
 {Run r;r.player->lostArmy=false;for(uint32 i=0;i<4;++i)r.Hit(i,nullptr,true);Tick(6000);assert(r.player->credits==1&&!r.player->fragments);}
 {Run r;r.player->fragments=1000;for(uint32 i=0;i<4;++i)r.Hit(i,nullptr,true);Tick(6000);assert(r.player->credits==1&&r.player->fragments==1000);}
 {Run r;r.player->fragments=999;for(uint32 i=0;i<4;++i)r.Hit(i,nullptr,true);Tick(6000);assert(r.player->fragments==1014);}
 {Run r;r.player->fullBags=true;for(uint32 i=0;i<4;++i)r.Hit(i,nullptr,true);Tick(6000);assert(r.player->credits==1&&!r.player->fragments&&r.player->errors==1);}
 {Run r;r.Hit(0,nullptr,true);auto old=r.ai->slots[1];ObjectAccessor::GetCreature(*r.vision,old)->DespawnOrUnsummon();assert(!r.ai->started&&r.ai->cooldown);auto stale=Spawn(116195,r.vision,*r.player);assert(!stale->inWorld);Tick(10100);assert(r.ai->ready&&!r.ai->dead[0]&&r.ai->slots[1].value!=old.value);}
 {Run r;r.ai->ClearRun();failSummons=1;r.ai->EnsureRitualists();assert(!r.ai->ready);uint32 d=100;auto c=ObjectAccessor::GetCreature(*r.vision,r.ai->slots[1]);c->AI()->DamageTaken(r.player,d,0);assert(d==0);r.ai->EnsureRitualists();assert(r.ai->ready);}
 {Run r;r.Hit(0);r.player->x+=200;Tick(10100);assert(!r.ai->started&&!r.vision->visible);Tick(10000);assert(r.ai->ready);}
 {Run r;for(uint32 id:{231296u,231351u,231354u,231329u})failedSpells.insert(id);r.Hit(0);Tick(25000);for(uint32 i=0;i<4;++i)r.Hit(i,nullptr,true);Tick(6000);assert(r.player->credits==1);}
 {Run r;r.Hit(0);Tick(11000);uint32 slimes=0,slams=0;for(auto&v:units)if(auto c=dynamic_cast<Creature*>(v.second.get())){if(c->entry==116195){++slimes;assert(c->victim==r.player);}if(c->entry==116187){slams+=std::count(c->casts.begin(),c->casts.end(),231298);uint32 damage=999;c->AI()->DamageTakenAfterScaling(r.player,damage,0);assert(damage==0);}}assert(slimes==5&&slams==1);}
 {Run r;for(uint32 i=0;i<4;++i)r.Hit(i,nullptr,true);store.active=false;Tick(6000);assert(!r.player->credits&&!r.player->fragments);}
 {Run r;for(uint32 i=0;i<4;++i){r.Hit(i,nullptr,true);auto c=ObjectAccessor::GetCreature(*r.vision,r.ai->slots[i]);r.ai->SummonedCreatureDies(c,nullptr);}Tick(6000);assert(r.player->credits==1);Tick(30100);assert(r.ai->ready&&!r.ai->started&&!r.ai->dead[0]);}
 {Run r;r.Hit(0);TaintContext a{r.vision,r.player,{1}};a.OnRemove(nullptr,0);assert(r.vision->casts.back()==232681);auto n=r.vision->casts.size();a.app.mode=0;a.OnRemove(nullptr,0);assert(r.vision->casts.size()==n);a.app.mode=2;a.OnRemove(nullptr,0);assert(r.vision->casts.size()==n+1);r.ai->ClearRun();a.OnRemove(nullptr,0);assert(r.vision->casts.size()==n+1);a.caster=nullptr;a.OnRemove(nullptr,0);}
 {Run r;for(uint32 i=0;i<4;++i){auto c=ObjectAccessor::GetCreature(*r.vision,r.ai->slots[i]);c->IsAIEnabled=false;r.Hit(i,&r.pet,true);}Tick(6000);assert(r.player->credits==1);}
 {Run r;auto rogue=Spawn(115132,r.vision,*r.player);r.ai->SummonedCreatureDies(rogue,&r.pet);assert(!r.ai->dead[0]&&!r.ai->outro);r.Hit(0);for(uint32 i=0;i<4;++i)r.Hit(i,nullptr,true);r.player->status=0;Tick(6000);assert(!r.player->credits&&!r.player->fragments);}
 {Run r;auto corpse=ObjectAccessor::GetCreature(*r.vision,r.ai->slots[3]);for(uint32 i=0;i<4;++i)r.Hit(i,nullptr,true);Tick(6000);assert(corpse->inWorld&&!corpse->alive);Tick(60100);assert(!corpse->inWorld&&r.ai->ready&&!r.ai->outro);}
 {Run r;Group raid;raid.raid=true;r.player->group=&raid;for(uint32 i=0;i<4;++i)r.Hit(i,&r.pet,true);Tick(6000);assert(!r.player->credits&&!r.player->fragments);}
 std::cout<<"PASS: actual encounter AI and native EventMap;24 death orders, pet scaled blow, rotation/phase/party/recipient gates, missing actors/retry, wipe, expiry, optional cast failures, slam/breath, repeats and one credit. World/packet/inventory/native spell-dispatch services are fixtures; client remains pending.\n";
}
