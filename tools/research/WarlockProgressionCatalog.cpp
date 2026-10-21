#include "DB2Store.h"
#include "DB2Structure.h"
#include "DB2LoadInfo.h"
#include "ObjectGuid.h"
#include "GossipDef.h"
#include "UnitDefines.h"
#include <iostream>
#include <set>
int main(int argc,char** argv){
 if(argc!=2)return 1;
 if(GossipMenu::GetRequiredNpcFlagForOption(GossipOptionNpc::ShipmentCrafter)!=(uint64(0x104)<<32) ||
    GossipMenu::GetRequiredNpcFlagForOption(GossipOptionNpc::GarrisonTalent)!=(uint64(0x200)<<32) ||
    GossipMenu::GetRequiredNpcFlagForOption(GossipOptionNpc::GarrisonRecruitment)!=(uint64(0x100)<<32) ||
    GossipMenu::GetRequiredNpcFlagForOption(GossipOptionNpc::AdventureMap)!=1)return 10;
 std::cout<<"PASS: original final-link game library separates recruitment, research and primary scouting gossip\n";
 DB2Storage<CriteriaTreeEntry> trees("CriteriaTree.db2",CriteriaTreeLoadInfo::Instance());
 DB2Storage<CriteriaEntry> criteria("Criteria.db2",CriteriaLoadInfo::Instance());
 DB2Storage<GarrMissionEntry> missions("GarrMission.db2",GarrMissionLoadInfo::Instance());
 DB2Storage<SpellEffectEntry> spells("SpellEffect.db2",SpellEffectLoadInfo::Instance());
 DB2Storage<CharShipmentEntry> shipments("CharShipment.db2",CharShipmentLoadInfo::Instance());
 DB2Storage<LFGDungeonsEntry> dungeons("LFGDungeons.db2",LfgDungeonsLoadInfo::Instance());
 if(!trees.Load(argv[1],0)||!criteria.Load(argv[1],0)||!missions.Load(argv[1],0)||!spells.Load(argv[1],0)||!shipments.Load(argv[1],0)||!dungeons.Load(argv[1],0))return 2;
 std::set<int> ids{48515,48517,53274,48536,50169,47550};
 bool more=true;while(more){more=false;for(auto t:trees)if(ids.count(t->Parent)&&ids.insert(t->ID).second)more=true;}
 for(auto t:trees)if(ids.count(t->ID)){
 std::cout<<"TREE "<<t->ID<<" parent "<<t->Parent<<" op "<<int(t->Operator)<<" amount "<<t->Amount<<" criteria "<<t->CriteriaID;
 if(auto c=criteria.LookupEntry(t->CriteriaID))std::cout<<" type "<<int(c->Type)<<" asset "<<c->Asset<<" mod "<<c->ModifierTreeId;
 std::cout<<'\n';}
 for(auto id:{827,928,839,829,830,831,832,833})if(auto m=missions.LookupEntry(id))std::cout<<"MISSION "<<id<<' '<<m->Name->Str[0]<<" type "<<int(m->GarrTypeID)<<" followertype "<<int(m->GarrFollowerTypeID)<<" followers "<<int(m->MaxFollowers)<<" cost "<<m->MissionCost<<" duration "<<m->MissionDuration<<'\n';
 for(auto s:spells)if(s->SpellID==201163||s->SpellID==207247||s->SpellID==219621||s->SpellID==219732||s->SpellID==210565||s->SpellID==210566)std::cout<<"SPELL "<<s->SpellID<<" effect "<<s->Effect<<" misc "<<s->EffectMiscValue[0]<<'\n';
 for(auto s:shipments)if(s->ContainerID==129||s->ContainerID==128)std::cout<<"SHIPMENT "<<s->ID<<" container "<<s->ContainerID<<" spell "<<s->SpellID<<" complete "<<s->OnCompleteSpellID<<" follower "<<s->GarrFollowerID<<" flags "<<int(s->Flags)<<'\n';
 for(auto d:dungeons)if(d->MapID==1501)std::cout<<"DUNGEON "<<d->ID<<" min "<<int(d->MinLevel)<<" max "<<d->MaxLevel<<" difficulty "<<int(d->DifficultyID)<<'\n';
}
