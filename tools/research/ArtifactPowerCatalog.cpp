// Read progression fixtures with the production 7.3.5 DB2 reader.
#include "DB2Store.h"
#include "DB2Structure.h"
#include "DB2LoadInfo.h"
#include <iostream>
int main(int argc,char** argv)
{
    if(argc!=2)return 1;
    DB2Storage<ArtifactEntry> artifacts("Artifact.db2",ArtifactLoadInfo::Instance());
    DB2Storage<ArtifactPowerEntry> powers("ArtifactPower.db2",ArtifactPowerLoadInfo::Instance());
    DB2Storage<ArtifactPowerRankEntry> ranks("ArtifactPowerRank.db2",ArtifactPowerRankLoadInfo::Instance());
    DB2Storage<ArtifactPowerLinkEntry> links("ArtifactPowerLink.db2",ArtifactPowerLinkLoadInfo::Instance());
    if(!artifacts.Load(argv[1],0)||!powers.Load(argv[1],0)||!ranks.Load(argv[1],0)||!links.Load(argv[1],0))return 2;
    for(auto a:artifacts)std::cout<<"artifact "<<a->ID<<' '<<unsigned(a->ArtifactCategoryID)<<' '<<a->ChrSpecializationID<<'\n';
    for(auto p:powers)std::cout<<"power "<<p->ID<<' '<<unsigned(p->ArtifactID)<<' '<<unsigned(p->Flags)<<' '<<unsigned(p->MaxPurchasableRank)<<' '<<unsigned(p->Tier)<<'\n';
    for(auto r:ranks)std::cout<<"rank "<<r->ArtifactPowerID<<' '<<unsigned(r->RankIndex)<<' '<<r->SpellID<<'\n';
    for(auto l:links)std::cout<<"link "<<l->PowerA<<' '<<l->PowerB<<'\n';
}
