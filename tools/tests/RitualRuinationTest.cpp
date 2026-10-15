#include <cassert>
#include <cstdint>
#include <cmath>
#include <functional>
#include <map>
#include <set>
#include <vector>
#include <iostream>
using uint32=uint32_t;
constexpr int CLASS_WARLOCK=9,QUEST_STATUS_INCOMPLETE=3,QUEST_STATUS_COMPLETE=1;
constexpr int MOVEMENTFLAG_MASK_MOVING=3,MOVEMENTFLAG_MASK_MOVING_OR_TURN=15,ROOT=16;
constexpr int TELE_TO_ZONE_MAP=32,TEMPSUMMON_MANUAL_DESPAWN=1;
#define TC_LOG_INFO(...) ((void)0)
struct ObjectGuid{int id=0;bool operator==(ObjectGuid const& o)const{return id==o.id;}static ObjectGuid Empty;};
ObjectGuid ObjectGuid::Empty;
struct Position{float x,y,z,o;};
struct Player;
struct Creature{bool alive=true;int entry=0;ObjectGuid guid;bool IsAlive(){return alive;}void SetPhaseId(std::set<uint32> const&,bool){}};
std::map<int,Creature> creatures;
struct ObjectAccessor{static Creature* GetCreature(Player&,ObjectGuid g){auto i=creatures.find(g.id);return i==creatures.end()?nullptr:&i->second;}};
struct Player{
 int cls=9,status=3,map=1220,zone=7543;bool reward=false,world=true,alive=true,tele=false,vehicle=false,transport=false,flight=false;
 bool aura=true,pet=false,savedPet=true,allowTeleport=true,allowSummon=true;
 int item=0,control=0,stop=0,spline=0,resummon=0,phase=0,binds=0,next=1,teleOptions=0;
 int flags=3;
 struct Movement {int* flags;bool HasMovementFlag(int f)const{return *flags&f;}} m_movementInfo{&flags};
#include "IsMoving.inc"
 Position pos{-666.047f,2613.08f,81.0715f,5.32719f};
 std::map<int,int> objectives;
 std::map<int,std::vector<ObjectGuid>> summons;
 std::vector<std::function<void()>> events;
 struct Phase{Player* p;void NotifyStoresReloaded(){++p->phase;}};
 int getClass(){return cls;}bool GetQuestRewardStatus(int q){assert(q==43254);return reward;}
 int GetQuestStatus(int q){assert(q==43254);return status;}
 int GetQuestObjectiveData(int q,int id){assert(q==43254);return objectives[id];}
 bool IsInWorld(){return world;}bool IsAlive(){return alive;}bool IsBeingTeleported(){return tele;}
 bool GetVehicle(){return vehicle;}bool GetTransport(){return transport;}bool isInFlight(){return flight;}
 int GetMapId(){return map;}int GetZoneId(){return zone;}
 std::set<uint32> const& GetPhases(){static std::set<uint32> phases{7028};return phases;}
 float GetExactDist(Position const* p){return std::sqrt(std::pow(pos.x-p->x,2)+std::pow(pos.y-p->y,2)+std::pow(pos.z-p->z,2));}
 bool HasAura(int id){assert(id==219917);return aura;}void RemoveAurasDueToSpell(int id){assert(id==219917);aura=false;}
 void StopMoving(){++stop;}void DisableSpline(){++spline;}
 void RemoveUnitMovementFlag(int f){flags&=~f;}void SetClientControl(Player* p,bool allow){assert(p==this && allow);++control;}
 void ResummonPetTemporaryUnSummonedIfAny(){++resummon;if(savedPet)pet=true;}
 Phase GetPhaseMgr(){return {this};}int GetGUIDLow(){return 1;}ObjectGuid GetGUID(){return {1};}
 void KilledMonsterCredit(int id,ObjectGuid){objectives[id]=1;}
 void AddQuestDelayedEvent(int q,int ms,std::function<void()> f){assert(q==43254 && ms==500);events.push_back(f);}
 bool TeleportTo(int m,Position const* p,int options){if(!allowTeleport)return false;map=m;pos=*p;teleOptions=options;tele=true;return true;}
 bool HasItemCount(int id,int amount){assert(id==128941 && amount==1);return item>=amount;}
 std::vector<ObjectGuid>* GetSummonList(int entry){return &summons[entry];}
 Creature* SummonCreature(int entry,Position const&,int type,int,int,ObjectGuid owner){assert(type==TEMPSUMMON_MANUAL_DESPAWN && owner==GetGUID());if(!allowSummon)return nullptr;int id=next++;creatures[id]={true,entry,{id}};summons[entry].push_back({id});return &creatures[id];}
 void BindQuestObject(int q,Creature*){assert(q==43254);++binds;}
 void Run(){auto current=std::move(events);events.clear();for(auto& f:current)f();}
};
#include "Ritual.inc"
void credits(Player& p,int n){for(int i=0;i<n;++i)p.objectives[RitualRuination::Objectives[i]]=1;}
int main(){
 using namespace RitualRuination;
 Player p;assert(p.isMoving());p.flags|=ROOT;FinishLanding(&p);
 assert(!p.isMoving() && p.flags==ROOT && !p.aura && p.control==1 && p.pet && p.resummon==1 && p.phase==1);
 FinishLanding(&p);assert(p.control==1); // one cleanup, not a recurring movement reset
 for(int mode=0;mode<7;++mode){Player q;if(mode==0)q.map=1;if(mode==1)q.zone=7502;if(mode==2)q.tele=true;if(mode==3)q.vehicle=true;if(mode==4)q.transport=true;if(mode==5)q.alive=false;if(mode==6)q.status=0;FinishLanding(&q);assert(q.aura && q.isMoving() && !q.control);}
 Player dismissed;dismissed.savedPet=false;FinishLanding(&dismissed);assert(!dismissed.pet);
 Player travel;travel.pos=BatSource;travel.zone=8445;assert(StartTravel(&travel));assert(travel.teleOptions==TELE_TO_ZONE_MAP && !travel.isMoving() && Stage(&travel)==0);
 travel.Run();assert(Stage(&travel)==0);travel.tele=false;travel.zone=7543;travel.Run();assert(Stage(&travel)==1 && !travel.aura && travel.pet);
 Player failed;failed.pos=BatSource;failed.zone=8445;failed.allowTeleport=false;assert(!StartTravel(&failed) && Stage(&failed)==0 && failed.events.empty());
 Player timeout;timeout.pos=BatSource;timeout.zone=8445;StartTravel(&timeout);for(int i=0;i<62;++i)timeout.Run();assert(Stage(&timeout)==0 && timeout.events.empty());
 Player abandoned;abandoned.pos=BatSource;abandoned.zone=8445;StartTravel(&abandoned);abandoned.status=0;abandoned.Run();assert(abandoned.objectives[110523]==0 && abandoned.events.empty());
 Player seq;seq.aura=false;assert(Stage(&seq)==0 && !CanPickup(&seq) && !CanRuin(&seq));
 for(int i=0;i<6;++i){assert(Stage(&seq)==unsigned(i));seq.objectives[Objectives[i]]=1;}assert(Stage(&seq)==6);
 // Every missing prerequisite prevents out-of-order pickup and sabotage.
 for(int missing=0;missing<3;++missing){Player q;credits(q,3);q.objectives[Objectives[missing]]=0;q.pos=Scepter;q.item=1;assert(!CanPickup(&q) && !CanRuin(&q));}
 Player loot;credits(loot,3);loot.pos=Scepter;loot.aura=false;assert(CanPickup(&loot) && !CanRuin(&loot));
 Recover(&loot);assert(Owned(&loot,110548) && loot.binds==1);Recover(&loot);assert(loot.binds==1);
 loot.item=1;assert(CanPickup(&loot));loot.objectives[110603]=1;assert(!CanPickup(&loot) && CanRuin(&loot));
 loot.item=0;assert(!CanRuin(&loot));loot.item=1;loot.objectives[110606]=1;assert(!CanRuin(&loot));
 Recover(&loot);assert(Owned(&loot,110800));int binds=loot.binds;Recover(&loot);assert(loot.binds==binds);
 Player retry;credits(retry,5);retry.allowSummon=false;Recover(&retry);assert(!Owned(&retry,110800));retry.allowSummon=true;Recover(&retry);assert(Owned(&retry,110800));
 Player returning;credits(returning,5);returning.aura=false;returning.tele=true;ConfirmEscape(&returning,3);assert(Stage(&returning)==5);returning.Run();assert(Stage(&returning)==5);
 returning.tele=false;returning.zone=7502;returning.pos=Dalaran;returning.Run();assert(Stage(&returning)==6 && Owned(&returning,101097));
 Player wrong;credits(wrong,5);wrong.pos.x+=1000;ConfirmEscape(&wrong,2);wrong.Run();wrong.Run();assert(Stage(&wrong)==5 && wrong.events.empty());
 Player other;other.status=0;Recover(&other);assert(!other.binds);other.status=3;other.reward=true;Recover(&other);assert(!other.binds);
 assert(!Active(nullptr));std::cout<<"PASS: production six-step gates, real-arrival credit, zone transfer, stale controller/movement cleanup, pet reload scope, item ownership, personal recovery/retry and confirmed escape\n";
}
