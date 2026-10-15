#include "HeartHandoffFixture.inc"
int main()
{
 auto& q=store.quests[38624];q.id=38624;q.NextQuestIdChain=39803;q.NextQuestID=39803;q.PrevQuestID=39652;q.special=32|128;q.objectives={{3,91767,1,0,0},{0,92360,150,2,3},{0,91768,1,2,1}};
 auto& next=store.quests[39803];next.id=39803;next.PrevQuestID=38624;store.quests[39652].id=39652;store.quests[39652].NextQuestID=38624;
 QuestMap _questTemplates;for(auto& entry:store.quests)_questTemplates[entry.first]=&entry.second;
 for(auto const& entry:_questTemplates){auto qinfo=entry.second;
#include "QuestPrevLoader.inc"
 }
 store.starters.emplace(97074,39803);
 for(auto team:{ALLIANCE,HORDE}){Player p;p.team=team;p.race=team==ALLIANCE?1:2;assert(!p.CanTakeQuest(&q,false));p.m_RewardedQuests.insert(39652);assert(p.CanTakeQuest(&q,false));p.Accept(38624);assert(!p.CanCompleteQuest(38624)&&!p.CanRewardQuest(&q,false));p.SetQuestObjectiveData(&q,&q.objectives[0],1);p.SetQuestObjectiveData(&q,&q.objectives[1],149);assert(!p.CanCompleteQuest(38624));p.SetQuestObjectiveData(&q,&q.objectives[1],150);assert(!p.CanCompleteQuest(38624));p.SetQuestObjectiveData(&q,&q.objectives[2],1);assert(p.CanCompleteQuest(38624));p.CompleteQuest(38624);assert(p.CanRewardQuest(&q,false));p.m_RewardedQuests.insert(38624);p.quests.erase(38624);assert(p.CanTakeQuest(&next,false));Creature npc;npc.entry=97074;creatures[npc.guid]=&npc;assert(p.GetNextQuest(npc.GetGUID(),&q)==&next);npc.entry=92218;assert(!p.GetNextQuest(npc.GetGUID(),&q));}
 std::cout<<"PASS: native both-race prerequisite/completion/reward/GetNextQuest;38624 needs boarding,150 kills and landing; actual Huginn97074 offers39803.\n";
}
