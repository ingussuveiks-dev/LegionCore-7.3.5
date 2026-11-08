#include "DB2Store.h"
#include "DB2Structure.h"
#include "DB2LoadInfo.h"
#include <iostream>
#include <iomanip>
int main(int argc,char**argv){
 if(argc!=2)return 1;
 DB2Storage<WorldSafeLocsEntry> safe("WorldSafeLocs.db2",WorldSafeLocsLoadInfo::Instance());
 DB2Storage<ScenarioStepEntry> steps("ScenarioStep.db2",ScenarioStepLoadInfo::Instance());
 DB2Storage<CriteriaTreeEntry> trees("CriteriaTree.db2",CriteriaTreeLoadInfo::Instance());
 DB2Storage<CriteriaEntry> criteria("Criteria.db2",CriteriaLoadInfo::Instance());
 if(!safe.Load(argv[1],0)||!steps.Load(argv[1],0)||!trees.Load(argv[1],0)||!criteria.Load(argv[1],0))return 2;
 std::cout<<std::setprecision(9);
 for(auto s:safe)if(s->MapID==1620)std::cout<<"SAFE\t"<<s->ID<<'\t'<<s->MapID<<'\t'<<s->Loc.X<<'\t'<<s->Loc.Y<<'\t'<<s->Loc.Z<<'\t'<<s->Loc.O<<'\t'<<s->Name->Str[0]<<'\n';
 for(auto s:steps)if(s->ScenarioID==1123){
  std::cout<<"STEP\t"<<s->ID<<'\t'<<unsigned(s->OrderIndex)<<'\t'<<s->Criteriatreeid<<'\n';
  for(auto tree:trees){auto parent=tree;bool member=false;for(unsigned depth=0;parent&&depth<64;++depth){if(parent->ID==s->Criteriatreeid)member=true;parent=trees.LookupEntry(parent->Parent);}if(!member)continue;
   auto c=criteria.LookupEntry(tree->CriteriaID);std::cout<<"TREE\t"<<tree->ID<<'\t'<<tree->Parent<<'\t'<<unsigned(tree->Operator)<<'\t'<<tree->Amount<<'\t'<<tree->CriteriaID<<'\t'<<(c?unsigned(c->Type):0)<<'\t'<<(c?c->Asset:0)<<'\n';}
 }
 std::cout<<"EOF\tkingslayers-graveyard\n";return 0;
}
