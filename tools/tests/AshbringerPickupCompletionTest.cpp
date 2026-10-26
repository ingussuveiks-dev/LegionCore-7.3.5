#include <cassert>
#include <cstdint>
#include <iostream>
#include <tuple>
#include <vector>
using uint8=uint8_t;
using uint32=uint32_t;
enum {CRITERIA_TYPE_SCRIPT_EVENT_2=92,CRITERIA_TYPE_COMPLETE_SCENARIO=152,CRITERIA_TYPE_COMPLETE_SCENARIO_COUNT=151};
#include "Constants.inc"
struct Player;
struct Unit {virtual ~Unit()=default;virtual Player* ToPlayer(){return nullptr;}};
struct InstanceScript;
struct Player:Unit
{
    InstanceScript* instance=nullptr;
    bool owned=false;
    std::vector<std::tuple<uint32,uint32,uint32>> credit;
    uint32 casts=0;
    Player* ToPlayer() override{return this;}
    InstanceScript* GetInstanceScript(){return instance;}
    bool HasItemCount(uint32 item){assert(item==120978);return owned;}
    void CastSpell(Player* target,uint32 spell,bool triggered){assert(target==this && spell==180850 && triggered);++casts;}
    void UpdateAchievementCriteria(uint32 type,uint32 value,uint32 quantity=0,uint32=0,Unit* =nullptr){credit.emplace_back(type,value,quantity);}
};
struct InstanceMap
{
    std::vector<Player*> players;
    template<class Callback>void ApplyOnEveryPlayer(Callback callback){for(Player* player:players)callback(player);}
};
struct InstanceScript
{
    InstanceMap* instance=nullptr;
    uint32 step=3;
    uint32 getScenarionStep(){return step;}
    void DoUpdateAchievementCriteria(uint32 type,uint32 miscValue1=0,uint32 miscValue2=0,uint32 miscValue3=0,Unit* unit=nullptr);
};
#include "InstanceCredit.inc"
struct Creature {uint32 despawns=0;void DespawnOrUnsummon(){++despawns;}};
struct GameObject
{
    Creature* trigger=nullptr;
    uint32 deletes=0;
    Creature* FindNearestCreature(uint32 entry,float radius){assert(entry==103454 && radius==10.0f);return trigger;}
    void Delete(){++deletes;}
};
struct Pickup
{
#include "Pickup.inc"
};
struct Step {bool IsBonusObjective()const{return false;}};
struct Scenario
{
    std::vector<Step*> steps;
    uint8 currentStep=0;
    uint32 scenarioId=775;
    bool rewarded=false;
#include "Completion.inc"
    void RewardModel(Player* player)
    {
        if(!IsCompleted(false)||rewarded)return;rewarded=true;
#include "RewardCredits.inc"
    }
};
int main()
{
    Step step;Scenario scenario;scenario.steps.assign(7,&step);scenario.currentStep=3;assert(!scenario.IsCompleted(false));
    Player player,party;InstanceMap map{{&player,&party}};InstanceScript instance{&map};player.instance=&instance;
    Pickup script;GameObject object;Creature trigger;object.trigger=&trigger;
#ifdef BASELINE
    script.OnLootStateChanged(&object,GO_JUST_DEACTIVATED,&player);
    assert(!scenario.IsCompleted(false) && player.credit.size()==2 && party.credit==player.credit);
    assert(player.credit[1]==std::make_tuple(152u,775u,1u) && player.casts==1);
    std::cout<<"PASS pinned85b4f0c2: actual stage3 pickup/instance Player fanout emits completion152/775 while native seven-step Scenario::IsCompleted is false; state is not actually advanced to completed\n";
#else
    for(uint32 stage=0;stage<8;++stage)for(uint32 state=0;state<5;++state)for(bool owned:{false,true})for(bool present:{false,true})
    {
        player.credit.clear();party.credit.clear();player.casts=0;player.owned=owned;instance.step=stage;object.trigger=present?&trigger:nullptr;object.deletes=0;trigger.despawns=0;
        script.OnLootStateChanged(&object,state,&player);bool admitted=stage==3&&(state==3||owned);
        assert(player.credit.size()==(admitted?1u:0u) && party.credit==player.credit && player.casts==(admitted?1u:0u));
        if(admitted)assert(player.credit.front()==std::make_tuple(92u,46130u,0u));
        assert(object.deletes==(admitted&&state==GO_ACTIVATED?1u:0u) && trigger.despawns==(admitted&&present?1u:0u));
    }
    player.credit.clear();party.credit.clear();Unit pet;script.OnLootStateChanged(&object,3,nullptr);script.OnLootStateChanged(&object,3,&pet);
    assert(player.credit.empty() && party.credit.empty());
    player.instance=nullptr;script.OnLootStateChanged(&object,3,&player);assert(player.credit.empty());player.instance=&instance;
    for(uint8 stage=0;stage<7;++stage){scenario.currentStep=stage;scenario.RewardModel(&player);assert(player.credit.empty());}
    scenario.currentStep=7;scenario.RewardModel(&player);scenario.RewardModel(&player);
    std::vector<std::tuple<uint32,uint32,uint32>> expectedCompletion={{152,775,1},{151,1,0}};
    assert(scenario.IsCompleted(false) && player.credit==expectedCompletion);
    std::cout<<"PASS160 actual pickup state/stage/item/actor cases retain46130/180850/trigger/delete without152; native IsCompleted/GetStepCount plus isolated Reward credit fragment only emits152/151 after7 modeled steps. Empty chest still starts native acquisition spell; accepted item storage/area eligibility/whole Reward/client traversal not exercised\n";
#endif
}
