#include <array>
#include <cassert>
#include <cmath>
#include <iostream>
#include <map>
#include <memory>
#include <set>
#include <string>
#include <vector>
#include <list>
#include <unordered_map>
#include "../../src/server/game/PrecompiledHeaders/gamePCH.h"
#include "Creature.h"
#include "Optional.h"
#include "NPCPackets.h"
#include "GossipDef.h"

namespace Host
{
unsigned errors=0;
#undef TC_LOG_ERROR
#undef TC_LOG_INFO
#define TC_LOG_ERROR(...) (++Host::errors)
#define TC_LOG_INFO(...) ((void)0)
uint32 getMSTime(){return 0;}uint32 GetMSTimeDiffToNow(uint32){return 0;}
template<class T>auto AsUnderlyingType(T value){return static_cast<uint32>(value);}
struct Field{
 uint32 n=0;std::string s;Field(uint32 v):n(v){}Field(int v):n(uint32(v)){}Field(char const* v):s(v){}
 uint32 GetUInt32(){return n;}uint16 GetUInt16(){return uint16(n);}uint8 GetUInt8(){return uint8(n);}bool GetBool(){return n!=0;}std::string GetString(){return s;}
};
using Row=std::array<Field,11>;
Row MakeRow(uint32 option=0,uint32 poi=177,uint32 submenu=1902){return {Field(1951),Field(10),Field(option),Field("Flight Master"),Field(submenu),Field(poi),Field(0),Field(0),Field(""),Field(45379),Field(0)};}
struct Result{std::vector<Row> rows;size_t index=0;Field*Fetch(){return rows[index].data();}bool NextRow(){return ++index<rows.size();}};
struct Database{std::vector<Row> rows;std::unique_ptr<Result> Query(char const*sql){assert(std::string(sql).find("MenuID, OptionID, OptionNpc, OptionText, ActionMenuID, ActionPoiID")!=std::string::npos);if(rows.empty())return nullptr;auto r=std::make_unique<Result>();r->rows=rows;return r;}}WorldDatabase;
struct PointOfInterest{float x=1800.22f,y=-4369.74f;uint32 flags=99;int32 icon=7,data=0;std::string icon_name="The Sky Tower";};
struct PointOfInterestLocale{std::vector<std::string> IconName;};
struct QuestStore{
 PointOfInterest poi;PointOfInterestLocale locale;bool localized=false;
 PointOfInterest const*GetPointOfInterest(uint32 id){return id==177?&poi:nullptr;}
 PointOfInterestLocale const*GetPointOfInterestLocale(uint32){return localized?&locale:nullptr;}
}quests;auto sQuestDataStore=&quests;
struct BroadcastStore{void*LookupEntry(uint32 id){return id==45379?this:nullptr;}}sBroadcastTextStore;
struct GossipMenuItems{
 uint32 MenuID=0,OptionIndex=0,OptionBroadcastTextID=0,BoxBroadcastTextID=0;
 GossipOptionNpc OptionNpc=GossipOptionNpc::None;uint32 ActionMenuID=0,ActionPoiID=0,BoxMoney=0;
 std::string OptionText,BoxText;bool BoxCoded=false;
};
struct GossipDataStoreMgr{std::multimap<uint32,GossipMenuItems> _gossipMenuItemsStore;void LoadGossipMenuItems();};
#include "../../build-extractors/runtime-test/gossip-poi/Loader.inc"
struct Player{std::vector<WorldPacket> packets;void SendDirectMessage(WorldPacket const*p){packets.push_back(*p);}};
struct Session{Player*player;int locale=0;Player*GetPlayer()const{return player;}int GetSessionDbLocaleIndex()const{return locale;}};
struct ObjectMgr{static void GetLocaleString(std::vector<std::string>const&v,int i,std::string&out){if(i>=0&&size_t(i)<v.size()&&!v[i].empty())out=v[i];}};
struct PlayerMenu{Session*_session;unsigned closes=0;void SendPointOfInterest(uint32)const;void SendCloseGossip(){++closes;}};
#include "../../build-extractors/runtime-test/gossip-poi/SendPoi.inc"
struct Selector{
 PlayerMenu*PlayerTalkClass;uint32 submenu=0,replies=0;
 void PrepareGossipMenu(void*,uint32 id){submenu=id;}void SendPreparedGossip(void*){++replies;}
 void Select(GossipMenuItemData const*menuItemData){void*source=nullptr;
#include "../../build-extractors/runtime-test/gossip-poi/Selection.inc"
 }
};
void CheckPacket(WorldPacket packet,std::string const&name){
 assert(packet.GetOpcode()==SMSG_GOSSIP_POI);
 assert(packet.ReadBits(14)==99);auto size=packet.ReadBits(6);assert(size==name.size());packet.ResetBitPos();
 float x,y;int32 icon,importance;packet>>x>>y>>icon>>importance;
 assert(std::fabs(x-1800.22f)<0.001f&&std::fabs(y+4369.74f)<0.001f&&icon==7&&importance==0);
 assert(packet.ReadString(size)==name&&packet.rpos()==packet.size());
}
}
int main(){using namespace Host;
 GossipDataStoreMgr loader;
 WorldDatabase.rows={MakeRow()};loader.LoadGossipMenuItems();assert(!errors);
 auto row=loader._gossipMenuItemsStore.begin()->second;
#ifdef POI_BASELINE
 assert(row.ActionPoiID==0&&row.ActionMenuID==1902);
 std::cout<<"COUNTERFACTUAL pinned loader erases valid Orgrimmar1951/10 ->177 without diagnostic, retaining only reply1902\n";
#else
 assert(row.ActionPoiID==177&&row.ActionMenuID==1902);
 Host::Player player;Session session{&player};Host::PlayerMenu menu{&session};Selector selector{&menu};
 GossipMenuItemData selection{row.ActionMenuID,row.ActionPoiID};selector.Select(&selection);
 assert(player.packets.size()==1&&selector.submenu==1902&&selector.replies==1&&!menu.closes);
 CheckPacket(player.packets.back(),"The Sky Tower");
 quests.localized=true;quests.locale.IconName={"Flight Master"};selector.Select(&selection);
 assert(player.packets.size()==2);CheckPacket(player.packets.back(),"Flight Master");quests.localized=false;
 auto packets=player.packets.size();menu.SendPointOfInterest(999);assert(player.packets.size()==packets);
 for(unsigned i=0;i<500;++i){errors=0;WorldDatabase.rows={MakeRow()};loader.LoadGossipMenuItems();assert(loader._gossipMenuItemsStore.size()==1&&loader._gossipMenuItemsStore.begin()->second.ActionPoiID==177&&!errors);}
 for(unsigned i=0;i<4;++i){errors=0;auto bad=MakeRow(i==0||i==3?uint32(GossipOptionNpc::Vendor):0,i==1?999:i==2?0:177,i==3?1902:0);WorldDatabase.rows={bad};loader.LoadGossipMenuItems();assert(loader._gossipMenuItemsStore.begin()->second.ActionPoiID==0);assert(errors==(i==2?0u:i==3?2u:1u));}
 errors=0;WorldDatabase.rows={MakeRow(255)};loader.LoadGossipMenuItems();assert(errors==1&&loader._gossipMenuItemsStore.begin()->second.ActionPoiID==177);
 WorldDatabase.rows.clear();errors=0;loader.LoadGossipMenuItems();assert(loader._gossipMenuItemsStore.empty()&&errors==1);
 std::cout<<"PASS full production loader/selection-tail/POI send, actual linked7.3.5 GossipPOI and ByteBuffer decode, reply/localization/missing/invalid diagnostics/reload; packet transport and client rendering remain separate\n";
#endif
}
