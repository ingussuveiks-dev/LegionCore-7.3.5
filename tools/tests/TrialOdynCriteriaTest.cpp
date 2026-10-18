#include "QuestFixture.inc"
#include "FavorCatalog.inc"
// Native asset admission and Player credit are executed; the intervening
// AchievementMgr progress/tree notification is a fixture bridge, not a live raid.
bool NativeEncounterAdmission(uint32 miscValue1)
{
    struct {uint32 Asset=FavorEncounter;} row;auto criteria=&row;
#include "EncounterAdmission.inc"
    return true;
}
void Encounter(Player& p,uint32 type,uint32 asset)
{
    if(type==FavorType&&NativeEncounterAdmission(asset))p.AchieveCriteriaCredit(FavorRoot);
}
int main()
{
    auto& q=store.quests[44729];q.id=44729;q.objectives={{14,FavorRoot,1,0,0}};
    for(uint32 team:{ALLIANCE,HORDE}){
        Player p;p.team=team;p.Accept(q.id);
        for(uint32 asset:{0u,1809u,1962u,2008u})Encounter(p,FavorType,asset);
        Encounter(p,181,FavorEncounter);assert(!p.completions&&p.events.empty()&&!p.GetQuestObjectiveData(q.id,FavorRoot));
        Encounter(p,FavorType,FavorEncounter);Encounter(p,FavorType,FavorEncounter);
        assert(p.GetQuestObjectiveData(q.id,FavorRoot)==1&&p.events.size()==1);
        Flush(p);assert(p.completions==1&&p.GetQuestStatus(q.id)==QUEST_STATUS_COMPLETE);
        Encounter(p,FavorType,FavorEncounter);Flush(p);assert(p.completions==1);
    }
    {Player p;Encounter(p,FavorType,FavorEncounter);assert(p.events.empty());p.Accept(q.id);Encounter(p,FavorType,FavorEncounter);p.quests[q.id].Status=QUEST_STATUS_NONE;Flush(p);assert(!p.completions);}
    {Player p;p.Accept(q.id);Encounter(p,FavorType,FavorEncounter);p.Accept(q.id);Flush(p);assert(!p.completions&&!p.GetQuestObjectiveData(q.id,FavorRoot));Encounter(p,FavorType,FavorEncounter);Flush(p);assert(p.completions==1);}
    {Player p;p.quests[q.id].Status=QUEST_STATUS_REWARDED;Encounter(p,FavorType,FavorEncounter);assert(p.events.empty());}
    std::cout<<"PASS: native asset admission and Player44729 credit/completion/lifetime:1958 only, both factions, once-only, no quest/rewarded/abandon/reaccept. Achievement progress bridge and reward persistence are fixture services; client pending.\n";
}
