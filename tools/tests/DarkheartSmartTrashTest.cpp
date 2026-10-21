#include <cassert>
#include <cmath>
#include <cstdint>
#include <fstream>
#include <iostream>
#include <list>
#include <sstream>
#include <string>
using uint32=uint32_t;
constexpr int SMART_EVENT_HEALTH_PCT=2,SMART_EVENT_RANGE=9,SMART_EVENT_FLAG_ALLOW_EVENT_IN_COMBAT=128;
constexpr int SMART_TARGET_HOSTILE_RANDOM_NOT_TOP=6,SMART_TARGET_HOSTILE_RANDOM_PLAYER=125,SELECT_TARGET_RANDOM=0;
struct Position{float m_positionX=0,m_positionY=0,m_positionZ=0;float GetPositionX(){return m_positionX;}float GetPositionY(){return m_positionY;}Position const& GetPosition()const{return *this;}void Relocate(Position const& p){*this=p;}};
struct Unit{virtual ~Unit()=default;};
Unit player,pet;
struct TargetAI{Unit* SelectTarget(int,int skip,float=0,bool players=false){return skip?nullptr:(players?&player:&pet);}};
struct Motion{Position result;void MoveCharge(float x,float y,float z){result={x,y,z};}};
struct Creature:Unit{
    float hp=100,distance=3,wall=100,orientation=0;bool combat=true;int collisions=0;TargetAI ai;Motion motion;
    bool isInCombat(){return combat;}int GetMaxHealth(){return 100;}float GetHealthPct(){return hp;}
    Unit* getVictim(){return &pet;}bool IsInRange(Unit*,float low,float high){return distance>=low&&distance<=high;}
    TargetAI* AI(){return &ai;}Creature* ToCreature(){return this;}bool IsCreature(){return true;}
    float GetRelativeAngle(float x,float y){return std::atan2(y,x)-orientation;}
    float GetDistance(Position const& p){return std::hypot(p.m_positionX,p.m_positionY);}
    Position GetFirstCollisionPosition(float dist,float angle){++collisions;dist=std::min(dist,wall);return {dist*std::cos(angle+orientation),dist*std::sin(angle+orientation),0};}
    void GetNearPoint2D(Position& p,float d,float a,bool=false){p={d*std::cos(a),d*std::sin(a),0};}
    Motion* GetMotionMaster(){return &motion;}void CastSpell(float,float,float,int,bool,void*,void*,int){}
};
struct Row{struct {uint32 event_flags=0;struct{uint32 min=0,max=0,repeatMin=1,repeatMax=1;}minMaxRepeat;}event;};
struct Smart{
    Creature* me;int actions=0;
    void RecalcTimer(Row&,uint32,uint32){}void ProcessAction(Row&,Unit* = nullptr){++actions;}
    void Run(int type,Row& e){switch(type){
#include "SmartHealth.inc"
#include "SmartRange.inc"
    }}
    std::list<Unit*> Select(int target){std::list<Unit*> list;auto l=&list;switch(target){
#include "SmartRandom.inc"
    }return list;}
};
constexpr int SPELL_EFFECT_HANDLE_LAUNCH=1,SPELL_EFFECT_HANDLE_HIT=2;
struct Effect{int TriggerSpell=0;};struct Info{Effect effect;Effect* GetEffect(int,int){return &effect;}};
enum{TARGET_DEST_CASTER_RADIUS=0,TARGET_DEST_CASTER_FRONT_RIGHT,TARGET_DEST_CASTER_BACK_RIGHT,TARGET_DEST_CASTER_BACK_LEFT,TARGET_DEST_CASTER_FRONT_LEFT,TARGET_DEST_CASTER_FRONT,TARGET_DEST_CASTER_BACK=48,TARGET_DEST_CASTER_RIGHT,TARGET_DEST_CASTER_LEFT,TARGET_UNK_125};
struct Destination:Position{Position _position;float GetPositionX(){return m_positionX;}float GetPositionY(){return m_positionY;}float GetPositionZ(){return m_positionZ;}};
struct Spell{
    Creature* m_caster;Destination destination;Destination* destTarget=&destination;bool canHitTargetInLOS=false;
    int effectHandleMode=SPELL_EFFECT_HANDLE_LAUNCH,m_diffMode=0,m_originalCasterGUID=0;Info info;Info* m_spellInfo=&info;
    void SelectBack(float dist){Destination dest;float angle=3.14159265359f;switch(TARGET_DEST_CASTER_BACK){
#include "NativeBackDestination.inc"
    }destination.m_positionX=dest.m_positionX;destination.m_positionY=dest.m_positionY;destination.m_positionZ=dest.m_positionZ;}
    void Charge(){int effIndex=0;
#include "NativeCharge.inc"
    }
};
int main(int argc,char** argv){
    assert(argc==2);Creature creature;Smart ai{&creature};
    std::ifstream rows(argv[1]);assert(rows);std::string line;int checked=0;
    while(std::getline(rows,line)){
        std::istringstream stream(line);int entry,event,target;Row row;
        stream>>entry>>event>>row.event.minMaxRepeat.min>>row.event.minMaxRepeat.max>>target;assert(stream);
        if(event==SMART_EVENT_HEALTH_PCT){
            auto threshold=row.event.minMaxRepeat.max;
            assert(row.event.minMaxRepeat.min==0);creature.hp=float(threshold)+1;ai.actions=0;ai.Run(event,row);assert(!ai.actions);
            creature.hp=float(threshold);ai.Run(event,row);assert(ai.actions==1);
            creature.hp=5;ai.Run(event,row);assert(ai.actions==2); // A large nonlethal hit skipped the old narrow band.
            creature.combat=false;ai.Run(event,row);assert(ai.actions==2);creature.combat=true;
            if(entry==100527){assert(target==125);auto picked=ai.Select(target);assert(picked.size()==1&&picked.front()==&player);assert(ai.Select(6).empty());}
        }else{
            assert(entry==99358&&event==9);creature.distance=9;ai.actions=0;ai.Run(event,row);assert(!ai.actions);
            creature.distance=3;ai.Run(event,row);assert(ai.actions==1); // Pet victim is accepted.
        }
        ++checked;
    }
    assert(checked==4);
    Spell spell;spell.m_caster=&creature;
    for(float wall:{100.0f,4.0f}){
        creature.wall=wall;spell.SelectBack(15);spell.Charge();
        assert(std::abs(creature.motion.result.m_positionX+std::min(wall,15.0f))<0.001f);
        assert(std::abs(creature.motion.result.m_positionY)<0.001f);
    }
    std::cout<<"PASS: migrated rows execute native SmartAI health/range/player selection and native backward/collision/charge destination branches\n";
}
