#include <cassert>
#include <cstdint>
#include <iostream>
#include <map>
#include <mutex>
#include <vector>
using uint32=uint32_t;
enum {QUEST_STATUS_NONE,QUEST_STATUS_INCOMPLETE,QUEST_STATUS_COMPLETE,QUEST_STATUS_REWARDED};
struct ObjectGuid {unsigned value;bool operator<(ObjectGuid const& r)const{return value<r.value;}};
struct Player;
struct Unit {virtual ~Unit()=default;virtual Player* ToPlayer(){return nullptr;}};
struct Creature;using TempSummon=Creature;
std::map<ObjectGuid,Creature*> creatures;
struct Player:Unit {
 ObjectGuid guid;uint32 map=1220,area=7810;int quest=QUEST_STATUS_INCOMPLETE,next=QUEST_STATUS_NONE;
 bool alive=true,teleport=false,flight=false,vehicle=false;unsigned casts=0;
 std::vector<ObjectGuid> summons;
 explicit Player(unsigned id):guid{id}{} Player* ToPlayer()override{return this;}
 ObjectGuid GetGUID(){return guid;}uint32 GetMapId(){return map;}uint32 GetAreaId(){return area;}
 int GetQuestStatus(unsigned id){assert(id==39426 || id==40229);return id==39426?quest:next;}
 bool IsAlive(){return alive;}bool IsBeingTeleported(){return teleport;}bool isInFlight(){return flight;}bool GetVehicle(){return vehicle;}
 auto GetSummonList(unsigned entry){assert(entry==96453);return &summons;}
 void CastSpell(Player* target,unsigned spell,bool triggered){assert(target==this && spell==191109 && triggered);++casts;}
};
struct Creature:Unit {
 Player* owner;bool alive=true,despawn=false;unsigned inheritedSummons=0,inheritedUpdates=0;
 Creature(unsigned id,Player* p):owner(p){creatures[{id}]=this;if(p)p->summons.push_back({id});}
 TempSummon* ToTempSummon(){return owner?this:nullptr;}Unit* GetSummoner(){return owner;}
 bool IsAlive(){return alive;}bool IsDespawn(){return despawn;}void DespawnOrUnsummon(){despawn=true;}
};
struct ObjectAccessor {static Creature* GetCreature(Player&,ObjectGuid guid){auto it=creatures.find(guid);return it==creatures.end()?nullptr:it->second;}};
struct SmartAI {
 Creature* me;explicit SmartAI(Creature* c):me(c){}virtual ~SmartAI()=default;
 virtual void IsSummonedBy(Unit*){++me->inheritedSummons;}virtual void UpdateAI(uint32){++me->inheritedUpdates;}
};
struct PlayerScript {
 explicit PlayerScript(char const*){}virtual ~PlayerScript()=default;
 virtual void OnLogin(Player*){}virtual void OnLogout(Player*){}virtual void OnMapChanged(Player*){}virtual void OnUpdate(Player*,uint32){}
};
#include "HighmountainBloodDebt.inc"
int main()
{
 Player p(1),q(2);player_blood_debt_navarrogg watcher;
 for(int state:{QUEST_STATUS_INCOMPLETE,QUEST_STATUS_COMPLETE,QUEST_STATUS_REWARDED}){
  p.quest=state;unsigned before=p.casts;watcher.OnLogin(&p);assert(p.casts==before+1);
 }
 p.quest=QUEST_STATUS_NONE;auto before=p.casts;watcher.OnLogin(&p);assert(p.casts==before);
 p.quest=QUEST_STATUS_INCOMPLETE;
 for(int state:{QUEST_STATUS_INCOMPLETE,QUEST_STATUS_COMPLETE,QUEST_STATUS_REWARDED}){
  p.next=state;BloodDebt::Recover(&p);assert(p.casts==before);
 }
 p.next=QUEST_STATUS_NONE;
 for(uint32 area:{7725u,7810u,7829u}){p.area=area;BloodDebt::Recover(&p);assert(p.casts==++before);}
 p.area=7783;BloodDebt::Recover(&p);assert(p.casts==before);p.area=7810;
 for(bool* flag:{&p.teleport,&p.flight,&p.vehicle}){*flag=true;BloodDebt::Recover(&p);assert(p.casts==before);*flag=false;}
 p.alive=false;BloodDebt::Recover(&p);assert(p.casts==before);p.alive=true;
 p.map=1;BloodDebt::Recover(&p);assert(p.casts==before);p.map=1220;
 Creature nav(10,&p),foreign(11,&q);npc_blood_debt_navarrogg ai(&nav);ai.IsSummonedBy(&p);assert(nav.inheritedSummons==1);
 BloodDebt::Recover(&p);assert(p.casts==before);
 Creature duplicate(12,&p);npc_blood_debt_navarrogg dup(&duplicate);dup.IsSummonedBy(&p);assert(duplicate.despawn && !duplicate.inheritedSummons);
 nav.despawn=true;Creature replacement(13,&p);npc_blood_debt_navarrogg rep(&replacement);rep.IsSummonedBy(&p);assert(!replacement.despawn && replacement.inheritedSummons==1);
 rep.UpdateAI(1000);assert(replacement.inheritedUpdates==1);
 watcher.OnUpdate(&p,1);p.quest=QUEST_STATUS_NONE;watcher.OnUpdate(&p,1);assert(replacement.despawn && !foreign.despawn);
 p.quest=QUEST_STATUS_COMPLETE;Creature ender(14,&p);npc_blood_debt_navarrogg enderAI(&ender);enderAI.IsSummonedBy(&p);assert(!ender.despawn);
 p.next=QUEST_STATUS_INCOMPLETE;enderAI.UpdateAI(1000);assert(ender.despawn);
 p.next=QUEST_STATUS_NONE;p.quest=QUEST_STATUS_REWARDED;before=p.casts;
 watcher.OnUpdate(&p,1);assert(p.casts==before+1);watcher.OnUpdate(&p,1000);assert(p.casts==before+1);
 watcher.OnUpdate(&p,4000);assert(p.casts==before+2);
 Creature last(15,&p);watcher.OnLogout(&p);assert(last.despawn && !foreign.despawn);
 std::cout<<"PASS: active/complete/rewarded recovery, next-quest stop, area/map/death/transport guards, native summon, duplicate suppression, pending-despawn replacement, SmartAI delegation, owner-only abandonment/logout cleanup and retry throttle.\n";
}
