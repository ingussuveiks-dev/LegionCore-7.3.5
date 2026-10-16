#include <crtdbg.h>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <sstream>
struct NativeEvents;
constexpr int QUEST_SPECIAL_FLAGS_SPEAKTO=128, GAMEOBJECT_TYPE_QUESTGIVER=2;
constexpr int TYPEID_UNIT=0,TYPEID_ITEM=1,TYPEID_CONTAINER=2,TYPEID_GAMEOBJECT=5;
constexpr int INVALIDREASON_QUEST_ALREADY_ON=1,DISABLE_TYPE_QUEST=0;
constexpr int SMART_EVENT_ACCEPTED_QUEST=19,SMART_EVENT_REWARD_QUEST=20,SMART_ACTION_SUMMON_CONVERSATION=206;
#include "NemesisFixture.inc"
#include "NativeGossipUse.inc"
#include "Talked.inc"
namespace DisableMgr {bool IsDisabledFor(int,uint32,Player*){return false;}}
struct Item:WorldObject {struct Template {uint32 GetMaxCount(){return 0;}} info;Template* GetTemplate(){return &info;}uint32 GetBagSlot(){return 0;}uint32 GetSlot(){return 0;}};
struct ScriptMgrFixture {uint32 calls=0;template<class T>void OnQuestAccept(Player* p,T*,Quest const* q){++calls;assert(p->GetQuestStatus(q->id)==QUEST_STATUS_COMPLETE);}} scriptMgr;auto sScriptMgr=&scriptMgr;
#include "CanTake.inc"
#include "Status.inc"
#include "Accept.inc"
#include "NativeConversationStructs.inc"
struct ConversationStore {
    std::vector<ConversationData> lines;std::vector<ConversationActor> actors;
    auto GetConversationData(uint32 id){assert(id==2252);return &lines;}
    std::vector<ConversationCreature> const* GetConversationCreature(uint32 id){assert(id==2252);return nullptr;}
    auto GetConversationActor(uint32 id){assert(id==2252);return &actors;}
} conversationStore;auto sConversationDataStore=&conversationStore;
struct ConversationDynamicFieldActors {uint32 actorId,creatureId,displayId,unk1,unk2,unk3;};
struct ConversationDynamicFieldCreatures {uint64 GuidLow,GuidHight;uint32 unk1,unk2;};
struct ConversationDynamicFieldLines {uint32 id,textId,unk1,unk2;};
struct Conversation;
std::vector<Conversation*> created;bool failConversation=false;uint32 attempts=0;
struct Conversation {
    std::map<uint8,ConversationDynamicFieldActors> actors;std::map<uint8,ConversationDynamicFieldLines> lines;Player* owner=nullptr;uint32 duration=0;
    float GetPositionX(){return 0;}float GetPositionY(){return 0;}void SetActor(ConversationDynamicFieldActors* p,uint8 i,bool){actors[i]=*p;}void SetLine(ConversationDynamicFieldLines* p,uint8 i,bool){lines[i]=*p;}void SetCreature(ConversationDynamicFieldCreatures*,uint8,bool){assert(false);}
    bool GenerateDynamicFieldData(uint32,WorldObject*,uint32&);
    bool CreateConversation(uint64,uint32 id,Unit* caster,SpellInfo const*,Position const&){++attempts;assert(caster&&caster->ToPlayer()&&caster->ToPlayer()->GetQuestStatus(39857)==QUEST_STATUS_COMPLETE);owner=caster->ToPlayer();if(failConversation||!GenerateDynamicFieldData(id,caster,duration))return false;created.push_back(this);return true;}
};
#include "NativeConversationData.inc"
struct Generator {uint64 id=0;uint64 Generate(){return ++id;}} generator;
struct ObjectMgrFixture {template<HighGuid>Generator* GetGenerator(){return &generator;}} objectMgr;auto sObjectMgr=&objectMgr;
using ObjectList=std::vector<WorldObject*>;
struct Event {
    struct {struct {uint32 quest=39857,cooldownMin=0,cooldownMax=0;} quest;} event;
    struct {struct {uint32 id=2252,targetX=0,targetY=0,targetZ=0;} conversation;} action;
};
struct NativeEvents {
    WorldObject* base=nullptr;uint32 callbacks=0;Event e;
    WorldObject* GetBaseObject(){return base;}
    ObjectList* GetTargets(Event const&,Unit* unit){return new ObjectList{unit};} // Installed target7: action invoker.
    void RecalcTimer(Event&,uint32,uint32){}
    void ProcessAction(Event&,Unit* unit,uint32){switch(SMART_ACTION_SUMMON_CONVERSATION){
#include "NativeConversationAction.inc"
    }}
    void ProcessEventsFor(uint32 event,Player* unit,uint32 var0,uint32,bool,void*,GameObject*){
        ++callbacks;switch(event){
#include "NativeQuestEvent.inc"
        }
    }
} nativeEvents;
NativeEvents* SmartGameObjectAI::GetScript(){return &nativeEvents;}
#include "NativeQuestAccept.inc"
using ScriptedAI=SmartAI;
void Creature::GetPlayerListInGrid(std::list<Player*>& list,float range){for(auto const& pair:players)if(pair.second->GetExactDist(this)<=range)list.push_back(pair.second);}
#include "Bargain.inc"
void ReadData(char const* path)
{
    std::ifstream stream(path);assert(stream);std::string line;uint32 sai=0;
    while(std::getline(stream,line)){
        std::istringstream in(line);std::string kind;in>>kind;
        if(kind=="actor"){ConversationActor a{};in>>a.entry>>a.id>>a.actorId>>a.creatureId>>a.displayId>>a.unk1>>a.unk2>>a.unk3>>a.duration;conversationStore.actors.push_back(a);}
        if(kind=="line"){ConversationData d{};in>>d.entry>>d.id>>d.idx>>d.textId>>d.unk1>>d.unk2;conversationStore.lines.push_back(d);}
        if(kind=="sai"){uint32 entry,source,id,event,quest,action,param,target;in>>entry>>source>>id>>event>>quest>>action>>param>>target;if(entry==244559){assert(source==1&&id==0&&event==19&&quest==39857&&action==206&&param==2252&&target==7);nativeEvents.e.event.quest.quest=quest;nativeEvents.e.action.conversation.id=param;++sai;}}
    }
    assert(sai==1&&conversationStore.actors.size()==1&&conversationStore.lines.size()==1);
}
int main(int argc,char** argv)
{
    _set_abort_behavior(0,_WRITE_ABORT_MSG|_CALL_REPORTFAULT);_CrtSetReportMode(_CRT_ASSERT,_CRTDBG_MODE_FILE);_CrtSetReportFile(_CRT_ASSERT,_CRTDBG_FILE_STDERR);assert(argc==2);ReadData(argv[1]);
    store.quests[39848].id=39848;
    auto& q=store.quests[39857];q.id=39857;q.PrevQuestID=39848;q.prevQuests={39848};q.NextQuestIdChain=39849;q.NextQuestID=39849;
    auto& next=store.quests[39849];next.id=39849;next.PrevQuestID=39857;next.prevQuests={39857};next.objectives={{1,128854,1,0,0}};store.starters.emplace(97480,39849);
    GameObject altar;altar.entry=244559;altar.guid={700};altar.guid.high=HighGuid::GameObject;altar.map=1463;altar.zone=7588;altar.dbGuid=127713;altar.info.type=2;objects[altar.guid]=&altar;go_stormheim_helheim_bargain ai(&altar);nativeEvents.base=&altar;
    Creature ashildir;ashildir.entry=97480;ashildir.guid={701};ashildir.map=1463;ashildir.zone=7588;creatures[ashildir.guid]=&ashildir;
    for(auto team:{ALLIANCE,HORDE})for(int failure=0;failure<3;++failure){
        Player p;p.guid={1000+team+uint32(failure)};p.map=1463;p.zone=7588;p.team=team;p.race=team==ALLIANCE?1:2;
        assert(!p.CanTakeQuest(&q,false));p.m_RewardedQuests.insert(39848);assert(p.CanTakeQuest(&q,false)&&!p.CanRewardQuest(&q,false)&&!p.CanTakeQuest(&next,false));
        auto count=attempts;auto callbacks=nativeEvents.callbacks;auto scripts=scriptMgr.calls;failConversation=failure==1;auto saved=conversationStore.actors;if(failure==2)conversationStore.actors.clear();
        p.AddQuestAndCheckCompletion(&q,&altar);assert(p.GetQuestStatus(39857)==QUEST_STATUS_COMPLETE&&p.completions==1&&p.CanRewardQuest(&q,false));
        assert(attempts==count+1&&nativeEvents.callbacks==callbacks+1&&scriptMgr.calls==scripts+1&&!p.CanTakeQuest(&q,false)&&!p.CanTakeQuest(&next,false));
        conversationStore.actors=saved;
        if(!failure){auto c=created.back();assert(c->owner==&p&&c->duration==3400&&c->actors.size()==1&&c->lines.size()==1&&c->actors[0].displayId==65043&&c->lines[0].id==4748&&c->lines[0].textId==0&&c->lines[0].unk1==620);}
        // Same actual Ashildir GUID supplies the next quest only after reward.
        p.m_RewardedQuests.insert(39857);p.quests.erase(39857);assert(!p.CanTakeQuest(&q,false)&&p.CanTakeQuest(&next,false)&&p.GetNextQuest(ashildir.guid,&q)==&next);
        p.Accept(39849);assert(!p.CanCompleteQuest(39849)&&!p.CanRewardQuest(&next,false));
    }
    {Player p;p.m_RewardedQuests.insert(39848);p.AddQuestAndCheckCompletion(&q,&altar);auto count=attempts;p.quests.erase(39857);p.Accept(39857);p.CompleteQuest(39857);assert(p.CanRewardQuest(&q,false)&&attempts==count);} // saved complete state does not replay acceptance.
    for(auto c:created)delete c;
    std::cout<<"PASS: full production AddQuestAndCheckCompletion, native CanTake/Status/Previous/Complete/Reward/GetNextQuest, actual bargain GO inherited QuestAccept -> native quest filter/action206 -> native conversation field generation using live World rows. Both factions, complete-before-callback, missing/failed conversation independence, direct actor/line4748 payload, saved complete and actual Ashildir39849 handoff. Session, AddQuest storage, transport and renderer are fixtures.\n";
}
