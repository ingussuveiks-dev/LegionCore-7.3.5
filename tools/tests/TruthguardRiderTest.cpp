#include <cassert>
#include <cstdint>
#include <functional>
#include <iostream>
#include <map>
#include <vector>
using uint32=uint32_t;
using uint16=uint16_t;
using uint8=uint8_t;
using int8=int8_t;
#include "Motion.inc"
#include "CastResults.inc"
uint32 endpointWarnings=0;
#define TC_LOG_WARN(...) (++endpointWarnings)
enum { NPC_HIPPOGRYPH_DALARAN_SUM=105886, NPC_HIPPOGRYPH_SCENARIO_SUM=106205,
    SPELL_FLY_TO_STORMHEIM=209908, SPELL_RETURN_TO_DALARAN=210360,
    QUEST_STATUS_INCOMPLETE=3, QUEST_STATUS_COMPLETE=1 };
using QuestStatus=uint32;
struct Unit;
struct Player;
struct TempSummon;
struct ObjectGuid
{
    uint32 value=0;
    bool IsEmpty() const { return value==0; }
    explicit operator bool() const { return !IsEmpty(); }
};
struct Vehicle { Unit* base=nullptr; Unit* GetBase() const { return base; } } vehicle,otherVehicle;
struct Unit
{
    virtual ~Unit()=default;
    virtual Player* ToPlayer() { return nullptr; }
    bool summoned=true;
    Unit* charmer=nullptr;
    Unit* parent=nullptr;
    Vehicle* m_vehicle=nullptr;
    bool isSummon() const { return summoned; }
    ObjectGuid GetCharmerGUID() const { return {charmer?1u:0u}; }
    Unit* GetCharmer() const { return charmer; }
    Unit* GetOwner() const { return parent; }
    Unit* GetAnyOwner() const;
    Unit* GetVehicleBase() const;
#include "ToTemp.inc"
#include "ToTempConst.inc"
};
struct Player : Unit
{
    std::vector<uint32> casts;
    uint32 scenario=0;
    uint32 quest=3;
    bool alive=true,rewarded=false;
    SpellCastResult result=SPELL_CAST_OK;
    std::function<void()> castHook;
    Player* ToPlayer() override { return this; }
    void SetScenarioId(uint32 identity) { scenario=identity; }
    uint16 GetScenarioId() const { return uint16(scenario); }
    uint32 GetGUIDLow() const { return 17; }
    QuestStatus GetQuestStatus(uint32 identity) const { assert(identity==42017); return quest; }
    bool GetQuestRewardStatus(uint32 identity) const { assert(identity==42017); return rewarded || quest==6; }
    bool IsAlive() const { return alive; }
    SpellCastResult CastSpell(Unit* target,uint32 spell,bool triggered) { assert(target==this && triggered); casts.push_back(spell); if(castHook) castHook(); return result; }
};
struct Motion { std::vector<uint32> paths; void MovePath(uint32 identity,bool repeat) { assert(!repeat); paths.push_back(identity); } } motion;
struct Creature : Unit
{
    uint32 entry=0,map=0,despawns=0;
    uint32 GetEntry() const { return entry; }
    uint32 GetMapId() const { return map; }
    Motion* GetMotionMaster() { return &motion; }
    void DespawnOrUnsummon(uint32 delay) { assert(delay==500); ++despawns; }
};
struct TempSummon : Creature
{
    ObjectGuid m_summonerGUID;
    Unit* GetSummoner() const;
};
namespace ObjectAccessor
{
    std::map<uint32,Unit*> objects;
    Unit* GetUnit(TempSummon const&,ObjectGuid identity) { auto found=objects.find(identity.value); return found==objects.end()?nullptr:found->second; }
}
#include "Summoner.inc"
#include "AnyOwner.inc"
#include "VehicleBase.inc"
struct BaseAI
{
    virtual ~BaseAI()=default;
    virtual void PassengerBoarded(Unit*,int8,bool)=0;
    virtual void MovementInform(uint32,uint32)=0;
    virtual void UpdateAI(uint32)=0;
    virtual void Reset()=0;
};
struct Script : BaseAI
{
    Creature* me;
    explicit Script(Creature* actor):me(actor) {}
#include "State.inc"
#include "Rider.inc"
#include "Boarded.inc"
#include "Arrived.inc"
#include "Update.inc"
#include "Reset.inc"
};
#ifndef TRUTHGUARD_ENDPOINT_TEST
int main()
{
    uint32 boardedCases=0,endpointCases=0;
    for(uint32 entry : {105883u,105886u,106203u,106205u,999u})
        for(uint32 map : {1220u,1495u,1u})
            for(bool summoned : {false,true})
                for(uint32 kind=0;kind<4;++kind)
                    for(uint32 riding=0;riding<3;++riding)
                        for(bool charmed : {false,true})
                        {
                            TempSummon actor; Creature other;
                            Player owner,stranger; Unit pet,guardian;
                            pet.parent=guardian.parent=&owner;
                            actor.entry=entry; actor.map=map; actor.summoned=summoned;
                            actor.charmer=charmed?&stranger:nullptr;
                            actor.m_summonerGUID={17};
                            ObjectAccessor::objects.clear();
                            if(kind<3) ObjectAccessor::objects[17]=kind==0?static_cast<Unit*>(&owner):kind==1?&pet:&guardian;
                            vehicle.base=&actor; otherVehicle.base=&other;
                            owner.m_vehicle=riding==1?&vehicle:riding==2?&otherVehicle:nullptr;
                            stranger.m_vehicle=&vehicle;
#ifndef BASELINE
                            bool eligible=summoned && kind==0 && riding==1 && ((entry==105886 && map==1220)||(entry==106205 && map==1495));
#endif
                            Script script(&actor); BaseAI* dispatch=&script;
                            for(Unit* passenger : {static_cast<Unit*>(nullptr),static_cast<Unit*>(&owner),static_cast<Unit*>(&stranger),&pet,&guardian})
                                for(bool apply : {false,true})
                                {
                                    motion.paths.clear(); dispatch->PassengerBoarded(passenger,0,apply);
#ifdef BASELINE
                                    bool starts=apply && (map==1220 || map==1495);
#else
                                    bool starts=apply && passenger==&owner && eligible;
#endif
                                    assert(motion.paths==(starts?std::vector<uint32>{entry*100}:std::vector<uint32>{}));
                                    ++boardedCases;
                                }
                            for(uint32 type : {uint32(WAYPOINT_MOTION_TYPE),uint32(POINT_MOTION_TYPE)})
                                for(uint32 point : {0u,4u,5u})
                                {
                                    owner.casts.clear(); stranger.casts.clear(); owner.scenario=stranger.scenario=0; actor.despawns=0;
                                    Script trial(&actor); BaseAI* trialDispatch=&trial; trialDispatch->MovementInform(type,point);
                                    bool final=type==WAYPOINT_MOTION_TYPE && point==4;
#ifdef BASELINE
                                    Player* recipient=charmed?&stranger:summoned && kind==0?&owner:nullptr;
                                    bool arrives=final && recipient;
#else
                                    Player* recipient=&owner;
                                    bool arrives=final && eligible;
#endif
                                    assert(actor.despawns==uint32(arrives));
                                    if(arrives)
                                    {
                                        std::vector<uint32> expected=entry==105886?std::vector<uint32>{209908}:entry==106205?std::vector<uint32>{210360}:std::vector<uint32>{};
                                        assert(recipient->casts==expected && recipient->scenario==(entry==105886?1082u:0u));
                                    }
                                    else assert(owner.casts.empty() && stranger.casts.empty() && !owner.scenario && !stranger.scenario);
#ifndef BASELINE
                                    assert(stranger.casts.empty() && !stranger.scenario);
#endif
                                    ++endpointCases;
                                }
                        }
#ifndef BASELINE
    TempSummon actor; actor.entry=105886; actor.map=1220; actor.m_summonerGUID={17};
    Player owner; vehicle.base=&actor; owner.m_vehicle=&vehicle; ObjectAccessor::objects[17]=&owner;
    Script script(&actor); motion.paths.clear(); script.PassengerBoarded(&owner,0,true); assert(motion.paths.size()==1);
    owner.m_vehicle=nullptr; script.MovementInform(WAYPOINT_MOTION_TYPE,4); assert(owner.casts.empty() && !actor.despawns);
    owner.m_vehicle=&vehicle; ObjectAccessor::objects.erase(17); script.MovementInform(WAYPOINT_MOTION_TYPE,4); assert(owner.casts.empty());
    ObjectAccessor::objects[17]=&owner; script.MovementInform(WAYPOINT_MOTION_TYPE,4); assert(owner.casts==std::vector<uint32>{209908} && owner.scenario==1082 && actor.despawns==1);
    actor.m_summonerGUID={}; Script emptyGuid(&actor); emptyGuid.MovementInform(WAYPOINT_MOTION_TYPE,4); assert(owner.casts.size()==1 && actor.despawns==1);
#endif
    std::cout << "PASS: " << boardedCases << " boarding and " << endpointCases << " endpoint extracted virtual cases; ";
#ifdef BASELINE
    std::cout << "pinned be077aa6 starts paths for null/foreign/pet passengers and endpoint requests reach unboarded owners or charmer instead of summoner.\n";
#else
    std::cout << "only original resolved Player summoner actually aboard the matching bird/map starts travel or receives original endpoint; native summoner/vehicle/owner helpers, missing actor and dismount/re-entry tested in controlled services, not live vehicle/client traversal.\n";
#endif
}
#endif
