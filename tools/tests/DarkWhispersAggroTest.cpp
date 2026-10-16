#include <cassert>
#include <cstdint>
#include <list>
#include <iostream>
using uint32=uint32_t;
constexpr int SMART_EVENT_UPDATE_OOC=1,SMART_ACTION_ATTACK_START=49,SMART_TARGET_CLOSEST_PLAYER=21;
struct Unit {virtual ~Unit()=default;Unit* ToUnit(){return this;}};
struct Player:Unit{};
struct Creature:Unit {
 bool combat=false,IsAIEnabled=true;Player* nearby=nullptr;Unit* victim=nullptr;float range=0;int attacks=0,callbacks=0;
 bool isInCombat(){return combat;}Creature* AI(){return this;}void AttackStart(Unit* p){victim=p;combat=true;++attacks;}
 Player* SelectNearestPlayer(float distance){range=distance;return nearby;}
 Creature* ToCreature(){return this;}bool IsCreature(){return true;}void JustSummoned(Creature*){++callbacks;}
 void IsSummonedBy(Creature*){++callbacks;}
};
struct TempSummon:Creature {Creature* owner=nullptr;Creature* GetSummoner(){return owner;}void InitSummon();};
#include "InitSummon.inc"
using ObjectList=std::list<Unit*>;
struct SmartScriptHolder {struct {struct {uint32 repeatMin=1000,repeatMax=1000;}minMaxRepeat;}event;
 struct {struct {uint32 dist=50;}playerDistance;}target;};
struct SmartScript{
 Creature* me;int attempts=0,repeats=0;bool IsUnit(Unit* p){return p!=nullptr;}
 void RecalcTimer(SmartScriptHolder&,uint32 min,uint32 max){assert(min==1000&&max==1000);++repeats;}
 ObjectList* GetTargets(SmartScriptHolder& e,Unit*){auto l=new ObjectList;switch(SMART_TARGET_CLOSEST_PLAYER){
#include "ClosestPlayer.inc"
 }return l;}
 void ProcessAction(SmartScriptHolder& e){++attempts;Unit* unit=nullptr;switch(SMART_ACTION_ATTACK_START){
#include "AttackAction.inc"
 }}
 void Poll(SmartScriptHolder& e){switch(SMART_EVENT_UPDATE_OOC){
#include "OocEvent.inc"
 }}
};
int main(){
 TempSummon demon;demon.InitSummon();assert(!demon.callbacks); // the old JUST_SUMMONED event has no invoker
 SmartScript script{&demon};SmartScriptHolder event;
 for(int i=0;i<100;++i)script.Poll(event);assert(!demon.attacks&&script.attempts==100);
 Player player;demon.nearby=&player;script.Poll(event);assert(demon.victim==&player&&demon.attacks==1&&demon.range==50);
 for(int i=0;i<10000;++i)script.Poll(event);assert(demon.attacks==1&&script.attempts==101);
 // Reacquire after an evade even if the original Mephistroth summoner vanished.
 demon.combat=false;script.Poll(event);assert(demon.attacks==2&&demon.victim==&player);
 Creature summoner;TempSummon owned;owned.owner=&summoner;owned.InitSummon();assert(owned.callbacks==1&&summoner.callbacks==1);
 std::cout<<"PASS: native ownerless summon callback gap, OOC event, nearest-player selector and attack action; delayed player arrival, active-combat exclusion and evade retry\n";
}
