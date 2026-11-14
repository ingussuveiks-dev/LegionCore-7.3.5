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
#include <set>
#include <vector>
#include <cmath>

// Read-only matching-data research. Enumerate every taxi path containing a
// Temple map1625 point, then inspect spell/event references; no NPC binding inferred.
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
    for(auto n:nodes) {
        ++nodeCount;
        if(n->ContinentID==1625||n->MountCreatureID[0]==109441||n->MountCreatureID[1]==109441)
            std::cout<<"NODE\t"<<n->ID<<'\t'<<n->ContinentID<<'\t'<<n->Name->Str[0]<<'\n';
    }
    for(auto p:points){++pointCount;grouped[p->PathID].push_back(p);}
    for(auto& entry:grouped){
        ++pathCount;auto& route=entry.second;
        bool candidate=std::any_of(route.begin(),route.end(),[](auto p){return p->ContinentID==1625;});
        if(!candidate)continue;++candidates;
        std::sort(route.begin(),route.end(),[](auto a,auto b){return a->NodeIndex<b->NodeIndex;});
        auto path=paths.LookupEntry(entry.first);
        std::cout<<"PATH\t"<<entry.first<<'\t'<<route.size()<<'\t'<<(path?1:0)<<'\t'<<(path?path->FromTaxiNode:0)<<'\t'<<(path?path->ToTaxiNode:0)<<'\n';
        for(auto p:route)std::cout<<"POINT\t"<<p->PathID<<'\t'<<unsigned(p->NodeIndex)<<'\t'<<p->ContinentID<<'\t'<<p->Loc.X<<'\t'<<p->Loc.Y<<'\t'<<p->Loc.Z<<'\t'<<p->ID<<'\t'<<unsigned(p->Flags)<<'\t'<<p->Delay<<'\t'<<p->ArrivalEventID<<'\t'<<p->DepartureEventID<<'\n';
    }
    std::cout<<"COMPLETE\t"<<nodeCount<<'\t'<<pathCount<<'\t'<<pointCount<<'\t'<<candidates<<'\n';
    DB2Storage<SpellEffectEntry> effects("SpellEffect.db2",SpellEffectLoadInfo::Instance());
    DB2Storage<SpellEntry> spells("Spell.db2",SpellLoadInfo::Instance());
    if(!effects.Load(argv[1],0)||!spells.Load(argv[1],0))return 6;
    effects.LoadFromDB();spells.LoadFromDB();
    std::set<uint32> selected{5699,5700,5701,5719,5720,5721,5727,5728,5729};
    for(auto e:effects)if(selected.count(uint32(e->EffectMiscValue[0]))||selected.count(uint32(e->EffectMiscValue[1]))){auto spell=spells.LookupEntry(e->SpellID);
        std::cout<<"SPELL_REFERENCE\t"<<e->SpellID<<'\t'<<(spell?spell->Name->Str[0]:"<absent>")<<'\t'<<e->Effect<<'\t'<<e->EffectMiscValue[0]<<'\t'<<e->EffectMiscValue[1]<<'\t'<<e->EffectTriggerSpell<<'\n';}
    DB2Storage<CriteriaEntry> criteria("Criteria.db2",CriteriaLoadInfo::Instance());
    if(!criteria.Load(argv[1],0))return 7;criteria.LoadFromDB();
    for(auto c:criteria)if(c->Asset==52731)
        std::cout<<"CRITERION_REFERENCE\t"<<c->ID<<'\t'<<unsigned(c->Type)<<'\t'<<c->Asset<<'\n';
    std::cout<<"EOF\tFuZanOpenMovementCatalog\n";
    HotfixDatabase.Close();
}
