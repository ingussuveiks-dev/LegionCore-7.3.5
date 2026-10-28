#include <algorithm>
#include <cassert>
#include <cmath>
#include <cstdint>
#include <iostream>
#include <map>
#include <memory>
#include <set>
#include <vector>
using uint32=uint32_t;
enum{CLASS_WARRIOR=1,QUEST_STATUS_INCOMPLETE=3,UNIT_FIELD_FLAGS=1,UNIT_FIELD_NPC_FLAGS=2,UNIT_NPC_FLAG_SPELLCLICK=16,UNIT_FLAG_NON_ATTACKABLE=1,UNIT_FLAG_IMMUNE_TO_PC=2,UNIT_FLAG_IMMUNE_TO_NPC=4,UNIT_STATE_LONG_JUMP=1,CURRENT_CHANNELED_SPELL=1,TEMPSUMMON_DEAD_DESPAWN=7,CRITERIA_TYPE_SCRIPT_EVENT_2=92,EFFECT_0=0,SPELL_EFFECT_JUMP_DEST=42};
enum ReactStates{REACT_PASSIVE,REACT_AGGRESSIVE};
enum SpellCastResult{SPELL_CAST_OK,SPELL_FAILED_BAD_TARGETS};
using SpellEffIndex=uint32;
uint32 warnings=0;
#define TC_LOG_WARN(...) (++warnings)
struct ObjectGuid{uint64_t id=0;void Clear(){id=0;}bool IsEmpty()const{return !id;}bool operator==(ObjectGuid b)const{return id==b.id;}bool operator!=(ObjectGuid b)const{return id!=b.id;}bool operator<(ObjectGuid b)const{return id<b.id;}};
struct Position{float x=0,y=0,z=0,o=0;void Relocate(Position const&p){*this=p;}float GetPositionX()const{return x;}float GetPositionY()const{return y;}float GetPositionZ()const{return z;}};
using WorldLocation=Position;
struct Map{}world,otherWorld;
struct Scenario{uint32 id=1037,step=7;uint32 GetScenarioId()const{return id;}uint32 GetCurrentStep()const{return step;}}scenario;
struct Manager{bool present=true;Scenario*GetScenario(uint32 id){return present&&id==42?&scenario:nullptr;}}manager;auto*sScenarioMgr=&manager;
struct SpellEffectData{uint32 Effect=42,TriggerSpell=206361;};
struct SpellInfo{uint32 Id=0;SpellEffectData effect;SpellEffectData*Effects[1]{&effect};};
struct Spell{SpellInfo info;SpellInfo const*GetSpellInfo(){return &info;}};
struct Player;struct Creature;struct TempSummon;struct SmartAI;
struct Unit:Position{
 struct Spline{bool done=true;bool Finalized(){return done;}}spline;Spline*movespline=&spline;
 virtual~Unit()=default;ObjectGuid guid;Map*map=&world;uint32 mapid=1539,phase=1;bool alive=true,inWorld=true,longJump=false,flight=false,los=true;float ground=0;std::set<std::pair<uint32,ObjectGuid>>auras;uint32 recoilCasts=0,arrivalCasts=0;
 virtual Player*ToPlayer(){return nullptr;}virtual Creature*ToCreature(){return nullptr;}
 ObjectGuid GetGUID()const{return guid;}bool IsInWorld()const{return inWorld;}bool IsAlive()const{return alive;}Map*GetMap()const{return map;}uint32 GetMapId()const{return mapid;}uint32 GetInstanceId()const{return 42;}
 bool InSamePhase(Unit*p)const{return phase==p->phase;}float GetExactDist2d(Position const*p)const{return std::hypot(x-p->x,y-p->y);}bool IsWithinDistInMap(Unit*p,float d)const{return GetExactDist2d(p)<=d;}
 bool HasAura(uint32 id,ObjectGuid caster)const{return auras.count({id,caster})!=0;}void RemoveAurasDueToSpell(uint32 id,ObjectGuid caster){auras.erase({id,caster});}
 bool AddAura(uint32 id,Unit*target){assert(id==130966&&target==this);auras.insert({id,guid});return true;}
 bool HasUnitState(uint32)const{return longJump;}bool isInFlight()const{return flight;}float GetAngle(Unit*p)const{return std::atan2(p->y-y,p->x-x);}float GetOrientation()const{return o;}
 Position GetFirstCollisionPosition(float d,float a){return {x+d*std::cos(a+o),y+d*std::sin(a+o),ground,0};}bool IsWithinLOS(float,float,float)const{return los;}float GetHeight(float,float,float,bool){return ground;}
 virtual SpellCastResult CastSpell(Unit*,uint32,bool){return SPELL_CAST_OK;}
};
std::map<ObjectGuid,Unit*>objects;
struct Player:Unit{uint32 klass=1,status=3,credits=0;bool ack=true;Player(){guid={uint64_t(objects.size()+1)};objects[guid]=this;x=10;}Player*ToPlayer()override{return this;}uint32 getClass()const{return klass;}uint32 GetQuestStatus(uint32 id){assert(id==41105);return status;}
 void UpdateAchievementCriteria(uint32 type,uint32 event,uint32 amount,uint32 misc,Unit*unit,bool ignoreGroup){assert(type==92&&event==49101&&!amount&&!misc&&unit==this&&ignoreGroup);++credits;if(ack)scenario.step=8;}
 SpellCastResult CastSpell(Unit*,uint32 id,bool)override{assert(id==206360||id==206361);if(id==206360)++recoilCasts;else++arrivalCasts;return SPELL_CAST_OK;}
};
struct Creature:Unit{uint32 entry=103398,dbGuid=370991,npcFlags=16,flags=0;SmartAI*ai=nullptr;bool IsAIEnabled=true,visible=true,despawned=false,spawnFail=false,channelFail=false,acceptedWithoutAura=false;uint32 summons=0,birthCasts=0,delegates=0,attacks=0;ReactStates react=REACT_AGGRESSIVE;Spell channel;bool channelActive=false;
 Creature(){guid={uint64_t(objects.size()+1)};objects[guid]=this;}Creature*ToCreature()override{return this;}uint32 GetEntry()const{return entry;}uint32 GetDBTableGUIDLow()const{return dbGuid;}TempSummon*ToTempSummon();
 SmartAI*AI(){return ai;}uint32 GetUInt32Value(uint32 f){return f==UNIT_FIELD_FLAGS?flags:npcFlags;}void SetFlag(uint32 f,uint32 v){(f==UNIT_FIELD_FLAGS?flags:npcFlags)|=v;}void RemoveFlag(uint32 f,uint32 v){(f==UNIT_FIELD_FLAGS?flags:npcFlags)&=~v;}
 ReactStates GetReactState(){return react;}void SetReactState(ReactStates r){react=r;}void SetVisible(bool v){visible=v;}void CombatStop(bool){}void DespawnOrUnsummon(){despawned=true;inWorld=false;}
 Spell*GetCurrentSpell(uint32){return channelActive?&channel:nullptr;}void InterruptSpell(uint32){channelActive=false;}
 SpellCastResult CastSpell(Unit*target,uint32 id,bool triggered)override{assert(target==this&&id==206390&&!triggered);++birthCasts;if(channelFail)return SPELL_FAILED_BAD_TARGETS;if(!acceptedWithoutAura){auras.insert({id,guid});channel.info.Id=id;channelActive=true;}return SPELL_CAST_OK;}
 Creature*SummonCreature(uint32,float,float,float,float,uint32,uint32);
};
struct TempSummon:Creature{ObjectGuid summoner;TempSummon(ObjectGuid g):summoner(g){entry=104276;dbGuid=0;}ObjectGuid GetSummonerGUID(){return summoner;}};
TempSummon*Creature::ToTempSummon(){return dynamic_cast<TempSummon*>(this);}
struct SmartAI{protected:Creature*me;public:explicit SmartAI(Creature*c):me(c){c->ai=this;}Creature*SourceForTest(){return me;}virtual~SmartAI()=default;virtual void Reset(){}virtual void UpdateAI(uint32){++me->delegates;}virtual void SpellHit(Unit*,SpellInfo const*){}virtual void SpellHitTarget(Unit*,SpellInfo const*){}void AttackStart(Player*){++me->attacks;}};
struct npc_stromkar_zakajz:SmartAI{using SmartAI::SmartAI;};
std::unique_ptr<TempSummon>born;std::unique_ptr<SmartAI>bornAI;
Creature*Creature::SummonCreature(uint32 e,float x,float y,float z,float o,uint32 type,uint32 life){assert(e==104276&&std::fabs(x-1882.18f)<.01&&std::fabs(y-2163.35f)<.01&&std::fabs(z-35.9247f)<.01&&std::fabs(o-2.00205f)<.01&&type==7&&!life);++summons;if(spawnFail)return nullptr;born=std::make_unique<TempSummon>(guid);bornAI=std::make_unique<npc_stromkar_zakajz>(born.get());return born.get();}
struct ObjectAccessor{static Creature*GetCreature(Creature&,ObjectGuid g){auto i=objects.find(g);return i==objects.end()?nullptr:i->second->ToCreature();}static Player*GetPlayer(Creature&,ObjectGuid g){auto i=objects.find(g);return i==objects.end()?nullptr:i->second->ToPlayer();}};
struct SpellTargetPosition{uint32 target_mapId=1539;};
struct SpellManager{bool authored=false;SpellTargetPosition position;SpellTargetPosition const*GetSpellTargetPosition(uint32){return authored?&position:nullptr;}}spells;auto*sSpellMgr=&spells;
struct Hook{void operator+=(int){}};
struct SpellScript{Unit*caster=nullptr,*target=nullptr,*original=nullptr;WorldLocation dest;bool prevented=false;virtual~SpellScript()=default;virtual bool Validate(SpellInfo const*){return true;}virtual void Register(){}Unit*GetCaster(){return caster;}Unit*GetExplTargetUnit(){return target;}Unit*GetOriginalTarget(){return original?original:target;}WorldLocation*GetHitDest(){return &dest;}void SetExplTargetDest(WorldLocation&l){dest=l;}void PreventHitDefaultEffect(uint32){prevented=true;}Hook OnCheckCast,OnEffectLaunch;};
#define PrepareSpellScript(...) public:
#define SpellCheckCastFn(...) 0
#define SpellEffectFn(...) 0
#include "../../src/server/scripts/Legion/ClassHalls/stromkar_awakening.h"

void ResetWorld(){bornAI.reset();born.reset();objects.clear();scenario={};manager={};spells={};warnings=0;}
void Channel(npc_stromkar_awakening&ai,Player&p){SpellInfo s;s.Id=204771;ai.SpellHit(&p,&s);}
void Blast(npc_stromkar_awakening&ai,Player&p){SpellInfo s;s.Id=206357;ai.SpellHit(ai.SourceForTest(),&s);ai.SpellHitTarget(&p,&s);}
void Stun(Player&p,bool on){if(on)p.auras.insert({206361,p.guid});else p.auras.erase({206361,p.guid});}
int main(){
 ResetWorld();{Creature c;Player p;npc_stromkar_awakening ai(&c);ai.UpdateAI(5000);assert(!p.credits&&!c.summons);Channel(ai,p);ai.UpdateAI(5000);assert(!p.credits&&!c.summons&&!ai.burst);Blast(ai,p);Stun(p,true);ai.UpdateAI(4999);assert(!c.summons);ai.UpdateAI(1);assert(c.summons==1&&!c.visible&&born->react==REACT_PASSIVE&&born->flags==7&&born->HasAura(206390,born->guid)&&!p.credits);ai.UpdateAI(2000);assert(!p.credits);Stun(p,false);ai.UpdateAI(2000);assert(p.credits==1&&scenario.step==8&&born->channelActive);ai.UpdateAI(1);assert(c.despawned&&!born->channelActive&&!born->HasAura(206390,born->guid)&&born->react==REACT_AGGRESSIVE&&!born->flags&&born->attacks==1);ai.UpdateAI(10000);assert(p.credits==1&&c.summons==1);}
 for(uint32 bad=0;bad<13;++bad){ResetWorld();Creature c;Player p;npc_stromkar_awakening ai(&c);if(bad==0)c.entry=1;if(bad==1)c.dbGuid=1;if(bad==2)c.mapid=1;if(bad==3)scenario.id=991;if(bad==4)scenario.step=8;if(bad==5)p.klass=16;if(bad==6)p.status=0;if(bad==7)p.alive=false;if(bad==8)p.map=&otherWorld;if(bad==9)p.phase=2;if(bad==10)c.alive=false;if(bad==11)c.inWorld=false;if(bad==12)manager.present=false;Channel(ai,p);Blast(ai,p);ai.UpdateAI(6000);assert(!p.credits&&!c.summons);}
 ResetWorld();{Creature c;Player p;npc_stromkar_awakening ai(&c);Channel(ai,p);Blast(ai,p);c.spawnFail=true;Stun(p,true);ai.UpdateAI(5000);assert(c.summons==1&&!p.credits);ai.UpdateAI(1999);assert(c.summons==1);c.spawnFail=false;ai.UpdateAI(1);assert(c.summons==2&&born);born->channelFail=true;born->auras.clear();Stun(p,false);ai.UpdateAI(2000);assert(!p.credits);born->channelFail=false;born->acceptedWithoutAura=true;ai.UpdateAI(2000);assert(!p.credits);born->acceptedWithoutAura=false;ai.UpdateAI(2000);assert(p.credits==1);}
 ResetWorld();{Creature c;Player p;npc_stromkar_awakening ai(&c);Channel(ai,p);Blast(ai,p);ai.UpdateAI(5000);assert(p.recoilCasts==1&&!p.credits);ai.UpdateAI(2000);assert(p.recoilCasts==1);ai.UpdateAI(60000);assert(!p.credits&&!ai.burst&&c.visible&&c.npcFlags==16&&born->despawned&&warnings);Channel(ai,p);Blast(ai,p);Stun(p,true);ai.UpdateAI(5000);assert(c.summons==2);}
 ResetWorld();{Creature c;Player p,q;npc_stromkar_awakening ai(&c);Channel(ai,p);Channel(ai,q);Blast(ai,p);SpellInfo ordinary;ai.SpellHitTarget(&q,&ordinary);/*non-blast cannot make q eligible*/Stun(p,true);ai.UpdateAI(5000);Stun(p,false);ai.UpdateAI(2000);assert(p.credits==1&&!q.credits);}
 ResetWorld();{Creature c;Player p,q;npc_stromkar_awakening ai(&c);Channel(ai,p);Channel(ai,q);Blast(ai,p);Blast(ai,q);Stun(p,true);Stun(q,true);ai.UpdateAI(5000);Stun(p,false);ai.UpdateAI(2000);assert(!p.credits&&!q.credits);Stun(q,false);ai.UpdateAI(2000);assert(p.credits+q.credits==1);}
 ResetWorld();{Creature c;Player p;npc_stromkar_awakening ai(&c);Channel(ai,p);Blast(ai,p);Stun(p,true);ai.UpdateAI(5000);p.status=0;ai.UpdateAI(1);assert(!p.credits&&!ai.burst&&born->despawned&&!p.HasAura(206361,p.guid)&&c.visible);}
 ResetWorld();{Creature c;Player p;npc_stromkar_awakening ai(&c);Channel(ai,p);Blast(ai,p);Stun(p,true);ai.UpdateAI(5000);born->IsAIEnabled=false;Stun(p,false);ai.UpdateAI(2000);assert(!p.credits);born->IsAIEnabled=true;p.ack=false;ai.UpdateAI(2000);assert(p.credits==1&&scenario.step==7);ai.UpdateAI(1000);assert(p.credits==1);p.ack=true;ai.UpdateAI(1000);assert(p.credits==2&&scenario.step==8);}
 ResetWorld();{Creature c;Player p;Unit pet,guardian;npc_stromkar_awakening ai(&c);SpellInfo channel;channel.Id=204771;ai.SpellHit(&pet,&channel);ai.SpellHit(&guardian,&channel);Blast(ai,p);ai.UpdateAI(5000);assert(!c.summons&&!p.credits);Channel(ai,p);SpellInfo forged;forged.Id=206357;ai.SpellHit(&p,&forged);ai.UpdateAI(5000);assert(!c.summons&&!p.credits);}
 ResetWorld();{Creature c;Player p;npc_stromkar_awakening ai(&c);Channel(ai,p);Blast(ai,p);Stun(p,true);ai.UpdateAI(5000);SmartAI foreign(born.get());Stun(p,false);ai.UpdateAI(2000);assert(!p.credits);born->ai=bornAI.get();born->flags|=16;ai.UpdateAI(2000);assert(p.credits==1);ai.UpdateAI(1);assert(born->flags==16);}
 ResetWorld();{Creature c;Player p;npc_stromkar_awakening ai(&c);Channel(ai,p);Blast(ai,p);Stun(p,true);ai.UpdateAI(5000);p.auras.insert({206361,{999}});ai.Reset();assert(!p.credits&&!ai.burst&&c.visible&&born->despawned&&!p.HasAura(206361,p.guid)&&p.HasAura(206361,{999}));}
 ResetWorld();{Creature c;Player p;npc_stromkar_awakening ai(&c);Channel(ai,p);Blast(ai,p);Stun(p,true);ai.UpdateAI(5000);born->alive=false;Stun(p,false);ai.UpdateAI(2000);assert(!p.credits&&c.summons==1);ai.UpdateAI(60000);assert(!p.credits&&c.summons==1&&c.visible);}
 ResetWorld();{Creature c;Player p;c.auras.insert({130966,c.guid});npc_stromkar_awakening ai(&c);Channel(ai,p);Blast(ai,p);Stun(p,true);c.channelFail=true;ai.UpdateAI(1000);assert(!c.summons&&!p.credits&&!c.HasAura(130966,c.guid)&&warnings);c.channelFail=false;ai.UpdateAI(2000);assert(c.HasAura(206390,c.guid)&&!c.summons);ai.UpdateAI(3000);assert(c.summons==1&&!c.HasAura(206390,c.guid));ai.Reset();assert(c.HasAura(130966,c.guid)&&!c.HasAura(206390,c.guid)&&!p.credits);}
 ResetWorld();{Creature c;Player p;npc_stromkar_awakening ai(&c);Channel(ai,p);Blast(ai,p);Stun(p,true);ai.UpdateAI(5000);Stun(p,false);p.longJump=true;p.spline.done=false;ai.UpdateAI(2000);assert(!p.credits);p.spline.done=true;ai.UpdateAI(2000);assert(p.credits==1&&p.longJump);/* server completion, no client ACK required */}
 ResetWorld();{Creature c;Player p;npc_stromkar_awakening ai(&c);Channel(ai,p);spell_stromkar_opening_recoil s;s.caster=&p;s.original=&c;s.target=nullptr;assert(s.Check()==SPELL_CAST_OK);s.Launch(0);assert(!s.prevented&&std::fabs(s.dest.x-40)<.001);p.los=false;s.prevented=false;s.Launch(0);assert(s.prevented);p.los=true;spells.authored=true;spells.position.target_mapId=1;s.prevented=false;s.Launch(0);assert(s.prevented);spells.position.target_mapId=1539;s.dest={25,0,0,0};s.prevented=false;s.Launch(0);assert(!s.prevented&&s.dest.x==25);p.ground=10;s.prevented=false;s.Launch(0);assert(s.prevented);Player stranger;s.caster=&stranger;assert(s.Check()==SPELL_FAILED_BAD_TARGETS);s.prevented=false;s.Launch(0);assert(s.prevented);}
 ResetWorld();{Creature c;Player p;npc_stromkar_awakening ai(&c);Channel(ai,p);Blast(ai,p);Stun(p,true);c.acceptedWithoutAura=true;ai.UpdateAI(1);assert(!warnings&&!c.summons&&ai.sourceChannelPending);c.auras.insert({206390,c.guid});ai.UpdateAI(4999);assert(!warnings&&!c.summons);ai.UpdateAI(1);assert(born);born->auras.clear();born->acceptedWithoutAura=true;Stun(p,false);ai.UpdateAI(2000);assert(!warnings&&!p.credits&&ai.bossChannelPending);born->auras.insert({206390,born->guid});ai.UpdateAI(2000);assert(!warnings&&p.credits==1);}
 ResetWorld();{Creature c;Player p;npc_stromkar_awakening ai(&c);Channel(ai,p);Blast(ai,p);Stun(p,true);c.acceptedWithoutAura=true;ai.UpdateAI(1);ai.UpdateAI(1999);assert(!warnings&&!c.summons);ai.UpdateAI(1);assert(warnings==1&&!c.summons&&!p.credits);}
 std::cout<<"PASS actual awakening controller/recoil: real blast and stun/expiry gate, spawn/channel/credit acknowledgement, interruption/retry/abort, shared participants and no click-only credit. Cast/aura/collision services controlled; no client rendering claim.\n";
}
