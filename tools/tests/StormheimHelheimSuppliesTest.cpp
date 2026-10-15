// Extracted production quest handlers; world storage, inventory operations,
// object lookup and packet delivery use the existing Heart handoff fixture.
#include <cstring>
#include <fstream>
#include <crtdbg.h>
#include <cstdlib>
#include "HelheimSuppliesFixture.inc"
#include "HasQuestForItem.inc"
#include "ItemAddedQuestCheck.inc"
#include "ItemRemovedQuestCheck.inc"
#include "AdjustQuestReqItemCount.inc"

enum {GAMEOBJECT_TYPE_CHEST=3,GAMEOBJECT_TYPE_FISHINGHOLE=25,
    GAMEOBJECT_TYPE_GATHERING_NODE=50,GAMEOBJECT_TYPE_CHALLENGE_MODE_REWARD=51};
struct GameObjectTemplate {
    uint32 type=GAMEOBJECT_TYPE_CHEST;
    struct {uint32 chestLoot=0;} chest,fishingHole,gatheringNode,challengeModeReward;
    uint32 GetLootId() const;
};
#include "GetLootId.inc"
constexpr uint32 QUEST_FLAGS_EX_KEEP_ADDITIONAL_ITEMS=0x1;

// Actual item-consumption case from RewardQuest. The entry deletion operation
// is a fixture: this does not execute the full reward or inventory subsystem.
void Consume(Player& player, Quest const* quest)
{
    auto DestroyItemCount=[&](uint32 id,uint32 count,bool){assert(player.bags[id]>=count);player.bags[id]-=count;};
    for (auto const& obj:quest->GetObjectives())
        switch(obj.Type) {
#include "RewardItemCase.inc"
        }
}
enum {LOOT_ITEM_TYPE_CURRENCY=1,RATE_DROP_ITEM_REFERENCED=1,RATE_DROP_ZONE_LOOT=2,RATE_DROP_CURRENCY=3,CONFIG_DROP_DUNGEON_ONLY_X1=1};
int qualityToRate[]={0};uint32 randomCalls=0;float rolled=0;
bool roll_chance_f(float chance){++randomCalls;return rolled<chance;}
struct World {uint32 rateCalls=0;float getRate(int){++rateCalls;return 1;}bool getBoolConfig(int){return true;}} world;auto sWorld=&world;
struct CurrencyTypesEntry{};struct CurrencyStore {CurrencyTypesEntry const* LookupEntry(uint32){return nullptr;}} sCurrencyTypesStore;
struct LootStore {char const* GetName()const{return "gameobject_loot_template";}} lootStore;
struct LootStoreItem {uint32 itemid=124512,currencyid=0,reference=0,groupid=0,mincount=1,maxcount=1,type=LOOT_ITEM_TYPE_ITEM;float chance=100;bool needs_quest=true;uint32 lootmode=0,ClassificationMask=0;bool Roll(bool,bool=false,bool=false)const;bool IsValid(LootStore const&,uint32)const;};
#include "LootRollAdmission.inc"

// Execute the full native ordinary loot processor. World condition lookup,
// inventory writes and count RNG are fixtures; both rows are ungrouped and
// live SQL verifies no extra conditions. Quest visibility is checked through
// the actual Player::HasQuestForItem before taking generated items.
struct LootItem {struct {uint32 ItemID=0;} item;};
using LootItemList=std::vector<LootItem>;using LootStoreItemList=std::vector<LootStoreItem>;
struct Loot {
    Player* owner=nullptr;uint32 _DifficultyMask=1,_ClassificationMask=1,chance=0;bool isBoss=false,bonusLoot=false,_specCheck=false,maxCount=false;
    LootItemList items;LootStoreItemList generated;
    Player const* GetLootOwner()const{return owner;}
    void AddItem(LootStoreItem const& entry){generated.push_back(entry);}
    void Take(){for(auto const& drop:generated)if(owner->HasQuestForItem(drop.itemid)){uint32 n=maxCount?drop.maxcount:drop.mincount;owner->bags[drop.itemid]+=n;owner->ItemAddedQuestCheck(drop.itemid,n);}}
};
struct LootTemplate {
    struct Group {void Process(Loot&,LootTemplate const*)const{}void ProcessAutoGroup(Loot&,LootTemplate const*)const{}};
    using LootGroups=std::vector<Group>;LootGroups Groups,AutoGroups;LootStoreItemList Entries;bool _isZoneLoot=false,conditions=true;
    bool CheckItemCondition(Player const*,uint32,uint32)const{return conditions;}
    void Process(Loot&,bool,uint8=0)const;
};
struct References {LootTemplate const* GetLootFor(uint32)const{return nullptr;}} LootTemplates_Reference;
#include "LootProcess.inc"

using SkillType=int;using LockType=int;
enum {SKILL_NONE=0,SKILL_LOCKPICKING=1,SKILL_HERBALISM=2,SKILL_MINING=3,SKILL_FISHING=4,SKILL_INSCRIPTION=5,SKILL_ARCHAEOLOGY=6,
 LOCKTYPE_PICKLOCK=1,LOCKTYPE_HERBALISM=2,LOCKTYPE_MINING=3,LOCKTYPE_FISHING=19,LOCKTYPE_INSCRIPTION=20,LOCKTYPE_ARCHAELOGY=22,
 LOCK_KEY_ITEM=1,LOCK_KEY_SKILL=2,LOCK_KEY_SPELL=3,MAX_LOCK_CASE=8,TYPEID_PLAYER=4,TARGET_GAMEOBJECT_ITEM_TARGET=23,
 SPELL_FAILED_LOW_CASTLEVEL=2,SPELL_FAILED_CUSTOM_ERROR=3,ARMY_UNIT_BERSERK=0,
 SPELL_CUSTOM_ERROR_REQUIRES_2_WITHERED_BERSERKER=1,SPELL_CUSTOM_ERROR_REQUIRES_AT_LEAST_10_WITHERED=2};
struct LockEntry {uint32 Type[8]{0,2},Index[8]{0,13},Skill[8]{};};
struct LockStore {LockEntry value;LockEntry const* LookupEntry(uint32 id){return id==2160?&value:nullptr;}} locks;auto& sLockStore=locks;
struct Item {uint32 GetEntry(){return 0;}};
struct LockEffect {int MiscValue=13;struct Target {uint32 GetTarget(){return 23;}} TargetA,TargetB;int CalcValue(Unit*,void*){return 0;}};
struct LockSpellInfo {uint32 Id=6478;LockEffect effect;LockEffect* GetEffect(uint32,uint32){return &effect;}};
struct Spell {
    Unit* m_caster;LockSpellInfo info;LockSpellInfo* m_spellInfo=&info;uint32 m_diffMode=0;Item* m_CastItem=nullptr;uint32 m_customError=0;
    SpellCastResult CanOpenLock(uint32,uint32,SkillType&,int32&,int32&);
};
#include "CanOpenLock.inc"

int main(int argc,char** argv)
{
    _set_abort_behavior(0,_WRITE_ABORT_MSG|_CALL_REPORTFAULT);
    _CrtSetReportMode(_CRT_ASSERT,_CRTDBG_MODE_FILE);_CrtSetReportFile(_CRT_ASSERT,_CRTDBG_FILE_STDERR);
    assert(argc==3);std::ifstream bones(argv[1]),seaweed(argv[2]);uint32 entry,selector,item,quest,mode,group,min,max,currency,reference,shared;float chance;
    bones>>entry>>selector>>item>>chance>>quest>>mode>>group>>min>>max;
    assert(bones&&entry==240586&&selector==entry&&item==122611&&chance==100&&quest==1&&mode==1&&!group&&min==1&&max==1);
    GameObjectTemplate pile;assert(!pile.GetLootId());pile.chest.chestLoot=selector;assert(pile.GetLootId()==240586);
    LootStoreItem bone;bone.itemid=item;bone.chance=chance;bone.mincount=min;bone.maxcount=max;bone.lootmode=mode;
    std::vector<LootStoreItem> kelp;
    while(seaweed>>entry>>item>>currency>>reference>>chance>>quest>>mode>>group>>min>>max>>shared){
        assert((entry==91575||entry==93377)&&item==122619&&!currency&&!reference&&quest==1&&!mode&&!group&&min==1&&max==5&&!shared);
        LootStoreItem drop;drop.itemid=item;drop.chance=chance;drop.mincount=min;drop.maxcount=max;drop.lootmode=mode;assert(drop.IsValid(lootStore,entry));kelp.push_back(drop);
    }
    assert(kelp.size()==2&&bone.IsValid(lootStore,240586));
    store.quests[39837].id=39837;
    for(auto id:{38324u,38339u}){auto& q=store.quests[id];q.id=id;q.PrevQuestID=39837;q.NextQuestID=38347;q.NextQuestIdChain=38347;q.ExclusiveGroup=-38324;q.special=QUEST_SPECIAL_FLAGS_DELIVER;q.objectives={{QUEST_OBJECTIVE_ITEM,id==38324?122611u:122619u,id==38324?8u:20u,0,0}};store.mExclusiveQuestGroups.emplace(-38324,id);}
    auto& next=store.quests[38347];next.id=38347;next.PrevQuestID=38324;store.starters.emplace(91531,38347);
    QuestMap _questTemplates;for(auto& row:store.quests)_questTemplates[row.first]=&row.second;
    for(auto const& row:_questTemplates){auto qinfo=row.second;
#include "QuestPrevLoader.inc"
    }
    Creature colborn;colborn.entry=91531;colborn.guid={11546819};creatures[colborn.guid]=&colborn;
    for(auto team:{ALLIANCE,HORDE})for(auto first:{38324u,38339u}){
        Player p;p.team=team;p.race=team==ALLIANCE?1:2;
        for(auto id:{38324u,38339u})assert(!p.CanTakeQuest(&store.quests[id],false));p.m_RewardedQuests.insert(39837);
        for(auto id:{38324u,38339u}){assert(p.CanTakeQuest(&store.quests[id],false));p.Accept(id);}
        assert(p.HasQuestForItem(122611)&&p.HasQuestForItem(122619)&&!p.CanTakeQuest(&next,false));
        Spell cast;cast.m_caster=&p;SkillType skill=0;int32 req=0,value=0;
        assert(cast.CanOpenLock(0,2160,skill,req,value)==SPELL_CAST_OK);
        cast.info.effect.MiscValue=5;assert(cast.CanOpenLock(0,2160,skill,req,value)!=SPELL_CAST_OK);
        // Opening alone and killing either source do not grant either item.
        p.KillCreditGO(240586,{127672});p.KilledMonsterCredit(91575);p.KilledMonsterCredit(93377);assert(!p.GetQuestObjectiveData(38324,122611)&&!p.GetQuestObjectiveData(38339,122619));
        for(uint32 n=1;n<=8;++n){Loot loot;loot.owner=&p;LootTemplate t;t.Entries={bone};t.Process(loot,true);assert(loot.generated.size()==1);loot.Take();assert(p.GetQuestObjectiveData(38324,122611)==int32(n));}
        assert(p.CanRewardQuest(&store.quests[38324],false)&&!p.CanTakeQuest(&next,false));
        for(auto const& drop:kelp){
            LootTemplate t;t.Entries={drop};Loot fail;fail.owner=&p;rolled=99;t.Process(fail,true);assert(fail.generated.empty());
            Loot pass;pass.owner=&p;rolled=0;t.Process(pass,true);assert(pass.generated.size()==1); // zero mode is a wildcard
            t.Entries[0].lootmode=2;Loot wrong;wrong.owner=&p;t.Process(wrong,true);assert(wrong.generated.empty());
            t.Entries[0].lootmode=0;t.conditions=false;t.Process(wrong,true);assert(wrong.generated.empty());
        }
        for(uint32 n=1;n<=4;++n){Loot loot;loot.owner=&p;loot.maxCount=true;LootTemplate t;t.Entries={kelp[n%2]};t.Process(loot,true);loot.Take();assert(p.GetQuestObjectiveData(38339,122619)==int32(n*5));}
        assert(p.completions==2&&!p.HasQuestForItem(122611)&&!p.HasQuestForItem(122619));
        // Removal, reload counts, bank-only reward refusal and required item consumption.
        p.ItemRemovedQuestCheck(122619,1);--p.bags[122619];assert(p.HasQuestForItem(122619));++p.bags[122619];p.ItemAddedQuestCheck(122619,1);
        p.bank[122611]=8;p.bags[122611]=0;p.AdjustQuestReqItemCount(&store.quests[38324]);assert(p.GetQuestObjectiveData(38324,122611)==8&&!p.CanRewardQuest(&store.quests[38324],false));p.bank[122611]=0;p.bags[122611]=8;
        for(auto id:{first,first==38324?38339u:38324u}){auto q=&store.quests[id];assert(p.CanRewardQuest(q,false));Consume(p,q);p.m_RewardedQuests.insert(id);p.quests.erase(id);assert(p.CanTakeQuest(&next,false)==(p.m_RewardedQuests.count(38324)&&p.m_RewardedQuests.count(38339)));}
        assert(!p.bags[122611]&&!p.bags[122619]&&p.GetNextQuest(colborn.GetGUID(),&store.quests[first])==&next);
    }
    for(auto id:{38324u,38339u}){auto q=&store.quests[id];uint32 item=q->objectives[0].ObjectID,amount=q->objectives[0].Amount;Player p;p.Accept(id);p.bags[item]=3;p.AdjustQuestReqItemCount(q);assert(p.GetQuestObjectiveData(id,item)==3);p.quests.erase(id);assert(!p.HasQuestForItem(item));p.Accept(id);p.AdjustQuestReqItemCount(q);assert(p.GetQuestObjectiveData(id,item)==3);p.bags[item]=amount+1;p.AdjustQuestReqItemCount(q);assert(p.GetQuestObjectiveData(id,item)==int32(amount));p.CompleteQuest(id);Consume(p,q);assert(p.bags[item]==1);}
    std::cout<<"PASS: native Gathering2160/6478 lock, chest selector, full ordinary loot Process/Roll including wildcard mode0 and failed rolls,8 bones/20 seaweed actual item updates, retry/removal/bank/reload/reward consumption, both factions and both parallel reward orders before38347. World, drop-count RNG, inventory and network are fixtures.\n";
}
