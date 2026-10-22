#include <cassert>
#include <cmath>
#include <cstdint>
#include <iostream>
#define TC_LOG_ERROR(...) ((void)0)
using uint32=uint32_t;
struct Position
{
    float x,y,z,o;
    float GetPositionX()const{return x;} float GetPositionY()const{return y;}
    float GetPositionZ()const{return z;} float GetOrientation()const{return o;}
    void Relocate(Position const& other){*this=other;}
};
struct Player;
struct Transport
{
    int id,added=0,removed=0;
    Position pose{100,200,300,0.6f};
    int GetGUID()const{return id;}
    void AddPassenger(Player*);
    void RemovePassenger(Player*);
    void CalculatePassengerPosition(float& x,float& y,float& z,float* o)
    {
        float oldX=x,oldY=y;
        x=pose.x+std::cos(pose.o)*oldX-std::sin(pose.o)*oldY;
        y=pose.y+std::sin(pose.o)*oldX+std::cos(pose.o)*oldY;
        z+=pose.z;*o+=pose.o;
    }
};
struct Player
{
    bool inWorld=false;Transport* transport=nullptr;
    Position position{0,0,-500,0};int teleports=0,relocations=0,fallResets=0;
    struct {struct {int Guid=0;Position Pos{0,0,0,0};int VehicleSeatIndex=0;} transport;} m_movementInfo;
    Transport* GetTransport(){return transport;} bool IsInWorld(){return inWorld;}
    void Relocate(float x,float y,float z,float o){assert(!inWorld);position={x,y,z,o};++relocations;}
    void SetFallInformation(int,float z){assert(z==position.z);++fallResets;}
    void NearTeleportTo(float x,float y,float z,float o){assert(inWorld);position={x,y,z,o};++teleports;}
};
void Transport::AddPassenger(Player* p){p->transport=this;++added;}
void Transport::RemovePassenger(Player* p){p->transport=nullptr;++removed;}
struct Instance {int GetId(){return 1557;}int GetInstanceId(){return 1;}};
struct ArrivalScript
{
    Transport* ship=nullptr;Position deck;uint32 _setupTimer=0;Instance data;Instance* instance=&data;
    Transport* GetGunship(){return ship;} Position const& Deck(){return deck;}int TransportEntry(){return 246607;}
#include "PutPlayerOnTransport.inc"
#include "OnPlayerEnter.inc"
};
int main()
{
    for(auto deck:{Position{11.1372f,-0.363736f,20.6586f,2.937f},Position{0.720768f,1.68496f,34.501f,6.2787f}})
        for(int mode=0;mode<3;++mode)
        {
            Transport ship{1},old{2};Player player;ArrivalScript script;script.ship=&ship;script.deck=deck;
            if(mode==1)player.transport=&ship;
            if(mode==2)player.transport=&old;
            script.OnPlayerEnter(&player);
            assert(player.GetTransport()==&ship&&player.relocations==1&&!player.teleports);
            assert(player.m_movementInfo.transport.Guid==1&&player.m_movementInfo.transport.VehicleSeatIndex==-1);
            assert(player.m_movementInfo.transport.Pos.z==deck.z&&player.fallResets==1&&script._setupTimer==100);
            assert((mode!=2||old.removed==1)&&(mode!=1||ship.added==0));
            float x=deck.x,y=deck.y,z=deck.z,o=deck.o;ship.CalculatePassengerPosition(x,y,z,&o);
            assert(std::fabs(player.position.x-x)<0.001f&&std::fabs(player.position.z-z)<0.001f);
            // Existing in-world recovery still uses a real near teleport.
            player.inWorld=true;script.PutPlayerOnTransport(&player,&ship);assert(player.teleports==1);
        }
    ArrivalScript missing;missing.deck={0,0,34,0};Player p;missing.OnPlayerEnter(&p);
    assert(!p.GetTransport()&&!p.relocations&&missing._setupTimer==100);
    std::cout<<"PASS: both factions attach/transform before self-create, stale offsets/old ships, existing passenger, missing transport and in-world recovery\n";
}
