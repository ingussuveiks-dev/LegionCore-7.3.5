#include <cassert>
#include <cstdint>
#include <iostream>
#include <vector>
using uint32=uint32_t;
constexpr uint32 MINUTE=60,IN_MILLISECONDS=1000,UNIT_STATE_CONTROLLED=7;
constexpr uint32 MOVEMENTFLAG_MASK_MOVING_OR_TURN=0xFF;
#define TC_LOG_DEBUG(...) do {} while(0)
#define TC_LOG_INFO(...) do {} while(0)
namespace WorldPackets { namespace Misc { struct TriggerMovie {uint32 MovieID;int* Write(){return nullptr;}};}}
struct Unit { bool controlled=false;bool HasUnitState(uint32)const{return controlled;} };
struct Player:Unit
{
 bool _movieMovementHeld=false,m_watching_movie=false,world=true,alive=true,teleport=false;
 uint32 _movieMovementRemaining=0,flags=0x1FF;
 bool vehicle=false,transport=false,flight=false,delay=false,clientControl=true;
 bool independentRoot=false,independentGravity=false;
 Unit *mover=this,*view=nullptr;
 int stops=0,packets=0;
 std::vector<bool> orders;
 bool IsInWorld()const{return world;} bool IsAlive()const{return alive;}
 bool IsBeingTeleported()const{return teleport;} bool GetVehicle()const{return vehicle;}
 bool GetTransport()const{return transport;}bool isInFlight()const{return flight;}
 Unit* GetUnitBeingMoved(){return mover;} Unit* GetViewpoint(){return view;}
 bool isWatchingMovie()const{return m_watching_movie;}
 bool IsMovieMovementHeld()const{return _movieMovementHeld;}
 const char* GetName(){return "test";}
 void StopMoving(){++stops;}void RemoveUnitMovementFlag(uint32 mask){flags&=~mask;}
 void SetCanDelayTeleport(bool b){delay=b;}
 void SendDirectMessage(int*){++packets;assert(!_movieMovementHeld||!clientControl);}
 void SetClientControl(Unit* target,bool allowMove)
 {
#include "MovieControl.inc"
  clientControl=allowMove;orders.push_back(allowMove);
 }
#include "MovieMethods.inc"
};
int main()
{
 Player p;p.independentRoot=p.independentGravity=true;
 p.SendMovieStart(636);
 assert(p.IsMovieMovementHeld()&&p.isWatchingMovie()&&p.delay&&!p.clientControl);
 assert(p.flags==0x100&&p.stops==1&&p.packets==1);
 p.SetClientControl(&p,true);assert(!p.clientControl); // Other scripts cannot restore input early.
 p.setWatchinMovie(false);p.SetCanDelayTeleport(false); // Native complete/skip handler.
 assert(!p.IsMovieMovementHeld()&&p.clientControl&&p.stops==2);
 assert(p.independentRoot&&p.independentGravity&&p.flags==0x100);
 auto n=p.orders.size();p.setWatchinMovie(false);assert(p.orders.size()==n);
 p.SendMovieStart(636);p.UpdateMovieMovement(599999);assert(p.IsMovieMovementHeld());
 p.UpdateMovieMovement(1);assert(!p.IsMovieMovementHeld()&&!p.delay&&!p.isWatchingMovie());
 Player overlap;overlap.SendMovieStart(474);overlap.SendMovieStart(474);assert(overlap.packets==1);
 overlap.setWatchinMovie(false);assert(overlap.clientControl);
 for(int i=0;i<8;++i)
 {
  Player q;Unit other;
  if(i==0)q.vehicle=true;if(i==1)q.transport=true;if(i==2)q.flight=true;
  if(i==3)q.mover=&other;if(i==4)q.view=&other;if(i==5)q.teleport=true;
  if(i==6)q.controlled=true;if(i==7)q.alive=false;
  q.SendMovieStart(636);assert(!q.IsMovieMovementHeld()&&q.orders.empty());
 }
 for(int i=0;i<6;++i)
 {
  Player q;Unit other;q.SendMovieStart(636);auto before=q.orders.size();
  if(i==0)q.vehicle=true;if(i==1)q.transport=true;if(i==2)q.flight=true;
  if(i==3)q.mover=&other;if(i==4)q.view=&other;if(i==5)q.world=false;
  q.EndMovieMovement();assert(!q.IsMovieMovementHeld()&&q.orders.size()==before);
 }
 Player rooted;rooted.SendMovieStart(636);rooted.controlled=true;
 rooted.setWatchinMovie(false);assert(!rooted.clientControl); // Preserve new fear/stun control.
 Player transfer;transfer.SendMovieStart(636);transfer.EndMovieMovement();
 assert(transfer.clientControl&&!transfer.IsMovieMovementHeld());
 std::cout<<"PASS: extracted movie start, skip/complete, timeout, control ownership, roots/gravity and excluded movers\n";
}
