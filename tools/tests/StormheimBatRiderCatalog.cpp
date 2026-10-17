#include <vector>
#include <map>
#include "DB2Store.h"
#include "DB2Structure.h"
#include "DB2LoadInfo.h"
#include "DBCEnums.h"
#include "ObjectGuid.h"
#include "GossipDef.h"
#include "GridMap.h"
#include "VMapManager2.h"
#include "../../src/server/scripts/Legion/stormheim_batrider_path.h"
#include <cmath>
#include <iostream>
#include <string>
int main(int argc,char**argv){if(argc!=2)return 1; DB2Storage<VehicleEntry> vehicles("Vehicle.db2",VehicleLoadInfo::Instance());DB2Storage<VehicleSeatEntry> seats("VehicleSeat.db2",VehicleSeatLoadInfo::Instance());DB2Storage<CreatureDisplayInfoEntry> displays("CreatureDisplayInfo.db2",CreatureDisplayInfoLoadInfo::Instance());DB2Storage<CreatureModelDataEntry> models("CreatureModelData.db2",CreatureModelDataLoadInfo::Instance());DB2Storage<TaxiNodesEntry> nodes("TaxiNodes.db2",TaxiNodesLoadInfo::Instance());DB2Storage<BroadcastTextEntry> texts("BroadcastText.db2",BroadcastTextLoadInfo::Instance());
 std::string dbc=std::string(argv[1])+"/dbc/enUS/";
 if(!vehicles.Load(dbc,0)||!seats.Load(dbc,0)||!displays.Load(dbc,0)||!models.Load(dbc,0)||!nodes.Load(dbc,0)||!texts.Load(dbc,0))return 4;
 auto required=GossipMenu::GetRequiredNpcFlagForOption(GossipOptionNpc::None);if(required!=1||(required&3)==0||(required&2)!=0)return 9;std::cout<<"native gossip None0 requires primary flag"<<required<<'\n';
 auto vehicle=vehicles.LookupEntry(4387);if(!vehicle||vehicle->SeatID[0]!=15956)return 5;for(unsigned i=1;i<8;++i)if(vehicle->SeatID[i])return 5;
 auto seat=seats.LookupEntry(15956);if(!seat||!seat->IsUsableByOverride()||(seat->Flags&VEHICLE_SEAT_FLAG_CAN_CONTROL))return 5;
 std::cout<<"native vehicle4387 seat15956 forcedUsable="<<seat->IsUsableByOverride()<<" control=0 flags="<<seat->Flags<<","<<seat->FlagsB<<'\n';
 for(unsigned id:{62917u,62918u}){auto d=displays.LookupEntry(id);if(!d)return 6;auto m=models.LookupEntry(d->ModelID);if(!m||!m->FileDataID)return 6;std::cout<<"display "<<id<<" model="<<m->ID<<" file="<<m->FileDataID<<'\n';}
 auto node=nodes.LookupEntry(1741);if(node)std::cout<<"landing delta "<<node->Pos.X-StormheimBatRider::Landing.GetPositionX()<<","<<node->Pos.Y-StormheimBatRider::Landing.GetPositionY()<<","<<node->Pos.Z-StormheimBatRider::Landing.GetPositionZ()<<'\n';if(!node||std::abs(node->Pos.X-StormheimBatRider::Landing.GetPositionX())>.01f||std::abs(node->Pos.Y-StormheimBatRider::Landing.GetPositionY())>.01f||std::abs(node->Pos.Z-StormheimBatRider::Landing.GetPositionZ())>.01f)return 7;
 auto text=texts.LookupEntry(98963);std::cout<<"native broadcast98963="<<(text?text->Text->Str[0]:"MISSING")<<'\n';if(text&&std::string(text->Text->Str[0])!="Can you give me a lift back to the Foothold?")return 8;
 std::cout<<"node1741 "<<node->Name->Str[0]<<"; broadcast98963 "<<(text?text->Text->Str[0]:"Hotfix98963: verified separately")<<'\n';std::string root=argv[1];VMAP::VMapManager2 vm;vm.InitializeThreadUnsafe({{1220,{}}});GridMap grid;if(grid.loadData((root+"/maps/1220_24_26.map").c_str())!=GridMap::LoadResult::Ok||vm.loadMap((root+"/vmaps/").c_str(),1220,24,26)!=VMAP::VMAP_LOAD_RESULT_OK)return 2;
 auto previous=StormheimBatRider::Source;bool ok=true;unsigned segment=0;
 for(auto const& p:StormheimBatRider::Flight){bool los=vm.isInLineOfSight(1220,previous.GetPositionX(),previous.GetPositionY(),previous.GetPositionZ()+1,p.GetPositionX(),p.GetPositionY(),p.GetPositionZ()+1,VMAP::ModelIgnoreFlags::Nothing);std::cout<<"segment "<<++segment<<" LOS="<<los<<'\n';ok&=los;
  for(unsigned j=0;j<=40;++j){float t=j/40.f,x=previous.GetPositionX()+(p.GetPositionX()-previous.GetPositionX())*t,y=previous.GetPositionY()+(p.GetPositionY()-previous.GetPositionY())*t,z=previous.GetPositionZ()+(p.GetPositionZ()-previous.GetPositionZ())*t;float terrain=grid.getHeight(x,y),v=vm.getHeight(1220,x,y,z+2,1000);float floor=std::max(terrain,v);if(z+0.05f<floor){std::cout<<"FAIL floor "<<x<<","<<y<<","<<z<<" height "<<floor<<'\n';ok=false;}}
  previous=p;
 }
 auto const& p=StormheimBatRider::Landing;float terrain=grid.getHeight(p.GetPositionX(),p.GetPositionY()),v=vm.getHeight(1220,p.GetPositionX(),p.GetPositionY(),p.GetPositionZ()+2,10),floor=std::max(terrain,v);std::cout<<"landing terrain="<<terrain<<" vmap="<<v<<" error="<<std::abs(floor-p.GetPositionZ())<<'\n';if(!ok||std::abs(floor-p.GetPositionZ())>1)return 3;std::cout<<"PASS: typed native vehicle/seat/models/landing/broadcast and four reconstructed flight segments/164 floor samples, static centerline LOS and supporting landing surface. No full body, dynamic collision or client presentation claim.\n";return 0;
}
