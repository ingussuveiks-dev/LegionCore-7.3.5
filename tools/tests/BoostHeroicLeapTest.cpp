#include <cassert>
#include <cstdint>
#include <cmath>
#include <set>
#include <limits>
#include <iostream>
using uint32=uint32_t;
namespace G3D {
struct Vector3 {
    float x,y,z;
    Vector3(float a=0,float b=0,float c=0):x(a),y(b),z(c){}
    static Vector3 zero(){return {};}
    Vector3 operator+(Vector3 const& b)const{return {x+b.x,y+b.y,z+b.z};}
};
}
struct Position {
    float x=0,y=0,z=0;
    float GetPositionX()const{return x;}float GetPositionY()const{return y;}float GetPositionZ()const{return z;}
    float GetExactDist(Position const* b)const{return std::sqrt((x-b->x)*(x-b->x)+(y-b->y)*(y-b->y)+(z-b->z)*(z-b->z));}
};
using WorldLocation=Position;
struct GameObjectModel {
    bool enabled=true,visible=true,gap=false,wall=false,roof=false,lowObstacle=false;
    bool isCollisionEnabled()const{return enabled;}
    float getHeight(float x,float y,float z,float distance,std::set<uint32>const&,bool)const {
        if(!visible||std::fabs(y)>2||x>15||(gap&&x>4&&x<8))return -std::numeric_limits<float>::infinity();
        // Native GameObjectModel starts its downward ray half a yard higher.
        if(z+.5f<0||z+.5f>distance)return -std::numeric_limits<float>::infinity();
        return 0;
    }
    bool isInLineOfSight(G3D::Vector3 const& a,G3D::Vector3 const& b,std::set<uint32>const&,bool)const {
        if(wall&&((a.x<=6&&b.x>=6)||(a.x>=6&&b.x<=6)))return false;
        if(roof&&((a.z<=1.5f&&b.z>=1.5f)||(a.z>=1.5f&&b.z<=1.5f)))return false;
        if(lowObstacle&&((a.x<=6&&b.x>=6)||(a.x>=6&&b.x<=6))&&std::min(a.z,b.z)<0.55f)return false;
        return true;
    }
};
struct Transport {GameObjectModel* m_model;std::set<uint32> phases{1};auto const& GetPhases()const{return phases;}};
namespace Movement {
constexpr float gravity=19.29110527038574f;
// Mock physics boundary; the linked native floor test uses real computeFallElevation.
float computeFallElevation(float t,bool,float v){return t*(v+t*gravity*.5f);}
}
#include "TransportJumpPath.h"
enum SpellCastResult{SPELL_FAILED_SUCCESS,SPELL_FAILED_DONT_REPORT,SPELL_FAILED_ROOTED,SPELL_FAILED_NOT_READY,SPELL_FAILED_NOPATH};
enum {SPELL_AURA_MOD_ROOT=1,SPELL_AURA_MOD_ROOTED=2,MOVEMENTFLAG_FALLING=1,MOVEMENTFLAG_FALLING_FAR=2,BG_STATUS_IN_PROGRESS=1,STATUS_IN_PROGRESS=1,EFFECT_1=1,SPELL_EFFECT_JUMP_DEST=42,SPELL_WARRIOR_HEROIC_LEAP_JUMP=94954};
using SpellEffIndex=int;
struct Battleground {int status=1;int GetStatus(){return status;}};
struct Map {bool bg=false;float floor=0;bool IsBattlegroundOrArena(){return bg;}bool IsBattleground(){return bg;}int GetDifficultyID(){return 0;}float GetHeight(std::set<uint32>const&,float,float,float,bool){return floor;}};
struct EffectData {int Effect=42,MiscValue=25,MiscValueB=100;};
struct SpellInfo {EffectData effect;EffectData const* GetEffect(int,int)const{return &effect;}};
struct SpellMgr {SpellInfo info;bool missing=false;SpellInfo const* GetSpellInfo(int){return missing?nullptr:&info;}} mgr;
SpellMgr* sSpellMgr=&mgr;
struct Player;
struct Unit {
    Position pos;Transport* ship=nullptr;Map map;Battleground bg;uint32 mapId=1557,area=0;bool root=false,water=false;
    struct Flags{bool fall=false;bool HasMovementFlag(int){return fall;}}m_movementInfo;
    std::set<uint32> phases{1};unsigned casts=0;Position castPos;int castSpell=0;
    Player* ToPlayer();Map* GetMap(){return &map;}bool HasAuraType(int){return root;}bool IsInWater(){return water;}
    void GetPosition(float& x,float& y,float& z){x=pos.x;y=pos.y;z=pos.z;}Position const& GetPosition(){return pos;}
    auto const& GetPhases(){return phases;}float GetPositionZ(){return pos.z;}Transport* GetTransport(){return ship;}
    float GetCollisionHeight(){return 2;}float GetObjectSize(){return .4f;}
    void CastSpell(float x,float y,float z,int spell,bool){++casts;castPos={x,y,z};castSpell=spell;}
};
struct Player:Unit {Battleground* GetBattleground(){return &bg;}uint32 GetMapId(){return mapId;}uint32 GetCurrentAreaID(){return area;}};
Player* Unit::ToPlayer(){return static_cast<Player*>(this);}
struct PathGenerator {
    Player* player;static unsigned terrain;bool result=false;
    explicit PathGenerator(Player* p):player(p){}
    bool CalculatePath(float,float,float){++terrain;return result=!player->ship;}
    int GetPathType(){return result?0:8;}
};
unsigned PathGenerator::terrain=0;constexpr int PATHFIND_NOPATH=8;
struct Check {
    Player* unit;WorldLocation* destination;
    Unit* GetCaster(){return unit;}WorldLocation const* GetExplTargetDest(){return destination;}
#include "Check.inc"
#include "Cast.inc"
};
int main() {
    GameObjectModel model;Transport ship{&model};Player player;player.ship=&ship;WorldLocation dest{12,0,0};Check check{&player,&dest};
#ifdef LEAP_BASELINE
    assert(check.CheckElevation()==SPELL_FAILED_NOPATH&&PathGenerator::terrain==1);
    std::cout<<"PASS pinned a9da566d counterfactual: actual Leap admission rejects a ship destination at mocked terrain-nav boundary.\n";
#else
    assert(check.CheckElevation()==SPELL_FAILED_SUCCESS&&!PathGenerator::terrain);
    model.gap=true;assert(check.CheckElevation()==SPELL_FAILED_SUCCESS);model.gap=false; // Leaps need no walkable floor midair.
    model.lowObstacle=true;assert(check.CheckElevation()==SPELL_FAILED_SUCCESS);model.lowObstacle=false;
    model.wall=true;assert(check.CheckElevation()==SPELL_FAILED_NOPATH);model.wall=false;
    model.roof=true;assert(check.CheckElevation()==SPELL_FAILED_NOPATH);model.roof=false;
    model.enabled=false;assert(check.CheckElevation()==SPELL_FAILED_NOPATH);model.enabled=true;
    model.visible=false;assert(check.CheckElevation()==SPELL_FAILED_NOPATH);model.visible=true; // old terrain fallback also refuses.
    dest.y=2;assert(check.CheckElevation()==SPELL_FAILED_SUCCESS); // Center supported at the edge; radius may overhang.
    dest.y=2.01f;assert(check.CheckElevation()==SPELL_FAILED_NOPATH);dest.y=0;
    dest.z=.3f;assert(check.CheckElevation()==SPELL_FAILED_SUCCESS); // Native ray must search below the selected point too.
    dest.z=-.3f;assert(check.CheckElevation()==SPELL_FAILED_SUCCESS);dest.z=0;
    dest.x=100;assert(check.CheckElevation()==SPELL_FAILED_NOPATH);dest.x=12;
    dest.z=5;assert(check.CheckElevation()==SPELL_FAILED_NOPATH);dest.z=0;
    dest.x=std::numeric_limits<float>::quiet_NaN();assert(check.CheckElevation()==SPELL_FAILED_NOPATH);dest.x=12;
    mgr.missing=true;assert(check.CheckElevation()==SPELL_FAILED_NOPATH);mgr.missing=false;
    mgr.info.effect.Effect=3;assert(check.CheckElevation()==SPELL_FAILED_NOPATH);mgr.info.effect.Effect=42;
    player.root=true;assert(check.CheckElevation()==SPELL_FAILED_ROOTED);player.root=false;
    check.destination=nullptr;assert(check.CheckElevation()==SPELL_FAILED_DONT_REPORT);check.destination=&dest;
    player.water=true;assert(check.CheckElevation()==SPELL_FAILED_SUCCESS);player.water=false;
    player.m_movementInfo.fall=true;player.pos.z=10;assert(check.CheckElevation()==SPELL_FAILED_SUCCESS);player.pos.z=0;player.m_movementInfo.fall=false;
    player.ship=nullptr;assert(check.CheckElevation()==SPELL_FAILED_SUCCESS);
    player.map.bg=true;player.bg.status=0;assert(check.CheckElevation()==SPELL_FAILED_NOT_READY);player.bg.status=1;
    player.mapId=489;dest.z=400;assert(check.CheckElevation()==SPELL_FAILED_NOPATH);dest.z=0;player.map.bg=false;
    player.ship=&ship;assert(check.CheckElevation()==SPELL_FAILED_SUCCESS);check.HandleDummy(0);
    assert(player.casts==1&&player.castSpell==94954&&player.castPos.x==12);
    std::cout<<"PASS actual Leap admission/cast and collision helper: deck landing, gap/low obstacle, wall/roof/edge, unavailable data, water/falling/root/BG/ordinary terrain and native94954 dispatch. World/model boundaries mocked; real ship models tested separately.\n";
#endif
}
