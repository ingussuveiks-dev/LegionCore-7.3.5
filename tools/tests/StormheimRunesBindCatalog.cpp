#include "DB2Store.h"
#include "DB2Structure.h"
#include "DB2LoadInfo.h"
#include <iostream>

int main(int argc,char** argv)
{
 if(argc!=2)return 1;
 DB2Storage<LockEntry> locks("Lock.db2",LockLoadInfo::Instance());
 DB2Storage<GameObjectDisplayInfoEntry> displays("GameObjectDisplayInfo.db2",GameobjectDisplayInfoLoadInfo::Instance());
 DB2Storage<AnimKitEntry> kits("AnimKit.db2",AnimKitLoadInfo::Instance());
 if(!locks.Load(argv[1],0)||!displays.Load(argv[1],0)||!kits.Load(argv[1],0))return 2;
 auto lock=locks.LookupEntry(1691);if(!lock)return 3;
 for(unsigned i=0;i<8;++i){std::cout<<"lock1691 slot="<<i<<" type="<<unsigned(lock->Type[i])<<" index="<<lock->Index[i]<<" skill="<<lock->Skill[i]<<'\n';
  if(lock->Type[i]!=(i==1?2:0)||lock->Index[i]!=(i==1?5:0)||lock->Skill[i])return 3;
 }
 for(auto id:{7356,28714}){auto d=displays.LookupEntry(id);if(!d||!d->FileDataID)return 4;std::cout<<"display"<<id<<" file="<<d->FileDataID<<" bounds="<<d->GeoBoxMin.X<<","<<d->GeoBoxMin.Y<<","<<d->GeoBoxMin.Z<<".."<<d->GeoBoxMax.X<<","<<d->GeoBoxMax.Y<<","<<d->GeoBoxMax.Z<<'\n';}
 for(auto id:{8859,11129}){auto k=kits.LookupEntry(id);if(!k)return 5;std::cout<<"animkit"<<id<<" oneShotDuration="<<k->OneShotDuration<<" stop="<<k->OneShotStopAnimKitID<<'\n';}
 std::cout<<"PASS: native Lock1691 ordinary Open/type5, no profession/item key; tablet/stone display and native animation-kit records exist. Playback remains client-unverified.\n";
}
