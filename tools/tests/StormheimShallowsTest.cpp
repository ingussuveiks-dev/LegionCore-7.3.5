// Native selectors, loot fill/roll/admission, quest handlers and reward request.
// Storage, RNG, inventory writes, world/session access and packet delivery are adapters.
struct GameObjectTemplate;struct Loot;
struct PersonalLootData {unsigned chance=0;};
#include "JournalsFixture.inc"
using Object=WorldObject;
constexpr uint32 TYPEMASK_UNIT=1,TYPEMASK_GAMEOBJECT=2,SPELL_EFFECT_HANDLE_HIT_TARGET=1;
#include "JournalRelations.inc"
#include "QuestMenu.inc"
bool WorldObject::hasInvolvedQuest(uint32 id){if(auto go=dynamic_cast<GameObject*>(this))return go->hasInvolvedQuest(id);auto bounds=store.enders.equal_range(GetEntry());for(auto i=bounds.first;i!=bounds.second;++i)if(i->second==id)return true;return false;}
namespace ObjectAccessor {Object* GetObjectByTypeMask(Player& player,ObjectGuid guid,uint32){return guid.high==HighGuid::GameObject?static_cast<Object*>(GetGameObject(player,guid)):GetCreature(player,guid);}}
namespace WorldPackets {namespace Quest {struct QuestGiverRequestReward {ObjectGuid QuestGiverGUID;uint32 QuestID;};}}
struct WorldSession {Player* _player;void HandleQuestGiverRequestReward(WorldPackets::Quest::QuestGiverRequestReward&);};
#include "JournalRequest.inc"
#include "ShallowsCredit.inc"
LootStore LootTemplates_Gameobject;
std::map<uint32,LootTemplate> tables;
void Loot::FillLoot(uint32 id,LootStore const&,Player* player,bool,bool,GameObject*){++fills;owner=player;tables.at(id).Process(*this,true);}
void Player::TestFill(GameObject* go,Loot* loot,Loot* lootPesonal,bool personal)
{
    uint32 lootid=go->GetGOInfo()->GetLootId();
    #include "ShallowsFill.inc"
}
int main(int argc,char** argv)
{
    _set_abort_behavior(0,_WRITE_ABORT_MSG|_CALL_REPORTFAULT);_CrtSetReportMode(_CRT_ASSERT,_CRTDBG_MODE_FILE);_CrtSetReportFile(_CRT_ASSERT,_CRTDBG_FILE_STDERR);
    assert(argc==2);std::ifstream input(argv[1]);assert(input);std::string line;
    while(std::getline(input,line)){
        std::istringstream in(line);std::string kind;in>>kind;
        if(kind=="quest"){uint32 id;in>>id;auto& q=store.quests[id];q.id=id;in>>q.flags>>q.FlagsEx>>q.AllowableRaces>>q.SourceItemId>>q.SourceItemIdCount>>q.NextQuestIdChain>>q.PrevQuestID>>q.NextQuestID>>q.special;}
        if(kind=="objective"){uint32 id,objective;int index;QuestObjective o;in>>id>>objective>>o.Type>>o.ObjectID>>o.Amount>>index>>o.Flags;o.StorageIndex=int8(index);store.quests[id].objectives.push_back(o);}
        if(kind=="starter"||kind=="npc-ender"){uint32 npc,q;in>>npc>>q;(kind=="starter"?store.starters:store.enders).emplace(npc,q);}
        if(kind=="loot"){uint32 entry,required;LootStoreItem d;in>>entry>>d.itemid>>d.chance>>required>>d.lootmode>>d.groupid>>d.mincount>>d.maxcount>>d.reference>>d.currencyid;d.needs_quest=required;itemStore.items[d.itemid].stack=1;assert(d.IsValid(lootStore,entry));tables[entry].Entries.push_back(d);}
    }
    auto& q=store.quests.at(40046);q.special|=QUEST_SPECIAL_FLAGS_DELIVER|QUEST_SPECIAL_FLAGS_CAST;
    assert(q.objectives.size()==2&&q.objectives[1].Amount==99&&q.objectives[1].Flags==28);
    Creature havi;havi.entry=97973;havi.guid={369621};SmartAI ai(&havi);creatures[havi.guid]=&havi;
    Creature other;other.entry=1;other.guid={2};creatures[other.guid]=&other;
    GameObject debris;debris.entry=252239;debris.guid={114950};debris.guid.high=HighGuid::GameObject;GameObjectTemplate data;debris.data=&data;objects[debris.guid]=&debris;
    for(auto team:{ALLIANCE,HORDE}){
        Player p;p.team=team;p.race=team==ALLIANCE?1:2;
        p.PrepareQuestMenu(havi.guid);assert(p.menu.quests.items.at(40046)==2);assert(p.CanTakeQuest(&q,false));p.AddQuestAndCheckCompletion(&q,&havi);
        Spell cast;cast.m_caster=&p;SkillType skill=0;int32 req=0,value=0;assert(cast.CanOpenLock(0,1691,skill,req,value)==SPELL_CAST_OK);cast.info.effect.MiscValue=13;assert(cast.CanOpenLock(0,1691,skill,req,value)!=SPELL_CAST_OK);
        Loot shared,personal;data.chest.chestLoot=0;p.TestFill(&debris,&shared,&personal);assert(!shared.fills&&shared.generated.empty()); // Original live failure.
        data.chest.chestLoot=252239;rolled=0;p.TestFill(&debris,&shared,&personal);assert(shared.fills==1&&shared.generated.size()==5&&!p.CanCompleteQuest(40046));
        // The chest's native credit effect is unrelated; no item is created.
        cast.info.effect.MiscValue=114487;cast.unitTarget=&p;cast.EffectKillCredit(0);assert(!p.bags[129174]&&!p.CanCompleteQuest(40046));
        for(unsigned n=0;n<99;++n)p.KillCreditGO(252239,debris.guid);assert(!p.CanCompleteQuest(40046)&&!p.completions);
        WorldSession session{&p};WorldPackets::Quest::QuestGiverRequestReward reward{havi.guid,40046};session.HandleQuestGiverRequestReward(reward);assert(p.menu.offers.empty());
        rolled=99;p.TestFill(&debris,&shared,&personal);assert(shared.generated.empty());
        rolled=29.657f;p.TestFill(&debris,&shared,&personal);assert(shared.generated.empty()); // Just above all existing chances.
        rolled=29.65f;p.TestFill(&debris,&shared,&personal);assert(shared.generated.size()==1&&shared.generated[0].itemid==129174);Take(p,shared);
        assert(p.GetQuestObjectiveData(40046,129174)==1&&p.completions==1&&p.CanRewardQuest(&q,false)&&!p.HasQuestForItem(129174));
        p.TestFill(&debris,&shared,&personal);Take(p,shared);assert(p.bags[129174]==1&&p.completions==1); // Native quest-loot admission suppresses extra plaque.
        WorldPackets::Quest::QuestGiverRequestReward wrong{other.guid,40046};session.HandleQuestGiverRequestReward(wrong);assert(p.menu.offers.empty());session.HandleQuestGiverRequestReward(reward);assert(p.menu.offers.size()==1);
        p.bags[129174]=0;p.bank[129174]=1;assert(!p.CanRewardQuest(&q,false));p.bank[129174]=0;p.bags[129174]=1;
        p.ItemRemovedQuestCheck(129174,1);p.bags[129174]=0;assert(!p.CanCompleteQuest(40046)&&p.HasQuestForItem(129174));p.bags[129174]=1;p.ItemAddedQuestCheck(129174,1);assert(p.CanRewardQuest(&q,false));
        Consume(p,&q);assert(!p.bags[129174]);p.quests.erase(40046);p.m_RewardedQuests.insert(40046);assert(!p.CanTakeQuest(&q,false));
        Player retry;retry.team=team;retry.race=p.race;retry.Accept(40046);retry.bags[129174]=1;retry.AdjustQuestReqItemCount(&q);assert(retry.CanCompleteQuest(40046)); // Optional99 is not a completion gate.
        retry.quests.clear();assert(!retry.HasQuestForItem(129174));retry.Accept(40046);retry.bags[129174]=0;retry.AdjustQuestReqItemCount(&q);assert(!retry.CanCompleteQuest(40046)&&retry.HasQuestForItem(129174));
        Player noQuest;assert(!shared.AllowedForPlayer(&noQuest,129174,0,LOOT_ITEM_TYPE_ITEM,true));assert(shared.AllowedForPlayer(&noQuest,139542,0,LOOT_ITEM_TYPE_ITEM,false));
        Player partner;partner.Accept(40046);Loot partnerLoot;rolled=0;partner.TestFill(&debris,&partnerLoot,&personal);Take(partner,partnerLoot);assert(partner.CanRewardQuest(&q,false)&&!p.bags[129174]);
    }
    std::cout<<"PASS: native missing/fixed SendLoot fill gate, all five original loot rolls, lock admission, unrelated chest credit, both factions, optional99 not required, one plaque objective, misses/retries/no duplicates, bank/removal/reward consumption and correct NPC reward admission. Full world/casting/loot-session/inventory/packet/client services remain adapters.\n";
}
