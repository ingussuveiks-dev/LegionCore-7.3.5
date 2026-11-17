#include <cassert>
#include <cstdint>
#include <cstdlib>
#include <iostream>
using uint32=uint32_t;
enum {TEXT_RANGE_PERSONAL=5,TEXT_RANGE_NORMAL=0};
struct Session {bool loading=false,logout=false;bool PlayerLoading(){return loading;}bool PlayerLogout(){return logout;}};
struct Player;
struct WorldObject {bool world=true;uint32 map=1514;virtual ~WorldObject()=default;virtual Player const* ToPlayer()const{return nullptr;}
 bool IsInMap(WorldObject const* o)const{return o&&world&&o->world&&map==o->map;}};
struct Player:WorldObject {Session session;bool visible=true;uint32 packets=0;
 Player const* ToPlayer()const override{return this;}Session* GetSession(){return &session;}
 bool canSeeOrDetect(WorldObject const*)const{return visible;}void SendDirectMessage(void const*){++packets;}};
Player* recipient=nullptr;
namespace ObjectAccessor {Player* FindPlayer(uint32 guid){return guid==42?recipient:nullptr;}}
struct Localizer {void operator()(Player const* p){auto s=const_cast<Player*>(p)->GetSession();
 if(p->world&&!s->loading&&!s->logout)++const_cast<Player*>(p)->packets;}};
void chat(WorldObject* source,WorldObject const* whisperTarget,int range){
 if(!source)return;Localizer localizer;
 #include "ChatPersonal.inc"
 assert(range!=TEXT_RANGE_PERSONAL); // ordinary range continues normally
}
void sound(WorldObject* source,uint32 whisperGuid,int range){
 void const* data=nullptr;
 #include "SoundPersonal.inc"
 assert(range!=TEXT_RANGE_PERSONAL);
}
int main(){_set_abort_behavior(0,_CALL_REPORTFAULT);uint32 cases=0;
 for(int mode=0;mode<9;++mode){WorldObject npc,other;Player p;recipient=&p;
  WorldObject* source=&npc;WorldObject const* target=&p;uint32 guid=42;
  if(mode==1)p.map=1;if(mode==2)p.visible=false;if(mode==3)p.session.loading=true;
  if(mode==4)p.session.logout=true;if(mode==5)p.world=false;if(mode==6){target=nullptr;guid=0;}
  if(mode==7){target=&other;guid=0;}if(mode==8)source=nullptr;
  chat(source,target,TEXT_RANGE_PERSONAL);sound(source,guid,TEXT_RANGE_PERSONAL);
  assert(p.packets==(mode==0?2u:0u));++cases;
 }
 WorldObject npc;Player p;recipient=&p;chat(&npc,&p,TEXT_RANGE_NORMAL);sound(&npc,42,TEXT_RANGE_NORMAL);assert(!p.packets);
 std::cout<<"PASS "<<cases<<" actual personal chat/sound routing cases, ordinary range continuation and unchanged SAY presentation. Session/packet services controlled.\n";
}
