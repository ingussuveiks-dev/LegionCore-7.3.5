#include "DB2Store.h"
#include "DB2Structure.h"
#include "DB2LoadInfo.h"
#include "Config.h"
#include "DatabaseEnv.h"
#include "MySQLThreading.h"
#include <cstdlib>
#include <iostream>
int main(int argc,char** argv)
{
 if(argc!=2)return 1;
 std::string error;auto config=std::getenv("LEGION_DIALOGUE_CONFIG");
 if(!config||!sConfigMgr->LoadInitial(config,{},error))return 2;
 MySQL::Library_Init();HotfixDatabase.SetConnectionInfo(sConfigMgr->GetStringDefault("HotfixDatabaseInfo",""),1,1);
 if(HotfixDatabase.Open()||!HotfixDatabase.PrepareStatements())return 3;
 HotfixDatabase.DirectExecute("SET SESSION TRANSACTION READ ONLY");HotfixDatabase.Execute("SET SESSION TRANSACTION READ ONLY");
 auto ro=HotfixDatabase.Query("SELECT @@session.transaction_read_only");if(!ro||!ro->Fetch()[0].GetBool())return 4;
 DB2Storage<TaxiPathEntry> paths("TaxiPath.db2",TaxiPathLoadInfo::Instance());
 DB2Storage<TaxiPathNodeEntry> points("TaxiPathNode.db2",TaxiPathNodeLoadInfo::Instance());
 DB2Storage<TaxiNodesEntry> nodes("TaxiNodes.db2",TaxiNodesLoadInfo::Instance());
 DB2Storage<SpellEffectEntry> effects("SpellEffect.db2",SpellEffectLoadInfo::Instance());
 if(!paths.Load(argv[1],0)||!points.Load(argv[1],0)||!nodes.Load(argv[1],0)||!effects.Load(argv[1],0))return 5;
 paths.LoadFromDB();points.LoadFromDB();nodes.LoadFromDB();effects.LoadFromDB();
 auto path=paths.LookupEntry(5626);if(!path)return 6;
 std::cout<<"PATH\t"<<path->ID<<'\t'<<path->FromTaxiNode<<'\t'<<path->ToTaxiNode<<'\t'<<path->Cost<<'\n';
 for(auto id:{path->FromTaxiNode,path->ToTaxiNode}){
  auto n=nodes.LookupEntry(id);if(!n)return 7;
  std::cout<<"NODE\t"<<n->ID<<'\t'<<n->Name->Str[0]<<'\t'<<n->ContinentID<<'\t'<<n->Pos.X<<'\t'<<n->Pos.Y<<'\t'<<n->Pos.Z<<'\t'<<n->ConditionID<<'\t'<<unsigned(n->Flags)<<'\t'<<n->MountCreatureID[0]<<'\t'<<n->MountCreatureID[1]<<'\n';
 }
 for(auto p:points)if(p->PathID==5626)std::cout<<"POINT\t"<<unsigned(p->NodeIndex)<<'\t'<<p->ContinentID<<'\t'<<p->Loc.X<<'\t'<<p->Loc.Y<<'\t'<<p->Loc.Z<<'\t'<<unsigned(p->Flags)<<'\t'<<p->Delay<<'\t'<<p->ArrivalEventID<<'\t'<<p->DepartureEventID<<'\n';
 for(auto e:effects)if(e->SpellID==209441)std::cout<<"EFFECT\t"<<unsigned(e->EffectIndex)<<'\t'<<e->Effect<<'\t'<<e->EffectMiscValue[0]<<'\t'<<unsigned(e->ImplicitTarget[0])<<'\t'<<unsigned(e->ImplicitTarget[1])<<'\n';
 HotfixDatabase.Close();MySQL::Library_End();
}
