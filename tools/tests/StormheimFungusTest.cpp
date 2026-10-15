// Extracted production quest handlers; world storage, inventory operations,
// object lookup and packet delivery use the existing Heart handoff fixture.
#include "FungusFixture.inc"
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
int main()
{
    auto& q=store.quests[38810];q.id=38810;q.NextQuestID=38811;q.NextQuestIdChain=38811;q.PrevQuestID=39788;
    q.objectives={{QUEST_OBJECTIVE_ITEM,124504,8,0,0}};
    // QuestData derives DELIVER from the native ITEM objective even though
    // quest_template_addon.SpecialFlags is zero.
    q.SetSpecialFlag(QUEST_SPECIAL_FLAGS_DELIVER);
    auto& next=store.quests[38811];next.id=38811;next.PrevQuestID=38810;
    for(auto id:{38778u,39788u}){auto& prev=store.quests[id];prev.id=id;prev.NextQuestID=38810;}
    QuestMap _questTemplates;for(auto& entry:store.quests)_questTemplates[entry.first]=&entry.second;
    for(auto const& entry:_questTemplates){auto qinfo=entry.second;
#include "QuestPrevLoader.inc"
    }
    store.starters.emplace(93231,38811);
    Creature vydhar;vydhar.entry=93231;vydhar.guid={292106};creatures[vydhar.guid]=&vydhar;
    Creature wrong;wrong.entry=93446;wrong.guid={291874};creatures[wrong.guid]=&wrong;
    GameObjectTemplate chest;
    assert(chest.GetLootId()==0); // original state bypassed Player::SendLoot FillLoot
    chest.chest.chestLoot=241866;assert(chest.GetLootId()==241866);
    for(auto team:{ALLIANCE,HORDE})for(auto previous:{38778u,39788u}) {
        Player p;p.team=team;p.race=team==ALLIANCE?1:2;
        assert(!p.CanTakeQuest(&q,false));p.m_RewardedQuests.insert(previous);
        assert(p.CanTakeQuest(&q,false)); // actual loader treats these as alternatives
        assert(!p.HasQuestForItem(124504));p.Accept(q.id);
        assert(p.HasQuestForItem(124504)&&!p.CanCompleteQuest(q.id)&&!p.CanRewardQuest(&q,false));
        // Failed/cancelled opening, opening without taking loot, pet combat or
        // irrelevant items never call ItemAddedQuestCheck for a Soulcap.
        p.ItemAddedQuestCheck(999,1);p.KilledMonsterCredit(124504);
        assert(p.GetQuestObjectiveData(q.id,124504)==0);
        for(uint32 n=1;n<=8;++n) {
            ++p.bags[124504];p.ItemAddedQuestCheck(124504,1);
            assert(p.GetQuestObjectiveData(q.id,124504)==int32(n));
            assert(p.GetQuestStatus(q.id)==(n<8?QUEST_STATUS_INCOMPLETE:QUEST_STATUS_COMPLETE));
            assert(p.HasQuestForItem(124504)==(n<8));
        }
        assert(p.completions==1&&p.CanRewardQuest(&q,false)&&!p.CanTakeQuest(&next,false));
        // Native removal check runs before the inventory operation decrements.
        p.ItemRemovedQuestCheck(124504,1);--p.bags[124504];
        assert(p.GetQuestStatus(q.id)==QUEST_STATUS_INCOMPLETE&&p.HasQuestForItem(124504));
        ++p.bags[124504];p.ItemAddedQuestCheck(124504,1);
        assert(p.CanRewardQuest(&q,false));
        p.bags[124504]=0;p.bank[124504]=8; // completion can include bank, reward cannot
        p.AdjustQuestReqItemCount(&q);assert(p.GetQuestObjectiveData(q.id,124504)==8);
        assert(!p.CanRewardQuest(&q,false));p.bags[124504]=8;p.bank[124504]=0;
        Consume(p,&q);assert(p.bags[124504]==0);
        p.m_RewardedQuests.insert(q.id);p.quests.erase(q.id);
        assert(!p.CanCompleteQuest(q.id)&&!p.CanRewardQuest(&q,false)&&!p.HasQuestForItem(124504));
        assert(p.CanTakeQuest(&next,false));
        assert(p.GetNextQuest(vydhar.GetGUID(),&q)==&next);
        assert(!p.GetNextQuest(wrong.GetGUID(),&q));
    }
    // Abandon/reaccept and login objective reconstruction use real item counts.
    Player retry;retry.Accept(q.id);retry.bags[124504]=3;retry.AdjustQuestReqItemCount(&q);
    assert(retry.GetQuestObjectiveData(q.id,124504)==3);retry.quests.erase(q.id);
    assert(!retry.HasQuestForItem(124504));retry.Accept(q.id);retry.AdjustQuestReqItemCount(&q);
    assert(retry.GetQuestObjectiveData(q.id,124504)==3&&retry.HasQuestForItem(124504));
    retry.bags[124504]=9;retry.AdjustQuestReqItemCount(&q);
    assert(retry.GetQuestObjectiveData(q.id,124504)==8);
    assert(retry.CanCompleteQuest(q.id));retry.CompleteQuest(q.id);Consume(retry,&q);
    assert(retry.bags[124504]==1); // consume required eight, preserve surplus
    std::cout<<"PASS: native chest selector, quest-item eligibility, eight actual item updates, removal/recovery, bank restriction, reward item case, both factions, alternative predecessors and actual Vydhar38811 handoff; abandonment/reacceptance/reload counts. Inventory/world/network remain fixtures.\n";
}
