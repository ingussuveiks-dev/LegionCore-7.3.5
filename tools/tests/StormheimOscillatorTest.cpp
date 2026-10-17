#include <sstream>
#include "OscillatorFixture.inc"
enum {ITEM_CLASS_MISCELLANEOUS=15,ITEM_CLASS_QUEST=12,ITEM_QUALITY_UNCOMMON=2,ITEM_QUALITY_RARE=3};
#include "NativeRareLoot.inc"
struct LootGameObject {GameObjectTemplate info;uint32 GetEntry(){return info.entry;}GameObjectTemplate const* GetGOInfo()const{return &info;}bool IsPersonal()const;};
#include "NativePersonal.inc"
LootTemplate const* activeTable=nullptr;
void Loot::FillLoot(uint32 lootId,LootStore const& store,Player* player,bool,bool,void* from){
    assert(lootId==240286);++fillCalls;owner=player;
    isRareOrGo=static_cast<LootGameObject*>(from)->IsPersonal();
    auto tab=activeTable;bool _isEmissaryLoot=false,_isTokenLoot=false,_isItemLoot=false,_IsPvPLoot=false,isOploteChest=false;uint32 _challengeLevel=0;void* _challenge=nullptr;
    #include "NativeLootDispatch.inc"
}
auto const& LootTemplates_Gameobject=lootStore;
void Open(Player& player,LootGameObject& object,Loot& personalLoot){
    auto go=&object;uint32 lootid=go->GetGOInfo()->GetLootId();bool personal=go->IsPersonal();auto lootPesonal=&personalLoot;
    auto GetMap=[&]{return player.GetMap();};
    #include "NativePersonalOpen.inc"
}
constexpr uint8 TYPE_GO=0,TYPE_GUID=3,TYPE_ZONE=4;
constexpr uint8 DIFFICULTY_NONE=0,DIFFICULTY_NORMAL=1,DIFFICULTY_HEROIC=2,DIFFICULTY_MYTHIC_KEYSTONE=8,DIFFICULTY_HC_SCENARIO=11,DIFFICULTY_N_SCENARIO=12,DIFFICULTY_MYTHIC_DUNGEON=23;
constexpr uint32 HOUR=3600;
namespace GameTime {uint32 now=1000;uint32 GetGameTime(){return now;}}
struct playerLootCooldown {uint32 entry=0,respawnTime=0;uint8 type=0,difficultyID=0;bool state=false;};
using PlayerLootCooldownMap=std::map<uint32,std::map<uint8,playerLootCooldown>>;
struct CooldownPlayer {std::map<uint8,PlayerLootCooldownMap> m_playerLootCooldown;FakeMap map;FakeMap* GetMap(){return &map;}void AddPlayerLootCooldown(uint32,uint32,uint8=0,bool=true,uint8=0);bool IsPlayerLootCooldown(uint32,uint8=0,uint8=0);};
#include "NativeLootCooldown.inc"
int main(int argc,char** argv){
    _set_abort_behavior(0,_WRITE_ABORT_MSG|_CALL_REPORTFAULT);_CrtSetReportMode(_CRT_ASSERT,_CRTDBG_MODE_FILE);_CrtSetReportFile(_CRT_ASSERT,_CRTDBG_FILE_STDERR);
    assert(argc==2);std::ifstream input(argv[1]);assert(input);std::string line;LootStoreItem drop;
    while(std::getline(input,line)){std::istringstream in(line);std::string kind;in>>kind;
        if(kind=="quest"){uint32 id,start;in>>id;auto& q=store.quests[id];q.id=id;in>>q.flags>>q.FlagsEx>>q.AllowableRaces>>start>>q.NextQuestIdChain>>q.PrevQuestID>>q.NextQuestID>>q.special;assert(!start);}
        if(kind=="objective"){uint32 id;int index;QuestObjective o;in>>id>>o.Type>>o.ObjectID>>o.Amount>>index>>o.Flags;o.StorageIndex=int8(index);assert(id==278005&&o.Type==QUEST_OBJECTIVE_ITEM&&o.ObjectID==123887&&o.Amount==1&&!index&&!o.Flags);store.quests[38558].objectives.push_back(o);}
        if(kind=="loot"){uint32 entry,selector,required;in>>entry>>selector>>drop.itemid>>drop.chance>>required>>drop.lootmode>>drop.groupid>>drop.mincount>>drop.maxcount;assert(entry==240286&&selector==entry&&drop.itemid==123887&&drop.chance==100&&required==1&&drop.lootmode==1&&!drop.groupid&&drop.mincount==1&&drop.maxcount==1);}
    }
    auto& q=store.quests[38558];q.special|=QUEST_SPECIAL_FLAGS_DELIVER;store.quests[39800].id=39800;store.starters.emplace(90866,38058);
    QuestMap _questTemplates;for(auto& row:store.quests)_questTemplates[row.first]=&row.second;for(auto const& row:_questTemplates){auto qinfo=row.second;
        #include "QuestPrevLoader.inc"
    }
    Creature giver;giver.entry=90866;giver.guid={290453};SmartAI giverAI(&giver);creatures[giver.guid]=&giver;
    LootTemplate table;table.Entries={drop};activeTable=&table;
    for(auto team:{ALLIANCE,HORDE}){ // Preserve live unrestricted mask; ordinary admission still needs Alliance39800 history.
        Player player;player.team=team;player.race=team==ALLIANCE?1:2;assert(!player.CanTakeQuest(&q,false));player.m_RewardedQuests.insert(39800);assert(player.CanTakeQuest(&q,false));player.Accept(38558);
        LootGameObject chest;Loot loot;assert(chest.IsPersonal());Open(player,chest,loot);assert(!loot.fillCalls&&loot.generated.empty()); // Actual native Data1=0 guard reproduces the empty personal chest.
        chest.info.chest.chestLoot=240286;SkillType skill=0;int32 req=0,value=0;Spell cast;cast.m_caster=&player;cast.info.Id=3365;cast.info.effect.MiscValue=5;assert(cast.CanOpenLock(0,1690,skill,req,value)==SPELL_CAST_OK);cast.info.effect.MiscValue=13;assert(cast.CanOpenLock(0,1690,skill,req,value)!=SPELL_CAST_OK);
        player.KillCreditGO(240286,{114753});player.KilledMonsterCredit(240286);assert(!player.GetQuestObjectiveData(38558,123887));
        Open(player,chest,loot);assert(loot.fillCalls==1&&loot.personal&&loot.objType==3&&loot.generated.size()==1&&!player.completions); // Opening alone does not collect.
        loot.Take();assert(player.bags[123887]==1&&player.GetQuestObjectiveData(38558,123887)==1&&player.completions==1&&!player.HasQuestForItem(123887));
        assert(player.CanRewardQuest(&q,false)&&!player.CanTakeQuest(&store.quests[38058],false));
        player.bank[123887]=1;player.bags[123887]=0;player.AdjustQuestReqItemCount(&q);assert(!player.CanRewardQuest(&q,false));player.bank[123887]=0;player.bags[123887]=1;
        Consume(player,&q);player.quests.erase(38558);player.m_RewardedQuests.insert(38558);RewardNext(player,giver,&q);assert(!player.bags[123887]&&player.CanTakeQuest(&store.quests[38058],false)&&player.menu.detailed.size()==1&&player.menu.detailed[0]==38058);
        Player other;other.Accept(38558);Loot separate;Open(other,chest,separate);separate.Take();assert(other.completions==1&&!player.bags[123887]); // Same public object, separate loot storage.
    }
    {Player player;player.Accept(38558);LootGameObject chest;chest.info.chest.chestLoot=240286;Loot loot;
        table.conditions=false;Open(player,chest,loot);assert(loot.generated.empty());table.conditions=true;
        loot._DifficultyMask=2;Open(player,chest,loot);assert(loot.generated.empty());loot._DifficultyMask=1;
        player.bags[123887]=1;player.AdjustQuestReqItemCount(&q);assert(player.GetQuestObjectiveData(38558,123887)==1);player.quests.clear();player.Accept(38558);player.AdjustQuestReqItemCount(&q);assert(player.GetQuestObjectiveData(38558,123887)==1);player.ItemRemovedQuestCheck(123887,1);player.bags[123887]=0;assert(player.HasQuestForItem(123887));Open(player,chest,loot);loot.Take();assert(player.bags[123887]==1);
    }
    {CooldownPlayer player;player.AddPlayerLootCooldown(240286,114753,TYPE_GUID,false);assert(player.IsPlayerLootCooldown(114753,TYPE_GUID));assert(!player.IsPlayerLootCooldown(240286,TYPE_GUID));GameTime::now+=601;assert(!player.IsPlayerLootCooldown(114753,TYPE_GUID));}
    std::cout<<"PASS: original empty personal chest reproduced through native selector/SendLoot guard; native personal flags/loot dispatch/RareOrGo processor, lock1690/Open3365, actual item completion/consumption and38058 reward-next admission pass. Repeat migration/personal settings, separate players, conditions/difficulty, bag/bank/retry and native600s GUID cooldown pass. World/inventory/FillLoot setup and packets are fixtures; client pending.\n";
}
