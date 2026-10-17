#include "HuntingKillFixture.inc"
#include "NativeRewardPlayer.inc"
void Kill(Player& player,uint32 entry,bool pet,uint64 guid)
{
    Creature victim;victim.entry=entry;victim.guid={guid};victim.data.Entry=entry;victim.data.KillCredit[0]=entry==107753?97091:0;creatures[victim.guid]=&victim;
    Unit guardian;guardian.ownerPlayer=&player;auto resolved=ResolveKiller(pet?guardian:static_cast<Unit&>(player),&victim);assert(resolved.first==&player&&resolved.second);
    KillRewarder reward;reward._victim=&victim;auto before=player.packets;reward._RewardPlayer(&player,false);assert(player.delays.back()==50&&player.packets==before);Deliver(player);
    assert(victim.IsPlayerRewarded(player.GetGUID()));reward._RewardPlayer(&player,false);assert(player.events.empty());creatures.erase(victim.guid);
}
int main(int argc,char** argv)
{
    _set_abort_behavior(0,_WRITE_ABORT_MSG|_CALL_REPORTFAULT);_CrtSetReportMode(_CRT_ASSERT,_CRTDBG_MODE_FILE);_CrtSetReportFile(_CRT_ASSERT,_CRTDBG_FILE_STDERR);
    assert(argc==2);std::ifstream input(argv[1]);assert(input);std::string line;auto& q=store.quests[39789];q.id=39789;
    while(std::getline(input,line)){std::istringstream in(line);std::string kind;in>>kind;
        if(kind=="quest"){uint32 id;in>>id;if(id==39789){uint32 flagsEx,start,next,prev;int exclusive;in>>q.flags>>flagsEx>>q.AllowableRaces>>start>>q.NextQuestIdChain>>q.PrevQuestID>>q.NextQuestID>>exclusive>>q.special;assert(flagsEx==2&&!start&&q.NextQuestIdChain==39793&&!q.PrevQuestID&&!exclusive&&!q.special);}}
        if(kind=="objective"){uint32 id,objective;QuestObjective o;int index;in>>id>>objective;if(id==39789){in>>o.Type>>o.ObjectID>>o.Amount>>index>>o.Flags;o.StorageIndex=int8(index);assert(o.Type==0&&o.ObjectID==97091&&o.Amount==10&&!index&&!o.Flags);q.objectives.push_back(o);}}
        if(kind=="spell"){uint32 entry,spell,mask;std::string difficulties;in>>entry>>spell>>mask>>difficulties;assert(!mask&&difficulties.empty());CreatureSpell data;assert(data.CanUseInDifficulty(0));}
        if(kind=="actor"){uint32 entry,c1,c2;in>>entry>>c1>>c2;if(entry==107753)assert(c1==97091&&!c2);if(entry==97091)assert(!c1&&!c2);}
    }
    assert(q.objectives.size()==1);q.special|=QUEST_SPECIAL_FLAGS_KILL;
    auto& after=store.quests[39793];after.id=39793;after.AllowableRaces=q.AllowableRaces;after.prevQuests={39789};store.starters.emplace(107675,39793);
    Creature starter;starter.entry=97258;SmartAI starterAI(&starter);Creature giver;giver.entry=107675;giver.guid={338664};giver.fields[UNIT_FIELD_NPC_FLAGS]=643;SmartAI ai(&giver);creatures[giver.guid]=&giver;
    for(auto team:{ALLIANCE,HORDE})for(bool pet:{false,true})for(auto variant:{0,1,2}){
        Player player;player.team=team;player.race=team==ALLIANCE?1:2;player.guid={uint64(1000+team+variant+int(pet)*100)};
        assert(player.CanTakeQuest(&q,false));player.AddQuestAndCheckCompletion(&q,&starter);assert(!player.CanCompleteQuest(q.id)&&!player.CanTakeQuest(&after,false));
        player.KilledMonsterCredit(97828);assert(!player.packets);
        WorldSession session{&player};WorldPackets::Quest::QuestGiverRequestReward request{giver.guid,q.id};session.HandleQuestGiverRequestReward(request);assert(!player.menu.offers);
        for(unsigned n=1;n<=10;++n){auto entry=variant==0?97091:variant==1?107753:n%2?97091:107753;Kill(player,entry,pet,10000+n);assert(player.GetQuestObjectiveData(q.id,97091)==n&&player.packets==n&&player.completions==unsigned(n==10));}
        assert(player.CanRewardQuest(&q,false));Kill(player,97091,pet,20000);assert(player.packets==10&&player.completions==1);
        session.HandleQuestGiverRequestReward(request);assert(player.menu.offers==1);giver.near=false;session.HandleQuestGiverRequestReward(request);assert(player.menu.offers==1);giver.near=true;
        assert(!player.CanTakeQuest(&after,false));player.quests.erase(q.id);player.m_RewardedQuests.insert(q.id);assert(player.CanTakeQuest(&after,false)&&player.GetNextQuest(giver.guid,&q)==&after);assert(!player.CanTakeQuest(&q,false));
    }
    {Player player;player.Accept(q.id);player.guid={5000};for(unsigned n=0;n<4;++n)Kill(player,107753,true,30000+n);player.quests.clear();player.Accept(q.id);assert(!player.GetQuestObjectiveData(q.id,97091));for(unsigned n=0;n<10;++n)Kill(player,97091,false,31000+n);assert(player.CanRewardQuest(&q,false));}
    {Player player;player.Accept(q.id);player.guid={5001};player.data[{q.id,97091}]=9;Kill(player,107753,true,32000);assert(player.GetQuestStatus(q.id)==QUEST_STATUS_COMPLETE);}
    std::cout<<"PASS: native10-worg counter, Snarler/Alpha credit substitution, owner/player/pet kill reward and per-victim guard, both factions,9-to10/cap, abandon/retry, saved counter and actual Rax reward request/range and39793 prerequisite/next relation. World/history/clock/packets remain fixtures; client pending.\n";
}
