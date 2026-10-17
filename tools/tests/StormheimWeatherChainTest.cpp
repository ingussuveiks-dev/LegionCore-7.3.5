#include <sstream>
#include "WeatherChainFixture.inc"
void Load()
{
    store.mExclusiveQuestGroups.clear();QuestMap _questTemplates;
    for(auto& row:store.quests){row.second.prevQuests.clear();_questTemplates[row.first]=&row.second;}
    auto& mExclusiveQuestGroups=store.mExclusiveQuestGroups;
    for(auto const& row:_questTemplates){auto qinfo=row.second;
        #include "QuestPrevLoader.inc"
        #include "WeatherGroupLoader.inc"
    }
}
int main(int argc,char** argv)
{
    _set_abort_behavior(0,_WRITE_ABORT_MSG|_CALL_REPORTFAULT);_CrtSetReportMode(_CRT_ASSERT,_CRTDBG_MODE_FILE);_CrtSetReportFile(_CRT_ASSERT,_CRTDBG_FILE_STDERR);
    assert(argc==2);std::ifstream input(argv[1]);assert(input);std::string line;
    while(std::getline(input,line)){std::istringstream in(line);std::string kind;in>>kind;
        if(kind=="chain"){uint32 id,start;in>>id;auto& q=store.quests[id];q.id=id;in>>q.flags>>q.FlagsEx>>q.AllowableRaces>>start>>q.NextQuestIdChain>>q.PrevQuestID>>q.NextQuestID>>q.special>>q.ExclusiveGroup;assert(!start);}
        if(kind=="starter"){uint32 id,q;in>>id>>q;store.starters.emplace(id,q);}
    }
    assert(store.quests.size()==8);
    auto repaired=store.quests;
    for(auto id:{38318u,38405u,38613u,38614u})store.quests[id].ExclusiveGroup=0;
    store.quests[38405].PrevQuestID=0;Load();
    {Player player;player.race=1;assert(player.CanTakeQuest(&store.quests[38405],false));} // Original early acceptance.
    for(auto ids:{std::pair<uint32,uint32>{38318,38410},{38405,38410},{38613,38615},{38614,38615}}){Player player;player.race=ids.first<38600?1:2;player.m_RewardedQuests.insert(ids.first);assert(player.CanTakeQuest(&store.quests[ids.second],false));} // Original OR bypass.
    store.quests=repaired;Load();
    for(auto team:{ALLIANCE,HORDE}){
        uint32 prerequisite=team==ALLIANCE?38312:38612,weather=team==ALLIANCE?38405:38614,infiltration=team==ALLIANCE?38318:38613,next=team==ALLIANCE?38410:38615;
        auto& q=store.quests[weather];assert(q.PrevQuestID==int32(prerequisite)&&q.ExclusiveGroup==-(team==ALLIANCE?38318:38613));
        for(unsigned bit=0;bit<64;++bit){Player player;player.race=uint64(1)<<bit;assert(player.SatisfyQuestRace(&q,false)==bool(q.AllowableRaces&player.race));}
        Player before;before.team=team;before.race=team==ALLIANCE?1:2;assert(!before.CanTakeQuest(&q,false));before.Accept(prerequisite);before.quests[prerequisite].Status=QUEST_STATUS_COMPLETE;assert(!before.CanTakeQuest(&q,false));before.quests.erase(prerequisite);before.m_RewardedQuests.insert(prerequisite);
        assert(before.CanTakeQuest(&q,false)&&before.CanTakeQuest(&store.quests[infiltration],false));
        Player enemy;enemy.race=team==ALLIANCE?2:1;enemy.m_RewardedQuests.insert(prerequisite);assert(!enemy.CanTakeQuest(&q,false));
        for(bool weatherFirst:{false,true}){
            Player player;player.team=team;player.race=team==ALLIANCE?1:2;player.m_RewardedQuests.insert(prerequisite);
            uint32 first=weatherFirst?weather:infiltration,second=weatherFirst?infiltration:weather;
            Creature giver;giver.entry=team==ALLIANCE?91519:92567;giver.guid={uint64(giver.entry)};SmartAI ai(&giver);creatures[giver.guid]=&giver;
            assert(!player.CanTakeQuest(&store.quests[next],false));
            player.m_RewardedQuests.insert(first);player.Accept(second);player.quests[second].Status=QUEST_STATUS_COMPLETE;
            assert(!player.CanTakeQuest(&store.quests[next],false));RewardNext(player,giver,&store.quests[first]);assert(player.menu.detailed.empty()&&ai.delegated==1);
            player.quests.erase(second);player.m_RewardedQuests.insert(second);assert(player.CanTakeQuest(&store.quests[next],false));
            RewardNext(player,giver,&store.quests[second]);assert(player.menu.detailed.size()==1&&player.menu.detailed[0]==next&&ai.delegated==2);
            // Reconstructed login history obeys the same native admission.
            Player relog;relog.race=player.race;relog.m_RewardedQuests=player.m_RewardedQuests;assert(relog.CanTakeQuest(&store.quests[next],false));relog.m_RewardedQuests.erase(first);assert(!relog.CanTakeQuest(&store.quests[next],false));
        }
    }
    std::cout<<"PASS: original early Alliance admission and either-predecessor bypass reproduced; native previous/next/group loading, full-width race checks, both parallel reward orders, completed-but-unrewarded rejection and actual NPC next-details/reward callback pass. Saved-history/world/packets are fixtures; client pending.\n";
}
