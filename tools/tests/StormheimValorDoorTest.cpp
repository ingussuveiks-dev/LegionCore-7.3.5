#include <crtdbg.h>
#include <fstream>
#include <sstream>
constexpr int TYPEID_UNIT=0,TYPEID_ITEM=1,TYPEID_CONTAINER=2,TYPEID_PLAYER=4,TYPEID_GAMEOBJECT=5;
constexpr int GAMEOBJECT_TYPE_QUESTGIVER=2,INVALIDREASON_QUEST_ALREADY_ON=1,DISABLE_TYPE_QUEST=0;
constexpr int QUEST_SPECIAL_FLAGS_SPEAKTO=128;
constexpr int UNIT_FIELD_NPC_FLAGS2=16,UNIT_FIELD_FLAGS_2=17,UNIT_FLAG2_ALLOW_ENEMY_INTERACT=0x4000;
constexpr int CREATURE_TYPEFLAGS_GHOST=2,CREATURE_TYPEFLAGS_DEAD_INTERACT=0x80,REP_UNFRIENDLY=1;
constexpr float INTERACTION_DISTANCE=5;
#include "ValorFixture.inc"
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
bool WorldObject::hasInvolvedQuest(uint32 q){return GetEntry()==97979&&(q==40001||q==40002);}
namespace WorldPackets{namespace Quest{struct QuestGiverRequestReward{ObjectGuid QuestGiverGUID;uint32 QuestID;};}}
struct WorldSession{Player* _player;void HandleQuestGiverRequestReward(WorldPackets::Quest::QuestGiverRequestReward&);};
#include "RequestReward.inc"
void ReadQuestData(char const* path)
{
    std::ifstream stream(path);assert(stream);std::string line;unsigned quests=0,relations=0,objectives=0;
    while(std::getline(stream,line)){
        std::istringstream in(line);std::string kind;in>>kind;
        if(kind=="quest"){
            uint32 id,type,minLevel,flagsEx,item,completeScript,reward,skill,points;in>>id>>type>>minLevel;
            auto& q=store.quests[id];q.id=id;in>>q.flags>>flagsEx>>item>>q.StartScript>>completeScript>>reward>>q.NextQuestIdChain>>q.AllowableRaces>>q.PrevQuestID>>q.NextQuestID>>q.special>>skill>>points;
            assert(id==40001&&type==2&&minLevel==98&&!q.HasFlag(QUEST_FLAGS_AUTOCOMPLETE));q.prevQuests={q.PrevQuestID};++quests;
        }
        if(kind=="starter"){uint32 entry,q;in>>entry>>q;store.starters.emplace(entry,q);++relations;}
        if(kind=="next-objective"){uint32 id;QuestObjective o;int index;in>>id>>o.Type>>o.ObjectID>>o.Amount>>index;o.StorageIndex=int8(index);store.quests[40002].objectives.push_back(o);++objectives;}
    }
    assert(quests==1&&relations==1&&objectives==2);
    auto& next=store.quests[40002];next.id=40002;next.PrevQuestID=40001;next.prevQuests={40001};store.starters.emplace(97979,40002);
}
int main(int argc,char** argv)
{
    _set_abort_behavior(0,_WRITE_ABORT_MSG|_CALL_REPORTFAULT);_CrtSetReportMode(_CRT_ASSERT,_CRTDBG_MODE_FILE);_CrtSetReportFile(_CRT_ASSERT,_CRTDBG_FILE_STDERR);
    assert(argc==2);ReadQuestData(argv[1]);auto& prior=store.quests[40078];prior.id=40078;prior.NextQuestIdChain=40001;auto& q=store.quests[40001];auto& next=store.quests[40002];
    Creature havi;havi.entry=92539;havi.guid={291280};havi.fields[UNIT_FIELD_NPC_FLAGS]=3;SmartAI ai(&havi);creatures[havi.guid]=&havi;
    Creature vethir;vethir.entry=97979;vethir.guid={291392};vethir.fields[UNIT_FIELD_NPC_FLAGS]=3;vethir.fields[UNIT_FIELD_FLAGS]=33536;vethir.fields[UNIT_FIELD_FLAGS_2]=2048;SmartAI vethirAI(&vethir);creatures[vethir.guid]=&vethir;
    for(auto team:{ALLIANCE,HORDE}){
        Player p;p.team=team;p.race=team==ALLIANCE?1:2;WorldSession session{&p};WorldPackets::Quest::QuestGiverRequestReward request{vethir.guid,40001};
        assert(!p.CanTakeQuest(&q,false)&&!p.CanCompleteQuest(40001)&&!p.CanRewardQuest(&q,false));session.HandleQuestGiverRequestReward(request);assert(!p.menu.offers);
        p.Accept(40078);p.CompleteQuest(40078);assert(!p.CanTakeQuest(&q,false));p.m_RewardedQuests.insert(40078);assert(p.CanTakeQuest(&q,false));
        assert(p.GetNextQuest(havi.guid,&prior)==&q&&p.GetNextQuest(vethir.guid,&prior)==nullptr);
        p.AddQuestAndCheckCompletion(&q,&havi);assert(p.GetQuestStatus(40001)==QUEST_STATUS_COMPLETE&&p.CanRewardQuest(&q,false)&&!p.CanTakeQuest(&q,false)&&!p.CanTakeQuest(&next,false));
        assert(p.GetNPCIfCanInteractWith(vethir.guid,UNIT_NPC_FLAG_QUESTGIVER)==&vethir);session.HandleQuestGiverRequestReward(request);assert(p.menu.offers==1);
        // Correct completed quest does not bypass actual giver/range/flag/lifecycle admission.
        request.QuestGiverGUID=havi.guid;session.HandleQuestGiverRequestReward(request);assert(p.menu.offers==1);request.QuestGiverGUID=vethir.guid;
        for(int mode=0;mode<8;++mode){
            p.world=mode!=0;p.alive=mode!=1;p.flying=mode==2;vethir.alive=mode!=3;vethir.charmer=mode==4?ObjectGuid(99):ObjectGuid();vethir.hostile=mode==5;vethir.near=mode!=6;vethir.fields[UNIT_FIELD_NPC_FLAGS]=mode==7?1:3;
            session.HandleQuestGiverRequestReward(request);assert(p.menu.offers==1);
        }
        p.world=p.alive=vethir.alive=vethir.near=true;p.flying=vethir.hostile=false;vethir.charmer.Clear();vethir.fields[UNIT_FIELD_NPC_FLAGS]=3;
        // Abandon/reaccept and saved completed state need no transient actor or kill credit.
        p.quests.erase(40001);assert(p.CanTakeQuest(&q,false));p.AddQuestAndCheckCompletion(&q,&havi);auto complete=p.completions;session.HandleQuestGiverRequestReward(request);assert(p.menu.offers==2&&p.completions==complete);
        Player saved;saved.team=team;saved.m_RewardedQuests.insert(40078);saved.quests=p.quests;WorldSession restored{&saved};restored.HandleQuestGiverRequestReward(request);assert(saved.menu.offers==1);
        // Reward storage is a fixture; exercise native status/eligibility and exact next-giver relation afterwards.
        p.m_RewardedQuests.insert(40001);p.quests.erase(40001);assert(!p.CanCompleteQuest(40001)&&!p.CanRewardQuest(&q,false)&&!p.CanTakeQuest(&q,false));
        assert(p.CanTakeQuest(&next,false)&&p.GetNextQuest(vethir.guid,&q)==&next&&p.GetNextQuest(havi.guid,&q)==nullptr);
        p.AddQuestAndCheckCompletion(&next,&vethir);assert(p.GetQuestStatus(40002)==QUEST_STATUS_INCOMPLETE&&!p.CanRewardQuest(&next,false));
    }
    std::cout<<"PASS: native acceptance/completion/status/prerequisite/race/reward/next handlers and full request-reward -> questgiver -> NPC admission. Both factions, unaccepted/rewarded rejection, wrong giver, dead/flying/charmed/hostile/distant/unflagged actor, abandonment/reaccept and saved complete state.40002 remains incomplete with its own two objectives. Storage, faction store, range and packet delivery are fixtures; native catalog/geometry are checked separately.\n";
}
