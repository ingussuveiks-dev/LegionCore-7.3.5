#include <algorithm>
#include <cassert>
#include <cstdint>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <set>
#include <vector>
using uint32=uint32_t;
enum {SMART_TARGET_CLOSEST_PLAYER=21,SMART_TARGET_OWNER_OR_SUMMONER=23,SMART_ACTION_UPDATE_ACHIEVEMENT_CRITERIA=205,SMART_EVENT_LINK=61,SMART_EVENT_FLAG_NOT_REPEATABLE=1};
enum CriteriaTypes {SCRIPT_EVENT_2=92};
struct Player;
struct TempSummon;
struct Unit;
struct WorldObject {
    virtual Unit* ToUnit(){return nullptr;}
    virtual ~WorldObject()=default;
};
struct Unit:WorldObject {
    Unit* charmer=nullptr;
    Unit* owner=nullptr;
    Unit* summoner=nullptr;
    bool temporary=false;
    Unit* ToUnit() override{return this;}
    virtual Player* ToPlayer(){return nullptr;}
    bool GetCharmerGUID() const{return charmer!=nullptr;}
    Unit* GetCharmer() const{return charmer;}
    Unit* GetOwner() const{return owner;}
    TempSummon* ToTempSummon() const;
    Unit* GetAnyOwner() const;
    Player* GetCharmerOrOwnerPlayerOrPlayerItself();
};
struct Player:Unit {
    uint32 credit=0;
    Player* ToPlayer() override{return this;}
    void UpdateAchievementCriteria(CriteriaTypes type,uint32 asset,uint32 second,uint32 third,Unit*,bool enabled){
        assert(type==SCRIPT_EVENT_2&&asset==47608&&!second&&!third&&enabled);
        ++credit;
    }
};
struct TempSummon:Unit {
    Player* nearest=nullptr;
    explicit TempSummon(Unit* source){temporary=true;summoner=source;}
    Unit* GetSummoner() const{return summoner;}
    Player* SelectNearestPlayer(float range){assert(range==100.0f);return nearest;}
};
TempSummon* Unit::ToTempSummon() const{return temporary?static_cast<TempSummon*>(const_cast<Unit*>(this)):nullptr;}
Player* Unit::GetCharmerOrOwnerPlayerOrPlayerItself(){Unit* controller=charmer?charmer:owner;return controller?controller->ToPlayer():ToPlayer();}
#include "Owner.inc"
#include "CreditPlayer.inc"
using ObjectList=std::vector<WorldObject*>;
struct SmartScriptHolder {
    uint32 entryOrGuid=99431,event_id=0,link=0,eventType=0,actionType=0,targetType=0;
    bool runOnce=false;
    struct {uint32 event_phase_mask=0,event_flags=0;} event;
    struct {struct {uint32 dist=0;} playerDistance;} target;
    struct {struct {uint32 type=0,misc1=0,misc2=0,misc3=0;} achievementCriteria;} action;
    uint32 GetActionType() const{return actionType;}
    uint32 GetEventType() const{return eventType;}
    uint32 GetScriptType() const{return 0;}
};
uint32 errors=0;
#define TC_LOG_ERROR(...) (++errors)
#define TC_LOG_DEBUG(...) ((void)0)
struct SmartScript {
    TempSummon* me=nullptr;
    std::vector<SmartScriptHolder> mEvents;
    std::vector<uint32> actions;
    explicit SmartScript(TempSummon* actor):me(actor){}
    bool IsInPhase(uint32){return true;}
    WorldObject* GetBaseObject(){return me;}
    ObjectList* GetTargets(SmartScriptHolder const& nativeEvent,Unit*){
        ObjectList* targets=new ObjectList;
        switch(nativeEvent.targetType){
#include "Targets.inc"
        default:break;
        }
        return targets;
    }
    SmartScriptHolder FindLinkedEvent(uint32);
    void ProcessEvent(SmartScriptHolder& nativeEvent,Unit* unit=nullptr,uint32 var0=0,uint32 var1=0,bool bvar=false,void const* spell=nullptr,void* gob=nullptr){
#include "RepeatGuard.inc"
        nativeEvent.runOnce=true;
        actions.push_back(nativeEvent.actionType);
        switch(nativeEvent.actionType){
#include "Credit.inc"
        default:break;
        }
#include "FollowLink.inc"
    }
    void Run(uint32 identity){auto found=std::find_if(mEvents.begin(),mEvents.end(),[identity](auto const& entry){return entry.event_id==identity;});assert(found!=mEvents.end());ProcessEvent(*found);}
};
#include "FindLink.inc"
int main(int argc,char** argv)
{
    _set_abort_behavior(0,_CALL_REPORTFAULT);
    assert(argc==2);
    std::ifstream input(argv[1]);
    SmartScriptHolder definition;
    assert(input>>definition.event_id>>definition.link>>definition.eventType>>definition.actionType>>definition.targetType>>definition.target.playerDistance.dist>>definition.event.event_flags>>definition.action.achievementCriteria.type>>definition.action.achievementCriteria.misc1>>definition.action.achievementCriteria.misc2>>definition.action.achievementCriteria.misc3);
    assert(definition.event_id==5&&definition.link==6);
    for(uint32 source:{0u,1u,2u,3u,4u,5u}){
        Player owner,nearby;
        Unit pet,guardian,unowned;
        pet.owner=&owner;guardian.owner=&owner;
        Unit* summoner=source==0?static_cast<Unit*>(&owner):source==1?&pet:source==2?&guardian:source==3?&unowned:nullptr;
        TempSummon bunny(summoner);
        bunny.nearest=&nearby;
        if(source==5)bunny.owner=&owner;
        SmartScript script(&bunny);
        script.mEvents.push_back(definition);
        for(uint32 identity:{6u,7u,8u}){SmartScriptHolder follow;follow.event_id=identity;follow.link=identity==8?0:identity+1;follow.eventType=61;follow.actionType=12;follow.event.event_flags=1;script.mEvents.push_back(follow);}
        script.Run(5);
        assert(nearby.credit==0);
        assert(owner.credit==(source<=2?1u:0u));
        assert(script.actions==std::vector<uint32>({205,12,12,12}));
        script.Run(5);
        assert(owner.credit==(source<=2?1u:0u)&&nearby.credit==0);
        assert(script.actions.size()==4&&errors==0);
    }
    Player owner,other;
    TempSummon bunny(&owner);
    bunny.nearest=nullptr;
    SmartScript script(&bunny);
    auto solo=definition;solo.link=0;
    script.mEvents={solo};script.Run(5);assert(owner.credit==1);
    bunny.summoner=nullptr;bunny.nearest=&other;
    script.mEvents={solo};script.Run(5);assert(owner.credit==1&&other.credit==0);
    std::cout<<"PASS: native owner/summoner target and credit/LINK/runOnce bodies award the actual harpoon owner, including pet/guardian owners, not a closer player; missing/disconnected/unowned summoners do not fall back to bystanders, companions remain linked. Controlled object/scenario sinks, not client traversal.\n";
}
