#include <crtdbg.h>
#include <fstream>
#include <sstream>
#include <algorithm>
constexpr int TYPEID_UNIT=0,TYPEID_ITEM=1,TYPEID_CONTAINER=2,TYPEID_PLAYER=4,TYPEID_GAMEOBJECT=5;
constexpr int GAMEOBJECT_TYPE_QUESTGIVER=2,INVALIDREASON_QUEST_ALREADY_ON=1,DISABLE_TYPE_QUEST=0;
constexpr int QUEST_SPECIAL_FLAGS_SPEAKTO=128,MAX_KILL_CREDIT=2,QUEST_OBJECTIVE_FLAG_TRACKED_ON_MINIMAP=1,QUEST_OBJECTIVE_FLAG_SEQUENCED=2;
constexpr int UNIT_FIELD_NPC_FLAGS2=16,UNIT_FIELD_FLAGS_2=17,UNIT_FLAG2_ALLOW_ENEMY_INTERACT=0x4000;
constexpr int CREATURE_TYPEFLAGS_GHOST=2,CREATURE_TYPEFLAGS_DEAD_INTERACT=0x80,REP_UNFRIENDLY=1;
constexpr float INTERACTION_DISTANCE=5;using GameobjectTypes=int;constexpr int GO_FLAG_INTERACT_DISTANCE_USES_TEMPLATE_MODEL=0x80000;constexpr int DATA_SKOVALD=3,DATA_ODYN=4,DONE=3,NPC_ODYN=95676;
struct FakeMap;struct InstanceScript;
#include "AegisFixture.inc"
#include "NativeGossipUse.inc"
#include "Talked.inc"
namespace DisableMgr{bool IsDisabledFor(int,uint32,Player*){return false;}}
struct FactionEntry{bool CanHaveReputation()const{return false;}};
struct FactionTemplateEntry{uint32 Faction=148;};
struct FactionStore{FactionEntry row;FactionEntry* LookupEntry(uint32){return &row;}} sFactionStore;
struct FactionTemplateStore{FactionTemplateEntry row;FactionTemplateEntry* LookupEntry(uint32){return &row;}} sFactionTemplateStore;
namespace ObjectAccessor{Object* GetObjectByTypeMask(Player& p,ObjectGuid g,int){if(objects.count(g))return objects[g];return GetCreature(p,g);}}
constexpr int TYPEMASK_UNIT=1,TYPEMASK_GAMEOBJECT=2;
struct Item:WorldObject{struct Template{uint32 GetMaxCount(){return 0;}} info;Template* GetTemplate(){return &info;}uint32 GetBagSlot(){return 0;}uint32 GetSlot(){return 0;}};
struct ScriptMgr{bool OnGossipHello(Player*,GameObject*){return false;}template<class T>void OnQuestAccept(Player*,T*,Quest const*){}} scriptMgr;auto sScriptMgr=&scriptMgr;
#include "CanTake.inc"
#include "Status.inc"
#include "Accept.inc"
#include "Interact.inc"
bool WorldObject::hasInvolvedQuest(uint32 q){return (GetEntry()==97988||GetEntry()==251991)&&q==40072;}
namespace WorldPackets{namespace Quest{struct QuestGiverRequestReward{ObjectGuid QuestGiverGUID;uint32 QuestID;};}}
struct WorldSession{Player* _player;void HandleQuestGiverRequestReward(WorldPackets::Quest::QuestGiverRequestReward&);};
#include "RequestReward.inc"


struct InstanceScript {int skovald=0,odyn=0;int GetBossState(int boss){return boss==DATA_SKOVALD?skovald:odyn;}};
struct ConditionMgr {bool pass=true;bool IsPlayerMeetingCondition(Unit*,uint32 id){return !id||pass;}} conditionMgr;auto sConditionMgr=&conditionMgr;
namespace GameTime {uint32 GetGameTime(){return 0;}}
#include "NativeGOInteract.inc"
#include "Aegis.inc"
#include "NativeQuestgiverUse.inc"
void Setup(Player& p,GameObject& go,InstanceScript& instance,FakeMap& map)
{
    p.mapObject=&map;p.map=1477;p.instanceId=42;p.zone=7672;p.phases.insert(3554);p.Accept(40072);
    go.mapObject=&map;go.map=1477;go.instanceId=42;go.zone=7672;go.entry=251991;go.dbGuid=25178659;go.guid={25178659};go.guid.high=HighGuid::GameObject;go.info.type=2;go.info.questgiver.gossipID=20352;go.instanceScript=&instance;
    static_cast<Position&>(go)=HallsOfValorAegis::Pedestal;static_cast<Position&>(p)=go;objects[go.guid]=&go;
}
int main(int argc,char** argv)
{
    _set_abort_behavior(0,_WRITE_ABORT_MSG|_CALL_REPORTFAULT);_CrtSetReportMode(_CRT_ASSERT,_CRTDBG_MODE_FILE);_CrtSetReportFile(_CRT_ASSERT,_CRTDBG_FILE_STDERR);
    assert(argc==2);std::ifstream input(argv[1]);assert(input);std::string line;unsigned objectives=0;
    while(std::getline(input,line)){
        std::istringstream in(line);std::string kind;in>>kind;
        if(kind=="quest"){uint32 id,item;in>>id;auto& q=store.quests[id];q.id=id;in>>q.flags>>q.AllowableRaces>>item>>q.NextQuestIdChain>>q.PrevQuestID>>q.NextQuestID>>q.special;if(q.PrevQuestID)q.prevQuests={q.PrevQuestID};}
        if(kind=="objective"){uint32 id,quest;int index;QuestObjective o;in>>id>>quest>>o.Type>>o.ObjectID>>o.Amount>>index>>o.Flags;o.StorageIndex=int8(index);store.quests[quest].objectives.push_back(o);++objectives;}
        if(kind=="go-starter"){uint32 actor,quest;in>>actor>>quest;store.starters.emplace(actor,quest);}
    }
    assert(objectives==2&&HallsOfValorAegis::Definition());auto& q=store.quests[40072];q.special|=QUEST_SPECIAL_FLAGS_KILL;store.quests[40005].id=40005;
    {Player p;FakeMap map;InstanceScript i;GameObject go;Setup(p,go,i,map);GameObjectAI old(&go);i.skovald=i.odyn=DONE;p.KilledMonsterCredit(95676);go.Use(&p);assert(!p.GetQuestObjectiveData(40072,110425)&&!p.CanCompleteQuest(40072));} // Original questgiver use cannot supply the missing claim.
    for(auto team:{ALLIANCE,HORDE}){
        Player candidate;candidate.team=team;candidate.race=team==ALLIANCE?1:2;assert(!candidate.CanTakeQuest(&q,false));candidate.Accept(40005);candidate.CompleteQuest(40005);assert(!candidate.CanTakeQuest(&q,false));candidate.m_RewardedQuests.insert(40005);assert(candidate.CanTakeQuest(&q,false));
        for(int mode=0;mode<28;++mode){
            Player p;p.team=team;FakeMap map;InstanceScript instance;GameObject go;Setup(p,go,instance,map);go_halls_of_valor_quest_aegis ai(&go);instance.skovald=instance.odyn=DONE;p.KilledMonsterCredit(95676);assert(!p.CanCompleteQuest(40072));
            switch(mode){
              case 1:instance.skovald=0;break;case 2:instance.odyn=0;break;case 3:p.data[{40072,95676}]=0;break;
              case 4:p.quests.clear();break;case 5:p.quests[40072].Status=QUEST_STATUS_COMPLETE;break;case 6:p.alive=false;break;case 7:p.world=false;break;case 8:go.world=false;break;
              case 9:go.spawned=false;break;case 10:go.flags=GO_FLAG_NOT_SELECTABLE;break;case 11:go.flags=GO_FLAG_IN_USE;break;case 12:p.x+=30;break;case 13:p.phase=false;break;
              case 14:p.visible=false;break;case 15:p.los=false;break;case 16:p.combat=true;break;case 17:p.flying=true;break;case 18:p.vehicle=true;break;case 19:p.transport=true;break;case 20:p.teleport=true;break;
              case 21:p.phases.clear();break;case 22:go.dbGuid=999;break;case 23:go.entry=251992;break;case 24:go.x+=2;break;case 25:go.instanceScript=nullptr;break;case 26:go.instanceId=0;break;
              case 27:conditionMgr.pass=false;go.info.questgiver.conditionID1=123;break;
            }
            auto before=p.packets;go.Use(&p);assert(p.GetQuestObjectiveData(40072,110425)==(mode==0?1:0)&&p.packets==before+unsigned(mode==0));
            if(!mode){assert(p.GetQuestStatus(40072)==QUEST_STATUS_COMPLETE&&p.prepared==1&&p.sent==1&&p.completions==1);go.Use(&p);assert(p.packets==before+1&&p.completions==1);
                WorldSession session{&p};WorldPackets::Quest::QuestGiverRequestReward request{go.guid,40072};session.HandleQuestGiverRequestReward(request);assert(p.menu.offers==1);
                assert(!p.GetNextQuest(go.guid,&q)&&!p.CanTakeQuest(&store.quests[43349],false));p.quests.clear();p.m_RewardedQuests.insert(40072);assert(p.CanTakeQuest(&store.quests[43349],false));go.Use(&p);assert(p.packets==before+1);}
            conditionMgr.pass=true;objects.erase(go.guid);
        }
        {Player p,party;FakeMap map;InstanceScript i;GameObject go;Setup(p,go,i,map);party.Accept(40072);go_halls_of_valor_quest_aegis ai(&go);i.skovald=i.odyn=DONE;
         GroupReference b{&party,nullptr},a{&p,&b};Group group;group.first=&a;p.group=party.group=&group;
         p.KilledMonsterCredit(95676);party.KilledMonsterCredit(95676);go.Use(&p);assert(p.completions==1&&!party.GetQuestObjectiveData(40072,110425));
         Setup(party,go,i,map);party.KilledMonsterCredit(95676);go.Use(&party);assert(party.completions==1&&go.spawned&&!go.flags);}
        {Player p;FakeMap map;InstanceScript i;GameObject go;Setup(p,go,i,map);go_halls_of_valor_quest_aegis ai(&go);i.skovald=i.odyn=DONE;p.KilledMonsterCredit(95676);
         p.quests.clear();p.Accept(40072);go.Use(&p);assert(p.GetQuestObjectiveData(40072,110425)==0);assert(!p.CanCompleteQuest(40072));p.KilledMonsterCredit(95676);go.Use(&p);assert(p.completions==1);}
        {Player p;FakeMap map;InstanceScript i;GameObject go;Setup(p,go,i,map);go_halls_of_valor_quest_aegis ai(&go);i.skovald=i.odyn=DONE;p.data[{40072,95676}]=1;go.Use(&p);assert(p.completions==1);} // Saved counter re-entry, no new boss credit minted.
    }
    std::cout<<"PASS: actual GameObject::Use questgiver prefix/branch invokes production GossipUse; required boss states plus earned Odyn counter, individual Aegis claim, native completion/reward interaction and43349 admission. Both factions,28 boundary cases, duplicate, party isolation, saved progress and abandon/retry passed. Map/phase/condition/packet/store and menu preparation are fixture services; client pending.\n";
}
