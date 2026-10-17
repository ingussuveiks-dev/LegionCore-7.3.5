// World/cast/transport services remain fixtures; movie state and near-transfer
// ordering below are extracted production methods/branches.
bool _movieMovementHeld=false,m_watching_movie=false,delay=false,pending=false,flight=false,controlled=false,clientControl=true;
uint32 _movieMovementRemaining=0,flags=255,moviePackets=0,transferRequests=0,transfers=0;
WorldLocation m_teleport_dest;uint32 m_teleport_options=0;Unit* mover=this;Unit* view=nullptr;
Unit* GetUnitBeingMoved(){return mover;}Unit* GetViewpoint(){return view;}bool HasUnitState(int){return controlled;}
bool isInFlight(){return flight;}char const* GetName(){return "fixture";}void StopMoving(){}
void RemoveUnitMovementFlag(uint32 mask){flags&=~mask;}void SetClientControl(Unit*,bool value){clientControl=value;}
bool isWatchingMovie(){return m_watching_movie;}void SetCanDelayTeleport(bool value){delay=value;}
bool IsCanDelayTeleport(){return delay;}bool IsHasDelayedTeleport(){return pending;}void SetDelayedTeleportFlag(bool value){pending=value;}
void SetGlobalTeleport(bool){}
bool TeleportTo(uint32,float,float,float,float,uint32=0);bool SafeTeleport(WorldLocation const& dest,uint32 options){return TeleportTo(dest.map,dest.x,dest.y,dest.z,dest.o,options);}
void TickTransfer(){
#include "NativeDelayedUpdate.inc"
}
#include "MovieMethods.inc"
