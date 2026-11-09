#include <algorithm>
#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <map>
#include <set>
#include <string>
#include <vector>
using uint8=uint8_t;using uint32=uint32_t;using int32=int32_t;using ObjectGuid=uint64_t;
constexpr uint32 POWER_MANA=0,POWER_ENERGY=3,UNIT_FIELD_POWER=1,UNIT_FIELD_MAX_POWER=2,TYPEID_PLAYER=4,EMOTE_ONESHOT_CHEER=4;
struct Unit;using Creature=Unit;using Player=Unit;
unsigned sentinelReads=0;struct Ref{Player*player=nullptr;Player*getSource(){return player;}};
struct PlayerList{std::vector<Ref>refs;Ref empty;struct Iter{PlayerList*list;Ref*operator->(){if(list->refs.empty()){++sentinelReads;return &list->empty;}return &list->refs.front();}};bool isEmpty()const{return refs.empty();}Iter begin()const{return{const_cast<PlayerList*>(this)};}};
struct Map{using PlayerList=::PlayerList;PlayerList players;PlayerList const&GetPlayers(){return players;}}map;
std::map<ObjectGuid,Player*>players;
struct Unit{bool failCast=false;uint32 type=3,powerType=POWER_MANA,maximum=0,value=0,cheers=0;ObjectGuid id=1;void SetPowerType(uint32 p){powerType=p;}void SetMaxPower(uint32 p,uint32 n){if(p==powerType)maximum=n;}uint32 GetMaxPower(uint32 p){return p==powerType?maximum:0;}void SetPower(uint32 p,uint32 n){if(p==powerType)value=std::min(n,maximum);}uint32 GetPower(uint32 p){return p==powerType?value:0;}void SetInt32Value(uint32 field,int32 n){if(field==UNIT_FIELD_MAX_POWER)maximum=n;else value=n;}void CastSpell(Unit*,uint32 id,bool){if(id==205283&&!failCast)SetPower(POWER_ENERGY,0);}Map*GetMap(){return &map;}uint32 GetTypeId(){return type;}ObjectGuid GetGUID(){return id;}static Player*GetPlayer(Unit&,ObjectGuid id){auto p=players.find(id);return p==players.end()?nullptr:p->second;}void HandleEmoteCommand(uint32){++cheers;}};
namespace Trinity{namespace Containers{template<class T>auto SelectRandomContainerElement(T const&x){return*x.begin();}}}
struct EventLog{std::map<uint32,uint32>scheduled;void ScheduleEvent(uint32 e,uint32 delay){scheduled[e]=delay;}};
struct AnnounceBase{Creature*me;std::set<ObjectGuid>playersGUID;std::set<int32>announceID{1,2,3};std::vector<ObjectGuid>spoken;EventLog events;explicit AnnounceBase(Creature*c):me(c){}void Talk(int32,ObjectGuid target){spoken.push_back(target);}};
#include "EleventhSetups.inc"
void Check(bool v,char const*why){if(!v){std::cerr<<why<<'\n';std::abort();}}
int main(int argc,char**argv){Check(argc==2,"Case required");std::string key=argv[1];unsigned count=0;
if(key!="Announcer"){for(auto const&e:eleventhSetups)if(key==e.key){Unit actor;
#ifdef COUNTERFACTUAL
 e.old(&actor);Check(actor.GetMaxPower(POWER_ENERGY)==0&&actor.GetPower(POWER_ENERGY)==0,"original controller cannot address unselected ENERGY");
#else
 e.fixed(&actor);Check(actor.powerType==POWER_ENERGY&&actor.GetMaxPower(POWER_ENERGY)==100&&actor.GetPower(POWER_ENERGY)==e.start,"native authored starting pool");actor.SetPower(POWER_ENERGY,100);e.fixed(&actor);Check(actor.GetPower(POWER_ENERGY)==e.start,"retry restores authored start");actor.SetPower(POWER_ENERGY,100);actor.failCast=true;e.fixed(&actor);Check(actor.GetPower(POWER_ENERGY)==e.start,"refused reset cast cannot retain full meter");actor.failCast=false;actor.SetPower(POWER_ENERGY,100);Check(actor.GetPower(POWER_ENERGY)==100,"legitimate full-meter input remains representable");
#endif
 ++count;}}
else{
 Unit actor,player;player.type=TYPEID_PLAYER;player.id=20;
#ifdef COUNTERFACTUAL
 OldAnnounce script(&actor);script.playersGUID={10,20};script.Tick();Check(sentinelReads==1,"original empty map iterator dereference");map.players.refs={{&player}};players={{20,&player}};for(unsigned i=0;i<3;++i)script.Tick();Check(script.playersGUID.size()==2&&script.spoken.empty()&&script.events.scheduled[2]==8000,"missing cached first player stalls original intro indefinitely");
#else
 FixedAnnounce script(&actor);script.playersGUID={10,20};script.Tick();Check(!sentinelReads&&script.playersGUID.size()==2&&script.events.scheduled[2]==8000,"empty map waits without consuming roster or progressing");map.players.refs={{&player}};players={{20,&player}};script.Tick();Check(script.spoken==std::vector<ObjectGuid>{20}&&script.playersGUID.empty()&&player.cheers==1,"skip missing GUID then announce only actual roster player once");script.Tick();Check(script.spoken.size()==1&&script.events.scheduled[3]==8000,"existing next-event delay and no duplicate voice");map.players.refs.clear();script.Tick();Check(script.events.scheduled[2]==8000,"map loss after roster completion waits safely");
#endif
 count=1;
}
Check(count>0,"No selected case");std::cout<<"PASS "<<key<<" selected source contexts="<<count<<";actual resource statements or complete intro case/player branch,external endpoints controlled,not whole encounter/client\n";}
