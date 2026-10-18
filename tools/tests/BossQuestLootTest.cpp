#include <cassert>
#include <cstdint>
#include <iostream>
#include <vector>
using uint32=uint32_t;using uint8=uint8_t;
enum{LOOT_ITEM_TYPE_ITEM=0,LOOT_ITEM_TYPE_CURRENCY=1,EXPANSION_LEGION=6,RATE_DROP_ITEM_REFERENCED=1,RATE_DROP_ZONE_LOOT=2,RATE_DROP_CURRENCY=3,CONFIG_DROP_DUNGEON_ONLY_X1=1};
#define TC_LOG_DEBUG(...) ((void)0)
int qualityToRate[]={0};float rowRoll=0;bool roll_chance_i(uint32 n){return n>0;}bool roll_chance_f(float n){return rowRoll<n;}
struct World{float getRate(int){return 1;}bool getBoolConfig(int){return true;}}world;auto sWorld=&world;
struct ItemTemplate{uint32 GetQuality()const{return 0;}};
struct ObjectMgr{ItemTemplate item;bool missing=false;ItemTemplate const* GetItemTemplate(uint32){return missing?nullptr:&item;}}manager;auto sObjectMgr=&manager;
struct Map{bool raid=true;bool IsRaid()const{return raid;}bool IsDungeon()const{return true;}};
struct Player{Map map;bool allowed=true;Map const* GetMap()const{return &map;}bool CanGetItemForLoot(ItemTemplate const*,bool)const{return allowed;}};
struct LootStoreItem{
 uint32 itemid=0,currencyid=0,reference=0,type=0,lootmode=0,mincount=1,maxcount=1;float chance=100;bool needs_quest=false;
 LootStoreItem(uint32 item=0,uint32 currency=0,uint32 ref=0,float c=100,bool quest=false,uint32 mode=0,uint32=0,uint32 lo=1,uint32 hi=1):itemid(item),currencyid(currency),reference(ref),type(currency?1:0),lootmode(mode),mincount(lo),maxcount(hi),chance(c),needs_quest(quest){}
 bool Roll(bool,bool=false,bool=false)const;
};
#include "BossRowRoll.inc"
using LootStoreItemList=std::vector<LootStoreItem>;
struct Loot{
 Player* owner;bool isBoss=true,bonusLoot=false,_specCheck=false;uint32 chance=0,_DifficultyMask=1,_ExpansionID=6,money=0,groups=0;
 std::vector<LootStoreItem> selected;Player const* GetLootOwner()const{return owner;}
 void AddItem(LootStoreItem const& i){selected.push_back(i);}void generateMoneyLoot(uint32,uint32,bool){++money;}
};
struct LootTemplate{
 struct Group{void ProcessBossLoot(Loot& l,LootTemplate const*)const{++l.groups;}};
 using LootGroups=std::vector<Group>;LootGroups AutoGroups{{}};std::vector<LootStoreItem> Entries;bool condition=true;
 bool CheckItemCondition(Player const*,uint32,uint32)const{return condition;}
 void ProcessBossLoot(Loot&)const;void OldProcessBossLoot(Loot&)const;
};
#include "BossLoot.inc"
#include "OldBossLoot.inc"
int main(){
 Player p;LootTemplate t;t.Entries={{142487,0,0,100,true,0,0,30,60},{999,0,0,100,false},{0,42,0,100,false}};
 {Loot old{&p};t.OldProcessBossLoot(old);assert(old.selected.empty());}
 for(uint32 mask:{1u,2u,4u,8u}){Loot l{&p};l._DifficultyMask=mask;t.ProcessBossLoot(l);assert(l.selected.size()==1&&l.selected[0].itemid==142487&&!l.groups);}
 {Loot l{&p};l.chance=100;t.ProcessBossLoot(l);assert(l.selected.size()==3&&l.groups==1);}
 {Loot l{&p};auto saved=t.Entries;t.Entries={{0,42,0,100,true}};t.ProcessBossLoot(l);assert(l.selected.empty());t.Entries=saved;}
 {Loot l{&p};l.bonusLoot=true;t.ProcessBossLoot(l);assert(l.selected.size()==1&&l.selected[0].itemid==147581);}
 {Loot l{&p};l.bonusLoot=true;l.chance=100;t.ProcessBossLoot(l);assert(l.selected.empty()&&l.groups==1);}
 {Loot l{&p};l.bonusLoot=true;l._ExpansionID=5;t.ProcessBossLoot(l);assert(l.selected.empty()&&l.money==1);}
 {Loot l{&p};t.condition=false;t.ProcessBossLoot(l);assert(l.selected.empty());t.condition=true;}
 {Loot l{&p};p.allowed=false;t.ProcessBossLoot(l);assert(l.selected.empty());p.allowed=true;}
 {Loot l{&p};manager.missing=true;t.ProcessBossLoot(l);assert(l.selected.empty());manager.missing=false;}
 {Loot l{&p};t.Entries[0].lootmode=2;t.ProcessBossLoot(l);assert(l.selected.empty());t.Entries[0].lootmode=0;}
 {Loot l{&p};t.Entries[0].chance=50;rowRoll=60;t.ProcessBossLoot(l);assert(l.selected.empty());rowRoll=40;t.ProcessBossLoot(l);assert(l.selected.size()==1);rowRoll=0;t.Entries[0].chance=100;}
 {Loot l{&p};p.map.raid=false;t.ProcessBossLoot(l);assert(l.selected.size()==3);}
 std::cout<<"PASS: old failed-gear-roll quest loss reproduced; actual boss processor and row RNG preserve quest drops, masks, conditions, item admission, item chance, ordinary/bonus/group/currency/dungeon behavior. Loot storage, quest visibility and weekly cooldown are separate services.\n";
}
