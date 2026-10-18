// Reuse the fixture services, but execute the real exported loot/condition and
// Player item-objective handlers against the new migration's actual row.
#include "RockawayLootFixture.inc"
int main(int argc,char**argv)
{
    assert(argc==2);
    auto& q=store.quests[45088];q.id=45088;q.special=QUEST_SPECIAL_FLAGS_DELIVER;
    q.objectives={{QUEST_OBJECTIVE_ITEM,142487,1000,0,0}};
    Gate::store.templates[44746]={44746,1};Gate::store.templates[44737]={44737,1};
    uint32 entry,item,quest,mode,group,min,max,wq;float chance;
    std::ifstream data(argv[1]);data>>entry>>item>>chance>>quest>>mode>>group>>min>>max>>wq;
    assert(entry==229113&&item==142487&&chance==100&&quest==1&&mode==0&&group==0&&min==15&&max==15&&wq==44746);
    LootStoreItem row;row.itemid=item;row.chance=chance;row.needs_quest=true;row.lootmode=mode;row.mincount=min;row.maxcount=max;
    LootTemplate loot;loot.Entries={row};Condition condition;condition.SourceGroup=entry;condition.ConditionValue1=wq;
    assert(conditionMgr.addToLootTemplate(&condition,&loot));
    for(uint32 team:{ALLIANCE,HORDE})
    {
        Player p;p.team=team;p.Accept(45088);Loot result;result.owner=&p;rolled=0;
        loot.Process(result,true);
        Gate::store._worldQuest.clear();result.Take();assert(!p.bags[item]);
        Gate::store._worldQuest[1][44737]={};result.Take();assert(!p.bags[item]);
        Gate::store._worldQuest[1][44746]={};result.Take();assert(p.bags[item]==15);
        p.bags[item]=999;p.AdjustQuestReqItemCount(&q);result.Take();assert(p.bags[item]==1014&&p.GetQuestObjectiveData(45088,item)==1000);
        result.Take();assert(p.bags[item]==1014); // native objective stops eligibility
        Consume(p,&q);assert(p.bags[item]==14);
        p.quests.erase(45088);result.Take();assert(p.bags[item]==14);
        p.Accept(45088);p.AdjustQuestReqItemCount(&q);result.Take();assert(p.bags[item]==29);
    }
    std::cout<<"PASS: actual migration loot row through native Process/Roll, condition loader/type60 and Player fragment objective; both factions, unrelated/inactive landing,999/1000, excess, no quest and reacceptance. Inventory services remain fixtures.\n";
}
