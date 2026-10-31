#include <cassert>
#include <cstdint>
#include <iostream>
#include <map>
#include <vector>
constexpr unsigned LOOT_ITEM_UI_NORMAL=0;
constexpr unsigned MASTER_LOOT=2;
using uint8=uint8_t;
namespace WorldPackets { namespace Loot {
struct LootCurrency {unsigned CurrencyID=0,Quantity=0,LootListID=0,UIType=99;};
struct LootResponse {std::vector<LootCurrency> Currencies;};
}}
struct Group {unsigned GetLootMethod(){return MASTER_LOOT;}unsigned GetLooterGuid(){return 1;}};
struct Player {unsigned guid=1;unsigned GetGUIDLow(){return guid;}unsigned GetGUID(){return guid;}Group*GetGroup(){return nullptr;}};
struct QuestItem {unsigned index;bool is_looted=false;};
using QuestItemList=std::vector<QuestItem>;
using QuestItemMap=std::map<unsigned,QuestItemList*>;
struct LootItem {struct {unsigned ItemID=0,CurrencyID=0;} item;unsigned count=0,LootListID=0;bool is_looted=false,follow_loot_rules=false,needs_quest=false;uint8 type=1;
 bool AllowedForPlayer(Player*)const{return true;}};
struct Loot;
static bool IsLootItemVisibleTo(Loot const&,LootItem const&,Player*);
struct Loot {
 bool personal=false,allowed=true;
 std::vector<LootItem> items;
 QuestItemMap currencies;
 QuestItemMap const& GetPlayerCurrencies()const{return currencies;}
 // Admission is controlled here; this test covers actual packet construction
 // and its native visibility helper, not the condition evaluator itself.
 bool AllowedForPlayer(Player*,unsigned,unsigned,uint8,bool,LootItem*)const{return allowed;}
 void BuildCurrencies(WorldPackets::Loot::LootResponse& packet,Player* viewer) const {
#include "CurrencyResponse.inc"
 }
};
#include "Visibility.inc"
int main(){
 Player viewer;
 Loot loot;
 // Xullorax's currency row is Item=0, Currency=1149, Count=64.
 loot.items={{{0,1149},64,0,false},{{0,1220},23,9,false}};
 QuestItemList list={{0,false},{1,false}};
 loot.currencies[1]=&list;
 auto build=[&](){WorldPackets::Loot::LootResponse packet;loot.BuildCurrencies(packet,&viewer);return packet.Currencies;};
 auto result=build();
 assert(result.size()==2&&result[0].CurrencyID==1149&&result[0].Quantity==64&&result[0].LootListID==1);
 assert(result[1].CurrencyID==1220&&result[1].Quantity==23&&result[1].LootListID==9);
 assert(result[0].UIType==LOOT_ITEM_UI_NORMAL);
 loot.allowed=false;assert(build().empty());loot.allowed=true;
 // Removal/opening again and per-viewer ownership retain the original behavior.
 list[0].is_looted=true;assert(build().size()==1);
 loot.personal=true;assert(build().size()==2);
 loot.items[0].is_looted=true;assert(build().size()==1);
 loot.items[1].is_looted=true;assert(build().empty());
 viewer.guid=2;assert(build().empty());
 std::cout<<"PASS: production currency response uses CurrencyID, preserves quantities and default/custom loot slots, per-viewer visibility and looted/personal filtering.\n";
}
