#include <algorithm>
#include <cassert>
#include <cmath>
#include <cstdint>
#include <iostream>
#include <map>
#include <memory>
#include <set>
#include <stdexcept>
#include <vector>
using uint32=uint32_t;using uint64=uint64_t;
#include "Constants.inc"
#define TC_LOG_DEBUG(...) ((void)0)
#define TC_LOG_ERROR(...) throw std::runtime_error("Unexpected native SmartAI error")
struct Player;struct Creature;struct Unit;struct SmartAI;struct GameObject;struct SpellInfo{};
struct WorldObject
{
    uint32 entry=0,phase=1;float x=0;bool live=true,expired=false;
    virtual ~WorldObject()=default;
    Unit* ToUnit();Creature* ToCreature();Player* ToPlayer();GameObject* ToGameObject();
    bool IsQuestObjectExpired()const{return expired;}
};
struct TempSummon{Unit* summoner=nullptr;Unit* GetSummoner()const{return summoner;}};
struct Vehicle
{
    struct Seat{struct{uint64 Guid=0;}Passenger;};std::map<int,Seat> Seats;
};
struct Unit:WorldObject
{
    uint64 guid=1;Unit* owner=nullptr;Unit* charmer=nullptr;TempSummon* summon=nullptr;
    bool IsAlive()const{return live;}bool isInCombat()const{return false;}
    bool GetCharmerGUID()const{return charmer!=nullptr;}Unit* GetCharmer()const{return charmer;}
    TempSummon const* ToTempSummon()const{return summon;}Unit* GetOwner()const{return owner;}
    Unit* GetAnyOwner()const;
    Player* GetCharmerOrOwnerPlayerOrPlayerItself();
    uint64 GetGUID()const{return guid;}uint32 GetGUIDLow()const{return uint32(guid);}
    bool HasAura(uint32)const{return false;}Vehicle* GetVehicleKit(){return nullptr;}
};
struct Player:Unit
{
    uint32 calls=0;
    void RewardPlayerAndGroupAtEvent(uint32 credit,WorldObject* anchor){assert(credit==90298&&anchor==this);++calls;}
};
struct Creature:Unit
{
    SmartAI* ai=nullptr;SmartAI* AI(){return ai;}uint32 GetEntry()const{return entry;}
    Player* GetLootRecipient(){return nullptr;}
};
struct GameObject:WorldObject{SmartAI* ai=nullptr;SmartAI* AI(){return ai;}};
Unit* WorldObject::ToUnit(){return dynamic_cast<Unit*>(this);}Creature* WorldObject::ToCreature(){return dynamic_cast<Creature*>(this);}
Player* WorldObject::ToPlayer(){return dynamic_cast<Player*>(this);}GameObject* WorldObject::ToGameObject(){return dynamic_cast<GameObject*>(this);}
Player* Unit::GetCharmerOrOwnerPlayerOrPlayerItself(){if(auto player=ToPlayer())return player;return owner?owner->ToPlayer():nullptr;}
bool IsUnit(WorldObject* object){return object&&object->ToUnit();}bool IsPlayer(WorldObject* object){return object&&object->ToPlayer();}
bool IsCreature(WorldObject* object){return object&&object->ToCreature();}bool IsGameObject(WorldObject* object){return object&&object->ToGameObject();}
struct ObjectAccessor{static Player* FindPlayer(uint64){return nullptr;}};
struct ObjectGuid{static constexpr uint64 Empty=0;};
#include "AnyOwner.inc"
#include "CreditPlayer.inc"
using ObjectList=std::vector<WorldObject*>;
struct Row{uint32 entry,id,link,event,flags,p1,p2,action,a1,a2,target,t1,t2;};
struct SmartScriptHolder
{
    uint32 entryOrGuid=0,event_id=0,link=0,source_type=0,type=0;bool active=true,runOnce=false;
    struct Event{uint32 type=0,event_flags=0,event_phase_mask=0,event_chance=100;struct{uint32 id=0,value=0,cooldownMin=0,cooldownMax=0;}dataSet;}event;
    struct Action{struct{uint32 field=0,data=0;}setData;struct{uint32 creature=0;}killedMonster;}action;
    struct Target{uint32 type=0;struct{uint32 creature=0,dist=0;}unitDistance;struct{uint32 dist=0,spell=0;}playerDistance;}target;
    uint32 GetActionType()const{return type;}uint32 GetEventType()const{return event.type;}uint32 GetTargetType()const{return target.type;}
    uint32 GetScriptType()const{return source_type;}
    explicit SmartScriptHolder(Row row):entryOrGuid(row.entry),event_id(row.id),link(row.link),type(row.action)
    {event.type=row.event;event.event_flags=row.flags;event.dataSet.id=row.p1;event.dataSet.value=row.p2;action.setData={row.a1,row.a2};action.killedMonster.creature=row.a1;target.type=row.target;target.unitDistance={row.t1,row.t2};target.playerDistance={row.t1,row.t2};}
};
using ConditionList=std::set<uint32>;
struct ConditionSourceInfo{WorldObject* source;WorldObject* base;ConditionSourceInfo(WorldObject* a,WorldObject* b):source(a),base(b){}};
struct ConditionMgr
{
    std::set<uint32> deadEntries;
    ConditionList GetConditionsForSmartEvent(uint32 entry,uint32 id,uint32){return id==0&&deadEntries.count(entry)?ConditionList{entry}:ConditionList{};}
    bool IsObjectMeetToConditions(ConditionSourceInfo const& info,ConditionList const& conditions)
    {if(conditions.empty())return true;WorldObject* object=info.base;bool condMeets=false;switch(36){
#include "AliveCondition.inc"
      }return !condMeets;}
} conditionMgr;
auto sConditionMgr=&conditionMgr;
bool roll_chance_i(uint32){return true;}
struct SmartScript;
struct SmartAI
{
    SmartScript* script;SmartScript* GetScript(){return script;}void SetData(uint32 id,uint32 value);void IsSummonedBy(Unit* summoner);
};
std::vector<WorldObject*> world;
struct SmartScript
{
    Creature* me;bool trigger=false;uint64 mLastInvoker=0;uint32 visualRequests=0;std::vector<SmartScriptHolder> mEvents;
    explicit SmartScript(Creature* actor,std::vector<Row> const& rows):me(actor){for(auto row:rows)if(row.entry==me->entry)mEvents.emplace_back(row);}
    WorldObject* GetBaseObject(){return me;}Unit* GetLastInvoker(){return nullptr;}
    bool IsInPhase(uint32){return true;}void RecalcTimer(SmartScriptHolder&,uint32,uint32){}
    ObjectList* GetWorldObjectsInDist(float range){auto list=new ObjectList;for(auto object:world)if(object->phase==me->phase&&std::abs(object->x-me->x)<=range)list->push_back(object);return list;}
    ObjectList* GetTargets(SmartScriptHolder const& e,Unit*)
    {
        auto l=new ObjectList;
        switch(e.GetTargetType()){
#include "Targets.inc"
        default:throw std::runtime_error("Unsupported fixture target");}
        return l;
    }
    SmartScriptHolder FindLinkedEvent(uint32 id){for(auto e:mEvents)if(e.event_id==id)return e;throw std::runtime_error("Missing LINK");}
    void ProcessEventsFor(uint32 type,Unit* unit=nullptr,uint32 var0=0,uint32 var1=0,bool bvar=false,SpellInfo const* spell=nullptr,GameObject* gob=nullptr)
    {for(auto& e:mEvents)if(e.GetEventType()==type&&type!=SMART_EVENT_LINK)ProcessEvent(e,unit,var0,var1,bvar,spell,gob);}
    void ProcessEvent(SmartScriptHolder& e,Unit* unit=nullptr,uint32 var0=0,uint32 var1=0,bool bvar=false,SpellInfo const* spell=nullptr,GameObject* gob=nullptr)
    {
#include "EventGuards.inc"
        switch(e.GetEventType()){
        case SMART_EVENT_LINK:case SMART_EVENT_JUST_SUMMONED:ProcessAction(e,unit,var0,var1,bvar,spell,gob);break;
#include "DataEvent.inc"
        default:break;}
    }
    void ProcessAction(SmartScriptHolder& e,Unit* unit=nullptr,uint32 var0=0,uint32 var1=0,bool bvar=false,SpellInfo const* spell=nullptr,GameObject* gob=nullptr)
    {
#include "ActionPrefix.inc"
        switch(e.GetActionType()){
#include "Actions.inc"
        case 11:++visualRequests;break; // Cast execution is a boundary, not a claimed visual simulation.
        default:break;}
#include "LinkTail.inc"
    void SetPhase(uint32){}void ResetBaseObject(){}void InitTimer(SmartScriptHolder& e){e.active=true;}
    void OnReset();
};
#include "SetData.inc"
#include "Summoned.inc"
#include "Reset.inc"
std::vector<Row> baseline={
#include "BeforeRows.inc"
};
std::vector<Row> repaired={
#include "AfterRows.inc"
};
uint32 Run(std::vector<Row> const& rows,uint32 entry,float distance,bool alive,bool fixed,bool repeat=false,bool ownerMissing=false,bool phaseMismatch=false)
{
    conditionMgr.deadEntries={90230,90241,93619};if(fixed)conditionMgr.deadEntries.insert(101943);
    Player owner,bystander;owner.x=0;bystander.x=0;owner.calls=bystander.calls=0;
    Creature gem,corpse;gem.entry=86963;gem.x=0;corpse.entry=entry;corpse.x=distance;corpse.live=alive;if(phaseMismatch)corpse.phase=2;
    TempSummon summon{ownerMissing?nullptr:&owner};gem.summon=&summon;
    SmartScript gemScript(&gem,rows),corpseScript(&corpse,rows);SmartAI gemAI{&gemScript},corpseAI{&corpseScript};gem.ai=&gemAI;corpse.ai=&corpseAI;
    world={&owner,&bystander,&gem,&corpse};gemAI.IsSummonedBy(&owner);
    if(repeat)gemAI.IsSummonedBy(&owner);
    bool supported=entry==90230||entry==90241||entry==93619||entry==101943;
    if(fixed){assert(!bystander.calls);if(supported&&!ownerMissing&&!alive&&distance<=15&&!phaseMismatch){assert(owner.calls==1);assert(corpseScript.visualRequests==1);corpseScript.OnReset();gemAI.IsSummonedBy(&owner);assert(owner.calls==2);}}
    return owner.calls;
}
int main()
{
    assert(Run(baseline,101943,5,false,false)==0);
    assert(Run(baseline,90230,12,false,false)==0);
    for(uint32 entry:{90230u,90241u,93619u,101943u}){
        assert(Run(repaired,entry,15,false,true,true)==2); // One credit before actual native Reset, then new-life allowance.
        assert(!Run(repaired,entry,15.1f,false,true));assert(!Run(repaired,entry,5,true,true));
        assert(!Run(repaired,entry,5,false,true,false,true));assert(!Run(repaired,entry,5,false,true,false,false,true));
    }
    assert(!Run(repaired,99999,1,false,true));
    // A single placed gem can collect the actual eight distinct corpses, then
    // a second use must not reuse them. Player quest counters/group rules are
    // deliberately outside this fixture.
    conditionMgr.deadEntries={90230,90241,93619,101943};
    Player owner,bystander;Creature gem;gem.entry=86963;TempSummon summon{&owner};gem.summon=&summon;
    SmartScript gemScript(&gem,repaired);SmartAI gemAI{&gemScript};gem.ai=&gemAI;
    world={&owner,&bystander,&gem};std::vector<std::unique_ptr<Creature>> bodies;
    std::vector<std::unique_ptr<SmartScript>> scripts;std::vector<std::unique_ptr<SmartAI>> ais;
    for(uint32 i=0;i<8;++i){auto corpse=std::make_unique<Creature>();corpse->entry=101943;corpse->live=false;corpse->x=float(i+1);
        auto script=std::make_unique<SmartScript>(corpse.get(),repaired);auto ai=std::make_unique<SmartAI>();ai->script=script.get();corpse->ai=ai.get();world.push_back(corpse.get());bodies.push_back(std::move(corpse));scripts.push_back(std::move(script));ais.push_back(std::move(ai));}
    gemAI.IsSummonedBy(&owner);assert(owner.calls==8&&!bystander.calls);gemAI.IsSummonedBy(&owner);assert(owner.calls==8);
    std::cout<<"PASS baseline omitted Felguard and12-yard corpse; production SmartAI target/action/data-event/guards/LINK/owner/Reset extracts execute migrated rows: four dead types within15, living/outside/phase/unknown/owner-missing rejection, repeated corpse once until native Reset, native credit anchored to summoner, cast LINK retained. Real grid/cast/group/quest completion/client are boundaries\n";
}
