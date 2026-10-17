#include <cassert>
#include <array>
#include <sstream>
struct Garrison {bool GetPlotWithBuildingType(unsigned){assert(false);return false;}};
#include "BuiltScaleFixture.inc"
constexpr uint32 DISABLE_TYPE_QUEST=1,INVALIDREASON_QUEST_ALREADY_ON=2;
namespace DisableMgr {bool IsDisabledFor(uint32,uint32,Player*){return false;}}
#include "BuiltScaleStatus.inc"
#include "BuiltScaleStatusGate.inc"
#include "BuiltScaleAdmission.inc"
struct StarterItem:WorldObject {explicit StarterItem(uint32 id){entry=id;}ItemTemplate const* GetTemplate()const{return sObjectMgr->GetItemTemplate(entry);}bool hasQuest(uint32)const;};
#include "BuiltScaleHasQuest.inc"
void Player::PickUp(StarterItem* item)
{
    #include "BuiltScalePickup.inc"
}
constexpr uint32 ITEM_FLAG_HIDE_UNUSABLE_RECIPE=0x02000000,ITEM_FLAG2_FACTION_HORDE=1,ITEM_FLAG2_FACTION_ALLIANCE=2,ITEM_FLAGS_CU_IGNORE_QUEST_STATUS=2,GARR_BTYPE_WARMILL=1,CURRENCY_CATEGORY_ARCHAEOLOGY=2;
struct SkillLineAbilityEntry {uint32 SupercedesSpell=0,MinSkillLineRank=0;};
struct DB2Manager {SkillLineAbilityEntry const* GetSkillBySpell(uint32){assert(false);return nullptr;}} sDB2Manager;
#include "BuiltScaleAllowed.inc"
void Take(Player& player,Loot& loot,bool max=false)
{
    for(auto const& drop:loot.generated)if(loot.AllowedForPlayer(&player,drop.itemid,0,LOOT_ITEM_TYPE_ITEM,drop.needs_quest)){
        uint32 count=max?drop.maxcount:drop.mincount;player.bags[drop.itemid]+=count;player.ItemAddedQuestCheck(drop.itemid,count);
        if(sObjectMgr->GetItemTemplate(drop.itemid)->GetStartQuestID()){StarterItem starter(drop.itemid);player.PickUp(&starter);}
    }
}
int main(int argc,char** argv)
{
    _set_abort_behavior(0,_WRITE_ABORT_MSG|_CALL_REPORTFAULT);_CrtSetReportMode(_CRT_ASSERT,_CRTDBG_MODE_FILE);_CrtSetReportFile(_CRT_ASSERT,_CRTDBG_FILE_STDERR);
    for(auto item:{122609u,122610u,124037u}){auto& value=itemStore.items[item];value.stack=item==122609?50:1;value.start=item==122610?38337:item==124037?38616:0;}
    assert(argc==2);std::ifstream input(argv[1]);assert(input);std::string line;std::map<std::pair<uint32,uint32>,LootStoreItem> drops;
    while(std::getline(input,line)){std::istringstream in(line);std::string kind;in>>kind;
        if(kind=="quest"){uint32 id,package,start,next,startScript;in>>id;auto& q=store.quests[id];q.id=id;in>>q.flags>>q.FlagsEx>>q.AllowableRaces>>package>>start>>next>>startScript>>q.PrevQuestID>>q.NextQuestID>>q.ExclusiveGroup>>q.special;assert(package==651&&!start&&!next&&!startScript&&!q.PrevQuestID&&!q.NextQuestID&&!q.ExclusiveGroup&&!q.special);q.special|=QUEST_SPECIAL_FLAGS_DELIVER;}
        if(kind=="objective"){uint32 qid,id;int index;QuestObjective o;in>>qid>>id>>o.Type>>o.ObjectID>>o.Amount>>index>>o.Flags;o.StorageIndex=int8(index);assert(o.Type==1&&o.ObjectID==122609&&o.Amount==50&&!index&&!o.Flags);store.quests[qid].objectives.push_back(o);}
        if(kind=="loot"){uint32 entry,item,required;in>>entry>>item;auto& drop=drops[{entry,item}];drop.itemid=item;in>>drop.chance>>required>>drop.lootmode>>drop.groupid>>drop.mincount>>drop.maxcount;drop.needs_quest=required;assert(!drop.lootmode&&!drop.groupid&&drop.chance>0&&drop.chance<100);assert(drop.IsValid(lootStore,entry));}
    }
    assert(drops.size()==7&&store.quests.size()==2);
    for(auto team:{ALLIANCE,HORDE})for(bool max:{false,true}){
        uint32 id=team==ALLIANCE?38337:38616,starterId=team==ALLIANCE?122610:124037,wrongId=team==ALLIANCE?124037:122610;
        auto& q=store.quests[id];assert(q.objectives.size()==1);Player player;player.team=team;player.race=team==ALLIANCE?1:2;Loot loot;loot.owner=&player;
        assert(!player.HasQuestForItem(122609)&&!loot.AllowedForPlayer(&player,122609,0,LOOT_ITEM_TYPE_ITEM,true));
        StarterItem wrong(wrongId),starter(starterId);assert(starter.hasQuest(id)&&!wrong.hasQuest(id));
        assert(loot.AllowedForPlayer(&player,wrongId,0,LOOT_ITEM_TYPE_ITEM,false));player.PickUp(&wrong);assert(player.quests.empty()&&player.menu.detailed.empty()); // Native unrestricted item can drop, but cannot start the wrong faction's quest.
        player.canAdd=false;player.PickUp(&starter);assert(player.quests.empty());player.canAdd=true;
        LootTemplate starterTable;starterTable.Entries={drops.at({91202,starterId})};starterTable.Process(loot,true);assert(loot.generated.size()==1);Take(player,loot);
        assert(player.GetQuestStatus(id)==QUEST_STATUS_INCOMPLETE&&player.menu.detailed.size()==1&&!player.completions);
        player.KilledMonsterCredit(91202);assert(!player.GetQuestObjectiveData(id,122609));
        assert(!loot.AllowedForPlayer(&player,starterId,0,LOOT_ITEM_TYPE_ITEM,false));player.PickUp(&starter);assert(player.menu.detailed.size()==1); // Active status prevents restarting.
        uint32 kills=0;
        while(player.bags[122609]<50){
            auto entry=std::array<uint32,3>{91202,91486,91799}[kills%3];LootTemplate table;table.Entries={drops.at({entry,122609})};Loot next;next.owner=&player;
            rolled=100;table.Process(next,true);assert(next.generated.empty());rolled=0;table.Process(next,true);assert(next.generated.size()==1);
            bool takeMax=max&&player.bags[122609]<=35;Take(player,next,takeMax);++kills;
            assert(player.GetQuestObjectiveData(id,122609)==int32(player.bags[122609]));assert(player.completions==(player.bags[122609]==50?1:0));
        }
        assert(player.CanRewardQuest(&q,false)&&!player.HasQuestForItem(122609)&&!loot.AllowedForPlayer(&player,122609,0,LOOT_ITEM_TYPE_ITEM,true));
        player.bags[122609]=49;player.bank[122609]=1;assert(!player.CanRewardQuest(&q,false));player.bank[122609]=0;player.bags[122609]=50;
        player.ItemRemovedQuestCheck(122609,1);player.bags[122609]=49;assert(player.GetQuestStatus(id)==QUEST_STATUS_INCOMPLETE&&player.HasQuestForItem(122609));
        player.bags[122609]=50;player.ItemAddedQuestCheck(122609,1);assert(player.CanRewardQuest(&q,false));Consume(player,&q);assert(!player.bags[122609]);
        player.quests.erase(id);player.m_RewardedQuests.insert(id);assert(player.GetQuestStatus(id)==QUEST_STATUS_REWARDED&&!player.CanTakeQuest(&q,false));player.PickUp(&starter);assert(player.menu.detailed.size()==1);
        Player relog;relog.team=team;relog.race=player.race;relog.m_RewardedQuests=player.m_RewardedQuests;assert(!relog.CanTakeQuest(&q,false));
        Player retry;retry.team=team;retry.race=player.race;retry.PickUp(&starter);retry.bags[122609]=49;retry.AdjustQuestReqItemCount(&q);assert(retry.GetQuestObjectiveData(id,122609)==49&&!retry.CanCompleteQuest(id));
        retry.quests.clear();retry.data.clear();retry.PickUp(&starter);retry.AdjustQuestReqItemCount(&q);assert(retry.GetQuestObjectiveData(id,122609)==49&&retry.HasQuestForItem(122609));
    }
    std::cout<<"PASS: native item pickup/start, Item::hasQuest, full CanTakeQuest dispatcher with native status/race/previous checks, AllowedForPlayer and ordinary loot Process/Roll; both factions, wrong starter, log-full/active/rewarded refusal, actual50 scale item lifecycle, misses/min/max drops, bank/retry/history and reward consumption. Other admission/world/inventory/packet services are fixtures; client pending.\n";
}
