#include "HuntingLootFixture.inc"
int main(int argc,char** argv)
{
    _set_abort_behavior(0,_WRITE_ABORT_MSG|_CALL_REPORTFAULT);_CrtSetReportMode(_CRT_ASSERT,_CRTDBG_MODE_FILE);_CrtSetReportFile(_CRT_ASSERT,_CRTDBG_FILE_STDERR);
    assert(argc==2);std::ifstream input(argv[1]);assert(input);std::string line;LootStoreItem hair;
    itemStore.items[138021].stack=10;
    while(std::getline(input,line)){std::istringstream in(line);std::string kind;in>>kind;
        if(kind=="quest"){uint32 id,start,package;in>>id;auto& q=store.quests[id];q.id=id;in>>q.flags>>q.FlagsEx>>q.AllowableRaces>>start>>q.NextQuestIdChain>>q.PrevQuestID>>q.NextQuestID>>q.ExclusiveGroup>>q.special>>package;assert(!start&&!q.ExclusiveGroup&&!q.special);}
        if(kind=="objective"){uint32 id,objective;int index;QuestObjective o;in>>id>>objective>>o.Type>>o.ObjectID>>o.Amount>>index>>o.Flags;o.StorageIndex=int8(index);store.quests[id].objectives.push_back(o);}
        if(kind=="loot"){uint32 entry,required;in>>entry>>hair.itemid>>hair.chance>>required>>hair.lootmode>>hair.groupid>>hair.mincount>>hair.maxcount>>hair.reference>>hair.currencyid;hair.needs_quest=required;assert(entry==97828&&hair.itemid==138021&&required==1&&hair.mincount==1&&hair.maxcount==3&&!hair.lootmode&&!hair.groupid&&!hair.reference&&!hair.currencyid&&hair.IsValid(lootStore,entry));}
    }
    auto& q=store.quests[39793];assert(q.objectives.size()==1&&q.objectives[0].Type==1&&q.objectives[0].ObjectID==138021&&q.objectives[0].Amount==18&&!q.objectives[0].Flags&&q.PrevQuestID==39789&&!q.NextQuestIdChain);q.special|=QUEST_SPECIAL_FLAGS_DELIVER;
    QuestMap _questTemplates;for(auto& row:store.quests)_questTemplates[row.first]=&row.second;for(auto const& row:_questTemplates){auto qinfo=row.second;
        #include "QuestPrevLoader.inc"
    }
    Creature giver;giver.entry=107675;giver.guid={338664};SmartAI ai(&giver);creatures[giver.guid]=&giver;
    for(auto team:{ALLIANCE,HORDE})for(bool max:{false,true}){
        Player player;player.team=team;player.race=team==ALLIANCE?1:2;Loot empty;empty.owner=&player;
        assert(!player.CanTakeQuest(&q,false)&&!empty.AllowedForPlayer(&player,138021,0,LOOT_ITEM_TYPE_ITEM,true));
        player.Accept(39789);player.quests[39789].Status=QUEST_STATUS_COMPLETE;assert(!player.CanTakeQuest(&q,false));player.quests.erase(39789);player.m_RewardedQuests.insert(39789);
        assert(player.CanTakeQuest(&q,false));player.AddQuestAndCheckCompletion(&q,&giver);assert(!player.CanCompleteQuest(q.id));player.KilledMonsterCredit(97828);assert(!player.GetQuestObjectiveData(q.id,138021));
        unsigned loots=0;
        while(player.bags[138021]<18){
            LootTemplate table;table.Entries={hair};Loot loot;loot.owner=&player;rolled=100;table.Process(loot,true);assert(loot.generated.empty());rolled=0;table.Process(loot,true);assert(loot.generated.size()==1&&!player.completions);
            Take(player,loot,max);++loots;assert(player.GetQuestObjectiveData(q.id,138021)==int32(player.bags[138021]));assert(player.completions==(player.bags[138021]==18?1:0));
        }
        assert(loots==(max?6:18)&&player.CanRewardQuest(&q,false)&&!player.HasQuestForItem(138021)&&!empty.AllowedForPlayer(&player,138021,0,LOOT_ITEM_TYPE_ITEM,true));
        player.bags[138021]=17;player.bank[138021]=1;assert(!player.CanRewardQuest(&q,false));player.bank[138021]=0;player.bags[138021]=18;
        player.ItemRemovedQuestCheck(138021,1);player.bags[138021]=17;assert(!player.CanCompleteQuest(q.id)&&player.HasQuestForItem(138021));player.bags[138021]=18;player.ItemAddedQuestCheck(138021,1);assert(player.CanRewardQuest(&q,false));
        Consume(player,&q);assert(!player.bags[138021]);player.quests.erase(q.id);player.m_RewardedQuests.insert(q.id);assert(!player.CanTakeQuest(&q,false));RewardNext(player,giver,&q);assert(player.menu.detailed.empty());
        Player retry;retry.team=team;retry.race=player.race;retry.m_RewardedQuests.insert(39789);retry.AddQuestAndCheckCompletion(&q,&giver);retry.bags[138021]=17;retry.AdjustQuestReqItemCount(&q);assert(!retry.CanCompleteQuest(q.id));retry.bags[138021]=18;retry.ItemAddedQuestCheck(138021,1);assert(retry.CanRewardQuest(&q,false));
    }
    std::cout<<"PASS: native prerequisite requires39789 reward, ordinary goat loot Process/Roll/AllowedForPlayer, mode0/64.694% and1..3 endpoints, both factions and actual18-item lifecycle, kill-without-loot rejection, native10-stack metadata, bank/removal/retry/consumption and no invented next quest. Storage/stack splitting/count RNG/world/packets remain fixtures; client pending.\n";
}
