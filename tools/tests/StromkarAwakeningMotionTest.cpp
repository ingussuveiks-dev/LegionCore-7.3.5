#include <cassert>
#include <cstdint>
#include <iostream>
#include "../../src/server/game/Movement/SkyholdDeparture.h"
using uint32=uint32_t;
enum{UNIT_STATE_LONG_JUMP=1,TARGET_FLAG_UNIT_MASK=2,TARGET_FLAG_CORPSE_MASK=4,TARGET_FLAG_GAMEOBJECT_MASK=8};
struct ObjectGuid{uint32 value=0;bool IsEmpty()const{return !value;}static const ObjectGuid Empty;};const ObjectGuid ObjectGuid::Empty{};
struct Position{};
struct JumpArrivalCastArgs{uint32 SpellId;ObjectGuid Target;};
struct Motion{JumpArrivalCastArgs captured{};bool facing=false;void MoveJump(Position const&,float,float,uint32,bool orient,JumpArrivalCastArgs const*cast){captured=*cast;facing=orient;}};
struct Unit{bool player=true,longJump=true;uint32 map=1539;Motion motion;bool IsPlayer(){return player;}bool IsUnit(){return true;}void*ToGameObject(){return nullptr;}void*ToCorpse(){return nullptr;}uint32 GetMapId(){return map;}ObjectGuid GetGUID(){return {1};}Motion*GetMotionMaster(){return &motion;}void ClearUnitState(uint32){longJump=false;}};
using WorldObject=Unit;
struct Effect{uint32 TriggerSpell=206361;};
struct SpellInfo{uint32 Id=206360;Effect effect;Effect*GetEffect(uint32,uint32){return &effect;}};
struct Delay{uint32 Spell=0;ObjectGuid TargetGUID;};
struct Spell{
 Unit*m_caster;SpellInfo info;SpellInfo*m_spellInfo=&info;uint32 m_diffMode=0;Position position,*destTarget=&position;float speedXY=30,speedZ=10;Delay*delayCast=nullptr;
 struct Targets{Unit*object=nullptr;ObjectGuid GetObjectTargetGUID(){return {object?9u:0u};}Unit*GetUnitTarget()const{return object;}Unit*GetObjectTarget()const{return object;}void RemoveObjectTarget(){object=nullptr;}}m_targets;
 void Fixed(){uint32 effIndex=0;
#include "SelfTarget.inc"
 }
 void Baseline(){uint32 effIndex=0;
#include "BaselineSelfTarget.inc"
 }
};
struct Normalizer{Spell::Targets m_targets;Unit*m_originalTarget=nullptr;ObjectGuid m_originalTargetGUID;void Init(Unit*source){Spell::Targets targets;targets.object=source;
#include "OriginalTarget.inc"
 uint32 neededTargets=0;
#include "NormalizeUnitTarget.inc"
 }};
struct Guard{
 Unit*_owner;bool installed=false;
 void Fixed(uint32 id,int duration,JumpArrivalCastArgs const*arrivalCast){
#include "LaunchGuard.inc"
 installed=true;
 }
 void Baseline(uint32 id,int duration,JumpArrivalCastArgs const*arrivalCast){
#include "BaselineLaunchGuard.inc"
 installed=true;
 }
};
int main(){
 Unit source;Normalizer normalization;normalization.Init(&source);assert(!normalization.m_targets.GetUnitTarget()&&normalization.m_originalTarget==&source&&normalization.m_originalTargetGUID.value==1);
 Unit p;Spell spell{&p};spell.Baseline();assert(p.motion.captured.SpellId==206361&&p.motion.captured.Target.IsEmpty()&&!p.motion.facing);spell.Fixed();assert(p.motion.captured.Target.value==1&&p.motion.facing);
 for(uint32 bad=0;bad<3;++bad){Unit u;Spell s{&u};if(bad==0)u.player=false;if(bad==1)u.map=1;if(bad==2)s.info.effect.TriggerSpell=1;s.Fixed();assert(u.motion.captured.Target.IsEmpty()&&!u.motion.facing);}
 { Unit u,target;Spell s{&u};s.info.Id=197619;s.m_targets.object=&target;s.Fixed();assert(u.motion.facing);s.m_targets.object=nullptr;s.Fixed();assert(!u.motion.facing); }
 Unit execution;Spell exec{&execution};exec.info.Id=206455;exec.info.effect.TriggerSpell=206456;exec.Fixed();assert(execution.motion.captured.Target.value==1);
 JumpArrivalCastArgs cast{206361,{1}};
 for(int duration:{-1,0,100}){Unit old,now;Guard a{&old},b{&now};a.Baseline(206360,duration,&cast);b.Fixed(206360,duration,&cast);assert(a.installed);assert(b.installed==(duration>0));assert(now.longJump==(duration>0));}
 for(uint32 bad=0;bad<4;++bad){Unit u;Guard guard{&u};JumpArrivalCastArgs other=cast;if(bad==0)u.map=1;if(bad==1)u.player=false;if(bad==2)other.SpellId=1;guard.Fixed(206360,0,bad==3?nullptr:&other);assert(guard.installed&&u.longJump);}
 std::cout<<"PASS production destination-jump self-target assignment and MotionMaster launch guard with pinned native baseline: old recoil lacks self target/accepts failed launch; fixed actual206360/206361 preserves unrelated/map/NPC/Execute paths. Spline service controlled.\n";
}
