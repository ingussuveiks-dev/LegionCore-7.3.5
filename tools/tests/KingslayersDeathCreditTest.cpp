#include <cassert>
#include <cstdint>
#include <functional>
#include <iostream>
#include <list>
#include <memory>
#include <map>
#include <set>
#include <shared_mutex>
#include <vector>
using uint32=uint32_t;using uint64=uint64_t;
enum {CRITERIA_TYPE_SCRIPT_EVENT_2=92};
struct CriteriaTreeEntry {uint32 ID,CriteriaID;};
struct DB2Manager
{
    std::map<uint32,std::vector<CriteriaTreeEntry const*>> trees;
    std::vector<CriteriaTreeEntry const*> const* GetCriteriaTreeList(uint32 id){auto it=trees.find(id);return it==trees.end()?nullptr:&it->second;}
} db2Manager;
auto& sDB2Manager=db2Manager;
struct Scenario
{
    uint32 currentTree=48588,_criteriaTreeForUpdate=0;
    bool CanUpdateCriteria(uint32 criteriaId,uint32 recursTree=0)const;
};
#include "ScenarioGate.inc"
enum class HighGuid {Conversation};
struct ObjectGuid
{
    uint64 high=0,low=0;
    bool IsPlayer()const{return high==1;}
    bool IsEmpty()const{return !high&&!low;}
    uint32 GetGUIDLow()const{return uint32(low);}
};
struct Map {};
struct WorldObject
{
    Map* map=nullptr;ObjectGuid objectGuid;bool inWorld=true,preDelete=false;
    Map* GetMap()const{return map;}
    bool IsInWorld()const{return inWorld;}
    bool IsPreDelete()const{return preDelete;}
};
struct Player;
struct Unit:WorldObject
{
    bool playerType=false;ObjectGuid owner,charmer;
    virtual ~Unit()=default;
    ObjectGuid GetCharmerGUID()const{return charmer;}
    ObjectGuid GetOwnerGUID()const{return owner;}
    Player const* ToPlayer()const;
    Player* ToPlayer(){return const_cast<Player*>(static_cast<Unit const*>(this)->ToPlayer());}
    Player* GetCharmerOrOwnerPlayerOrPlayerItself()const;
#include "OwnerGuid.inc"
};
struct Player:Unit
{
    std::vector<uint32> criteria;
    Player(){playerType=true;objectGuid={1,1};}
    void UpdateAchievementCriteria(uint32 type,uint32 asset){assert(type==92&&asset==51615);criteria.push_back(asset);}
};
Player const* Unit::ToPlayer()const{return playerType?static_cast<Player const*>(this):nullptr;}
template<class T>struct HashMapHolder
{
    inline static bool _checkLock=false;
    inline static std::shared_mutex i_lockVector;
    inline static uint32 _size=32;
    inline static std::vector<T*> _objectVector=std::vector<T*>(32,nullptr);
#include "PlayerFind.inc"
};
struct ObjectAccessor
{
#include "PlayerLookup.inc"
};
#include "PlayerLookupMethod.inc"
#include "OwnerPlayer.inc"
struct Creature;
struct ScriptedAI
{
    Creature* me;explicit ScriptedAI(Creature* creature):me(creature){}
    virtual ~ScriptedAI()=default;virtual void JustDied(Unit*){}
    virtual void DoAction(int32_t action){assert(action==1);}
};
struct Creature:Unit
{
    ScriptedAI* ai=nullptr;Creature* previous=nullptr;Creature* next=nullptr;
    std::list<Creature*> adds;uint32 despawns=0,summoned=0;bool visible=false;
    Creature* FindNearestCreature(uint32 entry,float radius,bool alive){assert(entry==108222&&radius==60&&alive);return previous;}
    Creature* SummonCreature(uint32 entry,float x,float y,float z){assert(entry==108222&&x==-8310.60f&&y==284.40f&&z==156.83f);++summoned;return next;}
    ScriptedAI* AI(){return ai;}
    void DespawnOrUnsummon(){++despawns;}
    void SetVisible(bool value){visible=value;}
};
struct SummonList
{
    uint32 cleanup=0;std::function<void()> onCleanup;
    void DespawnAll(){++cleanup;if(onCleanup)onCleanup();}
};
void GetCreatureListWithEntryInGrid(std::list<Creature*>& out,Creature* me,uint32 entry,float range)
{assert(entry==108397&&range==70);out=me->adds;}
struct Generator {uint64 Generate(){return 1;}};
struct ObjectMgr
{
    Generator generator;
    template<HighGuid> Generator* GetGenerator(){return &generator;}
} objectMgr;
auto sObjectMgr=&objectMgr;
struct Conversation
{
    inline static std::vector<std::unique_ptr<Conversation>> owned;
    inline static std::vector<Unit*> casters;
    inline static bool fail=false;inline static uint32 live=0;
    Conversation(){++live;}~Conversation(){--live;}
    bool CreateConversation(uint64,uint32 id,Unit* caster,void*,Unit const& anchor)
    {
        assert(id==2448&&caster==&anchor);casters.push_back(caster);
        if(fail)return false;
        owned.emplace_back(this);return true;
    }
};
struct Merlis:ScriptedAI
{
    SummonList summons;explicit Merlis(Creature* actor):ScriptedAI(actor){}
#include "MerlisDeath.inc"
};
struct OutroAI:ScriptedAI
{
    uint32 actions=0;explicit OutroAI(Creature* actor):ScriptedAI(actor){}
    void DoAction(int32_t action)override{assert(action==1);++actions;}
};
void ResetConversations(){Conversation::owned.clear();assert(!Conversation::live);Conversation::casters.clear();Conversation::fail=false;}
int main()
{
    CriteriaTreeEntry merlisTree{48589,30475},nested{48588,0},prior{48587,30474},pickup{48591,30476};
    db2Manager.trees[48588]={&merlisTree};db2Manager.trees[48586]={&prior};db2Manager.trees[48590]={&pickup};db2Manager.trees[999]={&nested};
    Scenario scenario;assert(scenario.CanUpdateCriteria(48589));scenario.currentTree=48586;assert(!scenario.CanUpdateCriteria(48589));
    scenario.currentTree=48590;assert(!scenario.CanUpdateCriteria(48589));scenario._criteriaTreeForUpdate=48588;assert(scenario.CanUpdateCriteria(48589));
    scenario._criteriaTreeForUpdate=0;scenario.currentTree=999;assert(scenario.CanUpdateCriteria(48589));
    scenario.currentTree=0;assert(!scenario.CanUpdateCriteria(48589));
    Map map;[[maybe_unused]] Map otherMap;Player owner,friendPlayer;owner.map=&map;friendPlayer.map=&map;friendPlayer.objectGuid={1,2};
    HashMapHolder<Player>::_objectVector[1]=&owner;HashMapHolder<Player>::_objectVector[2]=&friendPlayer;
    Creature actor,previous,outro,add1,add2;actor.map=&map;actor.previous=&previous;actor.next=&outro;actor.adds={&add1,&add2};
    OutroAI outroAI(&outro);outro.ai=&outroAI;Merlis ai(&actor);
    Unit pet;pet.map=&map;pet.owner=owner.objectGuid;
    ai.JustDied(&pet);
#ifdef BASELINE
    assert(owner.criteria.empty()&&Conversation::casters==std::vector<Unit*>{&pet});
    std::cout<<"PASS counterfactual ab20ad22: actual Merlis hook loses owned-pet51615 and creates2448 on the pet; old null dereference not executed\n";
#else
    assert(owner.criteria.size()==1&&Conversation::casters==std::vector<Unit*>{&owner});
    assert(previous.despawns==1&&outroAI.actions==1&&outro.visible&&add1.despawns==1&&add2.despawns==1&&ai.summons.cleanup==1);
    ResetConversations();
    ai.JustDied(&owner);assert(owner.criteria.size()==2&&Conversation::casters==std::vector<Unit*>{&owner});
    ResetConversations();
    Unit guardian;guardian.map=&map;guardian.owner=owner.objectGuid;
    ai.JustDied(&guardian);assert(owner.criteria.size()==3&&Conversation::casters==std::vector<Unit*>{&owner});
    ResetConversations();
    guardian.charmer=friendPlayer.objectGuid;ai.JustDied(&guardian);
    assert(owner.criteria.size()==3&&friendPlayer.criteria.size()==1&&Conversation::casters==std::vector<Unit*>{&friendPlayer});
    guardian.charmer={};ResetConversations();
    for(int variant=0;variant<6;++variant)
    {
        Unit unowned;unowned.map=&map;
        Unit* killer=&unowned;
        if(variant==0)killer=nullptr;
        if(variant==2)unowned.owner={1,20};
        if(variant==3){unowned.owner=owner.objectGuid;unowned.map=&otherMap;}
        if(variant==4){unowned.owner=owner.objectGuid;owner.preDelete=true;}
        if(variant==5){unowned.owner=owner.objectGuid;owner.inWorld=false;}
        ai.JustDied(killer);assert(owner.criteria.size()==3&&friendPlayer.criteria.size()==1&&Conversation::casters.empty());
        owner.preDelete=false;owner.inWorld=true;
    }
    // A despawn boundary may retire the killer; resolve its player first.
    auto disappearing=std::make_unique<Unit>();disappearing->map=&map;disappearing->owner=owner.objectGuid;
    ai.summons.onCleanup=[&]{disappearing.reset();};ai.JustDied(disappearing.get());ai.summons.onCleanup={};
    assert(owner.criteria.size()==4&&Conversation::casters==std::vector<Unit*>{&owner});ResetConversations();
    actor.previous=nullptr;actor.next=nullptr;actor.adds.clear();Conversation::fail=true;
    ai.JustDied(&guardian);assert(owner.criteria.size()==5&&!Conversation::live);ResetConversations();
    // Fresh encounter after reset/re-entry uses the same native attribution, with no stale player cache.
    Merlis fresh(&actor);fresh.JustDied(&guardian);assert(owner.criteria.size()==6&&fresh.summons.cleanup==1);
    assert(friendPlayer.criteria.size()==1); // No new blanket party fanout added by this hook.
    std::cout<<"PASS native Merlis death hook + native charmer/owner selection + native in-map live player lookup: direct/pet/guardian/charm, null/unowned/missing/map/predelete/offline, removed killer, missing actors/conversation failure and fresh encounter; existing cleanup and personal51615/2448 recipient retained. Scenario/party/faction/combat/loot/client boundaries are not simulated\n";
#endif
    ResetConversations();
}
