#include "DB2Store.h"
#include "DB2Structure.h"
#include "DB2LoadInfo.h"
#include "Config.h"
#include "DatabaseEnv.h"
#include "MySQLThreading.h"
#include <algorithm>
#include <cstdlib>
#include <iostream>
#include <iomanip>
#include <limits>
#include <map>
#include <vector>
#include <cmath>

// Read-only research: inspect every ordered native path, including unnamed paths.
// Search rectangles are broad discovery bounds, never authored flight geometry.
int main(int argc,char** argv)
{
    if(argc!=2)return 1;
    std::cout<<std::setprecision(std::numeric_limits<float>::max_digits10);
    std::string error;auto config=std::getenv("LEGION_DIALOGUE_CONFIG");
    if(!config||!sConfigMgr->LoadInitial(config,{},error))return 2;
    MySQL::Library_Init();HotfixDatabase.SetConnectionInfo(sConfigMgr->GetStringDefault("HotfixDatabaseInfo",""),1,1);
    if(HotfixDatabase.Open()||!HotfixDatabase.PrepareStatements())return 3;
    HotfixDatabase.DirectExecute("SET SESSION TRANSACTION READ ONLY");HotfixDatabase.Execute("SET SESSION TRANSACTION READ ONLY");
    auto ro=HotfixDatabase.Query("SELECT @@session.transaction_read_only");if(!ro||!ro->Fetch()[0].GetBool())return 4;
    DB2Storage<TaxiNodesEntry> nodes("TaxiNodes.db2",TaxiNodesLoadInfo::Instance());
    DB2Storage<TaxiPathEntry> paths("TaxiPath.db2",TaxiPathLoadInfo::Instance());
    DB2Storage<TaxiPathNodeEntry> points("TaxiPathNode.db2",TaxiPathNodeLoadInfo::Instance());
    if(!nodes.Load(argv[1],0)||!paths.Load(argv[1],0)||!points.Load(argv[1],0))return 5;
    nodes.LoadFromDB();paths.LoadFromDB();points.LoadFromDB();
    std::map<uint32,std::vector<TaxiPathNodeEntry const*>> grouped;
    size_t nodeCount=0,pathCount=0,pointCount=0,candidates=0;
    for(auto n:nodes){++nodeCount;if(n->MountCreatureID[0]==100967||n->MountCreatureID[1]==100967)
        std::cout<<"MOUNT_NODE\t"<<n->ID<<'\t'<<n->Name->Str[0]<<'\n';}
    for(auto p:points){++pointCount;grouped[p->PathID].push_back(p);}
    auto dalaran=[](TaxiPathNodeEntry const* p){return p->ContinentID==1220&&p->Loc.X>-1800&&p->Loc.X<200&&p->Loc.Y>3300&&p->Loc.Y<5200;};
    auto highmountain=[](TaxiPathNodeEntry const* p){return p->ContinentID==1220&&p->Loc.X>1900&&p->Loc.X<4400&&p->Loc.Y>2300&&p->Loc.Y<4800;};
    for(auto& entry:grouped){auto& route=entry.second;std::sort(route.begin(),route.end(),[](auto a,auto b){return a->NodeIndex<b->NodeIndex;});
        auto a=route.front(),b=route.back();bool candidate=(dalaran(a)&&highmountain(b))||(highmountain(a)&&dalaran(b));
        auto path=paths.LookupEntry(entry.first);auto from=path?nodes.LookupEntry(path->FromTaxiNode):nullptr;auto to=path?nodes.LookupEntry(path->ToTaxiNode):nullptr;
        ++pathCount;
        std::cout<<"PATH\t"<<entry.first<<'\t'<<route.size()<<'\t'<<a->ContinentID<<'\t'<<a->Loc.X<<'\t'<<a->Loc.Y<<'\t'<<a->Loc.Z<<'\t'<<b->ContinentID<<'\t'<<b->Loc.X<<'\t'<<b->Loc.Y<<'\t'<<b->Loc.Z<<'\t'<<(from?from->Name->Str[0]:"<no TaxiPath source>")<<'\t'<<(to?to->Name->Str[0]:"<no TaxiPath destination>")<<'\n';
        if(candidate){++candidates;std::cout<<"CANDIDATE\t"<<entry.first<<'\n';for(auto p:route)
            std::cout<<"POINT\t"<<p->PathID<<'\t'<<unsigned(p->NodeIndex)<<'\t'<<p->ContinentID<<'\t'<<p->Loc.X<<'\t'<<p->Loc.Y<<'\t'<<p->Loc.Z<<'\t'<<p->Delay<<'\t'<<p->ArrivalEventID<<'\t'<<p->DepartureEventID<<'\n';}
    }
    std::cout<<"ZERO_NODE\t"<<(nodes.LookupEntry(0)?"PRESENT":"ABSENT")<<'\n';
    if(auto path=paths.LookupEntry(5762))std::cout<<"PATH_DEFINITION\t5762\t"<<path->FromTaxiNode<<'\t'<<path->ToTaxiNode<<'\n';
    else std::cout<<"PATH_DEFINITION\t5762\tABSENT\n";
    DB2Storage<SpellEffectEntry> effects("SpellEffect.db2",SpellEffectLoadInfo::Instance());
    DB2Storage<SpellEntry> spells("Spell.db2",SpellLoadInfo::Instance());
    if(!effects.Load(argv[1],0)||!spells.Load(argv[1],0))return 6;
    effects.LoadFromDB();spells.LoadFromDB();
    unsigned references=0;
    for(auto e:effects)if(e->EffectMiscValue[0]==5762||e->EffectMiscValue[1]==5762){++references;auto spell=spells.LookupEntry(e->SpellID);
        std::cout<<"SPELL_REFERENCE\t"<<e->SpellID<<'\t'<<(spell?spell->Name->Str[0]:"<absent spell>")<<'\t'<<e->Effect<<'\t'<<e->EffectMiscValue[0]<<'\t'<<e->EffectMiscValue[1]<<'\n';}
    std::cout<<"REFERENCE_COUNT\t"<<references<<'\n';
    std::cout<<"COMPLETE\t"<<nodeCount<<'\t'<<pathCount<<'\t'<<pointCount<<'\t'<<candidates<<'\n';
    HotfixDatabase.Close();
}
