#include <crtdbg.h>
#include <fstream>
#include <sstream>
#include <algorithm>
constexpr int TYPEID_UNIT=0,TYPEID_ITEM=1,TYPEID_CONTAINER=2,TYPEID_PLAYER=4,TYPEID_GAMEOBJECT=5;
constexpr int GAMEOBJECT_TYPE_QUESTGIVER=2,INVALIDREASON_QUEST_ALREADY_ON=1,DISABLE_TYPE_QUEST=0;
constexpr int QUEST_SPECIAL_FLAGS_SPEAKTO=128,MAX_KILL_CREDIT=2;
constexpr int UNIT_FIELD_NPC_FLAGS2=16,UNIT_FIELD_FLAGS_2=17,UNIT_FLAG2_ALLOW_ENEMY_INTERACT=0x4000;
constexpr int CREATURE_TYPEFLAGS_GHOST=2,CREATURE_TYPEFLAGS_DEAD_INTERACT=0x80,REP_UNFRIENDLY=1;
constexpr float INTERACTION_DISTANCE=5;
#include "SpineFixture.inc"
#include "NativeGossipUse.inc"
#include "Talked.inc"
namespace DisableMgr{bool IsDisabledFor(int,uint32,Player*){return false;}}
struct FactionEntry{bool CanHaveReputation()const{return false;}};
struct FactionTemplateEntry{uint32 Faction=148;};
struct FactionStore{FactionEntry row;FactionEntry* LookupEntry(uint32){return &row;}} sFactionStore;
struct FactionTemplateStore{FactionTemplateEntry row;FactionTemplateEntry* LookupEntry(uint32){return &row;}} sFactionTemplateStore;
namespace ObjectAccessor{Object* GetObjectByTypeMask(Player& p,ObjectGuid g,int){return GetCreature(p,g);}}
constexpr int TYPEMASK_UNIT=1,TYPEMASK_GAMEOBJECT=2;
struct Item:WorldObject{struct Template{uint32 GetMaxCount(){return 0;}} info;Template* GetTemplate(){return &info;}uint32 GetBagSlot(){return 0;}uint32 GetSlot(){return 0;}};
struct ScriptMgr{template<class T>void OnQuestAccept(Player*,T*,Quest const*){}} scriptMgr;auto sScriptMgr=&scriptMgr;
#include "CanTake.inc"
#include "Status.inc"
#include "Accept.inc"
#include "Interact.inc"
bool WorldObject::hasInvolvedQuest(uint32 q){return GetEntry()==97986&&q==40004;}
namespace WorldPackets{namespace Quest{struct QuestGiverRequestReward{ObjectGuid QuestGiverGUID;uint32 QuestID;};}}
struct WorldSession{Player* _player;void HandleQuestGiverRequestReward(WorldPackets::Quest::QuestGiverRequestReward&);};
#include "RequestReward.inc"

#include "NativeKilledMonster.inc"
struct KillRewarder {Group* _group=nullptr;Unit* _victim=nullptr;void _RewardKillCredit(Player*);void _RewardPlayer(Player* p,bool){_RewardKillCredit(p);}void SelectGroup(){bool isDungeon=false;
#include "NativeGroupSelection.inc"
}};
#include "NativeKillReward.inc"
using Difficulty=unsigned;
struct CreatureSpell {std::vector<Difficulty> Difficulties;bool CanUseInDifficulty(Difficulty)const;};
#include "NativeSpellDifficulty.inc"
std::pair<Player*,bool> ResolveKiller(Unit& attacker,Unit* victim)
{
    auto GetCharmerOrOwnerPlayerOrPlayerItself=[&]{return attacker.GetCharmerOrOwnerPlayerOrPlayerItself();};
#include "KillOwner.inc"
    return {player,isRewardAllowed};
}
void Deliver(Player& p){auto events=std::move(p.events);p.events.clear();for(auto& f:events)f();}
int main(int argc,char** argv)
{
    _set_abort_behavior(0,_WRITE_ABORT_MSG|_CALL_REPORTFAULT);_CrtSetReportMode(_CRT_ASSERT,_CRTDBG_MODE_FILE);_CrtSetReportFile(_CRT_ASSERT,_CRTDBG_FILE_STDERR);
    assert(argc==2);std::ifstream input(argv[1]);assert(input);std::string line;unsigned objectives=0,spells=0;
    auto& q=store.quests[40004];q.id=40004;
    while(std::getline(input,line)){
        std::istringstream in(line);std::string kind;in>>kind;
        if(kind=="quest"){uint32 id,type,level,flagsEx,item,reward,next;int exclusive;uint64 races;in>>id>>type>>level>>q.flags>>flagsEx>>races>>item>>reward>>q.PrevQuestID>>next>>exclusive>>q.special;assert(id==40004&&type==2&&level==98&&!item&&!reward&&!next);assert(q.PrevQuestID==40002&&exclusive==-40003);q.AllowableRaces=races;}
        if(kind=="objective"){uint32 id,flags;int index;QuestObjective o;in>>id>>o.Type>>o.ObjectID>>o.Amount>>index>>flags;assert(flags==1&&o.Type==0&&o.Amount==1&&index>=3&&index<=6);o.Flags=flags;o.StorageIndex=int8(index);q.objectives.push_back(o);++objectives;}
        if(kind=="spell"){unsigned entry,spell,mask;std::string restrictions;in>>entry>>spell>>mask;in>>restrictions;assert(!mask&&restrictions.empty());CreatureSpell data;assert(data.CanUseInDifficulty(0));++spells;}
    }
    assert(objectives==4&&spells==10);store.quests[40002].id=40002;if(q.PrevQuestID)q.prevQuests={q.PrevQuestID};q.special|=QUEST_SPECIAL_FLAGS_KILL; // Native quest loader derives this from MONSTER objectives.
    Creature giver;giver.entry=97986;giver.guid={338656};giver.fields[UNIT_FIELD_NPC_FLAGS]=3;creatures[giver.guid]=&giver;SmartAI giverAI(&giver);Creature starter;starter.entry=97979;SmartAI starterAI(&starter);
    std::vector<uint32> order{97822,97859,97942,97944};
    for(auto team:{ALLIANCE,HORDE})for(bool petKill:{false,true}){
        do{
            Player p;p.guid={1000+team+unsigned(petKill)};p.team=team;p.race=team==ALLIANCE?1:2;
            p.m_RewardedQuests.insert(40002);assert(p.CanTakeQuest(&q,false));p.AddQuestAndCheckCompletion(&q,&starter);assert(!p.CanCompleteQuest(40004)&&!p.CanRewardQuest(&q,false));
            p.KilledMonsterCredit(97948);p.KilledMonsterCredit(105123);p.TalkedToCreature(97948,{});assert(!p.packets);
            unsigned kills=0;
            for(auto entry:order){
                Creature victim;victim.entry=entry;victim.guid={entry};victim.data.Entry=entry;victim.data.KillCredit[0]=97948;victim.data.KillCredit[1]=105123;creatures[victim.guid]=&victim;
                Unit pet;pet.ownerPlayer=&p;auto resolved=ResolveKiller(petKill?pet:static_cast<Unit&>(p),&victim);assert(resolved.first==&p&&resolved.second);
                KillRewarder reward;reward._victim=&victim;reward._RewardKillCredit(resolved.first);assert(p.delays.back()==50&&!p.GetQuestObjectiveData(40004,entry));
                Deliver(p);++kills;assert(p.GetQuestObjectiveData(40004,entry)==1&&p.packets==kills&&p.completions==unsigned(kills==4));
                p.KilledMonster(&victim.data,victim.guid);assert(p.packets==kills);creatures.erase(victim.guid);
            }
            assert(p.CanRewardQuest(&q,false)&&!p.CanTakeQuest(&q,false));
            WorldSession session{&p};WorldPackets::Quest::QuestGiverRequestReward request{giver.guid,40004};session.HandleQuestGiverRequestReward(request);assert(p.menu.offers==1);
            giver.near=false;session.HandleQuestGiverRequestReward(request);assert(p.menu.offers==1);giver.near=true;
            p.quests.clear();p.m_RewardedQuests.insert(40004);assert(!p.CanTakeQuest(&q,false));p.KilledMonsterCredit(order[0]);assert(p.packets==4);
        }while(std::next_permutation(order.begin(),order.end()));
    }
    Creature victim;victim.entry=97822;victim.data.Entry=97822;victim.guid={97822};creatures[victim.guid]=&victim;KillRewarder reward;reward._victim=&victim;
    {Player p;reward._RewardKillCredit(&p);Deliver(p);assert(!p.packets);p.Accept(40004);reward._RewardKillCredit(&p);p.quests.clear();Deliver(p);assert(!p.packets);p.Accept(40004);reward._RewardKillCredit(&p);creatures.erase(victim.guid);Deliver(p);assert(!p.packets);creatures[victim.guid]=&victim;}
    {Player p;p.Accept(40004);p.KilledMonsterCredit(97822);p.quests.clear();p.Accept(40004);assert(!p.GetQuestObjectiveData(40004,97822)&&!p.CanCompleteQuest(40004));}
    {Player killer,nearby,far,released,disconnected;for(auto p:{&killer,&nearby,&far,&released,&disconnected})p->Accept(40004);far.rewardRange=false;released.alive=false;released.corpse=true;disconnected.contact=false;
     GroupReference e{&disconnected,nullptr},d{&released,&e},c{&far,&d},b{&nearby,&c},a{&killer,&b};Group group;group.first=&a;reward._group=&group;reward.SelectGroup();
     for(auto p:{&killer,&nearby,&far,&released,&disconnected})Deliver(*p);assert(killer.GetQuestObjectiveData(40004,97822)==1&&nearby.GetQuestObjectiveData(40004,97822)==1&&!far.packets&&!released.packets&&!disconnected.packets);}
    {Unit ambient;assert(!ResolveKiller(ambient,&victim).first);victim.rewardAllowed=false;Player p;assert(!ResolveKiller(p,&victim).second);}
    std::cout<<"PASS: native kill owner/reward callback/template credit/completion and questgiver reward admission;24 kill orders, both factions, player/pet blows, unrelated credit/talk, repeat cap, missing actor, unaccepted/rewarded, abandon/reaccept and group range/life/contact. World, storage,50ms clock and packet delivery are fixtures; client pending.\n";
}
