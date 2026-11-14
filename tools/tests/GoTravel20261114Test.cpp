// Complete actual travel handlers;world data/terrain/SQL/movement are controlled.
#include <algorithm>
#include <cmath>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <map>
#include <memory>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>
#include "../../src/server/game/Chat/CommandIntegerArgument.h"
#include "../../src/server/game/Chat/CommandFloatArgument.h"
using uint8=uint8_t;using uint16=uint16_t;using uint32=uint32_t;using uint64=uint64_t;using int32=int32_t;
static void Require(bool value,char const*message){if(!value)throw std::runtime_error(message);}
#include "TravelConstants.inc"
#include "NativeCoordinates.inc"
struct Vec{float X=7,Y=8,Z=9;};
struct MapEntry{bool scenario=false;bool IsScenario()const{return scenario;}};
template<class T>struct Store{std::map<uint32,T>rows;T const*LookupEntry(uint32 id){auto it=rows.find(id);return it==rows.end()?nullptr:&it->second;}};
Store<MapEntry>sMapStore;
struct Map{uint32 id=1;float floor=10,water=5;bool instance=false;float GetHeight(float,float,float)const{return floor;}float GetWaterLevel(float,float)const{return water;}bool Instanceable()const{return instance;}uint32 GetId()const{return id;}char const*GetMapName()const{return "Controlled map";}};
struct MapService{std::map<uint32,Map>maps;Map*CreateBaseMap(uint32 id){auto it=maps.find(id);return it==maps.end()?nullptr:&it->second;}}mapService;auto sMapMgr=&mapService;
struct MapManager{static bool IsValidMapCoord(uint32 id,float x,float y){return sMapStore.LookupEntry(id)&&Trinity::IsValidMapCoord(x,y);}static bool IsValidMapCoord(uint32 id,float x,float y,float z){return sMapStore.LookupEntry(id)&&Trinity::IsValidMapCoord(x,y,z);}static bool IsValidMapCoord(uint32 id,float x,float y,float z,float o){return sMapStore.LookupEntry(id)&&Trinity::IsValidMapCoord(x,y,z,o);}};
struct Motion{uint32 expired=0;void MovementExpired(){++expired;}};
struct Player{uint32 map=1,zone=1,recalls=0,cleanups=0,transfers=0,difficulty=0;float x=1,y=2,z=100,orientation=.5f;bool flying=false,accepted=true;Motion motion;uint32 GetMapId(){return map;}uint32 GetZoneId(){return zone;}float GetPositionX(){return x;}float GetPositionY(){return y;}float GetPositionZ(){return z;}float GetOrientation(){return orientation;}bool isInFlight(){return flying;}Motion*GetMotionMaster(){return &motion;}void CleanupAfterTaxiFlight(){++cleanups;}void SaveRecallPosition(){++recalls;}bool TeleportTo(uint32 id,float a,float b,float c,float o){Require(MapManager::IsValidMapCoord(id,a,b,c,o),"Invalid pose reached teleport");++transfers;map=id;x=a;y=b;z=c;orientation=o;return accepted;}void SetDungeonDifficultyID(uint32 id){++difficulty;Require(id==DIFFICULTY_N_SCENARIO||id==DIFFICULTY_HC_SCENARIO,"Wrong scenario difficulty");}};
struct Session{Player player;Player*GetPlayer(){return &player;}};
struct ChatHandler{Session session;uint32 messages=0;bool error=false;Session*GetSession(){return &session;}char*extractKeyFromLink(char*args,char const*){return args?strtok(args," "):nullptr;}template<class...T>void PSendSysMessage(T...){++messages;}template<class...T>void SendSysMessage(T...){++messages;}void SetSentErrorMessage(bool value){error=value;}};
struct WorldSafeLocsEntry{uint32 MapID=1;Vec Loc;};Store<WorldSafeLocsEntry>sWorldSafeLocsStore;
struct TaxiNodesEntry{uint32 ContinentID=2;Vec Pos;};Store<TaxiNodesEntry>sTaxiNodesStore;
struct AreaTriggerEntry{uint32 ContinentID=1;Vec Pos;};Store<AreaTriggerEntry>sAreaTriggerStore;
struct Name{char const*Str[1]={"Controlled zone"};}zoneName;
struct AreaTableEntry{uint32 ID=1,ParentAreaID=0,ContinentID=1;Name*ZoneName=&zoneName;};Store<AreaTableEntry>sAreaTableStore;
struct DB2Manager{bool normal=true,heroic=true;uint32 conversions=0;bool GetMapDifficultyData(uint32,uint32 difficulty){return difficulty==DIFFICULTY_N_SCENARIO?normal:heroic;}void Zone2MapCoordinates(float&x,float&y,uint32 id){Require(id==1,"Parent zone not resolved");++conversions;x*=2;y*=3;}}sDB2Manager;
struct GameObjectData{float posX=20,posY=21,posZ=22,orientation=.25f;uint32 mapid=1;};
struct ObjectManager{std::map<uint64,GameObjectData>objects;uint64 requested=0;GameObjectData const*GetGOData(uint64 id){requested=id;auto it=objects.find(id);return it==objects.end()?nullptr:&it->second;}uint32 GetDBCLocaleIndex(){return 0;}}objectManager;auto sObjectMgr=&objectManager;
enum class HighGuid{Creature};
struct ObjectGuid{using LowType=uint64;uint32 map=0,entry=0;uint64 low=0;template<HighGuid>static ObjectGuid Create(uint32 map,uint32 entry,uint64 low){return {map,entry,low};}};
struct Creature{float x=50,y=51,z=52,o=.75f;float GetPositionX(){return x;}float GetPositionY(){return y;}float GetPositionZ(){return z;}float GetOrientation(){return o;}};
struct Accessor{Creature*live=nullptr;ObjectGuid requested;Creature*GetCreature(Player&,ObjectGuid guid){requested=guid;return live;}}accessor;auto sObjectAccessor=&accessor;
struct Field{double number=0;uint64 integer=0;float GetFloat(){return float(number);}uint16 GetUInt16(){return uint16(integer);}uint32 GetUInt32(){return uint32(integer);}uint64 GetUInt64(){return integer;}};
struct QueryRow{std::vector<Field>fields{{30,0},{31,0},{32,0},{.6,0},{0,1},{0,7},{0,99}};uint32 count=1;uint32 GetRowCount(){return count;}Field*Fetch(){return fields.data();}};
using QueryResult=std::shared_ptr<QueryRow>;
struct WorldService{bool exists=true;std::string where;QueryRow row;uint32 queries=0;QueryResult PQuery(char const*query,char const*condition){Require(std::string(query).find("SELECT position_x")==0,"Unexpected production query");++queries;where=condition;return exists?std::make_shared<QueryRow>(row):nullptr;}void EscapeString(std::string&value){size_t pos=0;while((pos=value.find('\'',pos))!=std::string::npos){value.insert(pos,"\\");pos+=2;}}}WorldDatabase;
struct GmTicket{uint32 calls=0;void TeleportTo(Player*player){++calls;player->TeleportTo(1,70,71,72,0);}};
struct TicketService{GmTicket ticket;uint32 requested=0;bool exists=true;GmTicket*GetTicket(uint32 id){requested=id;return exists&&id==7?&ticket:nullptr;}}ticketService;auto sTicketMgr=&ticketService;
#include "TravelSelected.inc"
static uint32 cases=0;
template<class Fn>static bool Command(Fn fn,ChatHandler&handler,char const*args){std::string text=args;return fn(&handler,text.c_str());}
static void Reset(){sMapStore.rows={{1,{}},{2,{}}};mapService.maps={{1,{1,10,5,false}},{2,{2,20,25,false}}};sWorldSafeLocsStore.rows={{7,{1,{7,8,9}}}};sTaxiNodesStore.rows={{7,{2,{5,6,7}}}};sAreaTriggerStore.rows={{7,{1,{10,11,12}}}};sAreaTableStore.rows={{1,{1,0,1,&zoneName}},{2,{2,1,2,&zoneName}},{3,{3,999,1,&zoneName}}};sDB2Manager={};objectManager={};objectManager.objects={{7,{}},{(uint64(1)<<32)+7,{40,41,42,.4f,2}}};accessor={};WorldDatabase={};ticketService={};}
static bool Pristine(ChatHandler&h){auto&p=h.session.player;return !p.transfers&&!p.recalls&&!p.cleanups&&!p.motion.expired&&!p.difficulty;}
static void Rejects(bool(*fn)(ChatHandler*,char const*),std::vector<char const*>args){for(auto arg:args)for(bool flight:{false,true}){Reset();ChatHandler h;h.session.player.flying=flight;Require(!Command(fn,h,arg)&&Pristine(h),"Invalid travel arguments changed travel state");++cases;}}
static void Admission(bool(*fn)(ChatHandler*,char const*),char const*arg){for(bool flight:{false,true})for(bool accepted:{false,true}){Reset();ChatHandler h;h.session.player.flying=flight;h.session.player.accepted=accepted;Require(Command(fn,h,arg)==accepted,"Native teleport result not returned");auto&p=h.session.player;Require(p.transfers==1&&p.recalls==uint32(!flight)&&p.cleanups==uint32(flight)&&p.motion.expired==uint32(flight),"Native flying/recall/teleport flow changed");++cases;}}
int main(){try{
#ifdef GO_OBJECT_BASELINE
 Reset();ChatHandler h;Require(Command(CaseGoObject::Selected::HandleGoObjectCommand,h,"4294967303")&&objectManager.requested==7&&h.session.player.x==20,"Original GO GUID narrowing not reproduced");std::cout<<"COUNTERFACTUAL actual original go-object aliases64-bit spawn4294967303 to7;controlled store/teleport only\n";return 0;
#endif
#ifdef REPAIR_GOCREATURE
 Rejects(CaseGoCreature::Selected::HandleGoCreatureCommand,{""," ","id","id -1","id 99x","id 4294967395","-1","7x","18446744073709551616"});
 Admission(CaseGoCreature::Selected::HandleGoCreatureCommand,"4294967303");
 {Reset();ChatHandler h;WorldDatabase.row.fields[5].integer=(uint64(1)<<32)+7;Creature live;accessor.live=&live;Require(Command(CaseGoCreature::Selected::HandleGoCreatureCommand,h,"4294967303")&&WorldDatabase.where=="WHERE guid = '4294967303'"&&accessor.requested.low==(uint64(1)<<32)+7&&h.session.player.x==50,"Live creature/native GUID lookup lost width");++cases;}
 for(char const*arg:{"id 99","Some'Name","0"}){Reset();ChatHandler h;Require(Command(CaseGoCreature::Selected::HandleGoCreatureCommand,h,arg)&&WorldDatabase.queries==1,"Entry/name fallback route lost");if(std::string(arg)=="Some'Name")Require(WorldDatabase.where.find("Some\\'Name")!=std::string::npos,"Name not escaped before SQL service");++cases;}
 {Reset();WorldDatabase.exists=false;ChatHandler h;Require(!Command(CaseGoCreature::Selected::HandleGoCreatureCommand,h,"7")&&Pristine(h),"Missing creature query mutated travel");++cases;}
#endif
#ifdef REPAIR_GOGRAVEYARD
 Rejects(CaseGoGraveyard::Selected::HandleGoGraveyardCommand,{""," ","0","-1","7x","4294967303","8"});Admission(CaseGoGraveyard::Selected::HandleGoGraveyardCommand,"7");
 {Reset();sWorldSafeLocsStore.rows[7].Loc.Z=NAN;ChatHandler h;Require(!Command(CaseGoGraveyard::Selected::HandleGoGraveyardCommand,h,"7")&&Pristine(h),"Bad safe-location pose mutated travel");++cases;}
#endif
#ifdef REPAIR_GOGRID
 Rejects(CaseGoGrid::Selected::HandleGoGridCommand,{""," ","32","nan 32","32 inf","32x 32","32 32 1x","32 32 -1","32 32 4294967297","100 32","32 32 3"});Admission(CaseGoGrid::Selected::HandleGoGridCommand,"32 32 1");
 for(uint32 mode:{0u,1u,2u,3u}){Reset();if(!mode)mapService.maps.erase(1);else{mapService.maps[1].floor=mode==1?NAN:mode==2?INVALID_HEIGHT:-200000;mapService.maps[1].water=0;}ChatHandler h;Require(!Command(CaseGoGrid::Selected::HandleGoGridCommand,h,"32 32 1")&&Pristine(h),"Missing/invalid ground reached travel preparation");++cases;}
 {Reset();ChatHandler h;Require(Command(CaseGoGrid::Selected::HandleGoGridCommand,h,"32 32 2")&&h.session.player.map==2&&h.session.player.z==25,"Grid map/water tuple mixed");++cases;}
#endif
#ifdef REPAIR_GOOBJECT
 Rejects(CaseGoObject::Selected::HandleGoObjectCommand,{""," ","0","-1","7x","18446744073709551616","8"});Admission(CaseGoObject::Selected::HandleGoObjectCommand,"4294967303");
 {Reset();ChatHandler h;Require(Command(CaseGoObject::Selected::HandleGoObjectCommand,h,"4294967303")&&objectManager.requested==(uint64(1)<<32)+7&&h.session.player.map==2&&h.session.player.x==40,"GO identifier truncated or wrong data pose");++cases;}
 {Reset();objectManager.objects[7].orientation=NAN;ChatHandler h;Require(!Command(CaseGoObject::Selected::HandleGoObjectCommand,h,"7")&&Pristine(h),"Invalid GO orientation changed travel");++cases;}
#endif
#ifdef REPAIR_GOTAXINODE
 Rejects(CaseGoTaxinode::Selected::HandleGoTaxinodeCommand,{""," ","0","-1","7x","4294967303","8"});Admission(CaseGoTaxinode::Selected::HandleGoTaxinodeCommand,"7");
 {Reset();sTaxiNodesStore.rows[7].Pos={0,0,0};ChatHandler h;Require(!Command(CaseGoTaxinode::Selected::HandleGoTaxinodeCommand,h,"7")&&Pristine(h),"Native absent taxi pose admitted");++cases;}
#endif
#ifdef REPAIR_GOTRIGGER
 Rejects(CaseGoTrigger::Selected::HandleGoTriggerCommand,{""," ","0","-1","7x","4294967303","8"});Admission(CaseGoTrigger::Selected::HandleGoTriggerCommand,"7");
 {Reset();sAreaTriggerStore.rows[7].Pos.X=NAN;ChatHandler h;Require(!Command(CaseGoTrigger::Selected::HandleGoTriggerCommand,h,"7")&&Pristine(h),"Invalid trigger destination changed travel");++cases;}
#endif
#ifdef REPAIR_GOZONEXY
 Rejects(CaseGoZoneXY::Selected::HandleGoZoneXYCommand,{""," ","50","nan 50","50 inf","50x 50","101 50","-1 50","50 50 1x","50 50 4294967297","50 50 -1","50 50 3","50 50 4"});Admission(CaseGoZoneXY::Selected::HandleGoZoneXYCommand,"50 50 2");
 {Reset();ChatHandler h;Require(Command(CaseGoZoneXY::Selected::HandleGoZoneXYCommand,h,"50 50 2")&&h.session.player.map==1&&h.session.player.x==100&&h.session.player.y==150&&sDB2Manager.conversions==1,"Child area used wrong parent/map coordinate conversion");++cases;}
 for(uint32 mode:{0u,1u,2u,3u}){Reset();if(!mode)mapService.maps.erase(1);if(mode==1)mapService.maps[1].instance=true;if(mode==2){mapService.maps[1].floor=-200000;mapService.maps[1].water=0;}if(mode==3)mapService.maps[1].floor=NAN;ChatHandler h;Require(!Command(CaseGoZoneXY::Selected::HandleGoZoneXYCommand,h,"50 50 2")&&Pristine(h),"Zone missing/instanced map/ground mutated travel");++cases;}
 {Reset();ChatHandler h;Require(Command(CaseGoZoneXY::Selected::HandleGoZoneXYCommand,h,"+0 0")&&h.session.player.x==0&&h.session.player.y==0,"Native valid signed zero rejected");++cases;}
#endif
#ifdef REPAIR_GOXYZ
 Rejects(CaseGoXYZ::Selected::HandleGoXYZCommand,{""," ","1","bad 2 3","1 2x 3","1 2 nan","nan 2 3","1 2 3 1x","1 2 3 -1","1 2 3 4294967297","1 2 3 1 nan","1 2 3 1 .5x","1e1000 2 3","20000 2 3"});Admission(CaseGoXYZ::Selected::HandleGoXYZCommand,"1 2 3 2 1.5");
 for(uint32 mode:{0u,1u,2u,3u}){Reset();if(!mode)mapService.maps.erase(1);else{mapService.maps[1].floor=mode==1?NAN:mode==2?INVALID_HEIGHT:-200000;mapService.maps[1].water=0;}ChatHandler h;Require(!Command(CaseGoXYZ::Selected::HandleGoXYZCommand,h,"1 2")&&Pristine(h),"XYZ missing generated ground mutated travel");++cases;}
 for(bool normal:{false,true}){Reset();sMapStore.rows[2].scenario=true;sDB2Manager.normal=normal;ChatHandler h;Require(Command(CaseGoXYZ::Selected::HandleGoXYZCommand,h,"1 2 3 2 .75")&&h.session.player.difficulty==1&&h.session.player.orientation==.75f,"Native scenario difficulty admission/explicit orientation changed");++cases;}
 {Reset();ChatHandler h;Require(Command(CaseGoXYZ::Selected::HandleGoXYZCommand,h,"1 2 20000")&&h.session.player.z==20000,"Native finite explicit high Z was artificially bounded");++cases;}
#endif
#ifdef REPAIR_GOTICKET
 Rejects(CaseGoTicket::Selected::HandleGoTicketCommand,{""," ","0","-1","7x","4294967303"});
 for(bool flying:{false,true}){Reset();ChatHandler h;h.session.player.flying=flying;Require(Command(CaseGoTicket::Selected::HandleGoTicketCommand,h,"7")&&ticketService.requested==7&&ticketService.ticket.calls==1&&h.session.player.transfers==1&&h.session.player.recalls==uint32(!flying)&&h.session.player.cleanups==uint32(flying),"Native ticket void teleport flow changed");++cases;}
 {Reset();ChatHandler h;Require(Command(CaseGoTicket::Selected::HandleGoTicketCommand,h,"8")&&Pristine(h)&&h.messages==1,"Native handled-missing-ticket result changed");++cases;}
#endif
#ifdef REPAIR_GOZ
 Rejects(CaseGoZ::Selected::HandleGoZCommand,{""," ","nan","inf","1x","+nan","-nan","+1e1000"});Admission(CaseGoZ::Selected::HandleGoZCommand,"+5");
 for(char const*arg:{"5","+5","-5","20000"}){Reset();ChatHandler h;float expected=*arg=='+'?105:*arg=='-'?95:std::strtof(arg,nullptr);Require(Command(CaseGoZ::Selected::HandleGoZCommand,h,arg)&&h.session.player.z==expected&&h.session.player.orientation==.5f,"Native absolute/relative Z semantics changed");++cases;}
 {Reset();ChatHandler h;h.session.player.z=3e38f;Require(!Command(CaseGoZ::Selected::HandleGoZCommand,h,"+3e38")&&Pristine(h),"Relative overflow reached travel state");++cases;}
#endif
 std::cout<<"PASS "<<cases<<" complete actual travel-handler cases/native coordinate overloads;controlled terrain/stores/SQL/teleport,not actual client GM travel\n";return 0;
}catch(std::exception const&error){std::cerr<<error.what()<<'\n';return 1;}}
