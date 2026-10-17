#include "DB2Store.h"
#include "DB2Structure.h"
#include "DB2LoadInfo.h"
#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
int main(int argc,char** argv)
{
 if(argc!=3)return 1;
 DB2Storage<ItemSparseEntry> items("ItemSparse.db2",ItemSparseLoadInfo::Instance());
 DB2Storage<ItemEffectEntry> uses("ItemEffect.db2",ItemEffectLoadInfo::Instance());
 DB2Storage<GameObjectDisplayInfoEntry> objects("GameObjectDisplayInfo.db2",GameobjectDisplayInfoLoadInfo::Instance());
 DB2Storage<CreatureDisplayInfoEntry> actors("CreatureDisplayInfo.db2",CreatureDisplayInfoLoadInfo::Instance());
 DB2Storage<CreatureModelDataEntry> models("CreatureModelData.db2",CreatureModelDataLoadInfo::Instance());
 DB2Storage<BroadcastTextEntry> texts("BroadcastText.db2",BroadcastTextLoadInfo::Instance());
 DB2Storage<PlayerConditionEntry> conditions("PlayerCondition.db2",PlayerConditionLoadInfo::Instance());
 if(!items.Load(argv[1],0)||!uses.Load(argv[1],0)||!objects.Load(argv[1],0)||!actors.Load(argv[1],0)||!models.Load(argv[1],0)||!texts.Load(argv[1],0)||!conditions.Load(argv[1],0))return 2;
 for(auto id:{126946,128324}){
  auto i=items.LookupEntry(id);if(!i||i->Stackable!=(id==126946?8:1)||i->StartQuestID!=(id==126946?0:39405)||i->Flags[0]!=67584||i->Flags[1]!=8192)return 3;
  if(i->MaxCount!=(id==126946?0:1)||i->Bonding!=(id==126946?4:1))return 3;
  unsigned effects=0;for(auto e:uses)effects+=e->ItemID==id;if(effects)return 3;
  std::cout<<"item"<<id<<" "<<i->Display->Str[0]<<" start="<<i->StartQuestID<<" stack="<<i->Stackable<<" max="<<i->MaxCount<<" bonding="<<unsigned(i->Bonding)<<" uses="<<effects<<'\n';
 }
 for(auto id:{15781,29023,17805,14702,15405,25948}){auto d=objects.LookupEntry(id);if(!d||!d->FileDataID)return 4;std::cout<<"object"<<id<<" file="<<d->FileDataID<<" bounds="<<d->GeoBoxMin.X<<","<<d->GeoBoxMin.Y<<","<<d->GeoBoxMin.Z<<".."<<d->GeoBoxMax.X<<","<<d->GeoBoxMax.Y<<","<<d->GeoBoxMax.Z<<'\n';}
 for(auto id:{68940,76630}){auto a=actors.LookupEntry(id);if(!a)return 5;auto m=models.LookupEntry(a->ModelID);if(!m||!m->FileDataID)return 5;std::cout<<"actor"<<id<<" model="<<m->ID<<" file="<<m->FileDataID<<'\n';}
 // These five texts have live Hotfix rows. Native base strings alone do not
 // describe the installed text; Sigrid uses the female Text1 field.
 std::ifstream overlay(argv[2]);if(!overlay)return 6;unsigned total=0;std::string line;
 while(std::getline(overlay,line)){std::istringstream in(line);std::string kind;in>>kind;if(kind!="broadcast")continue;int id,male,female,sound1,sound2;in>>id>>male>>female>>sound1>>sound2;if(male+female<=0)return 6;++total;std::cout<<"text"<<id<<" nativePresent="<<bool(texts.LookupEntry(id))<<" HotfixMaleBytes="<<male<<" HotfixFemaleBytes="<<female<<" sounds="<<sound1<<","<<sound2<<'\n';}if(total!=5)return 6;
 auto c=conditions.LookupEntry(36152);if(!c){std::cout<<"legacy condition36152 absent in26972 native data; no Hotfix row (checked separately). Native missing-condition lookup is permissive.\n";return 0;}
 std::cout<<"condition36152 race="<<c->RaceMask<<" class="<<c->ClassMask<<" gender="<<int(c->Gender)<<" min="<<c->MinLevel<<" max="<<c->MaxLevel<<" prevLogic="<<c->PrevQuestLogic<<" currLogic="<<c->CurrQuestLogic<<" completedLogic="<<c->CurrentCompletedQuestLogic<<'\n';
 for(unsigned n=0;n<4;++n)std::cout<<"conditionQuestSlot"<<n<<" prev="<<c->PrevQuestID[n]<<" curr="<<c->CurrQuestID[n]<<" complete="<<c->CurrentCompletedQuestID[n]<<'\n';
}
