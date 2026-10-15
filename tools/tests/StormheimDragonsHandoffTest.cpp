#include "HeartHandoffFixture.inc"
int main()
{
 auto& q=store.quests[39652];q.id=39652;q.NextQuestIdChain=38624;q.NextQuestID=38624;q.PrevQuestID=40568;q.objectives={{0,96466,1,0,0},{0,96467,1,2,1}};
 auto& next=store.quests[38624];next.id=38624;next.PrevQuestID=39652;
 for(auto id:{38414u,40568u}){auto& previous=store.quests[id];previous.id=id;previous.NextQuestID=39652;}
 QuestMap _questTemplates;for(auto& entry:store.quests)_questTemplates[entry.first]=&entry.second;
 for(auto const& entry:_questTemplates){auto qinfo=entry.second;
#include "QuestPrevLoader.inc"
 }
 store.starters.emplace(92218,38624);
 for(auto team:{ALLIANCE,HORDE})for(auto previous:{38414u,40568u}){Player p;p.team=team;p.race=team==ALLIANCE?1:2;p.m_RewardedQuests.insert(previous);assert(p.CanTakeQuest(&q,false));p.Accept(39652);assert(!p.CanCompleteQuest(39652)&&!p.CanRewardQuest(&q,false));p.SetQuestObjectiveData(&q,&q.objectives[0],1);assert(!p.CanCompleteQuest(39652));p.SetQuestObjectiveData(&q,&q.objectives[1],1);assert(p.CanCompleteQuest(39652));p.CompleteQuest(39652);assert(p.CanRewardQuest(&q,false));p.m_RewardedQuests.insert(39652);p.quests.erase(39652);assert(p.CanTakeQuest(&next,false));Creature npc;npc.entry=92218;creatures[npc.guid]=&npc;assert(p.GetNextQuest(npc.GetGUID(),&q)==&next);npc.entry=96465;assert(!p.GetNextQuest(npc.GetGUID(),&q));}
 std::cout<<"PASS: native loader/completion/reward/prerequisite/GetNextQuest;39652 needs both stages,38414 or40568 can precede it for either faction,92218 offers38624.\n";
}
