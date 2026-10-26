#include "OwnerHost.inc"
constexpr uint32 TYPEID_PLAYER=4,DAY=86400;
struct ScriptedAI
{
    Creature* me;uint32 speeches=0;
    explicit ScriptedAI(Creature* value):me(value){}
    virtual ~ScriptedAI()=default;
    virtual void JustDied(Unit*){}
    void Talk(uint32 group){assert(group==4);++speeches;}
};
struct Creature:Unit
{
    Creature* display=nullptr;bool failObject=false;uint32 actors=0,objects=0,displayId=0;
    Creature* SummonCreature(uint32 entry,float x,float y,float z,float o){
        assert(entry==112817&&x==-1353.04f&&y==6307.06f&&z==8.60f&&o==1.99f);++actors;return display;
    }
    void SetDisplayId(uint32 value){assert(value==72343);displayId=value;}
    void* SummonGameObject(uint32 entry,float x,float y,float z,float o,float a,float b,float c,float d,uint32 duration){
        assert(entry==254087&&x==-1353.04f&&y==6307.06f&&z==8.60f&&o==1.99f&&!a&&!b&&!c&&!d&&duration==DAY);
        ++objects;return failObject?nullptr:this;
    }
};
struct Eliza:ScriptedAI
{
    explicit Eliza(Creature* value):ScriptedAI(value){}
#include "ElizaDeath.inc"
};
int main()
{
    CriteriaTreeEntry eliza{46707,29469},pickup{46691,29463},prior{46673,29461},nested{46690,0};
    db2Manager.trees[46690]={&eliza,&pickup};db2Manager.trees[46672]={&prior};db2Manager.trees[999]={&nested};
    Scenario scenario;scenario.currentTree=46690;assert(scenario.CanUpdateCriteria(46707)&&scenario.CanUpdateCriteria(46691));
    scenario.currentTree=46672;assert(!scenario.CanUpdateCriteria(46707));
    scenario.currentTree=0;assert(!scenario.CanUpdateCriteria(46707));
    scenario._criteriaTreeForUpdate=46690;assert(scenario.CanUpdateCriteria(46707));
    scenario._criteriaTreeForUpdate=0;scenario.currentTree=999;assert(scenario.CanUpdateCriteria(46707));
    Map map,other;Player owner,friendPlayer;owner.map=&map;friendPlayer.map=&map;friendPlayer.objectGuid={1,2};
    HashMapHolder<Player>::_objectVector[1]=&owner;HashMapHolder<Player>::_objectVector[2]=&friendPlayer;
    Creature actor,display;actor.map=&map;actor.display=&display;Eliza ai(&actor);
    Unit pet;pet.map=&map;pet.owner=owner.objectGuid;
    ai.JustDied(&pet);
#ifdef BASELINE
    assert(owner.criteria.empty()&&ai.speeches==1&&actor.actors==1&&actor.objects==1&&display.displayId==72343);
    std::cout<<"PASS pinned38f88905 counterfactual: Eliza death loses owned-pet48582 but still reveals the actor/object. Original null dereference not executed.\n";
#else
    assert(owner.criteria.size()==1&&ai.speeches==1&&actor.actors==1&&actor.objects==1&&display.displayId==72343);
    ai.JustDied(&owner);assert(owner.criteria.size()==2);
    Unit guardian;guardian.map=&map;guardian.owner=owner.objectGuid;
    ai.JustDied(&guardian);assert(owner.criteria.size()==3);
    guardian.charmer=friendPlayer.objectGuid;ai.JustDied(&guardian);
    assert(owner.criteria.size()==3&&friendPlayer.criteria.size()==1);guardian.charmer={};
    for(int variant=0;variant<6;++variant){
        Unit unowned;unowned.map=&map;Unit* killer=&unowned;
        if(variant==0)killer=nullptr;
        if(variant==2)unowned.owner={1,20};
        if(variant==3){unowned.owner=owner.objectGuid;unowned.map=&other;}
        if(variant==4){unowned.owner=owner.objectGuid;owner.preDelete=true;}
        if(variant==5){unowned.owner=owner.objectGuid;owner.inWorld=false;}
        auto speeches=ai.speeches,actors=actor.actors,objects=actor.objects;
        ai.JustDied(killer);
        assert(owner.criteria.size()==3&&friendPlayer.criteria.size()==1&&ai.speeches==speeches+1&&actor.actors==actors+1&&actor.objects==objects+1);
        owner.preDelete=false;owner.inWorld=true;
    }
    actor.display=nullptr;actor.failObject=true;
    ai.JustDied(&pet);assert(owner.criteria.size()==4); // Failure to create presentation does not erase earned death credit.
    Eliza fresh(&actor);fresh.JustDied(&guardian);assert(owner.criteria.size()==5&&fresh.speeches==1);
    assert(friendPlayer.criteria.size()==1); // Native shared-scenario rules, no new proximity/group awards.
    std::cout<<"PASS actual Eliza death + native owner/charmer + live in-map player lookup + native scenario membership: direct/pet/guardian/charm, null/unowned/missing/wrong-map/offline/predelete, failed display/object and fresh encounter. Speech/reveal always retained; death scheduling/combat/group/faction/client loot are separate.\n";
#endif
}
