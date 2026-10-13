#include <cassert>
#include <iostream>
#include "stormheim_fleet_state.h"
int main()
{
    using namespace StormheimFleet;
    State alliance;alliance.alliance=true;
    assert(!alliance.AirKill() && !alliance.Flare(0) && !alliance.Final());
    assert(alliance.BoardGun() && !alliance.BoardGun());
    for(int i=0;i<30;++i){assert(alliance.AirKill());assert(alliance.stage==(i==29?2u:1u));}
    assert(!alliance.AirKill() && !alliance.Captain(3));
    for(uint32_t i=0;i<3;++i){assert(alliance.Captain(i));assert(!alliance.Captain(i));}
    assert(alliance.stage==3 && !alliance.Final());
    for(uint32_t i=0;i<7;++i){assert(alliance.Flare(i));assert(!alliance.Flare(i));}
    assert(alliance.stage==4 && !alliance.Flare(7) && alliance.Final() && !alliance.Final());
    State horde;assert(!horde.BoardGun() && !horde.Captain(0));horde.stage=1;
    for(int i=0;i<30;++i)assert(horde.AirKill());
    assert(horde.WaveDone() && horde.stage==3 && horde.WaveDone() && horde.stage==4 && !horde.WaveDone());
    for(uint32_t i=0;i<7;++i)assert(horde.Flare(i));
    assert(horde.Final() && horde.stage==6 && !horde.Final());
    assert(ShouldYield(100,100,80) && ShouldYield(100,100,1000000000) && !ShouldYield(100,100,79));
    assert(ShouldYield(2000000000,2000000000,1600000000) && !ShouldYield(2000000000,2000000000,1599999999));
    assert(!ShouldYield(0,0,100) && !ShouldYield(100,100,0));
    assert(Bits(0)==0 && Bits(127)==7 && Bits(5)==2);
    State other;other.alliance=true;assert(other.stage==0 && other.air==0 && !other.final);
    std::cout<<"PASS: production action state for all eleven faction stages, 30 distinct air kills, three captain bits, seven flare bits, duplicate/wrong-stage rejection and scaled lethal surrender boundary.\n";
}
