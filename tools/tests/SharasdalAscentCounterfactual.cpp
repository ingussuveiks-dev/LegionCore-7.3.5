#include "ReplyHost.inc"
template<class T>
struct NativeWaypoint {
 uint32 _currentNode=5;struct Nodes{uint32 size()const{return 6;}} nodes;Nodes* _path=&nodes;
 void MovementInform(Creature&);
};
#include "NativeWaypointDispatch.inc"
struct AscentRun:Run{
 AscentRun(){actor.db=362063;actor.z=261.755f;actor.motion.path.id=10993810;actor.nearest=&player;}
 void Stage(uint32 value){scenario.step=value;actor.instance.step=value;}
 void Arrive(){NativeWaypoint<Creature>{}.MovementInform(actor);}
};
int main(){
 _set_abort_behavior(0,_CALL_REPORTFAULT);
 {AscentRun r;r.Stage(2);r.Arrive();assert(r.actor.talks==std::vector<uint32>{4}&&r.player.ascentCasts.empty());r.Stage(3);r.Tick(60000);assert(r.player.ascentCasts.empty()&&r.second.ascentCasts.empty());}
 {AscentRun r;r.Stage(3);r.second.gm=true;r.second.record=false;r.actor.nearest=&r.second;r.Arrive();assert(r.actor.casts==std::vector<uint32>{83810}&&r.second.ascentCasts==std::vector<uint32>{208580}&&r.player.ascentCasts.empty()&&!r.player.calls&&!r.second.calls);}
 {AscentRun r;r.Stage(3);r.actor.nearest=nullptr;r.Arrive();assert(r.actor.casts==std::vector<uint32>{83810});r.actor.nearest=&r.player;r.Tick(60000);assert(r.player.ascentCasts.empty());}
 std::cout<<"PASS EXPECTED BROKEN current066dddfa6 complete Erunak and native zero-based waypoint dispatch: real final node5 at native2 is forgotten after later native3; controlled first-grid visitor can be a GM without40341 instead of the eligible caller; absent visitor at final node is never retried. Matching26972 roots/spell effects independently read; actual native Kraliss completion, grid choice/geometry, casts and client scene services remain controlled, no actual client or fabricated50336 grant.\n";
}
