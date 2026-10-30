#include <cassert>
#include <cstdint>
#include <functional>
#include <fstream>
#include <iostream>
#include <list>
#include <map>
#include <set>
#include <vector>
using uint32=uint32_t;
constexpr int SMART_ACTION_INVOKER_CAST=85, SMART_ACTION_CALL_KILLEDMONSTER=33, SMART_ACTION_UPDATE_ACHIEVEMENT_CRITERIA=205;
constexpr int SMART_TARGET_ACTION_INVOKER_PLAYER=130,SMART_ACTION_ADD_AURA=75,SMART_ACTION_CAST=11,SMART_ACTION_CAST_CUSTOM=210;
constexpr int SMART_EVENT_HEALTH_PCT=2,SMART_EVENT_FLAG_NOT_REPEATABLE=1,SMART_SCRIPT_TYPE_CREATURE=0,SMART_TARGET_VICTIM=2;
constexpr int SPELL_EFFECT_KILL_CREDIT=90, SPELL_EFFECT_KILL_CREDIT2=134;
using CriteriaTypes=int;
#define TC_LOG_DEBUG(...) ((void)0)
#define TC_LOG_ERROR(...) ((void)0)
#include "CastFlags.inc"
struct Unit; struct Player; struct Creature; struct Vehicle;
struct ObjectGuid {
    Unit* unit=nullptr;
    bool IsPlayer() const;
    static ObjectGuid Empty;
};
ObjectGuid ObjectGuid::Empty;
struct Map{} firstMap,secondMap;
struct WorldObject {
    bool world=true,phase=true,scope=true;float distance=1;Map* map=&firstMap;
    virtual ~WorldObject()=default;
    virtual Unit* ToUnit() { return nullptr; }
    virtual Player* ToPlayer() { return nullptr; }
    virtual ObjectGuid GetGUID() { return {ToUnit()}; }
    bool IsInWorld(){return world;}Map* GetMap(){return map;}bool InSamePhase(WorldObject*){return phase;}
    bool IsWithinDistInMap(WorldObject*,float range){return distance<=range;}
    bool HasQuestObjectScope(){return !scope;}bool CanTargetQuestObject(WorldObject*){return scope;}
};
struct Unit:WorldObject {
    Unit* owner=nullptr;
    Unit* charmer=nullptr;
    bool available=true, aura=false,castWorks=true,lastTriggered=false;
    int casts=0, interrupted=0,stories=0;
    uint32 lastSpell=0;
    Unit* lastTarget=nullptr;
    Vehicle* vehicle=nullptr;
    std::vector<std::function<void()>> delayed;
    Unit* ToUnit() override { return this; }
    virtual Player* ToPlayer() { return nullptr; }
    virtual Player const* ToPlayer() const { return nullptr; }
    ObjectGuid GetGUID() { return {this}; }
    ObjectGuid GetCharmerOrOwnerGUID() const { return {charmer ? charmer : owner}; }
    Player* GetCharmerOrOwnerPlayerOrPlayerItself() const;
    Vehicle* GetVehicleKit() { return vehicle; }
    bool HasAura(uint32) { return aura; }
    void InterruptNonMeleeSpells(bool) { ++interrupted; }
    template<class F> void AddDelayedEvent(int delay,F f) { assert(delay==200); delayed.emplace_back(f); }
    void Flush() { auto pending=std::move(delayed); delayed.clear(); for(auto& f:pending) f(); }
    void CastSpell(Unit* target,uint32 spell,bool);
};
struct Player:Unit {
    int credits=0, events=0, criteria=0;
    Unit* criterionInvoker=nullptr;
    WorldObject* eventSource=nullptr;
    Player* ToPlayer() override { return this; }
    Player const* ToPlayer() const override { return this; }
    void KilledMonsterCredit(uint32) { ++credits; }
    void RewardPlayerAndGroupAtEvent(uint32,WorldObject* source) { ++events; eventSource=source; }
    void UpdateAchievementCriteria(CriteriaTypes,uint32,uint32,uint32,Unit* actualInvoker,bool) { ++criteria; criterionInvoker=actualInvoker; }
};
struct Creature:Unit { Player* loot=nullptr; Player* GetLootRecipient() { return loot; } };
struct Vehicle { struct Seat { struct { ObjectGuid Guid; } Passenger; }; std::map<int,Seat> Seats; };
bool ObjectGuid::IsPlayer() const { return unit && unit->ToPlayer(); }
struct ObjectAccessor {
    static Player* GetPlayer(Unit const&,ObjectGuid guid) { return guid.unit && guid.unit->available ? guid.unit->ToPlayer() : nullptr; }
    static Player* FindPlayer(ObjectGuid guid) { return guid.unit && guid.unit->available ? guid.unit->ToPlayer() : nullptr; }
    static Unit* GetUnit(Unit const&,ObjectGuid guid) { return guid.unit && guid.unit->available ? guid.unit : nullptr; }
    static WorldObject* GetWorldObject(WorldObject const&,ObjectGuid guid){return guid.unit&&guid.unit->available?guid.unit:nullptr;}
};
#include "OwnerPlayer.inc"
struct SpellEffectInfo { int Effect=0; bool IsEffect() const { return Effect!=0; } };
struct SpellInfo {
    bool personal=false,group=false;
    SpellEffectInfo credit,extra;
    SpellEffectInfo const* Effects[2];
    SpellInfo(bool p=false,bool g=false,bool mixed=false):personal(p),group(g),credit{p?134:(g?90:0)},extra{mixed?2:0},Effects{&credit,&extra} {}
    bool HasEffect(int effect) const { return (effect==134 && personal) || (effect==90 && group); }
};
struct SpellManager {
    SpellInfo personal{true,false},group{false,true},ordinary{},mixed{true,false,true},conversation{},energy{};
    SpellManager(){conversation.extra.Effect=219;energy.extra.Effect=30;}
    SpellInfo const* GetSpellInfo(uint32 id) { if(id==198287)return &personal; if(id==900)return &group; if(id==123)return &ordinary; if(id==200832||id==204364||id==197760)return &conversation; if(id==152435)return &energy; if(id==901)return &mixed; return nullptr; }
} spellManager;
auto sSpellMgr=&spellManager;
void Unit::CastSpell(Unit* target,uint32 spell,bool triggered) {
    ++casts; lastSpell=spell; lastTarget=target;lastTriggered=triggered;
    if(!castWorks)return;
    // Count Player-owned presentations; this is not a claim that a pet caster
    // cannot create a Conversation. Native creation/playback are tested apart.
    if((spell==200832||spell==204364||spell==197760)&&ToPlayer())++stories;
    // Model the unchanged player-only/self-target requirement of198287.
    if(spell==198287) if(Player* player=ToPlayer()) player->KilledMonsterCredit(100732);
}
#include "CreditHelpers.inc"
#include "PlayerResolver.inc"
using ObjectList=std::list<WorldObject*>;
struct SmartScriptHolder {
    int type=33,targetType=7;
    int eventType=0,sourceType=0,link=0;struct{uint32 event_flags=0;}event;
    uint32 GetActionType()const{return type;}uint32 GetTargetType()const{return targetType;}
    uint32 GetEventType()const{return eventType;}uint32 GetScriptType()const{return sourceType;}
    struct {struct{uint32 dist=50;}playerDistance;}target;
    struct {
        struct { uint32 spell=198287,flags=0; } cast;
        struct { uint32 creature=100732; } killedMonster;
        struct { int type=92; uint32 misc1=50959,misc2=0,misc3=0; } achievementCriteria;
    } action;
};
bool IsSpellValid(SmartScriptHolder&,uint32 identity){return sSpellMgr->GetSpellInfo(identity)!=nullptr;}
bool ValidateCast(SmartScriptHolder& event){switch(event.GetActionType()){
#include "CastValidation.inc"
    default:return false;
}return true;}
struct SmartScript {
    Creature* me=nullptr;
    WorldObject* base=nullptr;
    bool trigger=false;
    Unit* invoker=nullptr;
    ObjectList selection;
    WorldObject* GetBaseObject() { return base; }
    Unit* GetLastInvoker() { return invoker; }
    ObjectList* GetTargets(SmartScriptHolder& event,Unit* unit) {
        if(event.GetTargetType()!=130)return selection.empty()?nullptr:new ObjectList(selection);
        Unit* currentInvoker=unit?unit:GetLastInvoker();auto* targets=new ObjectList;
        switch(event.GetTargetType()){
#include "PlayerTarget.inc"
        }
#include "TargetTail.inc"
    bool IsUnit(WorldObject* object) { return object && object->ToUnit(); }
    bool IsPlayer(WorldObject* object) { return object && object->ToUnit() && object->ToUnit()->ToPlayer(); }
    void Run(SmartScriptHolder& e,Unit* unit) { invoker=unit; switch(e.type) {
#include "KillAction.inc"
#include "CriteriaAction.inc"
#include "InvokerCast.inc"
    }}
};
int main(int argc,char** argv) {
    Player owner,other;
    Unit pet,guardian,charmed,npc,ownerless,npcOwned;
    pet.owner=&owner; guardian.owner=&owner; charmed.charmer=&owner; npcOwned.owner=&npc;
    WorldObject gameobject;
    for(Unit* u:{static_cast<Unit*>(&owner),&pet,&guardian,&charmed}) assert(GetScriptCreditPlayer(u)==&owner);
    for(WorldObject* u:std::initializer_list<WorldObject*>{&ownerless,&npcOwned,&gameobject,nullptr}) assert(!GetScriptCreditPlayer(u));
    owner.available=false; assert(!GetScriptCreditPlayer(&pet)); owner.available=true;
    assert(GetScriptCreditPlayer(&owner)==&owner);
    Creature victim; WorldObject base;
    SmartScript script; script.me=&victim; script.base=&base;
    SmartScriptHolder action;
    script.selection={&pet}; action.type=33; script.Run(action,&pet); assert(owner.events==1);
    action.type=205; script.Run(action,&pet); assert(owner.criteria==1);
    owner.events=owner.criteria=0;
    // Player, pets and guardians in the same selection grant one reward per owner.
    script.selection={&owner,&pet,&guardian,&charmed,&gameobject,&npcOwned,&other};
    action.type=33; script.Run(action,&pet); assert(owner.events==1 && other.events==1 && owner.eventSource==&owner);
    action.type=205; script.Run(action,&pet); assert(owner.criteria==1 && other.criteria==1 && owner.criterionInvoker==&pet);
    // Existing loot recipient takes priority, including when somebody else's pet kills.
    victim.loot=&other; script.selection={&pet}; action.type=33; script.Run(action,&pet);
    assert(other.events==2 && owner.events==1); victim.loot=nullptr;
    Vehicle vehicle; vehicle.Seats[0].Passenger.Guid=owner.GetGUID(); pet.vehicle=&vehicle;
    script.selection={&pet,&owner}; script.Run(action,&pet); assert(owner.events==2); pet.vehicle=nullptr;
    // No actor/base, as with a trigger: retain the original event source for group range checks.
    script.me=nullptr; script.base=nullptr; script.trigger=true; script.Run(action,&pet);
    assert(owner.events==3 && owner.eventSource==&pet);
    script.Run(action,nullptr); script.Run(action,&ownerless); assert(owner.events==3);
    // Pet-triggered kill-credit spell: caster and invoker-target both become the player.
    script.me=&victim; script.base=&base; script.trigger=false;
    action.type=85; script.selection={&pet,&owner}; script.Run(action,&pet);
    owner.Flush(); pet.Flush(); assert(owner.casts==1 && pet.casts==0 && owner.lastTarget==&owner && owner.credits==1);
    // Explicit corpse target is retained. Native SELF credit and linked casts still belong to the player.
    script.selection={&victim}; script.Run(action,&guardian); owner.Flush();
    assert(owner.casts==2 && owner.lastTarget==&victim && owner.credits==2);
    action.action.cast.spell=900; script.selection={&pet}; script.Run(action,&pet); owner.Flush(); assert(owner.casts==3);
    // Ordinary spells, including repeated target records, retain their real caster and targets.
    action.action.cast.spell=123; script.selection={&pet,&pet}; script.Run(action,&pet); pet.Flush();
    assert(pet.casts==2 && pet.lastTarget==&pet && owner.casts==3);
    action.action.cast.spell=999; script.selection={&pet}; script.Run(action,&pet); pet.Flush(); assert(pet.casts==3);
    action.action.cast.spell=901; script.Run(action,&pet); pet.Flush(); assert(pet.casts==4 && owner.casts==3);
    // A missing/disconnected owner never redirects credit to an unrelated player.
    owner.available=false; action.action.cast.spell=198287; script.Run(action,&pet); pet.Flush();
    assert(owner.credits==2 && other.credits==0); owner.available=true;
    // Caster normalization does not replace the stored event invoker.
    script.Run(action,&pet); assert(script.GetLastInvoker()==&pet); owner.Flush();
    {
        SmartScriptHolder story;story.type=85;story.targetType=130;story.action.cast={200832,SMARTCAST_INVOKER_PLAYER};
        if(argc>=2){std::ifstream input(argv[1]);assert(input>>story.type>>story.action.cast.spell>>story.action.cast.flags>>story.targetType>>story.target.playerDistance.dist);}
        assert(story.type==85&&story.action.cast.flags==128&&story.targetType==130&&((story.action.cast.spell==200832&&story.target.playerDistance.dist==50)||((story.action.cast.spell==204364||story.action.cast.spell==197760)&&story.target.playerDistance.dist==70)||(story.action.cast.spell==152435&&story.target.playerDistance.dist==100)));
        uint32 const storySpell=story.action.cast.spell,storyRange=story.target.playerDistance.dist;
        assert(ValidateCast(story));
        for(int target:{7,21}){story.targetType=target;assert(!ValidateCast(story));}story.targetType=130;
        story.target.playerDistance.dist=0;assert(!ValidateCast(story));story.target.playerDistance.dist=storyRange;
        Creature source;script.me=&source;script.base=&source;
        if(argc>=3){SmartScriptHolder raw;std::ifstream input(argv[2]);assert(input>>raw.type>>raw.action.cast.spell>>raw.action.cast.flags>>raw.targetType>>raw.target.playerDistance.dist);assert(raw.type==85&&raw.action.cast.spell==storySpell&&raw.action.cast.flags==0&&raw.targetType==7&&raw.target.playerDistance.dist==0&&ValidateCast(raw));script.selection={&pet};int before=owner.casts,petBefore=pet.casts;script.Run(raw,&pet);assert(pet.delayed.size()==1&&owner.delayed.empty());pet.Flush();assert(pet.casts==petBefore+1&&pet.lastSpell==storySpell&&pet.lastTarget==&pet&&owner.casts==before);std::cout<<"COUNTERFACTUAL raw SQL"<<storySpell<<"/flags0/target7 keeps pet caster and target; native payload semantics are tested separately\n";}
        for(Unit* killer:{static_cast<Unit*>(&owner),&pet,&guardian,&charmed}){
            int previous=owner.casts;script.Run(story,killer);assert(script.GetLastInvoker()==killer&&owner.delayed.size()==1&&owner.casts==previous);
            owner.Flush();assert(owner.casts==previous+1&&owner.lastTarget==&owner&&owner.lastSpell==storySpell);
        }
        int previous=owner.casts;
        script.Run(story,&pet);pet.available=false;pet.owner=&other;owner.Flush();assert(owner.casts==previous+1&&other.casts==0);pet.available=true;pet.owner=&owner;previous=owner.casts;
        for(Unit* killer:{&ownerless,&npcOwned,static_cast<Unit*>(nullptr)})script.Run(story,killer);
        owner.Flush();assert(owner.casts==previous);
        for(int blocked=0;blocked<8;++blocked){
            owner.world=true;source.world=true;owner.map=&firstMap;source.map=&firstMap;owner.phase=true;owner.distance=1;source.scope=true;owner.available=true;
            script.Run(story,&pet);assert(owner.delayed.size()==1);
            switch(blocked){case 0:source.available=false;break;case 1:owner.world=false;break;case 2:source.world=false;break;case 3:owner.map=&secondMap;break;case 4:owner.phase=false;break;case 5:owner.distance=float(storyRange)+0.01f;break;case 6:source.scope=false;break;case 7:owner.available=false;break;}
            previous=owner.casts;owner.Flush();assert(owner.casts==previous);source.available=true;
        }
        owner.world=true;source.world=true;owner.map=&firstMap;owner.phase=true;owner.distance=float(storyRange);source.scope=true;owner.available=true;
        previous=owner.casts;script.Run(story,&guardian);owner.Flush();assert(owner.casts==previous+1);
        for(int blocked=0;blocked<7;++blocked){
            owner.world=true;source.world=true;owner.map=&firstMap;owner.phase=true;owner.distance=1;source.scope=true;owner.available=true;
            switch(blocked){case 0:owner.available=false;break;case 1:owner.world=false;break;case 2:source.world=false;break;case 3:owner.map=&secondMap;break;case 4:owner.phase=false;break;case 5:owner.distance=float(storyRange)+1;break;case 6:source.scope=false;break;}
            previous=owner.casts;script.Run(story,&pet);owner.Flush();assert(owner.casts==previous&&owner.delayed.empty());
        }
        owner.world=true;owner.map=&firstMap;owner.phase=true;owner.distance=1;source.scope=true;owner.available=true;source.world=true;
        story.targetType=21;previous=owner.casts;script.Run(story,&pet);owner.Flush();assert(owner.casts==previous);
        story.targetType=130;story.target.playerDistance.dist=0;script.Run(story,&pet);owner.Flush();assert(owner.casts==previous);
        story.target.playerDistance.dist=storyRange;script.base=nullptr;script.Run(story,&pet);owner.Flush();assert(owner.casts==previous);
        script.base=&source;story.action.cast.flags=128|SMARTCAST_AURA_NOT_PRESENT;owner.aura=true;script.Run(story,&pet);owner.Flush();assert(owner.casts==previous);owner.aura=false;
        story.action.cast.flags=128|SMARTCAST_TRIGGERED|SMARTCAST_INTERRUPT_PREVIOUS;int interrupts=owner.interrupted;script.Run(story,&pet);owner.Flush();assert(owner.casts==previous+1&&owner.lastTriggered&&owner.interrupted==interrupts+1);
        story.action.cast.flags=128;owner.castWorks=false;int presentations=owner.stories;previous=owner.casts;script.Run(story,&pet);owner.Flush();assert(owner.casts==previous+1&&owner.stories==presentations&&owner.delayed.empty());owner.castWorks=true;previous=owner.casts;
        script.base=&source;story.action.cast.flags=0;script.Run(story,&pet);pet.Flush();assert(pet.lastSpell==storySpell&&owner.casts==previous);
    }
    std::cout<<"PASS: production SmartAI credit/criteria/invoker-cast; opt-in bounded Player caster/target, loader validation, delayed source/owner revalidation, pets/guardians/charmers/missing owners, dedup, loot/vehicle/trigger source, ordinary spell isolation and raw invoker preservation\n";
}
