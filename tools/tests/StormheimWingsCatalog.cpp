#include "DB2Store.h"
#include "DB2Structure.h"
#include "DB2LoadInfo.h"
#include "VMapManager2.h"
#include "GridMap.h"
#include "../../src/server/scripts/Legion/stormheim_wings_departure.h"
#include <iostream>
#include <cmath>
#include <map>
#include <memory>

int main(int argc,char** argv)
{
 if(argc!=2)return 1;std::string root=argv[1],dbc=root+"/dbc/enUS/";
 DB2Storage<LockEntry> locks("Lock.db2",LockLoadInfo::Instance());
 DB2Storage<GameObjectDisplayInfoEntry> displays("GameObjectDisplayInfo.db2",GameobjectDisplayInfoLoadInfo::Instance());
 DB2Storage<CreatureDisplayInfoEntry> actors("CreatureDisplayInfo.db2",CreatureDisplayInfoLoadInfo::Instance());
 DB2Storage<CreatureModelDataEntry> models("CreatureModelData.db2",CreatureModelDataLoadInfo::Instance());
 DB2Storage<SpellEntry> spells("Spell.db2",SpellLoadInfo::Instance());
 DB2Storage<SpellEffectEntry> effects("SpellEffect.db2",SpellEffectLoadInfo::Instance());
 if(!locks.Load(dbc.c_str(),0)||!displays.Load(dbc.c_str(),0)||!actors.Load(dbc.c_str(),0)||!models.Load(dbc.c_str(),0)||!spells.Load(dbc.c_str(),0)||!effects.Load(dbc.c_str(),0))return 2;
 auto lock=locks.LookupEntry(1690);auto circle=displays.LookupEntry(16973);auto dummy=spells.LookupEntry(193637);if(!lock||!circle||!dummy)return 3;
 for(unsigned i=0;i<8;++i)if(lock->Type[i]!=(i==1?2:0)||lock->Index[i]!=(i==1?5:0)||lock->Skill[i])return 3;
 std::cout<<"native lock1690 ordinary Open/type5; circle16973 file="<<circle->FileDataID<<";193637 "<<dummy->Name->Str[0]<<'\n';
 unsigned count=0;for(auto e:effects)if(e->SpellID==193637){++count;std::cout<<"effect193637 type="<<e->Effect<<" target="<<e->ImplicitTarget[0]<<" trigger="<<e->EffectTriggerSpell<<'\n';if(e->Effect!=3||e->ImplicitTarget[0]!=38||e->EffectTriggerSpell)return 4;}
 if(count!=1)return 4;
 for(auto id:{67198,68213}){auto d=actors.LookupEntry(id);if(!d)return 5;auto m=models.LookupEntry(d->ModelID);if(!m||!m->FileDataID)return 5;std::cout<<"actor"<<id<<" model="<<m->ID<<" file="<<m->FileDataID<<'\n';}
 VMAP::VMapManager2 vm;vm.InitializeThreadUnsafe({{1220,{}}});for(int x=27;x<=29;++x)for(int y=26;y<=28;++y)vm.loadMap((root+"/vmaps/").c_str(),1220,x,y);
 std::map<std::pair<int,int>,std::unique_ptr<GridMap>> terrain;
 struct Point{float x,y,z;};unsigned index=0;for(auto p:{Point{2033.7f,2186.07f,465.226f},{2132.36f,2372.49f,490.706f},{2057.1f,2473.28f,483.957f},{1998.81f,2385.1f,482.035f}}){
  auto via=StormheimWings::Departures[index++];Point points[]={p,{via.x,via.y,via.z},{via.x,via.y,p.z+40}};
  for(unsigned leg=0;leg<2;++leg){auto a=points[leg],b=points[leg+1];
   for(float dx:{-.5f,0.f,.5f})if(!vm.isInLineOfSight(1220,a.x+dx,a.y,a.z+2,b.x+dx,b.y,b.z+2,VMAP::ModelIgnoreFlags::Nothing))return 6;
   unsigned samples=unsigned(std::ceil(std::sqrt((b.x-a.x)*(b.x-a.x)+(b.y-a.y)*(b.y-a.y)+(b.z-a.z)*(b.z-a.z))))+1;
   for(unsigned n=0;n<=samples;++n){float t=float(n)/samples;Point q{a.x+(b.x-a.x)*t,a.y+(b.y-a.y)*t,a.z+(b.z-a.z)*t};
    int x=int(32-q.x/533.33333f),y=int(32-q.y/533.33333f);auto& grid=terrain[{x,y}];if(!grid){grid=std::make_unique<GridMap>();if(grid->loadData((root+"/maps/1220_"+std::to_string(x)+"_"+std::to_string(y)+".map").c_str())!=GridMap::LoadResult::Ok)return 7;}
    float ground=grid->getHeight(q.x,q.y),floor=vm.getHeight(1220,q.x,q.y,q.z+1,20);
    if((floor>-10000&&floor>q.z+.5f)||(ground>q.z+1&&floor<-10000)){std::cout<<"Underground departure "<<q.x<<","<<q.y<<","<<q.z<<" terrain="<<ground<<" floor="<<floor<<'\n';return 7;}
   }
  }
  std::cout<<"PASS: departure "<<index<<" both native collision legs and <=1yd terrain/cave samples via "<<via.x<<","<<via.y<<","<<via.z<<'\n';
 }
}
