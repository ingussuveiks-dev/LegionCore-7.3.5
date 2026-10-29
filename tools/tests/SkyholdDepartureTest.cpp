#include <cassert>
#include <cmath>
#include <cstdint>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <memory>
#include <vector>
using uint32=uint32_t;
using uint8=uint8_t;
using int32=int32_t;
#include "../../src/server/game/Movement/SkyholdDeparture.h"
using SpellEffIndex=uint32;
#define TC_LOG_DEBUG(...) ((void)0)
enum { SPELL_EFFECT_HANDLE_LAUNCH=1, TARGET_DEST_TARGET_BACK=55, UNIT_STATE_LONG_JUMP=1, MOTION_SLOT_CONTROLLED=1, EFFECT_MOTION_TYPE=3, EVENT_JUMP=0 };
struct ObjectGuid {
    uint32 value=0;
    static const ObjectGuid Empty;
    bool IsEmpty() const { return !value; }
    uint32 GetGUIDLow() const { return value; }
};
const ObjectGuid ObjectGuid::Empty{};
struct Position {
    float m_positionX=100,m_positionY=200,m_positionZ=30,m_orientation=1.25f;
    Position()=default;
    Position(float positionX,float positionY):m_positionX(positionX),m_positionY(positionY) { }
    float GetPositionX() const { return m_positionX; }
    float GetPositionY() const { return m_positionY; }
    float GetPositionZ() const { return m_positionZ; }
    float GetOrientation() const { return m_orientation; }
    void GetPosition(float& positionX,float& positionY,float& positionZ) const { positionX=m_positionX;positionY=m_positionY;positionZ=m_positionZ; }
    float GetRelativeAngle(void*) const { return 0; }
#include "Relocate.inc"
};
struct JumpArrivalCastArgs { uint32 SpellId=0;ObjectGuid Target{}; };
struct Item {}; struct AuraEffect {};
enum SpellCastResult {SPELL_CAST_OK,SPELL_FAILED_ERROR};
enum TriggerCastFlags {TRIGGERED_NONE,TRIGGERED_FULL_MASK};
struct Spline{bool done=false;bool Finalized(){return done;}};
struct Unit;
struct Player;
struct Creature;
struct EffectMovementGenerator;
namespace Movement {
constexpr float gravity=19.291105f;
float computeFallElevation(float duration,bool,float speed) { return gravity*duration*duration/2+speed*duration; }
struct SpellEffectExtraData { };
struct MoveSplineInit {
    Unit& owner;
    explicit MoveSplineInit(Unit& unit):owner(unit) { }
    void MoveTo(float,float,float,bool);
    void SetParabolic(float,uint32) { }
    void SetVelocity(float) { }
    void SetFacing(float) { }
    void SetSpellEffectExtraData(SpellEffectExtraData const&) { }
    int32 Launch();
};
}
struct MotionMaster {
    Unit* _owner;
    std::unique_ptr<EffectMovementGenerator> current;
    explicit MotionMaster(Unit* owner):_owner(owner) { }
    void Mutate(EffectMovementGenerator* movement,uint32 slot);
    void MoveJump(float,float,float,float,float,float,uint32,bool,JumpArrivalCastArgs const*,Movement::SpellEffectExtraData const*);
#include "MoveJumpPosition.inc"
    void Complete();
};
struct Unit {
    uint32 map=1539,jumps=0,casts=0,castSpell=0,state=0,splineTimer=777;
    bool flying=false,failCast=false,alive=true,m_cleanupDone=false; int32 launchResult=250; Spline spline; Spline* movespline=&spline;
    Position pose,destination;
    MotionMaster movement{this};
    virtual ~Unit()=default;
    virtual Player* ToPlayer() { return nullptr; }
    virtual Creature* ToCreature() { return nullptr; }
    bool IsPlayer() { return ToPlayer()!=nullptr; }
    bool IsAlive() const { return alive; }
    bool isInFlight() const { return flying; }
    uint32 GetMapId() const { return map; }
    ObjectGuid GetGUID() const { return {99}; }
    Unit* GetAnyOwner() { return nullptr; }
    bool HasAura(uint32) const { return false; }
    Unit* GetUnitForLinkedSpell(Unit*,Unit*,uint32,Unit*) { assert(false);return nullptr; }
    MotionMaster* GetMotionMaster() { return &movement; }
    Unit* getVictim() { return nullptr; }
    float GetOrientation() const { return pose.GetOrientation(); }
    float GetExactDist(float positionX,float positionY,float positionZ) const {
        float deltaX=positionX-pose.m_positionX,deltaY=positionY-pose.m_positionY,deltaZ=positionZ-pose.m_positionZ;
        return std::sqrt(deltaX*deltaX+deltaY*deltaY+deltaZ*deltaZ);
    }
    void SetSplineTimer(uint32 value) { splineTimer=value; }
    void AddUnitState(uint32 value) { state|=value; }
    void ClearUnitState(uint32 value) { state&=~value; }
    SpellCastResult CastSpell(Unit*,uint32,bool,Item* =nullptr,AuraEffect const* =nullptr,ObjectGuid ={});
    SpellCastResult CastSpell(Unit* target,uint32 identity,TriggerCastFlags,Item*,AuraEffect const*,ObjectGuid) {
        assert(target==this);++casts;castSpell=identity;return failCast?SPELL_FAILED_ERROR:SPELL_CAST_OK;
    }
};
#include "NativeCast.inc"
struct Player : Unit {
    float fallZ=-1;
    uint32 knockback=888;
    Player* ToPlayer() override { return this; }
    ObjectGuid GetSelection() const { return {}; }
    void SetFallInformation(uint32,float height) { fallZ=height; }
    void SetKnockBackTime(uint32 value) { knockback=value; }
};
struct Creature : Unit {
    struct Brain { void MovementInform(uint32,uint32) { } } brain;
    Creature* ToCreature() override { return this; }
    Brain* AI() { return &brain; }
};
struct ObjectAccessor { static Unit* GetUnit(Unit& owner,ObjectGuid identity) { return identity.value==99?&owner:nullptr; } };
struct EffectMovementGenerator {
    uint32 _pointId,_arrivalSpellId;
    ObjectGuid _arrivalSpellTargetGuid;
    bool _arrived=false;
    uint32 _arrivalMapId;
    EffectMovementGenerator(uint32 point,uint32 spell,ObjectGuid target,uint32 map=uint32(-1)):_pointId(point),_arrivalSpellId(spell),_arrivalSpellTargetGuid(target),_arrivalMapId(map) { }
    bool Update(Unit&,uint32);
    void Finalize(Unit&);
    void MovementInform(Unit&);
};
#include "Arrival.inc"
void MotionMaster::Mutate(EffectMovementGenerator* movement,uint32 slot) { assert(slot==MOTION_SLOT_CONTROLLED);current.reset(movement); }
void MotionMaster::Complete() { if(current){auto finished=std::move(current);_owner->spline.done=true;assert(!finished->Update(*_owner,1));finished->Finalize(*_owner);} }
void Movement::MoveSplineInit::MoveTo(float positionX,float positionY,float positionZ,bool) { owner.destination.Relocate(positionX,positionY,positionZ); }
int32 Movement::MoveSplineInit::Launch() { ++owner.jumps; owner.spline.done=false;return owner.launchResult; }
#include "MoveJump.inc"
struct SpellTriggerDelay { uint32 spell_trigger=0,targetaura=0,target=0,caster=0,effectMask=7;int aura=0; };
struct SpellMgr {
    std::vector<SpellTriggerDelay> delays;
    std::vector<SpellTriggerDelay> const* GetSpellTriggerDelay(uint32 identity) { return identity==192085?&delays:nullptr; }
} spellManager;
auto* sSpellMgr=&spellManager;
struct DelayCastEvent {
    ObjectGuid TargetGUID;
    uint32 Spell;
    DelayCastEvent(Unit&,ObjectGuid target,uint32 identity,bool,ObjectGuid):TargetGUID(target),Spell(identity) { }
};
struct Effect {
    uint32 TriggerSpell=0;
    int32_t MiscValue=0,MiscValueB=0;
    struct Target { uint32 GetTarget() const { return 0; } } TargetA;
};
struct SpellInfo {
    uint32 Id=192085;
    Effect effect;
    Effect* Effects[3]{&effect,&effect,&effect};
    Effect* GetEffect(uint32,uint32) { return &effect; }
};
struct Spell {
    Unit* m_caster;
    Unit* m_originalCaster=nullptr;
    Unit* unitTarget=nullptr;
    SpellInfo info;
    SpellInfo* m_spellInfo=&info;
    uint32 effectHandleMode=SPELL_EFFECT_HANDLE_LAUNCH,m_diffMode=0;
    Position location;
    Position* destTarget=&location;
    struct Targets {
        bool destination=true;
        bool HasDst() const { return destination; }
        Unit* GetUnitTarget() const { return nullptr; }
        ObjectGuid GetObjectTargetGUID() const { return {}; }
    } m_targets;
    DelayCastEvent* pending=nullptr;
    explicit Spell(Unit& caster):m_caster(&caster),location(caster.pose) { }

    void EffectJumpDest(SpellEffIndex);
    void CalculateJumpSpeeds(uint8,float,float&,float&);
    DelayCastEvent* GetSpellTriggerDelay(SpellEffIndex effect,uint32 triggered) { return NativeDelay(effect,triggered); }
    DelayCastEvent* NativeDelay(SpellEffIndex,uint32);
};
#include "Delay.inc"
#include "Jump.inc"
#include "Speeds.inc"

int main(int argc,char**argv){
 assert(argc==2);std::ifstream input(argv[1]);uint32 id,arrival,index;uint32 cases=0,pairs=0;
 while(input>>id>>arrival>>index){
  assert(SkyholdDeparture::Matches(id,arrival)||(id==206455&&arrival==206456));++pairs;
  for(uint32 state=0;state<7;++state){
   Player player;player.map=id==206455?1539:1479;player.pose.m_positionZ=98;
   Spell spell(player);spell.info.Id=id;spell.info.effect.TriggerSpell=arrival;spell.info.effect.MiscValue=300;
   spell.location.Relocate(1108.74f,7222.54f,39.45f); // controlled native launch endpoint, not a new world destination
   if(state==1)player.launchResult=0;
   spell.EffectJumpDest(index);assert(!player.casts);
   if(state==1){assert(!player.movement.current&&!(player.state&UNIT_STATE_LONG_JUMP));continue;}
   assert(player.movement.current);
   if(state==2){auto cancelled=std::move(player.movement.current);cancelled->Finalize(player);assert(!player.casts);continue;}
   if(state==3)player.alive=false;
   if(state==4)player.map=1220;
   if(state==5)player.m_cleanupDone=true;
   if(state==6)player.failCast=true;
   player.movement.Complete();player.movement.Complete();
#ifdef BASELINE
   assert(!player.casts); // actual Unit::CastSpell rejects the empty arrival target
#else
   assert(player.casts==(state==0||state==6?1u:0u));if(player.casts)assert(player.castSpell==arrival);
#endif
   ++cases;
  }
 }
 assert(pairs>0&&cases==pairs*5);
 for(uint32 map:{1479u,1539u}){Creature c;c.map=map;Spell spell(c);spell.info.Id=206455;spell.info.effect.TriggerSpell=206456;spell.EffectJumpDest(0);c.movement.Complete();assert(!c.casts);}
 {Player p;p.map=1479;Spell spell(p);spell.info.Id=206455;spell.info.effect.TriggerSpell=206456;spell.EffectJumpDest(0);p.movement.Complete();assert(!p.casts);}
 {Player p;p.map=1539;Spell spell(p);spell.info.Id=206455;spell.info.effect.TriggerSpell=123;spell.EffectJumpDest(0);p.movement.Complete();assert(!p.casts);}
 {Player p;p.map=1539;Spell spell(p);spell.info.Id=123;spell.info.effect.TriggerSpell=206456;spell.EffectJumpDest(0);p.movement.Complete();assert(!p.casts);}
 for(uint32 variant=0;variant<3;++variant){Player p;p.map=1479;Spell spell(p);spell.info.Id=203921;spell.info.effect.TriggerSpell=203920;if(variant==0)spell.effectHandleMode=0;if(variant==1)p.flying=true;if(variant==2)spell.m_targets.destination=false;spell.EffectJumpDest(0);assert(!p.jumps&&!p.casts);}
 for(uint32 map:{1479u,1220u}){Creature c;c.map=map;Spell spell(c);spell.info.Id=203921;spell.info.effect.TriggerSpell=203920;spell.EffectJumpDest(0);c.movement.Complete();assert(!c.casts);}
 {Player p;p.map=1220;Spell spell(p);spell.info.Id=203921;spell.info.effect.TriggerSpell=203920;spell.EffectJumpDest(0);p.movement.Complete();assert(!p.casts);}
 {Player p;p.map=1479;Spell spell(p);spell.info.Id=203921;spell.info.effect.TriggerSpell=123;spell.EffectJumpDest(0);p.movement.Complete();assert(!p.casts);}
 {Player p;p.map=1479;Spell spell(p);spell.info.Id=123;spell.info.effect.TriggerSpell=203920;spell.EffectJumpDest(0);p.movement.Complete();assert(!p.casts);}
#ifdef BASELINE
 std::cout<<"COUNTERFACTUAL pinned destination jump leaves native arrival target empty; actual Unit::CastSpell rejects "<<pairs<<" tested native pairs.\n";
#else
 std::cout<<"PASS actual destination jump, native Unit null/cleanup guard, MoveJump and real arrival Update/Finalize:"<<pairs<<" native pairs, successful self arrival, zero launch, interruption/death/map change/cleanup, failed cast, repeat completion, launch guards and unchanged foreign/NPC/metadata paths. Cast endpoint/spline lifecycle services controlled; actual client effects remain separate.\n";
#endif
}
