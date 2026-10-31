#include "Host.inc"
struct Run{
 WorldObject creator,peer;GroupReference second{&peer,nullptr},first{&creator,&second};Group group{&first};Conversation* value=nullptr;uint32 created,budget;
 explicit Run(uint32 start,uint32 duration,bool npc=true,bool grouped=false):created(start),budget(duration){
  live.clear();store={};store.identity=100;store.actors.push_back({100,0,1,2,3,4,5,6,uint32(duration-5000)});store.lines.push_back({100,1,0,0,60,0});currentTime=start;
  creator.player=!npc;creator.guid=npc?99:42;peer.guid=43;if(grouped)creator.group=&group;value=new Conversation;assert(value->CreateConversation(1,100,&creator,nullptr,{}));assert(value->deadline==duration&&value->lifetime==duration);
 }
 ~Run(){live.clear();}
 void At(uint32 elapsed){currentTime=created+elapsed;}
 bool Hidden(uint32 id=42,bool player=true){WorldObject viewer;viewer.guid=id;viewer.player=player;return value->CanNeverSee2(&viewer);}
 void Update(){value->Update(0);}
};
int main(){_set_abort_behavior(0,_CALL_REPORTFAULT);unsigned cases=0;
#ifdef BASELINE
 for(uint32 budget:{4711u,6000u,28788u,47816u,73000u}){Run r(100,budget);assert(!r.Hidden());r.At(budget+1);r.Update();assert(!r.value->removed&&r.Hidden());r.At(10*budget);r.Update();assert(!r.value->removed&&!r.Hidden(43));assert(r.value->playing.count(99)==0);++cases;}
 std::cout<<"PASS COUNTERFACTUAL9a38534f0 "<<cases<<" native payload/budget/visibility/update contexts: NPC cannot gain a Player caster key, persists after viewers expire, and admits an unrelated late fresh playback indefinitely. Expected broken object lifetime, no actual client or runtime leak count claim.\n";
#else
 for(uint32 start:{0u,100u,0xfffffff0u})for(uint32 budget:{4711u,6000u,28788u,47816u,73000u})for(uint32 mode=0;mode<5;++mode){Run r(start,budget);assert(r.value->_npcCaster&&r.value->_createdAt==start&&r.value->personal.empty());uint32 last=0;
  if(mode==1)assert(!r.Hidden());
  if(mode==2){last=budget/2;r.At(last);assert(!r.Hidden());}
  if(mode==3){last=budget;r.At(last);assert(!r.Hidden());}
  if(mode==4){for(uint32 i=0;i<8;++i){last=i*budget/7;r.At(last);assert(!r.Hidden(42+i));}}
  r.At(budget);r.Update();assert(!r.value->removed);r.At(budget+1);auto count=r.value->playing.size();assert(r.Hidden(1000)&&r.value->playing.size()==count);r.Update();
  if(mode<2)assert(r.value->removed);
  else{assert(!r.value->removed);r.At(last+budget);assert(!r.Hidden(mode==4?49:42));r.Update();assert(!r.value->removed);r.At(last+budget+1);assert(r.Hidden(mode==4?49:42));r.Update();assert(r.value->removed);}
  assert(!r.value->playing.count(99));++cases;
 }
 {Run r(10,6000);assert(!r.Hidden());auto first=r.value->playing.at(42);for(uint32 at:{1u,3000u,5999u,6000u}){r.At(at);assert(!r.Hidden()&&r.value->playing.at(42)==first);}r.At(6001);assert(r.Hidden()&&r.Hidden(43));r.Update();assert(r.value->removed);++cases;}
 {Run r(10,6000);for(uint32 at:{0u,3000u,6000u,6001u}){r.At(at);assert(r.Hidden(99,false)&&r.value->playing.empty());}r.Update();assert(r.value->removed);++cases;}
 for(bool npc:{false,true})for(uint32 age:{0u,1u,6000u}){Run r(100,6000,npc);r.At(age);r.value->questExpired=true;r.Update();assert(r.value->removed);++cases;}
 {Run r(100,6000,false);assert(!r.value->_npcCaster&&r.value->personal==std::set<uint32>{42});r.At(100000);r.Update();assert(!r.value->removed);assert(!r.Hidden());r.At(106000);r.Update();assert(!r.value->removed);r.At(106001);r.Update();assert(r.value->removed);++cases;}
 {Run r(100,6000,false,true);assert(r.value->personal==std::set<uint32>({42,43})&&!r.Hidden(43));r.At(10000);r.Update();assert(!r.value->removed);assert(!r.Hidden());r.At(16000);r.Update();assert(!r.value->removed);r.At(16001);r.Update();assert(r.value->removed);++cases;}
 for(bool npc:{false,true}){Run r(100,6000,npc);r.At(100000);r.Update();assert(r.value->removed==npc);++cases;}
 for(bool storedNpc:{false,true}){Run r(100,6000);r.value->lifetime=0;r.value->_npcCaster=storedNpc;r.value->playing.clear();r.At(100000);assert(!r.Hidden());r.At(106001);assert(r.Hidden());r.Update();assert(!r.value->removed);++cases;}
 {Run r(100,35000);store.actors[0].duration=0;Conversation fallback;fallback.entry=100;assert(fallback.Start(&r.creator)&&fallback.deadline==35000&&fallback._npcCaster);++cases;}
 {Run r(100,6000);store.lines.clear();Conversation missing;missing.entry=100;assert(!missing.Start(&r.creator)&&!missing._npcCaster&&missing._createdAt==0);++cases;}
 {Run r(100,6000);Conversation missing;assert(!missing.CreateConversation(1,100,nullptr,nullptr,{})&&!missing._npcCaster);++cases;}
 {Run r(100,6000);assert(r.value->actors.at(0).actorId==1&&r.value->actors.at(0).creatureId==2&&r.value->actors.at(0).displayId==3&&r.value->actors.at(0).unk1==4&&r.value->actors.at(0).unk2==5&&r.value->actors.at(0).unk3==6&&r.value->lines.at(0).id==1&&r.value->lines.at(0).unk1==60);r.At(6001);r.Update();assert(r.value->removed&&r.value->actors.size()==1&&r.value->lines.size()==1);++cases;}
 std::cout<<"PASS "<<cases<<" actual native generation/budget/player visibility/Update/CanNeverSee2 ASAN contexts: bounded NPC native creation window, full admitted late/boundary/multiple viewer clocks, no refreshing or post-window insertion, unseen/grid gap cleanup, uint32 uptime wrap and observed4711 wrap budget, nonplayer rejection, quest scope, unchanged player creator/unseen/group-first clock and explicit zero/static profile, fallback/missing factory/payload. Map/remove/registry/clock/definitions controlled; no actual visibility packets/client voice/full chain claim.\n";
#endif
}
