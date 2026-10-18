#include "QuestFixture.inc"
#include "JudgmentCatalog.inc"
// Native asset admission and Player credit are executed; the intervening
// AchievementMgr progress/tree notification is a fixture bridge, not a live raid.
bool NativeEncounterAdmission(uint32 miscValue1)
{
    struct {uint32 Asset=JudgmentEncounter;} row;auto criteria=&row;
#include "EncounterAdmission.inc"
    return true;
}
void Encounter(Player& p,uint32 type,uint32 asset)
{
    if(type==JudgmentType&&NativeEncounterAdmission(asset))p.AchieveCriteriaCredit(JudgmentRoot);
}
int main()
{
    auto& q=store.quests[44868];q.id=44868;q.objectives={{14,JudgmentRoot,1,0,0}};
    for(uint32 team:{ALLIANCE,HORDE}){
        Player p;p.team=team;p.Accept(q.id);
        for(uint32 asset:{0u,1809u,1958u,1962u})Encounter(p,JudgmentType,asset);
        Encounter(p,181,JudgmentEncounter);assert(!p.completions&&p.events.empty()&&!p.GetQuestObjectiveData(q.id,JudgmentRoot));
        Encounter(p,JudgmentType,JudgmentEncounter);Encounter(p,JudgmentType,JudgmentEncounter);
        assert(p.GetQuestObjectiveData(q.id,JudgmentRoot)==1&&p.events.size()==1);
        Flush(p);assert(p.completions==1&&p.GetQuestStatus(q.id)==QUEST_STATUS_COMPLETE);
        Encounter(p,JudgmentType,JudgmentEncounter);Flush(p);assert(p.completions==1);
    }
    {Player p;Encounter(p,JudgmentType,JudgmentEncounter);assert(p.events.empty());p.Accept(q.id);Encounter(p,JudgmentType,JudgmentEncounter);p.quests[q.id].Status=QUEST_STATUS_NONE;Flush(p);assert(!p.completions);}
    {Player p;p.Accept(q.id);Encounter(p,JudgmentType,JudgmentEncounter);p.Accept(q.id);Flush(p);assert(!p.completions&&!p.GetQuestObjectiveData(q.id,JudgmentRoot));Encounter(p,JudgmentType,JudgmentEncounter);Flush(p);assert(p.completions==1);}
    {Player p;p.quests[q.id].Status=QUEST_STATUS_REWARDED;Encounter(p,JudgmentType,JudgmentEncounter);assert(p.events.empty());}
    std::cout<<"PASS: native asset admission and Player44868 credit/completion/lifetime:2008 only, both factions, once-only, no quest/rewarded/abandon/reaccept. Achievement progress bridge and reward persistence are fixture services; client pending.\n";
}
