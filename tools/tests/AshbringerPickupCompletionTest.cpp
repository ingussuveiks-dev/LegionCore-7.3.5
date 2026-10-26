#include <cassert>
#include <cstdint>
#include <iostream>
#include <tuple>
#include <vector>
using uint8=uint8_t;
using uint32=uint32_t;
using ItemPosCountVec=std::vector<uint32>;
enum {CRITERIA_TYPE_SCRIPT_EVENT_2=92,CRITERIA_TYPE_COMPLETE_SCENARIO=152,CRITERIA_TYPE_COMPLETE_SCENARIO_COUNT=151};
#include "Constants.inc"
uint32 warnings=0;
#define TC_LOG_WARN(...) (++warnings)
struct Player;
struct Unit {virtual ~Unit()=default;virtual Player* ToPlayer(){return nullptr;}};
struct InstanceScript;
struct Player:Unit
{
    InstanceScript* instance=nullptr;
    bool owned=false;
    std::vector<std::tuple<uint32,uint32,uint32>> credit;
    uint32 casts=0;
    SpellCastResult castResult=SPELL_CAST_OK;
    InventoryResult capacity=EQUIP_ERR_OK;
    mutable uint32 capacityChecks=0;
    std::vector<InventoryResult> inventoryErrors;
    Player* ToPlayer() override{return this;}
    InstanceScript* GetInstanceScript(){return instance;}
    bool HasItemCount(uint32 item){assert(item==120978);return owned;}
    InventoryResult CanStoreItem(uint8 bag,uint8 slot,ItemPosCountVec& destination,uint32 item,uint32 count,void* existing,bool swap,uint32* noSpace,bool quest)const
    {
        assert(bag==NULL_BAG&&slot==NULL_SLOT&&destination.empty()&&item==120978&&count==1&&!existing&&!swap&&!noSpace&&!quest);
        ++capacityChecks;return capacity;
    }
#include "CapacityBridge.inc"
    void SendEquipError(InventoryResult result,void*,void*,uint32 item){assert(result!=EQUIP_ERR_OK&&item==120978);inventoryErrors.push_back(result);}
    uint32 GetGUIDLow()const{return 1;}
    SpellCastResult CastSpell(Player* target,uint32 spell,bool triggered){assert(target==this && spell==180850 && triggered);++casts;return castResult;}
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
struct GameObjectAI {void OnStateChanged(LootState,Unit*){}};
struct GameObject
{
    Creature* trigger=nullptr;
    uint32 deletes=0;
    LootState m_lootState=GO_READY;
    GameObjectAI* AI(){static GameObjectAI passive;return &passive;}
    void SetLootState(LootState,Unit* =nullptr);
    Creature* FindNearestCreature(uint32 entry,float radius){assert(entry==103454 && radius==10.0f);return trigger;}
    void Delete(){++deletes;}
};
struct Pickup
{
#include "Pickup.inc"
};
Pickup* activePickup=nullptr;
struct ScriptManager {void OnGameObjectLootStateChanged(GameObject* object,LootState state,Unit* target){activePickup->OnLootStateChanged(object,state,target);}} scriptManager;
auto* sScriptMgr=&scriptManager;
#include "SetLootState.inc"
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
    Pickup script;activePickup=&script;GameObject object;Creature trigger;object.trigger=&trigger;
#ifdef BASELINE
    script.OnLootStateChanged(&object,GO_JUST_DEACTIVATED,&player);
    assert(!scenario.IsCompleted(false) && player.credit.size()==2 && party.credit==player.credit);
    assert(player.credit[1]==std::make_tuple(152u,775u,1u) && player.casts==1);
    std::cout<<"PASS pinned85b4f0c2: actual stage3 pickup/instance Player fanout emits completion152/775 while native seven-step Scenario::IsCompleted is false; state is not actually advanced to completed\n";
#elif defined(CAPACITY_BASELINE)
    player.capacity=EQUIP_ERR_INV_FULL;object.SetLootState(GO_JUST_DEACTIVATED,&player);
    assert(player.credit.size()==1&&party.credit==player.credit&&player.casts==1&&trigger.despawns==1);
    assert(!player.capacityChecks&&player.inventoryErrors.empty()&&object.m_lootState==GO_JUST_DEACTIVATED);
    std::cout<<"PASS pinnedbf251a6d capacity counterfactual: full inventory does not prevent original46130/retrieval/trigger cleanup and completed state; no capacity query/error/reopen\n";
#elif defined(CAST_BASELINE)
    player.castResult=SPELL_FAILED_ERROR;object.SetLootState(GO_JUST_DEACTIVATED,&player);
    assert(player.credit.size()==1&&party.credit==player.credit&&player.casts==1&&trigger.despawns==1&&!warnings);
    assert(object.m_lootState==GO_JUST_DEACTIVATED);
    std::cout<<"PASS pinned269550a6 reported rejection still emits46130/removes103454 and retains completed state despite failed180850; no rejection diagnostic or reopen\n";
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
    instance.step=3;
    for(InventoryResult failure:{EQUIP_ERR_INV_FULL,EQUIP_ERR_ITEM_MAX_COUNT,EQUIP_ERR_ITEM_NOT_FOUND})for(bool owned:{false,true})for(bool present:{false,true})
    {
        player.credit.clear();party.credit.clear();player.casts=0;player.capacityChecks=0;player.inventoryErrors.clear();player.owned=owned;player.capacity=failure;
        trigger.despawns=0;object.trigger=present?&trigger:nullptr;object.deletes=0;
        object.SetLootState(GO_JUST_DEACTIVATED,&player);
        assert(player.capacityChecks==uint32(!owned)&&player.inventoryErrors.size()==size_t(!owned));
        assert(player.credit.size()==size_t(owned)&&party.credit==player.credit&&player.casts==uint32(owned));
        assert(trigger.despawns==uint32(owned&&present)&&!object.deletes);
        assert(object.m_lootState==(owned?GO_JUST_DEACTIVATED:GO_READY));
        if(!owned)
        {
            assert(player.inventoryErrors.front()==failure);
            object.SetLootState(GO_JUST_DEACTIVATED,&player);
            assert(player.credit.empty()&&party.credit.empty()&&!player.casts&&player.capacityChecks==2&&player.inventoryErrors.size()==2);
            assert(object.m_lootState==GO_READY&&!trigger.despawns);
            player.capacity=EQUIP_ERR_OK;object.SetLootState(GO_JUST_DEACTIVATED,&player);
            assert(player.credit.size()==1&&party.credit==player.credit&&player.casts==1&&player.capacityChecks==3);
            assert(trigger.despawns==uint32(present)&&object.m_lootState==GO_JUST_DEACTIVATED);
        }
    }
    for(uint32 wrongStage:{0u,2u,4u,7u})
    {
        player.credit.clear();party.credit.clear();player.casts=0;player.capacityChecks=0;player.inventoryErrors.clear();player.owned=false;player.capacity=EQUIP_ERR_INV_FULL;
        instance.step=wrongStage;object.SetLootState(GO_JUST_DEACTIVATED,&player);
        assert(player.credit.empty()&&party.credit.empty()&&!player.casts&&!player.capacityChecks&&player.inventoryErrors.empty());
    }
    instance.step=3;player.capacity=EQUIP_ERR_OK;
    for(SpellCastResult failure:{SPELL_FAILED_ERROR,SPELL_FAILED_DONT_REPORT,SPELL_FAILED_NOT_READY})for(bool owned:{false,true})for(bool present:{false,true})for(LootState state:{GO_JUST_DEACTIVATED,GO_ACTIVATED})
    {
        player.credit.clear();party.credit.clear();player.casts=0;player.castResult=failure;player.owned=owned;warnings=0;
        object.trigger=present?&trigger:nullptr;trigger.despawns=0;object.deletes=0;
        object.SetLootState(state,&player);bool admitted=state==GO_JUST_DEACTIVATED||owned;
        assert(player.credit.empty()&&party.credit.empty()&&!trigger.despawns&&!object.deletes&&player.casts==uint32(admitted)&&warnings==uint32(admitted));
        assert(object.m_lootState==(admitted?GO_READY:state));
        if(admitted)
        {
            object.SetLootState(state,&player);
            assert(player.credit.empty()&&party.credit.empty()&&!trigger.despawns&&!object.deletes&&player.casts==2&&warnings==2);
            player.castResult=SPELL_CAST_OK;object.SetLootState(state,&player);
            assert(player.credit.size()==1&&party.credit==player.credit&&player.casts==3&&warnings==2);
            assert(trigger.despawns==uint32(present)&&object.deletes==uint32(state==GO_ACTIVATED));
        }
    }
    std::cout<<"PASS24 reported-error/owned/actor/state combinations:18 admitted attempts retain no direct progression/cleanup/Delete, native READY permits repeated-error then accepted retry. SPELL_CAST_OK only admits original work, not proof of delayed199827 storage, effect completion or client/VFX\n";
    std::cout<<"PASS12 capacity-result/owned/missing-actor cases retain native errors and reopen before all original effects; six projected capacity recoveries retry original retrieval without an item-first gate or direct grant. Native CanStoreNewItem bridge/SetLootState/Instance fanout extracted; inventory services and accepted item creation remain controlled/separate\n";
    std::cout<<"PASS160 actual pickup state/stage/item/actor cases retain46130/180850/trigger/delete without152; native IsCompleted/GetStepCount plus isolated Reward credit fragment only emits152/151 after7 modeled steps. Empty chest still starts native acquisition spell; accepted item storage/area eligibility/whole Reward/client traversal not exercised\n";
#endif
}
