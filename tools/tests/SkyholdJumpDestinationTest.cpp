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
    void Launch();
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
    bool flying=false,failCast=false;
    Position pose,destination;
    MotionMaster movement{this};
    virtual ~Unit()=default;
    virtual Player* ToPlayer() { return nullptr; }
    virtual Creature* ToCreature() { return nullptr; }
    bool IsPlayer() { return ToPlayer()!=nullptr; }
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
    uint32 CastSpell(Unit* target,uint32 identity,bool triggered) {
        assert(target==this&&triggered);++casts;castSpell=identity;return failCast?1:0;
    }
};
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
    EffectMovementGenerator(uint32 point,uint32 spell,ObjectGuid target):_pointId(point),_arrivalSpellId(spell),_arrivalSpellTargetGuid(target) { }
    void Finalize(Unit&);
    void MovementInform(Unit&);
};
#include "Arrival.inc"
void MotionMaster::Mutate(EffectMovementGenerator* movement,uint32 slot) { assert(slot==MOTION_SLOT_CONTROLLED);current.reset(movement); }
void MotionMaster::Complete() { if(current){auto finished=std::move(current);finished->Finalize(*_owner);} }
void Movement::MoveSplineInit::MoveTo(float positionX,float positionY,float positionZ,bool) { owner.destination.Relocate(positionX,positionY,positionZ); }
void Movement::MoveSplineInit::Launch() { ++owner.jumps; }
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
    Effect* Effects[1]{&effect};
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
    ~Spell() { delete pending; }
    void EffectJumpDest(SpellEffIndex);
    void CalculateJumpSpeeds(uint8,float,float&,float&);
    DelayCastEvent* GetSpellTriggerDelay(SpellEffIndex effect,uint32 triggered) { pending=NativeDelay(effect,triggered);return pending; }
    DelayCastEvent* NativeDelay(SpellEffIndex,uint32);
};
#include "Delay.inc"
#include "Jump.inc"
#include "Speeds.inc"
int main(int argc,char** argv) {
    assert(argc==2);std::ifstream input(argv[1]);SpellTriggerDelay delay;
    assert(input>>delay.spell_trigger>>delay.target>>delay.caster>>delay.effectMask>>delay.aura>>delay.targetaura);
    spellManager.delays={delay};
    for(uint32 map:{1539u,1466u,1428u,1220u,1479u})for(float height:{-50.0f,0.0f,35.8446f,100.18f,745.833f}) {
        Player player;player.map=map;player.pose.m_positionZ=height;
        Spell spell(player);spell.EffectJumpDest(0);
        assert(player.jumps==1&&player.casts==0&&player.fallZ==height+20&&player.knockback==0&&player.splineTimer==0);
        assert(spell.location.m_positionX==player.pose.m_positionX&&spell.location.m_positionY==player.pose.m_positionY&&spell.location.m_orientation==1.25f);
#ifdef BASELINE
        assert(player.destination.m_positionZ==height&&spell.location.m_positionZ==height);
#else
        assert(player.destination.m_positionZ==height+20&&spell.location.m_positionZ==height+20);
#endif
        player.movement.Complete();player.movement.Complete();assert(player.casts==1&&player.castSpell==216016);
    }
    for(uint32 variant=0;variant<3;++variant) {
        Player player;Spell spell(player);
        if(variant==0)spell.effectHandleMode=0;
        if(variant==1)player.flying=true;
        if(variant==2)spell.m_targets.destination=false;
        spell.EffectJumpDest(0);assert(!player.jumps&&!player.casts&&player.knockback==888&&spell.location.m_positionZ==30);
    }
    for(uint32 identity:{206455u,197619u,199166u,199162u,199189u,199191u}) {
        Player player;player.map=identity>=199000&&identity<200000?1522:1539;
        Spell spell(player);spell.info.Id=identity;spell.location.m_positionX+=10;spell.EffectJumpDest(0);
        assert(player.jumps==1&&!player.casts);
        assert(player.destination.m_positionZ==(player.map==1522?257.026f:30.0f));
    }
    Creature creature;Spell nonplayer(creature);nonplayer.EffectJumpDest(0);assert(creature.jumps==1&&!creature.casts);
    creature.movement.Complete();assert(creature.casts==1&&creature.castSpell==216016);
    Player failure;failure.failCast=true;Spell failed(failure);failed.EffectJumpDest(0);failure.movement.Complete();assert(failure.casts==1);
#ifdef BASELINE
    std::cout<<"PASS: pinned native baseline computes Skyhold Z+20 for speeds/fall state but launches the original floor destination\n";
#else
    std::cout<<"PASS: actual native jump/speed/Position/MotionMaster/arrival handlers pass Skyhold Z+20 into spline MoveTo, preserving XY/orientation, delay216016 and launch guards; other jump destinations stay unchanged. Spline transport and completion services are controlled, not client ascent/teleport or exactly-once controller proof\n";
#endif
}
