#include "DB2Store.h"
#include "DB2Structure.h"
#include "DB2LoadInfo.h"
#include "Config.h"
#include "DatabaseEnv.h"
#include "MySQLThreading.h"
#include <algorithm>
#include <cstdlib>
#include <iomanip>
#include <iostream>
#include <map>
#include <set>
#include <vector>
int main(int argc,char** argv){
 if(argc!=2)return 1;std::string error;auto config=std::getenv("LEGION_DIALOGUE_CONFIG");if(!config||!sConfigMgr->LoadInitial(config,{},error))return 2;
 MySQL::Library_Init();HotfixDatabase.SetConnectionInfo(sConfigMgr->GetStringDefault("HotfixDatabaseInfo",""),1,1);if(HotfixDatabase.Open()||!HotfixDatabase.PrepareStatements())return 3;
 HotfixDatabase.DirectExecute("SET SESSION TRANSACTION READ ONLY");HotfixDatabase.Execute("SET SESSION TRANSACTION READ ONLY");
 DB2Storage<PathEntry> paths("Path.db2",PathLoadInfo::Instance());DB2Storage<PathNodeEntry> nodes("PathNode.db2",PathNodeLoadInfo::Instance());DB2Storage<LocationEntry> locations("Location.db2",LocationLoadInfo::Instance());
 DB2Storage<PathNodePropertyEntry> np("PathNodeProperty.db2",PathNodePropertyLoadInfo::Instance());DB2Storage<PathPropertyEntry> props("PathProperty.db2",PathPropertyLoadInfo::Instance());
 if(!paths.Load(argv[1],0)||!nodes.Load(argv[1],0)||!locations.Load(argv[1],0)||!np.Load(argv[1],0)||!props.Load(argv[1],0))return 4;
 paths.LoadFromDB();nodes.LoadFromDB();locations.LoadFromDB();np.LoadFromDB();props.LoadFromDB();
 std::map<uint32,std::vector<PathNodeEntry const*>> grouped;for(auto n:nodes)grouped[n->PathID].push_back(n);
 std::set<uint32> selected{16132,16133,16134,16136,16137,16138,16139,16141,16142,16145,16146,16147,16224,16225,16226,16227,16228,16238,16239,16240,16241,16245,16297,16298,17465,17467};
 std::cout<<std::setprecision(9);
 auto distance=[](LocationEntry const* l,float x,float y,float z){return l&&(l->Pos[0]-x)*(l->Pos[0]-x)+(l->Pos[1]-y)*(l->Pos[1]-y)+(l->Pos[2]-z)*(l->Pos[2]-z)<100;};
 for(auto& g:grouped){auto& points=g.second;std::sort(points.begin(),points.end(),[](auto a,auto b){return a->Sequence<b->Sequence;});auto a=locations.LookupEntry(points.front()->LocationID),b=locations.LookupEntry(points.back()->LocationID);if(!a||!b)return 5;
  bool candidate=distance(a,-698.346f,4494.7f,728.399f)||distance(b,-844.597f,4467.76f,736.042f)||distance(b,-881.08f,4496.95f,580.311f);
  if(candidate)selected.insert(g.first);
  if(!selected.count(g.first))continue;auto p=paths.LookupEntry(g.first);if(!p)return 6;
  std::cout<<"PATH\t"<<g.first<<'\t'<<unsigned(p->Type)<<'\t'<<unsigned(p->SplineType)<<'\t'<<unsigned(p->Flags)<<'\t'<<points.size()<<'\t'<<candidate<<'\n';
  for(auto node:points){auto l=locations.LookupEntry(node->LocationID);if(!l)return 7;std::cout<<"POINT\t"<<g.first<<'\t'<<node->Sequence<<'\t'<<l->ID;for(auto v:l->Pos)std::cout<<'\t'<<v;for(auto v:l->Rot)std::cout<<'\t'<<v;std::cout<<'\n';}
 }
 for(auto p:props)if(selected.count(p->PathID))std::cout<<"PROPERTY\t"<<p->PathID<<'\t'<<unsigned(p->PropertyIndex)<<'\t'<<p->Value<<'\n';
 for(auto p:np)if(selected.count(p->PathID))std::cout<<"NODE_PROPERTY\t"<<p->PathID<<'\t'<<p->Sequence<<'\t'<<unsigned(p->PropertyIndex)<<'\t'<<p->Value<<'\n';
 std::cout<<"COMPLETE\t"<<grouped.size()<<'\t'<<selected.size()<<'\n';HotfixDatabase.Close();
}
