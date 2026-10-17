#include <array>
#include "WeatherFixture.inc"
constexpr uint32 SCRIPT_COMMAND_QUEST_EXPLORED=7;
constexpr float DEFAULT_VISIBILITY_DISTANCE=90,INTERACTION_DISTANCE=5;
void LoadExplorationRows(unsigned count)
{
    struct Script {struct {uint32 QuestID=38614,Distance=0;} QuestExplored;} tmp;
    for(unsigned i=0;i<count;++i)switch(SCRIPT_COMMAND_QUEST_EXPLORED){
        #include "WeatherScriptLoader.inc"
    }
}
int main(int argc,char** argv)
{
    _set_abort_behavior(0,_WRITE_ABORT_MSG|_CALL_REPORTFAULT);_CrtSetReportMode(_CRT_ASSERT,_CRTDBG_MODE_FILE);_CrtSetReportFile(_CRT_ASSERT,_CRTDBG_FILE_STDERR);
    assert(argc==2);std::ifstream input(argv[1]);assert(input);std::string line;
    std::map<uint32,LootStoreItem> drops;
    while(std::getline(input,line)){std::istringstream in(line);std::string kind;in>>kind;
        if(kind=="quest"){uint32 id,start;in>>id;auto& q=store.quests[id];q.id=id;in>>q.flags>>q.FlagsEx>>q.AllowableRaces>>start>>q.NextQuestIdChain>>q.PrevQuestID>>q.NextQuestID>>q.special;assert(!start);}
        if(kind=="objective"){uint32 qid,id;int index;QuestObjective o;in>>qid>>id>>o.Type>>o.ObjectID>>o.Amount>>index>>o.Flags;o.StorageIndex=int8(index);assert(o.Type==QUEST_OBJECTIVE_ITEM&&o.Amount==1&&!o.Flags);store.quests[qid].objectives.push_back(o);}
        if(kind=="loot"||kind=="stone"){uint32 entry,selector,required;in>>entry>>selector;auto& drop=drops[entry];in>>drop.itemid>>drop.chance>>required>>drop.lootmode>>drop.groupid>>drop.mincount>>drop.maxcount;assert(selector==entry&&drop.chance==100&&required==1&&!drop.groupid&&drop.mincount==1&&drop.maxcount==1);assert(drop.IsValid(lootStore,entry));}
    }
    assert(drops.size()==5);
    for(auto id:{38405u,38614u}){assert(store.quests[id].objectives.size()==3);store.quests[id].special|=QUEST_SPECIAL_FLAGS_DELIVER;}
    // ScriptsData processes unused lists too: clearing only SQL SpecialFlags
    // leaves the native in-memory exploration lock restored at startup.
    auto& horde=store.quests[38614];assert(!(horde.special&2));LoadExplorationRows(4);assert(horde.special&2);
    {Player player;player.Accept(38614);for(auto item:{124038u,122685u,122686u}){player.bags[item]=1;player.AdjustQuestReqItemCount(&horde);}assert(!player.CanCompleteQuest(38614));}
    horde.special&=~2;LoadExplorationRows(0);assert(!(horde.special&2));
    for(auto pair:{std::pair<uint32,uint32>{38405,ALLIANCE},{38614,HORDE}}){
        auto& q=store.quests[pair.first];std::array<uint32,3> order{91517,241460,241462};
        do{
            Player player;player.team=pair.second;player.race=pair.second==ALLIANCE?1:2;assert(player.SatisfyQuestRace(&q,false));
            Player enemy;enemy.race=pair.second==ALLIANCE?2:1;assert(!enemy.SatisfyQuestRace(&q,false));player.Accept(q.id);
            assert(!player.CanCompleteQuest(q.id));player.KillCreditGO(241460,{115519});player.KilledMonsterCredit(91517);assert(!player.completions);
            unsigned taken=0;
            for(auto entry:order){
                LootTemplate table;table.Entries={drops.at(entry)};Loot loot;loot.owner=&player;
                if(entry==91517){ // Native ordinary creature processor: mode0 is valid, not a missing-difficulty filter.
                    assert(!drops.at(entry).lootmode);table.Process(loot,true);
                }else{
                    LootGameObject chest;chest.info.entry=entry;chest.info.chest.chestPersonalLoot=entry==241460?60122:60124;assert(chest.IsPersonal());activeTable=&table;
                    Open(player,chest,loot);assert(!loot.fillCalls&&loot.generated.empty()); // Original empty chest reproduced.
                    chest.info.chest.chestLoot=entry;
                    SkillType skill=0;int32 req=0,value=0;Spell spell;spell.m_caster=&player;spell.info.Id=3365;spell.info.effect.MiscValue=5;assert(spell.CanOpenLock(0,1690,skill,req,value)==SPELL_CAST_OK);
                    spell.info.effect.MiscValue=13;assert(spell.CanOpenLock(0,1690,skill,req,value)!=SPELL_CAST_OK);
                    Open(player,chest,loot);assert(loot.fillCalls==1&&loot.personal&&loot.objType==3);
                }
                assert(loot.generated.size()==1&&!player.completions);loot.Take();++taken;
                assert(player.GetQuestObjectiveData(q.id,drops.at(entry).itemid)==1);
                assert(player.completions==(taken==3?1:0));
            }
            assert(player.CanRewardQuest(&q,false));
            player.bags[122685]=0;player.bank[122685]=1;assert(!player.CanRewardQuest(&q,false));player.bank[122685]=0;player.bags[122685]=1;
            Consume(player,&q);for(auto item:{124038u,122685u,122686u})assert(!player.bags[item]);
        }while(std::next_permutation(order.begin(),order.end()));
        // Native inventory reconstruction and removal/retry, independent of collection order.
        Player retry;retry.Accept(q.id);retry.bags={{124038,1},{122685,1},{122686,1}};retry.AdjustQuestReqItemCount(&q);assert(retry.CanCompleteQuest(q.id));
        retry.ItemRemovedQuestCheck(122686,1);retry.bags[122686]=0;assert(!retry.CanCompleteQuest(q.id)&&retry.HasQuestForItem(122686));
        retry.bags[122686]=1;retry.ItemAddedQuestCheck(122686,1);assert(retry.CanRewardQuest(&q,false));
    }
    // Both faction-specific templates also use the same actual personal processor.
    for(auto entry:{240677u,240681u,241460u,241462u}){
        Player player;player.Accept(entry<241000?38405:38614);LootTemplate table;table.Entries={drops.at(entry)};activeTable=&table;
        LootGameObject chest;chest.info.entry=entry;chest.info.chest.chestPersonalLoot=(entry==240677||entry==241460)?60122:60124;chest.info.chest.chestLoot=entry;Loot loot;Open(player,chest,loot);loot.Take();assert(player.bags[drops.at(entry).itemid]==1);
        Player other;other.Accept(entry<241000?38405:38614);Loot separate;Open(other,chest,separate);separate.Take();assert(other.bags[drops.at(entry).itemid]==1);
    }
    {CooldownPlayer player;for(auto spawn:{std::pair<uint32,uint32>{241460,115519},{241462,115532}}){player.AddPlayerLootCooldown(spawn.first,spawn.second,TYPE_GUID,false);assert(player.IsPlayerLootCooldown(spawn.second,TYPE_GUID));assert(!player.IsPlayerLootCooldown(spawn.first,TYPE_GUID));}GameTime::now+=601;assert(!player.IsPlayerLootCooldown(115519,TYPE_GUID)&&!player.IsPlayerLootCooldown(115532,TYPE_GUID));}
    std::cout<<"PASS: both factions/all six collection orders, original empty personal chests and native script-loader exploration lock reproduced; actual selector/SendLoot/personal dispatch/RareOrGo and ordinary creature loot, mode0, lock, three item-only objectives, bank/removal/retry and consumption pass. World/inventory/packets and full death/FillLoot pipeline are fixtures; client pending.\n";
}
