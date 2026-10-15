// Extracted production quest handlers; world storage, inventory operations,
// object lookup and packet delivery use the existing Heart handoff fixture.
#include <cstring>
#include <fstream>
#include "RegalRemainsFixture.inc"
#include "HasQuestForItem.inc"
#include "ItemAddedQuestCheck.inc"
#include "ItemRemovedQuestCheck.inc"
#include "AdjustQuestReqItemCount.inc"

enum {GAMEOBJECT_TYPE_CHEST=3,GAMEOBJECT_TYPE_FISHINGHOLE=25,
    GAMEOBJECT_TYPE_GATHERING_NODE=50,GAMEOBJECT_TYPE_CHALLENGE_MODE_REWARD=51};
struct GameObjectTemplate {
    uint32 type=GAMEOBJECT_TYPE_CHEST;
    struct {uint32 chestLoot=0;} chest,fishingHole,gatheringNode,challengeModeReward;
    uint32 GetLootId() const;
};
#include "GetLootId.inc"
constexpr uint32 QUEST_FLAGS_EX_KEEP_ADDITIONAL_ITEMS=0x1;

// Actual item-consumption case from RewardQuest. The entry deletion operation
// is a fixture: this does not execute the full reward or inventory subsystem.
void Consume(Player& player, Quest const* quest)
{
    auto DestroyItemCount=[&](uint32 id,uint32 count,bool){assert(player.bags[id]>=count);player.bags[id]-=count;};
    for (auto const& obj:quest->GetObjectives())
        switch(obj.Type) {
#include "RewardItemCase.inc"
        }
}
enum {LOOT_ITEM_TYPE_CURRENCY=1,RATE_DROP_ITEM_REFERENCED=1,RATE_DROP_ZONE_LOOT=2,RATE_DROP_CURRENCY=3,CONFIG_DROP_DUNGEON_ONLY_X1=1};
int qualityToRate[]={0};uint32 randomCalls=0;
bool roll_chance_f(float){++randomCalls;return false;}
struct World {uint32 rateCalls=0;float getRate(int){++rateCalls;return 0;}bool getBoolConfig(int){return true;}} world;auto sWorld=&world;
struct CurrencyTypesEntry{};struct CurrencyStore {CurrencyTypesEntry const* LookupEntry(uint32){return nullptr;}} sCurrencyTypesStore;
struct LootStore {char const* GetName()const{return "gameobject_loot_template";}} lootStore;
struct LootStoreItem {uint32 itemid=124512,currencyid=0,reference=0,groupid=0,mincount=1,maxcount=1,type=LOOT_ITEM_TYPE_ITEM;float chance=100;bool needs_quest=true;bool Roll(bool,bool,bool)const;bool IsValid(LootStore const&,uint32)const;};
#include "LootRollAdmission.inc"
int main(int argc,char** argv)
{
    auto& q=store.quests[38817];q.id=38817;q.NextQuestID=38815;q.NextQuestIdChain=38815;q.PrevQuestID=39791;
    q.objectives={{QUEST_OBJECTIVE_ITEM,124512,8,0,1}};
    // QuestData derives DELIVER from the native ITEM objective even though
    // quest_template_addon.SpecialFlags is zero.
    q.SetSpecialFlag(QUEST_SPECIAL_FLAGS_DELIVER);
    auto& next=store.quests[38815];next.id=38815;next.PrevQuestID=38816;auto& parallel=store.quests[38816];parallel.id=38816;parallel.NextQuestID=38815;
    for(auto id:{38811u,39791u}){auto& prev=store.quests[id];prev.id=id;prev.NextQuestID=38817;}
    QuestMap _questTemplates;for(auto& entry:store.quests)_questTemplates[entry.first]=&entry.second;
    for(auto const& entry:_questTemplates){auto qinfo=entry.second;
#include "QuestPrevLoader.inc"
    }
    store.starters.emplace(97270,38815);
    Creature iounn;iounn.entry=97270;iounn.guid={338670};creatures[iounn.guid]=&iounn;
    Creature second;second.entry=97270;second.guid={341219};creatures[second.guid]=&second;
    Creature wrong;wrong.entry=93446;wrong.guid={291874};creatures[wrong.guid]=&wrong;
    assert(argc==2);std::ifstream loot(argv[1]);assert(loot);uint32 entry,selector,item;float chance;uint32 checked=0;
    while(loot>>entry>>selector>>item>>chance){GameObjectTemplate chest;assert(chest.GetLootId()==0);chest.chest.chestLoot=selector;assert(chest.GetLootId()==entry&&item==124512);
        LootStoreItem drop;drop.chance=chance;assert(drop.IsValid(lootStore,entry));
        for(bool rate:{false,true})for(bool dungeon:{false,true})for(bool zone:{false,true})assert(drop.Roll(rate,dungeon,zone));
        drop.groupid=128;assert(!drop.IsValid(lootStore,entry));drop.groupid=0;drop.mincount=0;assert(!drop.IsValid(lootStore,entry));++checked;}
    assert(checked==5&&randomCalls==0&&world.rateCalls==0);
    LootStoreItem ordinary;ordinary.chance=50;assert(!ordinary.Roll(true,false,false)&&randomCalls==1&&world.rateCalls==1);
    for(auto team:{ALLIANCE,HORDE})for(auto previous:{38811u,39791u}) {
        Player p;p.team=team;p.race=team==ALLIANCE?1:2;
        assert(!p.CanTakeQuest(&q,false));p.m_RewardedQuests.insert(previous);
        assert(p.CanTakeQuest(&q,false)); // actual loader treats these as alternatives
        assert(!p.HasQuestForItem(124512));p.Accept(q.id);
        assert(p.HasQuestForItem(124512)&&!p.CanCompleteQuest(q.id)&&!p.CanRewardQuest(&q,false));
        // Failed/cancelled opening, opening without taking loot, pet combat or
        // irrelevant items never call ItemAddedQuestCheck for an Ashildir bone.
        p.ItemAddedQuestCheck(999,1);p.KilledMonsterCredit(124512);
        assert(p.GetQuestObjectiveData(q.id,124512)==0);
        for(uint32 n=1;n<=8;++n) {
            ++p.bags[124512];p.ItemAddedQuestCheck(124512,1);
            assert(p.GetQuestObjectiveData(q.id,124512)==int32(n));
            assert(p.GetQuestStatus(q.id)==(n<8?QUEST_STATUS_INCOMPLETE:QUEST_STATUS_COMPLETE));
            assert(p.HasQuestForItem(124512)==(n<8));
        }
        assert(p.completions==1&&p.CanRewardQuest(&q,false)&&!p.CanTakeQuest(&next,false));
        // Native removal check runs before the inventory operation decrements.
        p.ItemRemovedQuestCheck(124512,1);--p.bags[124512];
        assert(p.GetQuestStatus(q.id)==QUEST_STATUS_INCOMPLETE&&p.HasQuestForItem(124512));
        ++p.bags[124512];p.ItemAddedQuestCheck(124512,1);
        assert(p.CanRewardQuest(&q,false));
        p.bags[124512]=0;p.bank[124512]=8; // completion can include bank, reward cannot
        p.AdjustQuestReqItemCount(&q);assert(p.GetQuestObjectiveData(q.id,124512)==8);
        assert(!p.CanRewardQuest(&q,false));p.bags[124512]=8;p.bank[124512]=0;
        Consume(p,&q);assert(p.bags[124512]==0);
        p.m_RewardedQuests.insert(q.id);p.quests.erase(q.id);
        assert(!p.CanCompleteQuest(q.id)&&!p.CanRewardQuest(&q,false)&&!p.HasQuestForItem(124512));
        assert(p.CanTakeQuest(&next,false));
        assert(p.GetNextQuest(iounn.GetGUID(),&q)==&next&&p.GetNextQuest(second.GetGUID(),&q)==&next);
        assert(!p.GetNextQuest(wrong.GetGUID(),&q));
    }
    // Abandon/reaccept and login objective reconstruction use real item counts.
    Player retry;retry.Accept(q.id);retry.bags[124512]=3;retry.AdjustQuestReqItemCount(&q);
    assert(retry.GetQuestObjectiveData(q.id,124512)==3);retry.quests.erase(q.id);
    assert(!retry.HasQuestForItem(124512));retry.Accept(q.id);retry.AdjustQuestReqItemCount(&q);
    assert(retry.GetQuestObjectiveData(q.id,124512)==3&&retry.HasQuestForItem(124512));
    retry.bags[124512]=9;retry.AdjustQuestReqItemCount(&q);
    assert(retry.GetQuestObjectiveData(q.id,124512)==8);
    assert(retry.CanCompleteQuest(q.id));retry.CompleteQuest(q.id);Consume(retry,&q);
    assert(retry.bags[124512]==1); // consume required eight, preserve surplus
    std::cout<<"PASS: five native chest selectors/loot admission/guaranteed rates, quest-item eligibility, eight actual item updates, removal/recovery, bank restriction, reward item case, both factions, alternative predecessors and actual Iounn38815 handoff; abandonment/reacceptance/reload counts. Inventory/world/network remain fixtures.\n";
}
