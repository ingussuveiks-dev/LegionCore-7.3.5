// Production loot/item/quest handlers with the existing supplies world and
// inventory fixture; no live characters or runtime data are modified.
#include "LightWaySupport.inc"
constexpr int MAX_KILL_CREDIT=2,MAX_CREATURE_QUEST_ITEMS=6;
#include "HasQuestForCreature.inc"
#include "PersonalQuest.inc"

std::pair<Player*,bool> ResolveKiller(Unit& attacker,Unit* victim)
{
    auto GetCharmerOrOwnerPlayerOrPlayerItself=[&]{return attacker.GetCharmerOrOwnerPlayerOrPlayerItself();};
#include "KillOwner.inc"
    return {player,isRewardAllowed};
}
void SelectCreatureLoot(Creature* creature,std::function<void(uint32)> generate)
{
#include "CreatureLootSelector.inc"
    generate(lootid);
}
void LootFor(Player& player,Creature& creature,LootStoreItem const& drop,bool pass=true)
{
    SelectCreatureLoot(&creature,[&](uint32 selector){
        assert(selector==91657);LootTemplate table;table.Entries={drop};Loot loot;loot.owner=&player;
        rolled=pass?0:drop.chance;table.Process(loot,true);
        assert(loot.generated.size()==(pass?1u:0u));loot.Take();
    });
}
int main(int argc,char** argv)
{
    _set_abort_behavior(0,_WRITE_ABORT_MSG|_CALL_REPORTFAULT);_CrtSetReportMode(_CRT_ASSERT,_CRTDBG_MODE_FILE);_CrtSetReportFile(_CRT_ASSERT,_CRTDBG_FILE_STDERR);
    assert(argc==2);std::ifstream input(argv[1]);uint32 entry,selector,item,currency,reference,questRequired,mode,group,min,max,shared;float chance;
    input>>entry>>selector>>item>>currency>>reference>>chance>>questRequired>>mode>>group>>min>>max>>shared;
    assert(input&&entry==91657&&selector==91657&&item==128854&&!currency&&!reference&&chance==99.155f&&questRequired==1&&!mode&&!group&&min==1&&max==1&&!shared);
    LootStoreItem drop;drop.itemid=item;drop.chance=chance;drop.mincount=min;drop.maxcount=max;drop.lootmode=mode;assert(drop.IsValid(lootStore,91657));
    auto& previous=store.quests[39857];previous.id=39857;
    auto& q=store.quests[39849];q.id=39849;q.flags=33554440;q.PrevQuestID=39857;q.prevQuests={39857};q.NextQuestID=39850;q.NextQuestIdChain=39850;q.special=QUEST_SPECIAL_FLAGS_DELIVER;q.objectives={{1,128854,1,0,0}};
    auto& next=store.quests[39850];next.id=39850;next.PrevQuestID=39849;next.prevQuests={39849};next.special=QUEST_SPECIAL_FLAGS_KILL;next.objectives={{0,97482,1,0,1},{0,97483,1,0,2},{0,97484,1,0,3}};store.starters.emplace(97480,39850);
    Creature blood;blood.entry=91657;blood.guid={340975};blood.data.lootid=selector;
    Creature ashildir;ashildir.entry=97480;ashildir.guid={11546796};SmartAI ashildirAI(&ashildir);creatures[ashildir.guid]=&ashildir;
    for(auto team:{ALLIANCE,HORDE})for(bool petKill:{false,true}){
        Player p;p.team=team;p.race=team==ALLIANCE?1:2;assert(!p.CanTakeQuest(&q,false));p.m_RewardedQuests.insert(39857);assert(p.CanTakeQuest(&q,false));
        assert(!p.HasQuestForItem(item)&&!blood.IsPersonalForQuest(&p));p.Accept(39849);
        assert(p.HasQuestForItem(item)&&blood.IsPersonalForQuest(&p)&&!p.CanCompleteQuest(39849)&&!p.CanRewardQuest(&q,false)&&!p.CanTakeQuest(&next,false));
        p.KilledMonsterCredit(91657);assert(!p.GetQuestObjectiveData(39849,item)&&!p.bags[item]);
        Unit pet;pet.ownerPlayer=&p;auto killer=ResolveKiller(petKill?pet:static_cast<Unit&>(p),&blood);assert(killer.first==&p&&killer.second);
        LootFor(p,blood,drop,false);assert(!p.bags[item]&&!p.GetQuestObjectiveData(39849,item)); // installed probability can miss
        LootFor(*killer.first,blood,drop);assert(p.bags[item]==1&&p.GetQuestObjectiveData(39849,item)==1&&p.completions==1&&p.CanRewardQuest(&q,false));
        LootFor(p,blood,drop);assert(p.bags[item]==1&&p.completions==1&&!p.HasQuestForItem(item));
        p.ItemRemovedQuestCheck(item,1);--p.bags[item];assert(p.GetQuestStatus(39849)==QUEST_STATUS_INCOMPLETE&&p.HasQuestForItem(item)&&!p.CanRewardQuest(&q,false));
        p.bank[item]=1;p.AdjustQuestReqItemCount(&q);assert(p.GetQuestObjectiveData(39849,item)==1&&p.CanCompleteQuest(39849));p.CompleteQuest(39849);assert(!p.CanRewardQuest(&q,false));
        p.bank[item]=0;p.bags[item]=1;assert(p.CanRewardQuest(&q,false));
        auto callbacks=ashildirAI.delegated;RewardNext(p,ashildir,&q);assert(p.menu.detailed.empty()&&ashildirAI.delegated==callbacks+1); // reward has not been recorded yet
        Consume(p,&q);p.m_RewardedQuests.insert(39849);p.quests.erase(39849);assert(!p.bags[item]&&p.CanTakeQuest(&next,false));
        RewardNext(p,ashildir,&q);assert(p.menu.detailed==std::vector<uint32>{39850}&&ashildirAI.delegated==callbacks+2&&p.GetNextQuest(ashildir.guid,&q)==&next);
        p.Accept(39850);assert(!p.CanCompleteQuest(39850));
    }
    {Player p;p.Accept(39849);p.bags[item]=1;p.AdjustQuestReqItemCount(&q);p.quests.erase(39849);assert(!p.HasQuestForItem(item)&&!blood.IsPersonalForQuest(&p));p.Accept(39849);p.AdjustQuestReqItemCount(&q);assert(p.GetQuestObjectiveData(39849,item)==1&&p.CanCompleteQuest(39849));}
    {Player p;p.Accept(39849);p.bags[item]=2;p.AdjustQuestReqItemCount(&q);assert(p.GetQuestObjectiveData(39849,item)==1);p.CompleteQuest(39849);Consume(p,&q);assert(p.bags[item]==1);} // native reward consumes amount1, not arbitrary extra inventory
    {Player p;p.Accept(39849);blood.data.lootid=0;bool generated=false;SelectCreatureLoot(&blood,[&](uint32){generated=true;});assert(!generated);blood.data.lootid=selector;
        LootTemplate table;table.Entries={drop};table.Entries[0].lootmode=2;Loot wrong;wrong.owner=&p;rolled=0;table.Process(wrong,true);assert(wrong.generated.empty());table.Entries[0].lootmode=0;table.conditions=false;table.Process(wrong,true);assert(wrong.generated.empty());}
    {Player p,other;p.Accept(39849);Unit pet;pet.ownerPlayer=&other;assert(ResolveKiller(pet,&blood).first==&other&&!blood.IsPersonalForQuest(&other));LootFor(other,blood,drop);assert(!other.bags[item]&&!p.bags[item]);}
    std::cout<<"PASS: actual Bloodbeard loot selector/row and native Process/Roll/IsValid, quest-personal eligibility, player/pet owner selection, item pickup/removal/reload/bank/reaccept/reward consumption, both factions and native Ashildir39850 reward-next dispatch. World, inventory writes, tap/threat distribution, count RNG and packets are fixtures; client checks pending.\n";
}
