#include "WagerLootFixture.inc"
int main(int argc,char** argv)
{
    _set_abort_behavior(0,_WRITE_ABORT_MSG|_CALL_REPORTFAULT);_CrtSetReportMode(_CRT_ASSERT,_CRTDBG_MODE_FILE);_CrtSetReportFile(_CRT_ASSERT,_CRTDBG_FILE_STDERR);
    assert(argc==2);std::ifstream input(argv[1]);assert(input);std::string line;std::map<uint32,LootStoreItem> drops;
    for(auto id:{138020u,138022u,138023u})itemStore.items[id].stack=10;
    while(std::getline(input,line)){std::istringstream in(line);std::string kind;in>>kind;
        if(kind=="quest"){uint32 id,start,package;in>>id;auto& q=store.quests[id];q.id=id;in>>q.flags>>q.FlagsEx>>q.AllowableRaces>>start>>q.NextQuestIdChain>>q.PrevQuestID>>q.NextQuestID>>q.ExclusiveGroup>>q.special>>package;assert(!start&&!q.ExclusiveGroup&&!q.special);}
        if(kind=="objective"){uint32 id,objective;int index;QuestObjective o;in>>id>>objective>>o.Type>>o.ObjectID>>o.Amount>>index>>o.Flags;o.StorageIndex=int8(index);assert(o.Type==1&&index==0&&!(o.Flags&4));store.quests[id].objectives.push_back(o);}
        if(kind=="loot"){uint32 entry,required;in>>entry;auto& d=drops[entry];in>>d.itemid>>d.chance>>required>>d.lootmode>>d.groupid>>d.mincount>>d.maxcount>>d.reference>>d.currencyid;d.needs_quest=required;assert(required==1&&d.mincount==1&&d.maxcount==1&&!d.lootmode&&!d.groupid&&!d.reference&&!d.currencyid&&d.IsValid(lootStore,entry));}
    }
    for(auto id:{39787u,39786u,39792u})store.quests[id].special|=QUEST_SPECIAL_FLAGS_DELIVER; // Native loader derives this from item objectives.
    QuestMap _questTemplates;for(auto& row:store.quests)_questTemplates[row.first]=&row.second;for(auto const& row:_questTemplates){auto qinfo=row.second;
        #include "QuestPrevLoader.inc"
    }
    assert(drops.size()==3);Creature snaggle,rax;snaggle.entry=107674;snaggle.guid={371030};rax.entry=107675;rax.guid={338664};SmartAI snaggleAI(&snaggle),raxAI(&rax);creatures[snaggle.guid]=&snaggle;creatures[rax.guid]=&rax;
    for(auto team:{ALLIANCE,HORDE}){
        Player p;p.team=team;p.race=team==ALLIANCE?1:2;
        for(auto id:{39787u,39786u,39792u})assert(!p.CanTakeQuest(&store.quests[id],false));
        p.m_RewardedQuests.insert(39789);
        for(auto id:{39787u,39786u,39792u}){
            auto& q=store.quests[id];auto const& o=q.objectives.at(0);auto& giver=id==39792?rax:snaggle;auto entry=id==39787?97124u:id==39786?107914u:107852u;auto const& drop=drops.at(entry);
            assert(q.objectives.size()==1&&o.ObjectID==drop.itemid&&o.Amount==(id==39792?8:1)&&!q.NextQuestIdChain&&!q.NextQuestID);
            Loot empty;empty.owner=&p;assert(!empty.AllowedForPlayer(&p,o.ObjectID,0,LOOT_ITEM_TYPE_ITEM,true));
            assert(p.CanTakeQuest(&q,false));p.AddQuestAndCheckCompletion(&q,&giver);assert(!p.CanCompleteQuest(id));
            p.KilledMonsterCredit(entry);p.KillCreditGO(244457,{25167470});assert(!p.GetQuestObjectiveData(id,o.ObjectID));
            auto before=p.completions;
            for(int n=1;n<=o.Amount;++n){LootTemplate table;table.Entries={drop};Loot loot;loot.owner=&p;rolled=100;table.Process(loot,true);assert(loot.generated.empty());rolled=0;table.Process(loot,true);assert(loot.generated.size()==1);Take(p,loot);assert(p.GetQuestObjectiveData(id,o.ObjectID)==n&&p.completions==before+(n==o.Amount?1:0));}
            assert(p.CanRewardQuest(&q,false)&&!p.HasQuestForItem(o.ObjectID)&&!empty.AllowedForPlayer(&p,o.ObjectID,0,LOOT_ITEM_TYPE_ITEM,true));
            auto next=id==39787?39786u:id==39786?39792u:0;
            if(next)assert(!p.CanTakeQuest(&store.quests[next],false)); // Complete but not handed in.
            p.bags[o.ObjectID]=o.Amount-1;p.bank[o.ObjectID]=1;assert(!p.CanRewardQuest(&q,false));p.bank[o.ObjectID]=0;p.bags[o.ObjectID]=o.Amount;
            p.ItemRemovedQuestCheck(o.ObjectID,1);p.bags[o.ObjectID]--;assert(!p.CanCompleteQuest(id)&&p.HasQuestForItem(o.ObjectID));p.bags[o.ObjectID]++;p.ItemAddedQuestCheck(o.ObjectID,1);assert(p.CanRewardQuest(&q,false));
            Consume(p,&q);assert(!p.bags[o.ObjectID]);p.quests.erase(id);p.m_RewardedQuests.insert(id);assert(!p.CanTakeQuest(&q,false));RewardNext(p,giver,&q);assert(p.menu.detailed.empty());if(next)assert(p.CanTakeQuest(&store.quests[next],false));
            Player retry;retry.team=team;retry.race=p.race;retry.m_RewardedQuests=p.m_RewardedQuests;retry.m_RewardedQuests.erase(id);retry.AddQuestAndCheckCompletion(&q,&giver);retry.bags[o.ObjectID]=o.Amount-1;retry.AdjustQuestReqItemCount(&q);assert(!retry.CanCompleteQuest(id));retry.bags[o.ObjectID]++;retry.ItemAddedQuestCheck(o.ObjectID,1);assert(retry.CanRewardQuest(&q,false));
        }
    }
    std::cout<<"PASS: both factions, actual wager1/1/8 item lifecycle, native loot validation/Process/Roll/AllowedForPlayer, kills and rock-click do not replace loot, Flags1 mandatory, cap/removal/bank/retry/consumption and rewarded39789->39787->39786->39792 gates. Inventory persistence, corpse permissions, world and packets remain fixtures; client pending.\n";
}
