// Actual production arrays and loader/state methods with controlled containers.
// Native full-core/object registration is exercised separately.
#include <algorithm>
#include <cassert>
#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <map>
#include <set>
#include <vector>
using uint32=uint32_t;using uint64=uint64_t;
enum DoorType {DOOR_TYPE_ROOM,DOOR_TYPE_PASSAGE,DOOR_TYPE_SPAWN_HOLE,MAX_DOOR_TYPES};
enum {NOT_STARTED,IN_PROGRESS,FAIL,DONE,SPECIAL,TO_BE_DECIDED};
enum {GO_STATE_ACTIVE,GO_STATE_READY};
#include "DoorBoundaryActual.inc"
struct DoorData{uint32 entry,bossId;DoorType type;uint32 boundary;};
struct DoorScope{uint32 map,bosses;DoorData const* data;size_t count;};
#include "LegionDoorArrays.inc"
struct BossInfo{uint32 state=NOT_STARTED;};
struct DoorInfo{BossInfo* bossInfo;DoorType type;BoundaryType boundary;DoorInfo(BossInfo* b,DoorType t,BoundaryType d):bossInfo(b),type(t),boundary(d){}};
struct GameObject{uint32 entry;int state=-1;uint32 GetEntry()const{return entry;}void SetGoState(int value){state=value;}};
struct InstanceScript{
 std::vector<BossInfo> bosses;std::multimap<uint32,DoorInfo> doors;
 void LoadDoorData(DoorData const*);void UpdateDoorState(GameObject*);
};
#define TC_LOG_DEBUG(...)
#include "LoadDoorDataActual.inc"
#include "UpdateDoorStateActual.inc"
int main(int argc,char**argv){
 assert(argc==2);auto map=std::strtoul(argv[1],nullptr,10);unsigned checked=0;
 for(auto const& scope:doorScopes)if(scope.map==map){
  InstanceScript script;script.bosses.resize(scope.bosses);script.LoadDoorData(scope.data);
  size_t rows=scope.count-(scope.data[scope.count-1].entry==0);
  assert(script.doors.size()==rows);++checked;
  std::set<uint32> entries;for(size_t i=0;i<rows;++i){auto const& row=scope.data[i];assert(row.entry&&row.bossId<scope.bosses&&row.type<MAX_DOOR_TYPES);entries.insert(row.entry);++checked;}
  for(auto entry:entries){
   std::set<uint32> owners;for(size_t i=0;i<rows;++i)if(scope.data[i].entry==entry)owners.insert(scope.data[i].bossId);
   unsigned combinations=1;for(auto id:owners){(void)id;combinations*=5;}
   for(unsigned variant=0;variant<combinations;++variant){
    for(auto& boss:script.bosses)boss.state=NOT_STARTED;auto code=variant;for(auto id:owners){script.bosses[id].state=code%5;code/=5;}
    bool expected=true;for(size_t i=0;i<rows;++i){auto const& row=scope.data[i];if(row.entry==entry){auto state=script.bosses[row.bossId].state;expected&=row.type==DOOR_TYPE_ROOM?state!=IN_PROGRESS:row.type==DOOR_TYPE_PASSAGE?state==DONE:state==IN_PROGRESS;}}
    GameObject door{entry};script.UpdateDoorState(&door);assert(door.state==(expected?GO_STATE_ACTIVE:GO_STATE_READY));++checked;
   }
  }
  GameObject unrelated{1};script.UpdateDoorState(&unrelated);assert(unrelated.state==-1);script.UpdateDoorState(nullptr);++checked;
  std::cout<<"PASS map="<<map<<" rows="<<rows<<" checks="<<checked<<" actual arrays/loader/state;controlled containers\n";return 0;
 }
 return 2;
}
