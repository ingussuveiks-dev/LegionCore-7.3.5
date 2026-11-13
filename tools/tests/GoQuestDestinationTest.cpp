// Complete production handler; quest/terrain/teleport services are controlled.
#include <algorithm>
#include <cmath>
#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <map>
#include <stdexcept>
#include <string>
#include <vector>
#include "../../src/server/game/Chat/CommandIntegerArgument.h"
using uint32=uint32_t;
constexpr uint32 LANG_COMMAND_QUEST_NOTFOUND=1, LANG_INVALID_TARGET_COORD=2;
constexpr float MAX_HEIGHT=100000.0f;
struct Point {float X,Y;};
struct POI {uint32 MapID;std::vector<Point> points;};
using QuestPOIVector=std::vector<POI>;
struct QuestStore {
 bool exists=true, hasData=true;QuestPOIVector data;
 void* GetQuestTemplate(uint32){return exists?this:nullptr;}
 QuestPOIVector const* GetQuestPOIVector(uint32){return hasData?&data:nullptr;}
} questStore;
auto sQuestDataStore=&questStore;
struct ObjectStore {bool IsTransportMap(uint32 id){return id==99;}} objectStore;
auto sObjectMgr=&objectStore;
struct Map {
 float floor=10, water=5;
 float GetHeight(float,float,float)const{return floor;}
 float GetWaterLevel(float,float)const{return water;}
};
struct MapStore {std::map<uint32,Map> maps;Map* CreateBaseMap(uint32 id){auto i=maps.find(id);return i==maps.end()?nullptr:&i->second;}} mapStore;
auto sMapMgr=&mapStore;
struct MapManager {
 static bool IsValidMapCoord(uint32 id,float x,float y){return id<100&&std::isfinite(x)&&std::isfinite(y)&&std::abs(x)<17067&&std::abs(y)<17067;}
 static bool IsValidMapCoord(uint32 id,float x,float y,float z){return IsValidMapCoord(id,x,y)&&std::isfinite(z)&&std::abs(z)<17067;}
};
struct Motion {uint32 expired=0;void MovementExpired(){++expired;}};
struct Player {
 bool flying=false, accepted=true;uint32 recalls=0,cleanups=0,transfers=0,map=0;float x=0,y=0,z=0;Motion motion;
 bool isInFlight(){return flying;}
 Motion* GetMotionMaster(){return &motion;}
 void CleanupAfterTaxiFlight(){++cleanups;}
 void SaveRecallPosition(){++recalls;}
 bool TeleportTo(uint32 id,float a,float b,float c,float o){if(o!=0)throw std::runtime_error("orientation changed");++transfers;map=id;x=a;y=b;z=c;return accepted;}
};
struct Session {Player player;Player* GetPlayer(){return &player;}};
struct ChatHandler {
 Session session;bool error=false;uint32 messages=0;
 Session* GetSession(){return &session;}
 char* extractKeyFromLink(char* text,char const* type){if(std::string(type)!="Hquest")throw std::runtime_error("wrong link type");return text;}
 template<class...T>void PSendSysMessage(T...){++messages;}
 void SetSentErrorMessage(bool b){error=b;}
};
struct Selected {
#include "GoQuestCommand.inc"
};
static void Require(bool value,char const* text){if(!value)throw std::runtime_error(text);}
static bool Run(ChatHandler& h,char const* text="42"){std::string args=text;return Selected::HandleGoQuestCommand(&h,args.c_str());}
int main(){try{
 mapStore.maps={{1,{10,5}},{2,{20,25}},{3,{NAN,NAN}},{4,{-100000,-100000}}};
#ifdef GO_QUEST_BASELINE
 questStore.data={{1,{{7,8}}},{2,{}}};ChatHandler h;
 Require(Run(h)&&h.session.player.map==2&&h.session.player.z==10,"Pinned mixed-map counterfactual changed");
 std::cout<<"COUNTERFACTUAL pinned handler mixes prior map1 coordinates/height with trailing empty map2;not repaired success\n";
#else
 uint32 cases=0;
 for(bool flying:{false,true}){
  for(auto const& entries:std::vector<QuestPOIVector>{{},{{1,{}}},{{1,{}},{2,{}}},{{77,{{1,2}}}},{{99,{{1,2}}}},{{1,{{NAN,2}}}},{{1,{{20000,2}}}},{{3,{{1,2}}}},{{4,{{1,2}}}}}){
   questStore.data=entries;ChatHandler h;h.session.player.flying=flying;
   Require(!Run(h)&&h.error&&!h.session.player.transfers&&!h.session.player.recalls&&!h.session.player.cleanups&&!h.session.player.motion.expired,"Missing/invalid POI mutated travel state");++cases;
  }
  for(auto const& entries:std::vector<QuestPOIVector>{{{1,{{7,8}}}},{{1,{{7,8}}},{2,{}}},{{2,{}},{1,{{7,8}}}},{{1,{{7,8},{9,10}}},{77,{{11,12}}}}}){
   questStore.data=entries;ChatHandler h;h.session.player.flying=flying;
   Require(Run(h)&&h.session.player.map==1&&h.session.player.x==7&&h.session.player.y==8&&h.session.player.z==10,"Last usable POI pose lost or mixed");
   Require(h.session.player.transfers==1&&h.session.player.recalls==uint32(!flying)&&h.session.player.cleanups==uint32(flying)&&h.session.player.motion.expired==uint32(flying),"Native travel preparation order changed");++cases;
  }
 }
 questStore.data={{1,{{7,8}}},{2,{{17,18}}}};ChatHandler h;
 Require(Run(h)&&h.session.player.map==2&&h.session.player.x==17&&h.session.player.y==18&&h.session.player.z==25,"Last valid POI/water policy changed");++cases;
 for(char const* args:{""," ","0","-1","42x","4294967296","999999999999999999999"}){ChatHandler bad;Require(!Run(bad,args)&&!bad.session.player.transfers&&!bad.session.player.recalls,"Invalid ID admitted");++cases;}
 for(bool absent:{false,true}){questStore.exists=!absent;questStore.hasData=absent;ChatHandler missing;Require(!Run(missing)&&missing.error&&!missing.session.player.transfers,"Missing quest/data admitted");++cases;}
 questStore.exists=questStore.hasData=true;ChatHandler rejected;rejected.session.player.accepted=false;Require(!Run(rejected)&&rejected.session.player.transfers==1,"Rejected native teleport reported success");++cases;
 std::cout<<"PASS "<<cases<<" complete production go-quest cases;coherent last usable POI,invalid/empty/missing terrain,ID errors,water,flying and teleport rejection. Services controlled;not live GM travel/client proof\n";
#endif
 return 0;
}catch(std::exception const& e){std::cerr<<e.what()<<'\n';return 1;}}
