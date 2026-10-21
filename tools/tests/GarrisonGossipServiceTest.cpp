#include <cassert>
#include <cstdint>
#include <iostream>
#include <map>
#include <set>
#include <vector>
using uint8=uint8_t;using uint16=uint16_t;using uint32=uint32_t;using uint64=uint64_t;
#include "Enums.inc"
#include "Classes.inc"
#include "QuestStatus.inc"
enum {UNIT_FIELD_NPC_FLAGS,UNIT_FIELD_NPC_FLAGS2,GARRISON_TYPE_GARRISON=2,GARRISON_TYPE_CLASS_ORDER=3,SHIPMENT_GET_BY_NPC=1};
constexpr uint32 GARRISON_SHIPMENT_FLAG_REQUIRE_QUEST_NOT_COMPLETE=1;
struct GossipMenu{static uint64 GetRequiredNpcFlagForOption(GossipOptionNpc);};
#include "RequiredFlags.inc"
struct WorldObject {
 uint32 primary=1,secondary=0,entry=0;uint64 guid=7;
 uint32 GetUInt32Value(int i)const{return i==UNIT_FIELD_NPC_FLAGS?primary:secondary;}
 bool HasFlag(int i,uint32 mask)const{return (GetUInt32Value(i)&mask)!=0;}
 uint32 GetEntry()const{return entry;}uint64 GetGUID()const{return guid;}
};
using Creature=WorldObject;
bool Admit(GossipOptionNpc optionNpc,WorldObject* source){uint64 npcflags=0;
#include "Flags.inc"
 for(int i=0;i<1;++i){
#include "Admission.inc"
 return true;}return false;}
namespace WorldPackets{namespace Garrison{
struct OpenShipmentNPCFromGossip{uint64 NpcGUID=0;uint32 CharShipmentContainerID=0;int Write()const{return int(CharShipmentContainerID);}};
struct GarrisonOpenUpgradeNpcResponse{uint64 NpcGUID=0;int Write()const{return 512;}};
}}
struct Garrison;
struct Player {
 uint8 playerClass=CLASS_WARLOCK;
 std::map<uint32,QuestStatus> quests;Garrison* _garrison=nullptr;
 int packets=0,lastPacket=0;
 uint8 getClass()const{return playerClass;}
 QuestStatus GetQuestStatus(uint32 id)const{auto i=quests.find(id);return i==quests.end()?QUEST_STATUS_NONE:i->second;}
 Garrison* GetGarrison()const{return _garrison;}Garrison* GetGarrisonPtr()const{return _garrison;}
 void SendDirectMessage(int p){++packets;lastPacket=p;}
 bool ServiceGate(GossipOptionNpc,Creature*);
};
struct CharShipmentEntry{uint32 ID=0,ContainerID=129,Flags=0,GarrFollowerID=649;};
namespace DB2Manager{using Shipments=std::multimap<uint32,CharShipmentEntry*>;using ShipmentConteinerMapBounds=std::pair<Shipments::iterator,Shipments::iterator>;}
struct DB2Facade{DB2Manager::Shipments rows;auto GetShipmentConteinerBounds(uint32 id){return rows.equal_range(id);}} sDB2Manager;
struct ContainerEntry{uint32 GarrTypeID=GARRISON_TYPE_CLASS_ORDER,GarrBuildingType=0;};
struct GarrShipment{uint32 ShipmentID=0,ContainerID=129,questReq=41797;ContainerEntry* cEntry=nullptr;uint32 selectShipment(Player*)const;};
struct ShipmentManager {
 GarrShipment* row=nullptr;
 GarrShipment const* GetGarrShipment(uint32 entry,int,uint8 playerClass)const{return row&&entry==106216&&playerClass==CLASS_WARLOCK?row:nullptr;}
} manager;
// Production uses an object singleton, not a pointer.
#define sGarrisonMgr manager
struct BuildingEntry{uint32 BuildingType=0;};
struct BuildingStore{BuildingEntry row;BuildingEntry const* AssertEntry(uint32)const{return &row;}} sGarrBuildingStore;
uint32 getQuestIdReqForShipment(uint32,uint32){return 0;} // class-order containers have no Draenor building gate
struct Garrison {
 Player* _owner=nullptr;std::set<uint32> talents;
 struct Site{uint32 GarrSiteID=0;} site;
 std::map<uint32,Site*> _siteLevel;
 struct Packet{bool Active=false;uint32 GarrBuildingID=0;};
 struct Plot{struct{Packet* PacketInfo=nullptr;}BuildingInfo;};
 std::map<uint32,Plot> _plots;
 bool hasTallent(uint32 id)const{return talents.count(id)!=0;}
 bool canAddShipmentOrder(Creature*);
 bool canStartUpgrade();
 void OnGossipSelect(WorldObject*);
};
#include "SelectShipment.inc"
#include "CanAddShipment.inc"
#include "CanStartUpgrade.inc"
#include "OpenShipment.inc"
bool Player::ServiceGate(GossipOptionNpc option,Creature* creature){
 if(!Admit(option,creature))return false;
 bool canTalk=true;
 switch(option){
#include "ShipmentGate.inc"
#include "TalentGate.inc"
 default:break;}
 return canTalk;
}
int main(){
 Player p;Garrison g;g._owner=&p;p._garrison=&g;
 ContainerEntry container;GarrShipment shipment;shipment.cEntry=&container;manager.row=&shipment;
 // Native DB2: 255 has the unfinished-quest flag; 321 is the normal shipment.
 CharShipmentEntry normal{321,129,0,649},tutorial{255,129,1,649};
 sDB2Manager.rows.emplace(129,&normal);sDB2Manager.rows.emplace(129,&tutorial);
 Creature dyala;dyala.entry=106216;dyala.secondary=272;
 Creature melinda;melinda.entry=108018;melinda.secondary=512;
 assert(!p.ServiceGate(GossipOptionNpc::ShipmentCrafter,&dyala)); // no site
 g._siteLevel[GARRISON_TYPE_CLASS_ORDER]=&g.site;
 assert(!p.ServiceGate(GossipOptionNpc::ShipmentCrafter,&dyala)); // quest absent
 p.quests[41797]=QUEST_STATUS_INCOMPLETE;
 assert(shipment.selectShipment(&p)==255);
 assert(p.ServiceGate(GossipOptionNpc::ShipmentCrafter,&dyala));
 g.OnGossipSelect(&dyala);assert(p.packets==1&&p.lastPacket==129);
 p.quests[41797]=QUEST_STATUS_REWARDED;
 assert(shipment.selectShipment(&p)==321);
 assert(p.ServiceGate(GossipOptionNpc::ShipmentCrafter,&dyala));
 p.quests[41797]=QUEST_STATUS_NONE;assert(!p.ServiceGate(GossipOptionNpc::ShipmentCrafter,&dyala));
 assert(!p.ServiceGate(GossipOptionNpc::GarrisonTalent,&melinda));
 p.quests[42601]=QUEST_STATUS_INCOMPLETE;assert(p.ServiceGate(GossipOptionNpc::GarrisonTalent,&melinda));
 p.quests[42601]=QUEST_STATUS_REWARDED;assert(p.ServiceGate(GossipOptionNpc::GarrisonTalent,&melinda));
 assert(!p.ServiceGate(GossipOptionNpc::GarrisonTalent,&dyala)); // recruiter bit != research bit
 p.playerClass=CLASS_MAGE;assert(!p.ServiceGate(GossipOptionNpc::GarrisonTalent,&melinda));
 p.playerClass=CLASS_WARLOCK;p._garrison=nullptr;
 assert(!p.ServiceGate(GossipOptionNpc::ShipmentCrafter,&dyala)&&!p.ServiceGate(GossipOptionNpc::GarrisonTalent,&melinda));
 std::cout<<"PASS: native service flag admission, quest/site/class gates, tutorial/normal shipment selection and shipment UI dispatch\n";
}
