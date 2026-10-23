#include <cassert>
#include <cstdint>
#include <cstdlib>
#include <fstream>
#include <functional>
#include <iostream>
#include <list>
#include <string>
#include <vector>
using uint32=uint32_t;using int32=int32_t;using uint8=uint8_t;using uint64=uint64_t;
enum SMART_EVENT { SMART_EVENT_TEXT_OVER=52,SMART_EVENT_LINK=61 };
enum SmartScriptType { SMART_SCRIPT_TYPE_CREATURE=0 };
enum { SMART_ACTION_TALK=1,SMART_ACTION_SET_NPC_FLAG=81,SMART_ACTION_SUMMON_CONVERSATION=206,
    SMART_TARGET_NONE=0,SMART_TARGET_ACTION_INVOKER=7,SMART_EVENT_FLAG_NOT_REPEATABLE=1,
    CLASS_HUNTER=3,QUEST_STATUS_INCOMPLETE=3,QUEST_STATUS_COMPLETE=1,UNIT_FIELD_NPC_FLAGS=1 };
uint32 errors=0,warnings=0;
#define TC_LOG_DEBUG(...) ((void)0)
#define TC_LOG_ERROR(...) (++errors)
#define TC_LOG_WARN(...) (++warnings)
struct Unit;struct Player;struct Creature;struct SmartAI;
struct ObjectGuid {Unit* unit=nullptr;static ObjectGuid Empty;};ObjectGuid ObjectGuid::Empty;
struct Position {void Relocate(float,float,float,float){}};
struct WorldObject {
    bool world=true,expired=false;uint32 map=1489,instance=42;
    virtual ~WorldObject()=default;
    virtual Unit* ToUnit(){return nullptr;}
    virtual Player* ToPlayer(){return nullptr;}
    virtual Creature* ToCreature(){return nullptr;}
    bool IsInWorld(){return world;}
    bool IsQuestObjectExpired(){return expired;}
    uint32 GetMapId(){return map;}
    uint32 GetInstanceId(){return instance;}
};
struct Unit:WorldObject {
    bool alive=true;uint32 npcFlags=0;
    Unit* ToUnit()override{return this;}
    ObjectGuid GetGUID(){return {this};}
    bool IsAlive(){return alive;}
    Position GetPosition(){return {};}
    void SetUInt32Value(uint32 field,uint32 value){assert(field==UNIT_FIELD_NPC_FLAGS);npcFlags=value;}
};
struct Player:Unit {
    bool owned=true,rewarded=false,teleporting=false,samePhase=true;
    uint32 quest=3,classId=3;Creature* nearby=nullptr;
    std::vector<std::function<void()>> delayed;
    Player* ToPlayer()override{return this;}
    uint32 getClass(){return classId;}
    uint32 GetQuestStatus(uint32 id){assert(id==40419);return quest;}
    bool GetQuestRewardStatus(uint32 id){assert(id==40419);return rewarded;}
    bool HasItemCount(uint32 id,uint32 count,bool bank){assert(id==128826&&count==1&&bank);return owned;}
    bool IsBeingTeleported(){return teleporting;}
    bool InSamePhase(WorldObject*){return samePhase;}
    uint32 GetGUIDLow(){return 1;}
    Creature* FindNearestCreature(uint32,float,bool){return nearby;}
    void AddDelayedEvent(uint64 delay,std::function<void()>&& callback){assert(delay==500);delayed.push_back(std::move(callback));}
    void Flush(){auto pending=std::move(delayed);delayed.clear();for(auto& callback:pending)callback();}
};
struct CreatureTemplate {std::string AIName="SmartAI";};
struct Creature:Unit {
    bool IsAIEnabled=true;uint32 scriptId=0;SmartAI* ai=nullptr;CreatureTemplate info;
    Creature* ToCreature()override{return this;}
    uint32 GetEntry(){return 100397;}
    uint32 GetScriptId(){return scriptId;}
    uint32 GetDBTableGUIDLow(){return 370758;}
    CreatureTemplate const* GetCreatureTemplate(){return &info;}
    SmartAI* AI(){return ai;}
};
struct Scenario {uint32 id=972,step=8;uint32 GetScenarioId(){return id;}uint32 GetCurrentStep(){return step;}};
struct ScenarioManager {bool present=true;Scenario scenario;Scenario* GetScenario(uint32 instance){return present&&instance==42?&scenario:nullptr;}} scenarioManager;
auto sScenarioMgr=&scenarioManager;
struct SmartScriptHolder {
    int32 entryOrGuid=100397;SmartScriptType source_type=SMART_SCRIPT_TYPE_CREATURE;
    uint32 event_id=0,link=0,type=0,actionType=0,targetType=0;
    bool runOnce=false;
    struct {uint32 event_flags=1,event_chance=100,event_phase_mask=0;struct {uint32 textGroupID=0,creatureEntry=100397;} textOver;} event;
    struct {
        struct {uint32 textGroupID=0,duration=0;} talk;
        struct {uint32 id=1654,targetX=0,targetY=0,targetZ=0;} conversation;
        struct {uint32 flag=1;} unitFlag;
    } action;
    uint32 GetEventType()const{return type;}uint32 GetActionType()const{return actionType;}
    uint32 GetTargetType()const{return targetType;}uint32 GetScriptType()const{return 0;}
};
using SmartAIEventList=std::vector<SmartScriptHolder>;
using ObjectList=std::list<WorldObject*>;
struct SmartManager {SmartAIEventList definitions;bool guidOverride=false;SmartAIEventList GetScript(int32 id,SmartScriptType){if(id<0)return guidOverride?SmartAIEventList(1):SmartAIEventList{};assert(id==100397);return definitions;}} smartManager;
auto sSmartScriptMgr=&smartManager;
struct ConditionManager {
    bool gated=true;
    bool IsObjectMeetingSmartEventConditions(int32,uint32 id,SmartScriptType,Unit* invoker,WorldObject*){
        if(id!=52||!gated)return true;
        Player* player=invoker?invoker->ToPlayer():nullptr;
        return player&&player->owned&&!player->rewarded&&(player->quest==1||player->quest==3)&&player->classId==3&&player->map==1489&&scenarioManager.scenario.id==972&&scenarioManager.scenario.step==8;
    }
} conditionManager;
auto sConditionMgr=&conditionManager;
struct Chat {uint32 group;Unit* owner;};
struct TextManager {std::vector<Chat> chats;void SendChat(Creature*,uint8 group,ObjectGuid owner){chats.push_back({group,owner.unit});}} textManager;
auto sCreatureTextMgr=&textManager;
enum class HighGuid {Conversation};
struct Generator {uint64 Generate(){return 1;}} generator;
struct ObjectManager {template<HighGuid> Generator* GetGenerator(){return &generator;}} objectManager;
auto sObjectMgr=&objectManager;
struct Conversation {
    static std::vector<Unit*> requested;
    bool CreateConversation(uint64,uint32 id,Unit* owner,void*,Position){assert(id==1654);requested.push_back(owner);return false;}
};std::vector<Unit*> Conversation::requested;
struct SpellInfo {};struct GameObject {};
struct SmartScript {
    Creature* me=nullptr;Player* closest=nullptr;SmartAIEventList mEvents;
    uint32 mTextTimer=0,mLastTextID=0,mTalkerEntry=0;ObjectGuid mTextGUID,mLastInvoker;
    bool mUseTextTimer=false;
    WorldObject* GetBaseObject(){return me;}
    Unit* GetLastInvoker(){return mLastInvoker.unit;}
    bool IsPlayer(WorldObject* object){return object&&object->ToPlayer();}
    bool IsCreature(WorldObject* object){return object&&object->ToCreature();}
    bool IsUnit(WorldObject* object){return object&&object->ToUnit();}
    bool IsInPhase(uint32){return true;}
#include "Readers.inc"
    ObjectList* GetTargets(SmartScriptHolder const& e,Unit* invoker){
        auto* l=new ObjectList;Unit* trigger=invoker?invoker:GetLastInvoker();
        switch(e.GetTargetType()){
#include "InvokerTarget.inc"
            case 1:l->push_back(me);break;
            case 21:if(closest)l->push_back(closest);break;
        }return l;
    }
    void ProcessEventsFor(SMART_EVENT,Unit* =nullptr,uint32=0,uint32=0,bool=false,SpellInfo const* =nullptr,GameObject* =nullptr);
    void ProcessEvent(SmartScriptHolder& e,Unit* unit=nullptr,uint32 var0=0,uint32 var1=0,bool bvar=false,SpellInfo const* spell=nullptr,GameObject* gob=nullptr){
#include "RepeatGuard.inc"
        switch(e.GetEventType()){
#include "TextOver.inc"
            case SMART_EVENT_LINK:ProcessAction(e,unit,var0,var1,bvar,spell,gob);break;
        }
    }
    SmartScriptHolder FindLinkedEvent(uint32);
    void ProcessAction(SmartScriptHolder& e,Unit* unit=nullptr,uint32 var0=0,uint32 var1=0,bool bvar=false,SpellInfo const* spell=nullptr,GameObject* gob=nullptr){
#include "ActionStart.inc"
        switch(e.GetActionType()){
#include "Talk.inc"
#include "Conversation.inc"
#include "Flag.inc"
        }
#include "LinkTail.inc"
    void Tick(uint32 diff){
#include "TextTimer.inc"
    }
};
#include "Dispatch.inc"
#include "FindLink.inc"
struct SmartAI {SmartScript* script=nullptr;SmartScript* GetScript(){return script;}};
namespace ArtifactAcquisition {
#include "Active.inc"
}
#include "Claim.inc"
struct Fixture {
    Creature actor;Player owner,peer;SmartScript script;SmartAI ai;
    Fixture(){scenarioManager={};conditionManager={};textManager.chats.clear();Conversation::requested.clear();warnings=errors=0;smartManager.guidOverride=false;actor.ai=&ai;ai.script=&script;script.me=&actor;script.closest=&peer;script.mEvents=smartManager.definitions;owner.nearby=&actor;}
    void Voice(){script.mUseTextTimer=true;script.mLastTextID=22;script.mTalkerEntry=100397;script.mTextTimer=15000;}
    void Tick(uint32 diff=500){script.Tick(diff);owner.Flush();}
};
int main(int argc,char** argv){
    _set_abort_behavior(0,_CALL_REPORTFAULT);assert(argc==2);std::ifstream input(argv[1]);
    for(uint32 count=0;count<4;++count){SmartScriptHolder row;uint32 first,second,unused;
        assert(input>>row.event_id>>row.link>>row.type>>row.event.event_flags>>row.event.textOver.textGroupID>>row.event.textOver.creatureEntry>>row.actionType>>first>>second>>row.targetType>>unused>>row.event.event_chance>>row.event.event_phase_mask);
        row.action.talk={first,second};row.action.conversation.id=first;row.action.unitFlag.flag=first;smartManager.definitions.push_back(row);
    }
    assert(smartManager.definitions.size()==4&&smartManager.definitions[0].targetType==7&&smartManager.definitions[1].targetType==7);
    {
        Fixture old;conditionManager.gated=false;old.owner.owned=false;scenarioManager.scenario.step=7;
        old.script.mEvents[0].targetType=21;old.script.mEvents[1].targetType=21;old.Voice();old.Tick(15001);
        assert(textManager.chats.size()==1&&textManager.chats[0].group==23&&Conversation::requested==std::vector<Unit*>{&old.peer});
    }
    {
        Fixture fixed;fixed.owner.owned=false;scenarioManager.scenario.step=7;fixed.Voice();fixed.Tick(15001);fixed.Tick(60001);
        assert(textManager.chats.empty()&&Conversation::requested.empty()&&!fixed.actor.npcFlags&&!fixed.script.HasRunEvent(52,SMART_EVENT_TEXT_OVER));
        fixed.owner.owned=true;scenarioManager.scenario.step=8;MarksmanshipClaim::Queue(&fixed.owner);
        assert(textManager.chats.size()==1&&textManager.chats[0].group==23&&textManager.chats[0].owner==&fixed.owner);
        assert(Conversation::requested==std::vector<Unit*>{&fixed.owner});
        for(uint32 repeat=0;repeat<20;++repeat)MarksmanshipClaim::Queue(&fixed.owner);
        assert(textManager.chats.size()==1&&Conversation::requested.size()==1);fixed.Tick(5001);
        assert(textManager.chats.size()==2&&textManager.chats[1].group==24&&fixed.actor.npcFlags==1&&errors==0);
    }
    {
        Fixture pending;pending.Voice();MarksmanshipClaim::Queue(&pending.owner);
        assert(pending.owner.delayed.size()==1&&textManager.chats.empty());
        for(uint32 tick=0;tick<30;++tick){pending.Tick();assert(textManager.chats.empty());}
        pending.Tick();assert(textManager.chats.size()==1&&Conversation::requested==std::vector<Unit*>{&pending.owner});
    }
    for(uint32 missing=0;missing<4;++missing){
        Fixture retry;if(missing==0)retry.owner.nearby=nullptr;if(missing==1)retry.actor.IsAIEnabled=false;if(missing==2)retry.owner.samePhase=false;if(missing==3)retry.actor.expired=true;
        MarksmanshipClaim::Queue(&retry.owner);assert(retry.owner.delayed.size()==1&&textManager.chats.empty());
        retry.owner.nearby=&retry.actor;retry.actor.IsAIEnabled=true;retry.owner.samePhase=true;retry.actor.expired=false;retry.Tick();assert(textManager.chats.size()==1);
    }
    for(uint32 invalid=0;invalid<10;++invalid){
        Fixture stop;stop.owner.nearby=nullptr;MarksmanshipClaim::Queue(&stop.owner);stop.owner.nearby=&stop.actor;
        switch(invalid){case 0:stop.owner.world=false;break;case 1:stop.owner.alive=false;break;case 2:stop.owner.teleporting=true;break;case 3:stop.owner.quest=0;break;case 4:stop.owner.rewarded=true;break;case 5:stop.owner.owned=false;break;case 6:stop.owner.map=1220;break;case 7:stop.owner.instance=99;break;case 8:scenarioManager.scenario.step=7;break;case 9:scenarioManager.present=false;break;}
        stop.Tick();assert(textManager.chats.empty()&&Conversation::requested.empty()&&stop.owner.delayed.empty());
    }
    for(uint32 custom=0;custom<4;++custom){
        Fixture untouched;auto saved=smartManager.definitions;
        if(custom==0)untouched.actor.scriptId=7;if(custom==1)untouched.actor.info.AIName="CustomAI";if(custom==2)smartManager.guidOverride=true;if(custom==3)smartManager.definitions[0].event.event_chance=50;
        MarksmanshipClaim::Queue(&untouched.owner);assert(textManager.chats.empty()&&Conversation::requested.empty()&&untouched.owner.delayed.empty());smartManager.definitions=saved;
    }
    {
        Fixture timeout;timeout.owner.nearby=nullptr;MarksmanshipClaim::Queue(&timeout.owner);
        for(uint32 tick=0;tick<120;++tick)timeout.owner.Flush();
        assert(warnings==1&&timeout.owner.delayed.empty()&&textManager.chats.empty()&&timeout.owner.owned);
    }
    MarksmanshipClaim::Queue(nullptr);
    std::cout<<"PASS: original native text timer/dispatch reproduces early bow speech and wrong nearest-player conversation; repaired callback requires claimed owner, waits actual line22, preserves native23 -> conversation1654 ->24 -> flag55 once\n";
    std::cout<<"PASS: actual claim helper and native timer/LINK/TALK/conversation request bodies cover missing actor/AI/phase retry, bounded diagnostics, invalidation and custom scripts/negative-GUID overrides without granting credit or replacing events; conversation creation/rendering is a controlled failed request, not client proof\n";
}
