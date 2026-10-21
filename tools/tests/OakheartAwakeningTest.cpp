#include <cassert>
#include <cstdint>
#include <iostream>
#include <map>
#include <set>
#include <sstream>
#include <string>
using uint32=uint32_t;using uint64=uint64_t;using int32=int32_t;
#include "DarkheartHeader.inc"
constexpr int DONE=3,UNIT_FIELD_FLAGS=0,REACT_PASSIVE=0,REACT_AGGRESSIVE=2,SPELL_CANCEL_DEEP_ROOTS=165953;
constexpr uint32 UNIT_FLAG_NON_ATTACKABLE=2,UNIT_FLAG_IMMUNE_TO_PC=0x100,UNIT_FLAG_IMMUNE_TO_NPC=0x200;
struct Unit;struct Creature;
struct ObjectGuid{uint64 value=0;bool operator<(ObjectGuid const& o)const{return value<o.value;}};
struct CreatureAI{int actions=0;virtual void DoAction(int32 a){assert(a==ACTION_AWAKEN_OAKHEART);++actions;}};
struct Unit{virtual ~Unit()=default;int entry=0;virtual Creature* ToCreature(){return nullptr;}int GetEntry(){return entry;}};
struct Vehicle{int removed=0;void RemoveAllPassengers(){++removed;}};
struct Creature:Unit{
    uint64 spawn=0;ObjectGuid guid;bool alive=true,IsAIEnabled=true;CreatureAI* ai=nullptr;uint32 flags=0x400000;
    int root=0,react=REACT_AGGRESSIVE,visuals=0,cancels=0,attacks=0;Vehicle vehicle;Unit* victim=nullptr;
    Creature* ToCreature()override{return this;}uint64 GetDBTableGUIDLow(){return spawn;}
    ObjectGuid GetGUID(){return guid;}bool IsAlive(){return alive;}CreatureAI* AI(){return ai;}
    void CastSpell(Creature*,int s,bool t){assert(s==204852&&t);++visuals;} // Optional visual may do nothing.
    void SetFlag(int,uint32 f){flags|=f;}void RemoveFlag(int,uint32 f){flags&=~f;}
    void RemoveAurasDueToSpell(int s){assert(s==165950);root=0;}void SetReactState(int r){react=r;}
    Vehicle* GetVehicleKit(){return &vehicle;}Unit* getVictim(){return victim;}float GetDistance(Unit*){return 10;}
};
struct Map{std::map<ObjectGuid,Creature*> entries;Creature* GetCreature(ObjectGuid id){auto i=entries.find(id);return i==entries.end()?nullptr:i->second;}};
struct BaseInstance{
    virtual void OnCreatureCreate(Creature*){}virtual void OnUnitDeath(Unit*){}
    virtual void ReadSaveDataMore(std::istringstream&){}virtual void WriteSaveDataMore(std::ostringstream&){}
    virtual uint32 GetData(uint32)const{return 0;}
};
struct Instance:BaseInstance{
    Map map;Map* instance=&map;ObjectGuid OakheartGUID,MalfurionGUID,DresaronGUID,MalfurionCageGUID;
    std::set<uint64> slainDwellers;int saves=0,bossState=0;
    int GetBossState(int)const{return bossState;}void SaveToDB(){++saves;}
#include "DwellerLifecycle.inc"
#include "DwellerGetData.inc"
};
struct OakBase:CreatureAI{
    virtual void Reset(){}virtual void MoveInLineOfSight(Unit*){}
};
struct Oak:OakBase{
    Creature* me;Instance* instance;
    Oak(Creature* c,Instance* i):me(c),instance(i){c->ai=this;}
    void _Reset(){}void AttackStart(Unit*){++me->attacks;}
    void DoCast(Creature*,int s,bool t){assert(t);if(s==165950)me->root=1;else{assert(s==165953);++me->cancels;}}
#include "OakAwakening.inc"
#include "OakLos.inc"
};
int main(){
    Instance instance;Creature boss;boss.entry=NPC_OAKHEART;boss.guid={900};Oak ai(&boss,&instance);
    instance.map.entries[boss.guid]=&boss;instance.OnCreatureCreate(&boss);ai.Reset();
    uint32 protectedFlags=UNIT_FLAG_NON_ATTACKABLE|UNIT_FLAG_IMMUNE_TO_PC|UNIT_FLAG_IMMUNE_TO_NPC;
    assert(boss.root&&boss.react==REACT_PASSIVE&&(boss.flags&protectedFlags)==protectedFlags);
    Unit player;ai.MoveInLineOfSight(&player);assert(!boss.attacks);
    Creature dweller;dweller.entry=NPC_NIGHTMARE_DWELLER;dweller.alive=false;
    dweller.spawn=0;instance.OnUnitDeath(&dweller);assert(!instance.saves);
    for(uint64 spawn:{50u,40u,30u,20u}){
        dweller.spawn=spawn;instance.OnUnitDeath(&dweller);instance.OnUnitDeath(&dweller);instance.OnCreatureCreate(&dweller);
        assert(!instance.GetData(DATA_OAKHEART_AWAKENED));
    }
    assert(instance.saves==4&&dweller.visuals==4);
    std::ostringstream saved;instance.WriteSaveDataMore(saved);
    Instance restored;std::istringstream input(saved.str());restored.ReadSaveDataMore(input);
    assert(restored.slainDwellers==instance.slainDwellers&&!restored.GetData(DATA_OAKHEART_AWAKENED));
    dweller.spawn=10;restored.OnUnitDeath(&dweller);assert(restored.GetData(DATA_OAKHEART_AWAKENED)); // Boss grid absent.
    Creature newBoss;newBoss.entry=NPC_OAKHEART;newBoss.guid={901};Oak newAI(&newBoss,&restored);newAI.Reset();
    restored.map.entries[newBoss.guid]=&newBoss;restored.OnCreatureCreate(&newBoss);
    assert(!newBoss.root&&newBoss.react==REACT_AGGRESSIVE&&(newBoss.flags&protectedFlags)==0&&(newBoss.flags&0x400000));
    newAI.MoveInLineOfSight(&player);assert(newBoss.attacks==1);
    newAI.Reset();assert(!newBoss.root&&newBoss.vehicle.removed>0); // Wipe preserves completed trash gate.
    dweller.spawn=10;instance.OnUnitDeath(&dweller);assert(instance.GetData(DATA_OAKHEART_AWAKENED));
    assert(!boss.root&&boss.cancels==1&&instance.saves==5); // Optional death visual had no progression effect.
    instance.OnUnitDeath(&dweller);assert(instance.saves==5&&boss.cancels==1);
    for(std::string malformed:{"","D2 5 1 2 3 4 5","D1 6 1 2 3 4 5 6","D1 5 1 2","D1 2 1 1","D1 1 0"}){
        std::istringstream stream(malformed);restored.ReadSaveDataMore(stream);assert(restored.slainDwellers.empty());
    }
    restored.bossState=DONE;assert(restored.GetData(DATA_OAKHEART_AWAKENED)); // Legacy boss-only completed save.
    std::cout<<"PASS: five distinct DB deaths, duplicate/corpse reload, pet-independent credit, deferred boss, partial persistence, legacy/malformed saves, wipe and native root/flag cleanup\n";
}
