// Selected native handlers execute; SmartAI event/LINK scheduling and world
// services below are fixture boundaries, not a full server/client integration.
#include "Fixture.inc"
using GuidSet=std::set<ObjectGuid>;
enum {EO_FLAG_ONE_ACTION=1,SMART_EVENT_EVENTOBJECT_ONTRIGGER=89,SMART_EVENT_EVENTOBJECT_OFFTRIGGER=91,SMART_ACTION_CALL_KILLEDMONSTER=33};
struct EventObjectTemplate {float radius=5;uint32 SpellID=0,WorldSafeLocID=0,Flags=0;};
struct EventObject:WorldObject {
    EventObjectTemplate data;EventObjectTemplate* eventTemplate=&data;bool m_isUpdate=false;
    GuidSet affectedPlayers,affectedPermamentPlayers;
    void GetPlayerListInGrid(std::list<Player*>& list,float radius){for(auto [guid,p]:players)if(p->world&&p->map==map&&GetExactDist(p)<=radius)list.push_back(p);}
    void Update(uint32);bool ActivatedForPlayer(Player*)const;
};
struct WorldSafeLocsEntry {uint32 MapID;struct {float X,Y,Z,O;} Loc;};
struct SafeStore {WorldSafeLocsEntry* LookupEntry(uint32){return nullptr;}} safeStore;auto& sWorldSafeLocsStore=safeStore;
constexpr float M_PI=3.14159265358979323846f;
struct Row {uint32 id,link,type,action,credit,target;};std::vector<Row> rows;
using ObjectList=std::vector<WorldObject*>;
struct SmartScript {
    Creature* me=nullptr;GameObject* go=nullptr;EventObject* event=nullptr;bool trigger=false;
    void OnInitialize(EventObject* e,void*){event=e;}
    WorldObject* GetBaseObject();
    bool IsUnit(WorldObject* o){return o&&o->ToUnit();}bool IsPlayer(WorldObject* o){return o&&o->ToPlayer();}
    struct Holder {struct {struct {uint32 creature;} killedMonster;} action;};
    ObjectList* GetTargets(Holder const&,Unit* trigger){auto l=new ObjectList;constexpr int SMART_TARGET_NONE=0,SMART_TARGET_ACTION_INVOKER=7;switch(SMART_TARGET_ACTION_INVOKER){
        #include "InvokerTarget.inc"
        }return l;}
    void Action(uint32 credit,Unit* unit){Holder e;e.action.killedMonster.creature=credit;switch(SMART_ACTION_CALL_KILLEDMONSTER){
        #include "Action33.inc"
    }}
    void ProcessEventsFor(uint32 type,Player* p,uint32 entry){
        assert(entry==510&&event&&event->entry==510);
        if(type!=SMART_EVENT_EVENTOBJECT_ONTRIGGER)return;
        // Fixture scheduler validates the actual exported chain, including the
        // otherwise unrelated preceding107587 credit and all following links.
        uint32 id=0;std::set<uint32> visited;
        do {auto i=std::find_if(rows.begin(),rows.end(),[id](auto const& r){return r.id==id;});assert(i!=rows.end()&&visited.insert(id).second);
            assert(i->type==(id?61:89)&&i->action==33&&i->target==7);Action(i->credit,p);id=i->link;
        }while(id);
        assert(visited.size()==4);
    }
};
#include "SmartBase.inc"
struct EventObjectScript {explicit EventObjectScript(char const*){}virtual bool OnTrigger(Player*,EventObject*,bool){return false;}};
#include "SmartEventObject.inc"
bool ScriptMgr::OnEventObject(Player* p,EventObject* e,bool apply){SmartEventObject script;return script.OnTrigger(p,e,apply);}
#include "EventUpdate.inc"
#include "Activated.inc"
namespace GossipOptionNpc {constexpr int None=0;}
constexpr int GAMEOBJECT_TYPE_GOOBER=10;
bool OptionAdmitted(GameObject* source,int option){struct Option{int OptionNpc;};std::map<int,Option> options{{0,{option}}};auto itr=options.begin();bool canTalk=true;
    #include "GOOption.inc"
    return canTalk;
}
enum {CONDITION_QUESTREWARDED=8,CONDITION_QUESTTAKEN=9,CONDITION_QUEST_COMPLETE=28,CONDITION_QUEST_NONE=14};
bool RewardedCondition(Player* object,bool negative){bool condMeets=false;uint32 ConditionValue1=43349;switch(CONDITION_QUESTREWARDED){
    #include "QuestRewardedCondition.inc"
    }return negative?!condMeets:condMeets;
}
struct PhaseUpdateData {std::set<int> types;uint32 _questId=0;void AddConditionType(int type){types.insert(type);}void AddQuestUpdate(uint32 const);};
#include "QuestPhaseUpdate.inc"
void SetupReturn(Player& p,GameObject& go,EventObject& event,FakeMap& map){
    p.map=1220;p.mapObject=&map;p.zone=7502;p.area=8045;p.phases.insert(4592);p.Accept(43349);p.guid={1};players[p.guid]=&p;
    event.entry=510;event.map=1220;event.x=-785.162f;event.y=4419.39f;event.z=602.464f;
    go.entry=251992;go.map=1220;go.mapObject=&map;go.dbGuid=25178660;go.guid={25178660};go.guid.high=HighGuid::GameObject;
    go.info.type=2;go.info.questgiver.gossipID=20353;go.x=-854.191f;go.y=4456.02f;go.z=592.614f;objects[go.guid]=&go;
    static GameObjectAI ordinaryQuestgiver;go.ai=&ordinaryQuestgiver;
    static_cast<Position&>(p)=event;
}
int main(int argc,char** argv){
    _set_abort_behavior(0,_WRITE_ABORT_MSG|_CALL_REPORTFAULT);_CrtSetReportMode(_CRT_ASSERT,_CRTDBG_MODE_FILE);_CrtSetReportFile(_CRT_ASSERT,_CRTDBG_FILE_STDERR);
    assert(argc==2);std::ifstream input(argv[1]);assert(input);std::string line;
    while(std::getline(input,line)){std::istringstream in(line);std::string kind;in>>kind;
        if(kind=="quest"){uint32 id,item;in>>id;auto& q=store.quests[id];q.id=id;in>>q.flags>>q.AllowableRaces>>item>>q.NextQuestIdChain>>q.PrevQuestID>>q.NextQuestID>>q.special;q.prevQuests={q.PrevQuestID};}
        if(kind=="objective"){uint32 id,quest;int index;QuestObjective o;in>>id>>quest>>o.Type>>o.ObjectID>>o.Amount>>index>>o.Flags;o.StorageIndex=int8(index);store.quests[quest].objectives.push_back(o);}
        if(kind=="smart"){Row row;in>>row.id>>row.link>>row.type>>row.action>>row.credit>>row.target;rows.push_back(row);}
    }
    auto& q=store.quests[43349];assert(q.objectives.size()==1&&rows.size()==4);q.special|=QUEST_SPECIAL_FLAGS_KILL;store.quests[40072].id=40072;
    for(auto team:{ALLIANCE,HORDE}){
        Player candidate;candidate.team=team;candidate.race=team==ALLIANCE?1:2;assert(!candidate.CanTakeQuest(&q,false));candidate.Accept(40072);candidate.CompleteQuest(40072);assert(!candidate.CanTakeQuest(&q,false));candidate.m_RewardedQuests.insert(40072);assert(candidate.CanTakeQuest(&q,false));
        for(int mode=0;mode<5;++mode){Player p;FakeMap map;GameObject go;EventObject event;SetupReturn(p,go,event,map);p.team=team;
            if(mode==1)p.x+=6;if(mode==2)p.phase=false;if(mode==3)p.alive=false;if(mode==4)p.quests.clear();
            event.Update(1);assert(p.GetQuestObjectiveData(43349,110409)==(mode==0));
            if(!mode){assert(p.GetQuestStatus(43349)==QUEST_STATUS_COMPLETE&&p.completions==1&&p.packets==1);event.Update(1);assert(p.packets==1);p.x+=6;event.Update(1);p.x-=6;event.Update(1);assert(p.packets==1);
                static_cast<Position&>(p)=go;go.Use(&p);assert(p.prepared==1&&p.sent==1);assert(!OptionAdmitted(&go,28)&&OptionAdmitted(&go,0));
                WorldSession session{&p};WorldPackets::Quest::QuestGiverRequestReward request{go.guid,43349};session.HandleQuestGiverRequestReward(request);assert(p.menu.offers==1);assert(!p.GetNextQuest(go.guid,&q));}
            players.clear();objects.clear();
        }
        {Player p;FakeMap map;GameObject go;EventObject event;SetupReturn(p,go,event,map);p.quests.clear();event.Update(1);p.Accept(43349);event.Update(1);assert(!p.GetQuestObjectiveData(43349,110409));p.x+=6;event.Update(1);p.x-=6;event.Update(1);assert(p.completions==1); // Already inside requires exit/re-entry; flags0 permits retry.
         p.quests.clear();p.Accept(43349);event.Update(1);assert(!p.GetQuestObjectiveData(43349,110409));p.x+=6;event.Update(1);p.x-=6;event.Update(1);assert(p.completions==2);players.clear();objects.clear();}
        {Player p;FakeMap map;GameObject go;EventObject event;SetupReturn(p,go,event,map);p.data[{43349,110409}]=1;p.CompleteQuest(43349);static_cast<Position&>(p)=go;WorldSession session{&p};WorldPackets::Quest::QuestGiverRequestReward request{go.guid,43349};session.HandleQuestGiverRequestReward(request);assert(p.menu.offers==1);players.clear();objects.clear();}
        {Player p,near,far;FakeMap map;GameObject go;EventObject event;SetupReturn(p,go,event,map);near.map=far.map=1220;near.Accept(43349);far.Accept(43349);far.rewardRange=false;
         GroupReference c{&far,nullptr},b{&near,&c},a{&p,&b};Group group;group.first=&a;p.group=near.group=far.group=&group;
         event.Update(1);assert(p.completions==1&&near.completions==1&&!far.completions); // Preserve native group sharing, not a new per-player claim.
         players.clear();objects.clear();}
        {Player p;assert(RewardedCondition(&p,true)&&!RewardedCondition(&p,false));p.Accept(43349);assert(RewardedCondition(&p,true));p.CompleteQuest(43349);assert(RewardedCondition(&p,true));p.quests.clear();p.m_RewardedQuests.insert(43349);assert(!RewardedCondition(&p,true)&&RewardedCondition(&p,false));
         PhaseUpdateData change;change.AddQuestUpdate(43349);assert(change._questId==43349&&change.types==std::set<int>({8,9,28,14}));}
    }
    std::cout<<"PASS:43349 existing native EventObject update -> SmartEventObject -> action33 -> player/group credit/completion, native GO use/reward request and filtered unrelated gossip option. Both factions, radius/phase/death/no quest, repeated entry, abandon/retry and saved counter pass. SmartAI scheduling, phase assignment, menus, persistence and packet services remain fixtures; no client verification.\n";
}
