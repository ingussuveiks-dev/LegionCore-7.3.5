#include <algorithm>
#include <cassert>
#include <cmath>
#include <cstdint>
#include <functional>
#include <iostream>
#include <memory>
#include <set>
#include <vector>
using uint8=uint8_t; using int8=int8_t; using uint32=uint32_t;
enum { NPC_CHESS_QUEEN=115395, NPC_CHESS_HORSE=115406, NPC_CHESS_ELEPHANT_DARK=115402,
    NPC_CHESS_ELEPHANT_HOLY=115401, NPC_CHESS_ROOK=115407, NPC_CHESS_KING=115388,
    REACT_PASSIVE=0, EFFECT_MOTION_TYPE=1, UNIT_STATE_CASTING=1 };
static uint32 randomState;
static uint32 urand(uint32 a,uint32 b) { randomState=randomState*1664525u+1013904223u; return a+randomState%(b-a+1); }
struct Position { float x=0,y=0,z=0; void Relocate(float a,float b,float c){x=a;y=b;z=c;} float GetPositionX()const{return x;}float GetPositionY()const{return y;}float GetPositionZ()const{return z;} };
struct Unit { virtual ~Unit()=default; };
struct SpellInfo { uint32 Id; };
struct EventMap {};
struct Spline { uint32 id=0; bool done=true; Position goal; uint32 GetId(){return id;} bool Finalized(){return done;} Position FinalDestination(){return goal;} };
struct Creature;
struct Motion { Creature* c; uint32 jumps=0,id=0; Position destination;
    void MoveJump(Position const&,float,float,uint32); void MoveJump(float,float,float,float,float); };
struct Creature : Unit {
    uint32 entry=NPC_CHESS_ROOK, selfCasts=0; bool alive=true, casting=false; Position p;
    Spline spline; Spline* movespline=&spline; Motion motion{this}; Position home;
    std::vector<Position> tiles; std::vector<std::function<void()>> delayed;
    void setRegeneratingHealth(bool){} void SetReactState(int){} uint32 GetEntry(){return entry;}
    bool IsAlive(){return alive;} bool HasUnitState(uint32){return casting;}
    float GetPositionX(){return p.x;} float GetPositionY(){return p.y;} float GetPositionZ(){return p.z;}
    float GetExactDist(Position const* d){return std::sqrt((p.x-d->x)*(p.x-d->x)+(p.y-d->y)*(p.y-d->y)+(p.z-d->z)*(p.z-d->z));}
    void InterruptNonMeleeSpells(bool,uint32){casting=false;}
    void SetHomePosition(float x,float y,float z,float){home={x,y,z};}
    Motion* GetMotionMaster(){return &motion;}
    Creature* FindNearestCreature(uint32,float,bool){return nullptr;}
    void CastSpell(Unit*,uint32){++selfCasts;}
    void CastSpell(float x,float y,float z,uint32 id,bool triggered=false){
        if(id==229570)assert(triggered);tiles.push_back({x,y,z});}
    template<class F> void AddDelayedEvent(uint32,F f){delayed.emplace_back(std::move(f));}
};
void Motion::MoveJump(Position const& p,float,float,uint32 i){++jumps;id=i;destination=p;c->spline.goal=p;++c->spline.id;c->spline.done=false;}
void Motion::MoveJump(float x,float y,float z,float a,float b){MoveJump({x,y,z},a,b,0);}
struct Scripted_NoMovementAI { Creature* me; explicit Scripted_NoMovementAI(Creature* c):me(c){}
    virtual ~Scripted_NoMovementAI()=default; virtual void Reset(){} virtual void JustDied(Unit*){}
    virtual void MovementInform(uint32,uint32){} virtual void SpellHit(Unit*,SpellInfo const*){}
    virtual void UpdateAI(uint32){}
    void SetCombatMovement(bool){} };
#include "ChessConstants.inc"
#include "ChessAI.inc"
static void require(bool b,char const* text){if(!b){std::cerr<<text<<'\n';std::exit(1);}}
static void pulse(npc_karazhan_chessAI& ai,Creature& c){SpellInfo s{229468};ai.SpellHit(&c,&s);}
static void finish(npc_karazhan_chessAI& ai,Creature& c){c.p=c.motion.destination;c.spline.done=true;ai.MovementInform(EFFECT_MOTION_TYPE,c.motion.id);ai.UpdateAI(1);}
static void hit(npc_karazhan_chessAI& ai,Creature& c){SpellInfo s{229567};ai.SpellHit(&c,&s);}
int main(){
#ifdef COUNTERFACTUAL
    Creature c;c.p={3915.8f,-2236.9f,779};npc_karazhan_chessAI ai(&c);pulse(ai,c);
    require(c.motion.jumps==0&&c.selfCasts==0,"Pinned disabled rook changed");
    std::cout<<"PASS expected-broken rook: native move pulse produces no jump or attack\n";return 0;
#else
    uint32 cases=0;
    for(unsigned seed=0;seed<32;++seed)for(int x=0;x<7;++x)for(int y=0;y<6;++y){
        randomState=seed;Creature c;c.p={3903.55f+x*5.9f,-2254.64f+y*5.9f,779};auto start=c.p;
        npc_karazhan_chessAI ai(&c);pulse(ai,c);require(c.motion.jumps==1,"Rook must choose a finite nonzero legal move");
        auto goal=c.motion.destination;require(goal.x>MIN_X&&goal.x<MAX_X&&goal.y>MIN_Y&&goal.y<MAX_Y,"Destination outside authored board");
        require((goal.x==start.x)!=(goal.y==start.y),"Move must use exactly one cardinal axis");
        require(c.selfCasts==0&&c.tiles.empty(),"No attack before actual arrival");pulse(ai,c);require(c.motion.jumps==1,"Repeated pulse stacked movement");
        ai.MovementInform(EFFECT_MOTION_TYPE,999);require(c.selfCasts==0,"Foreign motion acknowledged");
        finish(ai,c);require(c.selfCasts==1&&c.tiles.empty(),"Arrival must request native cast, not grant hits");
        require(c.home.x==goal.x&&c.home.y==goal.y&&c.home.z==goal.z,"Home must use committed landing pose");
        ai.MovementInform(EFFECT_MOTION_TYPE,229329);require(c.selfCasts==1,"Duplicate arrival cast");
        Unit stranger;SpellInfo s{229567};ai.SpellHit(&stranger,&s);require(c.tiles.empty(),"Foreign cast consumed attack");
        hit(ai,c);auto count=c.tiles.size();require(count>1,"No cardinal tiles");hit(ai,c);require(c.tiles.size()==count,"Duplicate spell hit repeated damage fields");
        std::set<std::pair<int,int>> cells;
        for(auto const& tile:c.tiles){require(tile.x>MIN_X&&tile.x<MAX_X&&tile.y>MIN_Y&&tile.y<MAX_Y,"Ray outside board");
            int dx=int(std::lround((tile.x-goal.x)/5.9f)),dy=int(std::lround((tile.y-goal.y)/5.9f));
            require(dx==0||dy==0,"Non-cardinal tile");require(cells.emplace(dx,dy).second,"Duplicate center/ray tile");require(tile.z==goal.z,"Unexpected tile elevation");}
        for(int dx=-7;dx<=7;++dx)for(int dy=-7;dy<=7;++dy)if(dx==0||dy==0){float tx=goal.x+dx*5.9f,ty=goal.y+dy*5.9f;
            require(bool(cells.count({dx,dy}))==(tx>MIN_X&&tx<MAX_X&&ty>MIN_Y&&ty<MAX_Y),"Missing board-edge tile");}
        ++cases;
    }
    for(unsigned mode=0;mode<6;++mode){Creature c;c.p={3915.8f,-2236.9f,779};npc_karazhan_chessAI ai(&c);pulse(ai,c);
        if(mode==0){ai.MovementInform(EFFECT_MOTION_TYPE,229329);} // Unfinished spline.
        if(mode==1){++c.spline.id;finish(ai,c);} // Replaced spline.
        if(mode==2){c.spline.done=true;c.spline.goal=c.p;ai.MovementInform(EFFECT_MOTION_TYPE,229329);ai.UpdateAI(1);} // Interrupted away from destination.
        if(mode==3){ai.Reset();finish(ai,c);}
        if(mode==4){c.alive=false;finish(ai,c);}
        if(mode==5){ai.JustDied(nullptr);finish(ai,c);}
        hit(ai,c);require(c.selfCasts==0&&c.tiles.empty(),"Cancelled/dead/reset turn attacked");}
    {Creature c;c.p={3915.8f,-2236.9f,779};npc_karazhan_chessAI ai(&c);pulse(ai,c);finish(ai,c);ai.Reset();hit(ai,c);require(c.tiles.empty(),"Late reset cast produced tiles");
        pulse(ai,c);require(c.motion.jumps==2,"Reset blocked retry");finish(ai,c);pulse(ai,c);require(c.motion.jumps==3,"Failed cast blocked next native pulse");}
    {Creature c;c.p={3915.8f,-2236.9f,779};c.alive=false;npc_karazhan_chessAI ai(&c);pulse(ai,c);require(c.motion.jumps==0,"Dead rook moved");}
    {Creature c;c.p={3939.0f,-2236.9f,779};npc_karazhan_chessAI ai(&c);pulse(ai,c);c.spline.done=true;ai.MovementInform(EFFECT_MOTION_TYPE,229329);ai.UpdateAI(1);
        require(!c.selfCasts,"Cross-cell arrival cast from old pose");c.p=c.motion.destination;ai.UpdateAI(1);require(c.selfCasts==1,"Committed cross-cell pose did not release cast");}
    for(auto entry:{NPC_CHESS_HORSE,NPC_CHESS_ELEPHANT_HOLY,NPC_CHESS_ELEPHANT_DARK,NPC_CHESS_QUEEN}){
        Creature c;c.entry=entry;c.p={3915.8f,-2236.9f,779};auto ai=new npc_karazhan_chessAI(&c);pulse(*ai,c);require(c.motion.jumps==1,"Shared figure no longer moves");delete ai;
        auto queued=std::move(c.delayed);for(auto& f:queued)f();require(c.selfCasts==0&&c.tiles.empty(),"Retired shared AI callback ran");}
    std::cout<<"PASS actual complete Chess AI: "<<cases<<" seeded board positions; all cardinal rays exactly once; native arrival/cast admission, interruptions, resets, death, retries and four shared figures\n";
#endif
}
