#include <cassert>
#include <cstdint>
#include <map>
#include <iostream>
using uint32=uint32_t;using uint64=uint64_t;using ObjectGuid=uint64;
#include "GuardIds.inc"
struct Unit{};struct Creature;
struct Map{uint32 id=1620;std::map<ObjectGuid,Creature*> creatures;uint32 GetId()const{return id;}Creature* GetCreature(ObjectGuid key)const{auto at=creatures.find(key);return at==creatures.end()?nullptr:at->second;}};
struct Creature:Unit{Map* map;uint64 db;uint32 entry;ObjectGuid guid;bool alive=true;Creature(Map* m,uint64 d,uint32 e,ObjectGuid g):map(m),db(d),entry(e),guid(g){}Map* GetMap()const{return map;}uint64 GetDBTableGUIDLow()const{return db;}uint32 GetEntry()const{return entry;}ObjectGuid GetGUID()const{return guid;}bool IsAlive()const{return alive;}};
struct InstanceScript{
 Map* instance;uint32 creates=0,removes=0,deaths=0;explicit InstanceScript(Map* map):instance(map){}virtual ~InstanceScript()=default;
 virtual void OnCreatureCreateForScript(Creature*){++creates;}virtual void OnCreatureRemoveForScript(Creature*){++removes;}virtual void CreatureDiesForScript(Creature*,Unit*){++deaths;}
 virtual ObjectGuid GetGuidData(uint32)const{return 987654321;}virtual uint32 GetData(uint32)const{return 123456789;}
};
struct Ledger:InstanceScript{
#include "GuardLedger.inc"
 explicit Ledger(Map* map):InstanceScript(map),altheaGuards{}{}
};
struct Run{Map map;Ledger ledger{&map};Creature a{&map,360869,108216,101},b{&map,360872,108216,102},c{&map,360871,108217,103};
 void Add(Creature& target){map.creatures[target.guid]=&target;ledger.OnCreatureCreateForScript(&target);}void Register(){Add(a);Add(b);Add(c);}void Death(Creature& target){target.alive=false;ledger.CreatureDiesForScript(&target,nullptr);}void Remove(Creature& target){ledger.OnCreatureRemoveForScript(&target);map.creatures.erase(target.guid);}
 bool All()const{return ledger.GetData(KingslayersGuards::AllDefeated)!=0;}
};
int main(){uint32 cases=0;
 {Run r;assert(!r.All()&&!r.ledger.GetGuidData(360869)&&r.ledger.GetData(42)==123456789&&r.ledger.GetGuidData(42)==987654321);r.ledger.CreatureDiesForScript(nullptr,nullptr);assert(!r.All()&&r.ledger.deaths==1);++cases;}
 uint32 orders[6][3]={{0,1,2},{0,2,1},{1,0,2},{1,2,0},{2,0,1},{2,1,0}};
 for(auto const& order:orders){Run r;r.Register();Creature* guards[]={&r.a,&r.b,&r.c};assert(r.ledger.creates==3);for(uint32 at=0;at<3;++at){r.Death(*guards[order[at]]);r.ledger.CreatureDiesForScript(guards[order[at]],nullptr);assert(r.All()==(at==2));}for(auto g:guards)r.Remove(*g);assert(r.All()&&r.ledger.removes==3&&r.ledger.deaths==6);++cases;}
 for(uint32 mode=0;mode<8;++mode){Run r;r.Register();Creature invalid=r.a;Map other;if(mode==0)invalid.db=1;if(mode==1)invalid.entry=108217;if(mode==2)invalid.guid=999;if(mode==3)invalid.map=&other;if(mode==4)r.map.id=1;if(mode!=5)invalid.alive=false;
  if(mode==6){r.Remove(r.a);r.Remove(r.b);r.Remove(r.c);invalid.alive=true;}
  if(mode==7){invalid.db=360871;invalid.entry=108216;}
  r.ledger.CreatureDiesForScript(&invalid,nullptr);assert(!r.All()&&!r.ledger.GetData(360869));++cases;
 }
 {Run r;r.Register();r.Death(r.a);r.Death(r.b);r.Remove(r.c);assert(!r.All()&&!r.ledger.GetData(360871));++cases;}
 {Run r;r.Register();r.Death(r.a);r.Death(r.b);r.Death(r.c);r.c.alive=true;assert(!r.All()&&!r.ledger.GetData(360871));r.Remove(r.c);assert(!r.All());++cases;}
 {Run r;r.Register();r.Death(r.a);r.Death(r.b);r.Death(r.c);r.c.alive=true;r.Remove(r.c);assert(!r.All()&&!r.ledger.GetData(360871));++cases;}
 {Run r;r.Register();r.Death(r.a);r.Death(r.b);r.Death(r.c);Creature replacement=r.c;replacement.guid=999;replacement.alive=true;r.Remove(r.c);r.Add(replacement);r.ledger.CreatureDiesForScript(&r.c,nullptr);assert(!r.All()&&r.ledger.GetGuidData(360871)==999);r.Death(replacement);assert(r.All());++cases;}
 {Run r;r.Register();r.a.alive=false;r.b.alive=false;r.c.alive=false;assert(!r.All());r.Remove(r.a);r.Remove(r.b);r.Remove(r.c);assert(!r.All());++cases;}
 {Run r;r.Register();r.Death(r.a);r.Death(r.b);r.Death(r.c);r.Remove(r.c);Creature corpse=r.c;corpse.guid=999;r.Add(corpse);assert(r.All()&&r.ledger.GetGuidData(360871)==999);Run fresh;assert(!fresh.All());++cases;}
 {Run r;r.Register();r.Death(r.a);r.Death(r.b);r.Death(r.c);r.Remove(r.a);r.Remove(r.b);r.Remove(r.c);assert(r.All());Ledger fresh(&r.map);assert(!fresh.GetData(KingslayersGuards::AllDefeated));++cases;}
 std::cout<<"PASS "<<cases<<" ASAN actual instance guard receipt contexts: three exact authored DB roles/full runtime GUIDs, all6 death orders, duplicate/foreign/live/unregistered callbacks, genuine corpse cleanup, no-death disappearance, same-GUID respawn, replacement/stale GUID and fresh instance isolation; base create/remove/death/data hooks retained. Native Unit::Kill dispatch and consumer tested separately\n";
}
