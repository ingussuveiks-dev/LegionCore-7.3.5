#include <cassert>
#include <cstdint>
#include <map>
#include <vector>
#include <iostream>
using uint32=uint32_t;
enum class HighGuid{EventObject};
struct ObjectGuid{template<HighGuid>static int Create(int,int,int guid){return guid;}};
struct EventObjectData{int id,mapid,guid;};
struct EventObjectTemplate{float radius=1;};
struct EventObject{EventObjectTemplate* eventTemplate;bool phase=true;};
struct DataStore{std::map<int,EventObjectData> data;auto const& GetEventObjectDataContainer(){return data;}} dataStore;
auto sEventObjectDataStore=&dataStore;
struct Map{std::map<int,EventObject*> live;EventObject* GetEventObject(int g){return live[g];}} map;
struct Player{bool world=true,tele=false,quest=true;int mapid=1107;float distance=0;
 bool IsInWorld(){return world;}bool IsBeingTeleported(){return tele;}int GetMapId(){return mapid;}
 Map* GetMap(){return &map;}bool InSamePhase(EventObject* e){return e->phase;}float GetDistance(EventObject*){return distance;}};
struct Conditions{bool empty=false;std::vector<int> GetConditionsForSmartEvent(int,int id,int type){assert(id==0&&type==13);return empty?std::vector<int>{}:std::vector<int>{1};}
 bool IsObjectMeetToConditions(Player* p,EventObject*,std::vector<int> const&){return p->quest;}} conditions;
auto sConditionMgr=&conditions;
#include "Valor.inc"
int main(){
 EventObjectTemplate t;EventObject altar{&t};map.live[7]=&altar;dataStore.data[7]={148,1107,7};Player p;
 assert(CommandersValor::Allowed(&p));p.quest=false;assert(!CommandersValor::Allowed(&p));p.quest=true;
 for(float distance:{1.001f,5.f,20.f,1000.f}){p.distance=distance;assert(!CommandersValor::Allowed(&p));}
 p.distance=1;assert(CommandersValor::Allowed(&p));p.mapid=1220;assert(!CommandersValor::Allowed(&p));p.mapid=1107;
 p.world=false;assert(!CommandersValor::Allowed(&p));p.world=true;p.tele=true;assert(!CommandersValor::Allowed(&p));p.tele=false;
 altar.phase=false;assert(!CommandersValor::Allowed(&p));altar.phase=true;
 map.live[7]=nullptr;assert(!CommandersValor::Allowed(&p));map.live[7]=&altar;
 conditions.empty=true;assert(!CommandersValor::Allowed(&p));conditions.empty=false;
 t.radius=0;p.distance=0;assert(!CommandersValor::Allowed(&p));t.radius=3;p.distance=2.5f;assert(CommandersValor::Allowed(&p));
 for(int id:{124,134,145,147,148,158,165,167,177,181,198}){dataStore.data[7].id=id;assert(CommandersValor::Allowed(&p));}
 dataStore.data[7].id=372;assert(!CommandersValor::Allowed(&p));
 std::cout<<"PASS: extracted production altar admission uses live map/phase/radius/quest conditions and rejects remote or stale buttons\n";
}
