#include "DB2Store.h"
#include "DB2Structure.h"
#include "DB2LoadInfo.h"
#include <iostream>
#include <map>
#include <cmath>
int main(int argc,char**argv){
 if(argc!=2)return 1;
 DB2Storage<TaxiNodesEntry> nodes("TaxiNodes.db2",TaxiNodesLoadInfo::Instance());
 DB2Storage<TaxiPathEntry> paths("TaxiPath.db2",TaxiPathLoadInfo::Instance());
 DB2Storage<TaxiPathNodeEntry> points("TaxiPathNode.db2",TaxiPathNodeLoadInfo::Instance());
 DB2Storage<SpellEffectEntry> effects("SpellEffect.db2",SpellEffectLoadInfo::Instance());
 DB2Storage<CreatureDisplayInfoEntry> displays("CreatureDisplayInfo.db2",CreatureDisplayInfoLoadInfo::Instance());
 DB2Storage<CreatureModelDataEntry> models("CreatureModelData.db2",CreatureModelDataLoadInfo::Instance());
 DB2Storage<QuestLineXQuestEntry> lines("QuestLineXQuest.db2",QuestLineXQuestLoadInfo::Instance());
 if(!nodes.Load(argv[1],0)||!paths.Load(argv[1],0)||!points.Load(argv[1],0)||!effects.Load(argv[1],0)||!displays.Load(argv[1],0)||!models.Load(argv[1],0)||!lines.Load(argv[1],0))return 2;
 auto path=paths.LookupEntry(5262);auto node=nodes.LookupEntry(1722);
 if(!path||!node||path->FromTaxiNode!=1722||path->ToTaxiNode!=1722||path->Cost||node->MountCreatureID[0]!=87442||node->MountCreatureID[1]!=87442)return 3;
 std::cout<<"taxi5262 native node1722 loop cost0 mount87442: "<<node->Name->Str[0]<<'\n';
 std::map<unsigned,TaxiPathNodeEntry const*> route;for(auto p:points)if(p->PathID==5262)route[p->NodeIndex]=p;
 if(route.size()!=7)return 4;for(unsigned i=0;i<7;++i){auto p=route.at(i);if(p->ContinentID!=1220||p->ArrivalEventID||p->DepartureEventID)return 4;std::cout<<"point "<<i<<" "<<p->Loc.X<<" "<<p->Loc.Y<<" "<<p->Loc.Z<<'\n';}
 auto first=route.at(0),last=route.at(6);if(std::abs(first->Loc.X-3181.99f)>.01f||std::abs(first->Loc.Y-1556.f)>.01f||std::abs(last->Loc.X-2495.3f)>.01f||std::abs(last->Loc.Y-2009.37f)>.01f)return 4;
 for(unsigned id:{60307u,65132u,66644u,23884u}){auto d=displays.LookupEntry(id);if(!d)return 5;auto m=models.LookupEntry(d->ModelID);if(!m||!m->FileDataID)return 5;std::cout<<"display "<<id<<" model "<<m->ID<<" file "<<m->FileDataID<<'\n';}
 unsigned credits=0;for(auto e:effects)if(e->SpellID==189100){std::cout<<"189100 effect"<<int(e->EffectIndex)<<" type="<<e->Effect<<" target="<<e->ImplicitTarget[0]<<" misc="<<e->EffectMiscValue[0]<<'\n';if(e->EffectIndex==1){if(e->Effect!=134||e->ImplicitTarget[0]!=1||e->EffectMiscValue[0]!=95391)return 6;++credits;}}if(credits!=1)return 6;
 for(auto q:lines)if(q->QuestID==38872||q->QuestID==38873||q->QuestID==39385||q->QuestID==39154)std::cout<<"chapter quest"<<q->QuestID<<" line="<<q->QuestLineID<<" order="<<int(q->OrderIndex)<<'\n';
 std::cout<<"PASS: native free quest taxi5262/seven authored points and Duchess model, Cullen/Nathanos/catapult models and189100 player-self group credit134/95391. Client flight and firing presentation pending.\n";
}
