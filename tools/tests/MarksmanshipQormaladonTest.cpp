#include <cassert>
#include <cstdint>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <set>
#include <sstream>
#include <string>
#include <vector>
using uint32 = uint32_t;
enum ReactStates { REACT_PASSIVE, REACT_DEFENSIVE, REACT_AGGRESSIVE };
enum DamageEffectType { DIRECT_DAMAGE, SPELL_DIRECT_DAMAGE, DOT };
enum { UNIT_FIELD_FLAGS=0, UNIT_FLAG_NON_ATTACKABLE=2, UNIT_FLAG_IMMUNE_TO_PC=256,
    CONDITION_SCENARION_STEP=44, CONDITION_OBJECT_ENTRY=31, SMART_EVENT_DATA_SET=38,
    SMART_EVENT_DAMAGED=9, SMART_EVENT_DEATH=6, SMART_ESCORT_ESCORTING=1, DONE=3,
    ENCOUNTER_FRAME_DISENGAGE=1, ENCOUNTER_FRAME_INSTANCE_END=2,
    SMART_ACTION_REMOVE_UNIT_FLAG=19, SMART_ACTION_UPDATE_ACHIEVEMENT_CRITERIA=205 };
using CriteriaTypes=uint32;
#define TC_LOG_DEBUG(...) ((void)0)
struct Creature; struct Unit; struct Player;
struct InstanceMap { uint32 GetInstanceId() const { return 42; } } instanceMap;
struct Map { bool instanced=true; InstanceMap* ToInstanceMap(){return instanced?&instanceMap:nullptr;} } worldMap;
struct WorldObject {
    virtual ~WorldObject()=default;
    virtual Unit* ToUnit(){return nullptr;}
    virtual Creature* ToCreature(){return nullptr;}
    virtual uint32 GetTypeId() const {return 0;}
    virtual uint32 GetEntry() const {return 0;}
    bool InInstance() const {return worldMap.instanced;}
    Map* GetMap() const {return &worldMap;}
    uint32 GetGUIDLow() const {return 123;}
};
struct Unit : WorldObject {
    uint32 flags=0; Player* owner=nullptr;
    Unit* ToUnit() override {return this;}
    virtual Player* ToPlayer(){return nullptr;}
    Player* GetCharmerOrOwnerPlayerOrPlayerItself(){return owner;}
    bool HasFlag(uint32,uint32 mask) const {return (flags&mask)!=0;}
    void SetFlag(uint32,uint32 mask){flags|=mask;}
    void RemoveFlag(uint32,uint32 mask){flags&=~mask;}
};
struct Scenario {uint32 id=972,step=0;uint32 GetScenarioId()const{return id;}uint32 GetCurrentStep()const{return step;}};
struct ScenarioMgr {Scenario scenario;bool present=true;Scenario* GetScenario(uint32 id){return present&&id==42?&scenario:nullptr;}} scenarioMgr;
auto* sScenarioMgr=&scenarioMgr;
struct Player : Unit {
    uint32 credits=0;Unit* invoker=nullptr;
    Player* ToPlayer() override {return this;}
    void UpdateAchievementCriteria(uint32 type,uint32 asset,uint32 a,uint32 b,Unit* unit,bool ignoreGroup){
        assert(type==92&&asset==49654&&!a&&!b&&ignoreGroup);++credits;invoker=unit;
        if(scenarioMgr.scenario.step==5)scenarioMgr.scenario.step=6;
    }
};
struct Creature : Unit {
    uint32 entry=101269,mapId=1489,health=100,stops=0;bool combat=false,alive=true;
    ReactStates reaction=REACT_AGGRESSIVE;
    Creature(){flags=256|32768;}
    Creature* ToCreature() override {return this;}
    uint32 GetTypeId() const override {return 3;}
    uint32 GetEntry()const override {return entry;}
    uint32 GetMapId()const{return mapId;}
    uint32 GetInstanceId()const{return 42;}
    uint32 GetHealth(Unit*)const{return health;}
    void SetHealth(uint32 value){health=value;}
    ReactStates GetReactState()const{return reaction;}
    void SetReactState(ReactStates value){reaction=value;}
    bool isInCombat()const{return combat;}
    void CombatStop(bool interrupt){assert(interrupt);combat=false;++stops;}
};
struct ConditionSourceInfo {
    WorldObject* mConditionTargets[2];
    ConditionSourceInfo(WorldObject* a,WorldObject* b):mConditionTargets{a,b}{}
};
struct Condition {
    uint32 group=0,ConditionTypeOrReference=0,ConditionTarget=0,ConditionValue1=0,ConditionValue2=0;
    std::string script;
    bool Check(WorldObject* object)const{
        if(!object)return false;
        bool condMeets=false;
        switch(ConditionTypeOrReference){
#include "Conditions.inc"
        default:assert(false);
        }
        return condMeets;
    }
};
struct ConditionScript {
    explicit ConditionScript(char const*){}
    virtual ~ConditionScript()=default;
    virtual bool OnConditionCheck(Condition*,ConditionSourceInfo&)=0;
};
struct Row {uint32 id,link,event,flags,param1,param2,actionType,action1,action2,target;bool once=false;};
using ObjectList=std::vector<WorldObject*>;
struct Action {
    struct {uint32 flag=0;} unitFlag;
    struct {uint32 type=0,misc1=0,misc2=0,misc3=0;} achievementCriteria;
};
struct Event {Action action;};
Player* GetScriptCreditPlayer(WorldObject*);
struct SmartScript {
    Creature* actor;std::vector<Row> rows;std::vector<Condition> conditions;
    uint32 damaged=0,deaths=0,text=0,cleanupSignal=0;Unit* lastDamager=nullptr;
    explicit SmartScript(Creature* value):actor(value){}
    Creature* GetBaseObject(){return actor;}
    bool IsUnit(WorldObject* object){return object&&object->ToUnit();}
    ObjectList* GetTargets(Event&,Unit* unit){return new ObjectList{unit?static_cast<WorldObject*>(unit):actor};}
    void Execute(uint32 type,Event& e,Unit* unit){
        switch(type){
#include "Actions.inc"
        default:assert(false);
        }
    }
    bool Allowed(uint32 group,Unit* unit);
    void ProcessEventsFor(uint32 type,Unit* unit=nullptr,uint32 id=0,uint32 value=0);
};
#include "Owner.inc"
struct Instance {
    uint32 state=0,saved=0,encounters=0;
    void SetBossState(uint32 boss,uint32 value){assert(boss==1);state=value;}
    void SaveToDB(){++saved;}
    void SendEncounterUnit(uint32,Creature*){++encounters;}
};
struct SmartAI {
    Creature* me;SmartScript script;Instance* instance=nullptr;uint32 _bossId=0,mInvincibilityHpLevel=0;
    uint32 initializes=0,resets=0,updates=0,attacks=0,pathEnds=0;bool escorting=false;
    explicit SmartAI(Creature* value):me(value),script(value){}
    virtual ~SmartAI()=default;
    virtual void InitializeAI(){++initializes;Reset();}
    virtual void Reset(){++resets;for(auto& row:script.rows)row.once=false;}
    virtual void UpdateAI(uint32 diff){assert(diff==100);++updates;}
    virtual void AttackStart(Unit*){++attacks;}
    virtual void SetData(uint32,uint32);
    virtual void DamageTaken(Unit*,uint32&,DamageEffectType);
    virtual void JustDied(Unit*);
    SmartScript* GetScript(){return &script;}
    bool HasEscortState(uint32 state){assert(state==SMART_ESCORT_ESCORTING);return escorting;}
    void EndPath(bool failed){assert(failed);++pathEnds;}
};
#include "SetData.inc"
#include "DamageTaken.inc"
#include "JustDied.inc"
#include "Qormaladon.inc"
bool SmartScript::Allowed(uint32 group,Unit* unit){
#include "Context.inc"
    for(auto const& condition:conditions)if(condition.group==group){
        if(!condition.Check(info.mConditionTargets[condition.ConditionTarget]))return false;
        if(!condition.script.empty()){
            assert(condition.script=="condition_marksmanship_qormaladon_ready");
            condition_marksmanship_qormaladon_ready custom;
            if(!custom.OnConditionCheck(nullptr,info))return false;
        }
    }
    return true;
}
// Boundary model: real SQL flags, native condition branches and native actions.
// Scheduling, event iteration and LINK dispatch are deliberately not a live map.
void SmartScript::ProcessEventsFor(uint32 type,Unit* unit,uint32 id,uint32 value){
    if(type==SMART_EVENT_DAMAGED){++damaged;lastDamager=unit;return;}
    if(type==SMART_EVENT_DEATH)++deaths;
    for(auto& row:rows){
        if(row.event!=type||row.once||!Allowed(row.id+1,unit))continue;
        if(type==SMART_EVENT_DATA_SET&&(row.param1!=id||row.param2!=value))continue;
        if(row.flags&1)row.once=true;
        Event e;
        if(row.actionType==19){assert(row.target==1);e.action.unitFlag.flag=row.action1;Execute(row.actionType,e,actor);}
        else if(row.actionType==205){assert(row.target==7);if(unit){e.action.achievementCriteria={row.action1,row.action2,0,0};Execute(row.actionType,e,unit);}++text;++cleanupSignal;}
    }
}
int main(){
    _set_abort_behavior(0,_CALL_REPORTFAULT);
    std::vector<Row> rows;std::ifstream input("rows.tsv");Row row{};
    while(input>>row.id>>row.link>>row.event>>row.flags>>row.param1>>row.param2>>row.actionType>>row.action1>>row.action2>>row.target)rows.push_back(row);
    assert(rows.size()==4&&rows[0].id==1&&rows[0].action1==256&&rows[1].id==10&&rows[1].link==11&&rows[1].action2==49654&&rows[1].flags==1);
    std::vector<Condition> conditions;std::ifstream gates("gates.tsv");std::string line;
    while(std::getline(gates,line)){std::istringstream fields(line);Condition c;fields>>c.group>>c.ConditionTypeOrReference>>c.ConditionTarget>>c.ConditionValue1>>c.ConditionValue2>>c.script;conditions.push_back(c);}
    assert(conditions.size()==3);
    auto configure=[&](SmartAI& ai){ai.script.rows=rows;ai.script.conditions=conditions;};
    Player player;Unit pet,guardian,npc;pet.owner=&player;guardian.owner=&player;
    std::vector<Unit*> attackers{&player,&pet,&guardian,&npc,nullptr};
    // Pinned native baseline: original immunity256 does not protect this NPC damage hook.
    scenarioMgr={};scenarioMgr.scenario.step=0;Creature before;SmartAI old(&before);uint32 lethal=UINT32_MAX;
    old.DamageTaken(&npc,lethal,DIRECT_DAMAGE);assert(lethal==UINT32_MAX&&old.script.damaged==1);
    assert(old.script.Allowed(11,&pet)); // No legacy conditions, early raw death event admitted.
    uint32 cases=0;
    for(uint32 step=0;step<=8;++step)for(bool immune:{false,true}){
        scenarioMgr={};scenarioMgr.scenario.step=step;
        Creature actor;if(!immune)actor.RemoveFlag(0,256);
        npc_marksmanship_qormaladon ai(&actor);configure(ai);ai.InitializeAI();
        bool blocked=step!=5||immune;
        assert(ai.initializes==1&&ai.resets==1&&actor.HasFlag(0,2)==blocked);
        assert(actor.HasFlag(0,256)==immune&&actor.HasFlag(0,32768));
        assert(actor.reaction==(blocked?REACT_PASSIVE:REACT_AGGRESSIVE));
        ai.UpdateAI(100);assert(ai.updates==1);
        for(Unit* attacker:attackers){
            uint32 attacks=ai.attacks;ai.AttackStart(attacker);assert(ai.attacks==attacks+uint32(!blocked));
            for(auto type:{DIRECT_DAMAGE,SPELL_DIRECT_DAMAGE,DOT})for(uint32 amount:{0u,1u,100u,UINT32_MAX}){
                uint32 damage=amount,events=ai.script.damaged;ai.DamageTaken(attacker,damage,type);
                assert(damage==(blocked?0u:amount)&&ai.script.damaged==events+uint32(!blocked));
                if(!blocked)assert(ai.script.lastDamager==attacker);
                ++cases;
            }
            actor.alive=false;assert(ai.script.Allowed(11,attacker)==!blocked);actor.alive=true;
        }
        // Rejected unlock must leave its once flag available for the real stage.
        ai.SetData(1,1);assert(actor.HasFlag(0,256)==(immune&&step!=5));
        assert(actor.HasFlag(0,2)==(step!=5));
    }
    // Actual SetData -> actual remove-flag action -> immediate gate release, no update delay.
    scenarioMgr={};Creature actor;actor.combat=true;actor.reaction=REACT_DEFENSIVE;
    npc_marksmanship_qormaladon ai(&actor);configure(ai);ai.InitializeAI();
    assert(actor.stops==1&&actor.HasFlag(0,256)&&actor.HasFlag(0,2));
    ai.UpdateAI(100);ai.SetData(1,1);assert(actor.stops==1&&actor.HasFlag(0,256));
    scenarioMgr.scenario.step=5;ai.UpdateAI(100);assert(actor.HasFlag(0,2));
    ai.SetData(9,9);assert(actor.HasFlag(0,256));
    ai.SetData(1,1);assert(!actor.HasFlag(0,258)&&actor.HasFlag(0,32768)&&actor.reaction==REACT_DEFENSIVE);
    ai.SetData(1,1);assert(!actor.HasFlag(0,258));
    // Existing NON_ATTACKABLE belongs to its original owner, including across Reset.
    scenarioMgr={};Creature protectedActor;protectedActor.flags|=2;
    npc_marksmanship_qormaladon protectedAi(&protectedActor);configure(protectedAi);protectedAi.InitializeAI();
    protectedAi.Reset();scenarioMgr.scenario.step=5;protectedAi.SetData(1,1);
    assert(protectedActor.HasFlag(0,2)&&!protectedActor.HasFlag(0,256));
    // Missing scenario fails closed, but other shared-map scenarios and other actors delegate.
    for(uint32 id:{0u,940u,1085u,1145u,1146u}){
        scenarioMgr={};scenarioMgr.present=id!=0;scenarioMgr.scenario.id=id;
        Creature shared;npc_marksmanship_qormaladon sharedAi(&shared);configure(sharedAi);sharedAi.InitializeAI();
        uint32 damage=100;sharedAi.DamageTaken(&pet,damage,SPELL_DIRECT_DAMAGE);
        assert(damage==(id?100u:0u)&&shared.HasFlag(0,2)==!id);
        assert(!sharedAi.script.Allowed(11,&pet));
    }
    for(bool otherEntry:{false,true}){
        scenarioMgr={};Creature shared;if(otherEntry)shared.entry=99999;else shared.mapId=1511;
        npc_marksmanship_qormaladon sharedAi(&shared);sharedAi.InitializeAI();
        uint32 damage=100;sharedAi.DamageTaken(&npc,damage,DOT);assert(damage==100&&!shared.HasFlag(0,2));
    }
    // Re-entry reconstructs protection from the actual actor flags and current stage.
    scenarioMgr={};scenarioMgr.scenario.step=5;Creature reentered;
    npc_marksmanship_qormaladon reentryAi(&reentered);configure(reentryAi);reentryAi.InitializeAI();
    assert(reentered.HasFlag(0,258));reentryAi.SetData(1,1);assert(!reentered.HasFlag(0,258));
    scenarioMgr.scenario.step=6;reentryAi.UpdateAI(100);assert(reentered.HasFlag(0,2));
    // Native SmartAI invincibility semantics are still delegated after legitimate unlock.
    scenarioMgr={};scenarioMgr.scenario.step=5;Creature clamped;clamped.RemoveFlag(0,256);
    npc_marksmanship_qormaladon clampedAi(&clamped);clampedAi.mInvincibilityHpLevel=10;
    uint32 damage=100;clampedAi.DamageTaken(&pet,damage,DIRECT_DAMAGE);assert(!damage&&clamped.health==10);
    // Inherited native death cleanup always runs, even if a direct-death path is rejected.
    for(bool unlocked:{false,true})for(Unit* killer:attackers){
        scenarioMgr={};scenarioMgr.scenario.step=5;Creature dead;if(unlocked)dead.RemoveFlag(0,256);
        npc_marksmanship_qormaladon deathAi(&dead);configure(deathAi);deathAi.InitializeAI();
        Instance instance;deathAi.instance=&instance;deathAi._bossId=1;deathAi.escorting=true;
        uint32 credits=player.credits;dead.alive=false;deathAi.JustDied(killer);
        bool credited=unlocked&&killer&&killer!=&npc;
        assert(player.credits==credits+uint32(credited));
        if(credited)assert(player.invoker==killer);
        assert(instance.state==DONE&&instance.saved==1&&instance.encounters==2&&deathAi.pathEnds==1);
        assert(deathAi.script.cleanupSignal==uint32(unlocked));
        deathAi.JustDied(killer);assert(player.credits==credits+uint32(credited));
    }
    condition_marksmanship_qormaladon_ready condition;
    ConditionSourceInfo missing(nullptr,nullptr);assert(!condition.OnConditionCheck(nullptr,missing));
    assert(!MarksmanshipInquisitor::Ready(nullptr));MarksmanshipInquisitor::Gate gate;assert(gate.Update(nullptr));
    std::cout<<"PASS "<<cases<<" stage/immunity/damage cases; native SetData/DamageTaken/JustDied/conditions/actions; player/pet/guardian owner credit, direct-death rejection and unchanged cleanup. Event scheduling/group/map traversal remain separate.\n";
    return 0;
}
