#define TRUTHGUARD_ENDPOINT_TEST
#include "TruthguardRiderTest.cpp"
struct Flight
{
    TempSummon actor;
    Player owner,stranger;
    Script script;
    explicit Flight(uint32 entry):script(&actor)
    {
        actor.entry=entry; actor.map=entry==105886?1220:1495; actor.m_summonerGUID={17};
        vehicle.base=&actor; otherVehicle.base=&stranger; owner.m_vehicle=&vehicle; owner.scenario=909;
        ObjectAccessor::objects.clear(); ObjectAccessor::objects[17]=&owner;
        endpointWarnings=0; motion.paths.clear();
    }
    void Arrive() { BaseAI* dispatch=&script; dispatch->MovementInform(WAYPOINT_MOTION_TYPE,4); }
    void Tick(uint32 diff) { BaseAI* dispatch=&script; dispatch->UpdateAI(diff); }
};
int main()
{
    uint32 cases=0;
    for(uint32 entry : {105886u,106205u})
        for(SpellCastResult result : {SPELL_FAILED_ERROR,SPELL_FAILED_BAD_TARGETS,SPELL_FAILED_DONT_REPORT})
            for(uint32 previous : {0u,909u,1082u,1068u})
            {
                Flight flight(entry); flight.owner.scenario=previous; flight.owner.result=result;
                flight.Arrive(); assert(flight.owner.casts.size()==1);
#ifdef ENDPOINT_BASELINE
                assert(flight.actor.despawns==1 && endpointWarnings==0);
                assert(flight.owner.scenario==(entry==105886?1082u:previous));
                flight.Arrive(); assert(flight.owner.casts.size()==2 && flight.actor.despawns==2);
#else
                assert(!flight.actor.despawns && endpointWarnings==1 && flight.owner.scenario==previous);
                assert(flight.script.arrivalRetryTimer==1000 && !flight.script.arrivalInProgress && !flight.script.arrivalAccepted);
                flight.Arrive(); flight.script.PassengerBoarded(&flight.owner,0,true); flight.Tick(0); flight.Tick(999);
                assert(flight.owner.casts.size()==1 && motion.paths.empty());
                flight.Tick(1); assert(flight.owner.casts.size()==2 && endpointWarnings==2 && !flight.actor.despawns && flight.owner.scenario==previous);
                flight.owner.result=SPELL_CAST_OK;
                flight.owner.castHook=[&] { if(entry==105886) { assert(flight.owner.scenario==1082); flight.owner.scenario=0; } flight.Arrive(); flight.Tick(2000); flight.script.PassengerBoarded(&flight.owner,0,true); };
                flight.Tick(1000);
                assert(flight.owner.casts.size()==3 && flight.actor.despawns==1 && flight.script.arrivalAccepted && !flight.script.arrivalInProgress && !flight.script.arrivalRetryTimer);
                flight.Arrive(); flight.Tick(100000); flight.script.Reset(); flight.Arrive(); flight.script.PassengerBoarded(&flight.owner,0,true);
                assert(flight.owner.casts.size()==3 && flight.actor.despawns==1 && motion.paths.empty());
                for(uint32 spell : flight.owner.casts) assert(spell==(entry==105886?209908u:210360u));
                assert(flight.stranger.casts.empty());
#endif
                ++cases;
            }
#ifndef ENDPOINT_BASELINE
    for(uint32 entry : {105886u,106205u})
        for(uint32 missing=0;missing<10;++missing)
        {
            Flight flight(entry); flight.owner.result=SPELL_FAILED_ERROR; flight.Arrive();
            if(missing==0) flight.owner.m_vehicle=nullptr;
            if(missing==1) flight.owner.m_vehicle=&otherVehicle;
            if(missing==2) ObjectAccessor::objects.erase(17);
            if(missing==3) flight.actor.map=1;
            if(missing==4) { flight.actor.charmer=&flight.stranger; flight.stranger.m_vehicle=&vehicle; flight.owner.m_vehicle=nullptr; }
            if(missing==5) flight.owner.quest=0;
            if(missing==6) flight.owner.quest=5;
            if(missing==7) flight.owner.quest=6;
            if(missing==8) flight.owner.rewarded=true;
            if(missing==9) flight.owner.alive=false;
            flight.Tick(1000); flight.Tick(10000);
            assert(flight.owner.casts.size()==1 && flight.stranger.casts.empty() && !flight.actor.despawns && !flight.script.arrivalRetryTimer);
            flight.actor.map=entry==105886?1220:1495; ObjectAccessor::objects[17]=&flight.owner; flight.owner.m_vehicle=&vehicle; flight.owner.result=SPELL_CAST_OK;
            flight.owner.quest=3; flight.owner.rewarded=false; flight.owner.alive=true;
            flight.script.PassengerBoarded(&flight.owner,0,true); assert(motion.paths.size()==1);
            flight.Arrive(); assert(flight.owner.casts.size()==2 && flight.actor.despawns==1);
        }
    for(SpellCastResult result : {SPELL_CAST_OK,SPELL_FAILED_ERROR})
    {
        Flight flight(105886); flight.owner.result=result;
        flight.owner.castHook=[&] { flight.Arrive(); flight.owner.scenario=1068; };
        flight.Arrive(); assert(flight.owner.casts.size()==1 && flight.owner.scenario==1068);
        assert(flight.actor.despawns==uint32(result==SPELL_CAST_OK));
    }
    for(uint32 entry : {105886u,106205u})
    {
        Flight flight(entry); flight.owner.quest=1; flight.Arrive(); assert(flight.owner.casts.size()==1 && flight.actor.despawns==1);
    }
    for(uint32 entry : {105886u,106205u})
    {
        Flight flight(entry); flight.owner.result=SPELL_FAILED_DONT_REPORT; flight.Arrive(); flight.Tick(UINT32_MAX);
        assert(flight.owner.casts.size()==2 && flight.script.arrivalRetryTimer==1000 && !flight.actor.despawns);
        flight.Tick(0); assert(flight.owner.casts.size()==2);
    }
#endif
    std::cout << "PASS: " << cases << " route/native-rejection/previous-selector combinations; ";
#ifdef ENDPOINT_BASELINE
    std::cout << "pinned618afd7c despawns after rejection, retains wrong outgoing selector and repeats endpoint casts/despawns on duplicate callbacks.\n";
#else
    std::cout << "1000ms AI-owned retry, conditional selector rollback, reentrant/duplicate/Reset guards, accepted-once cleanup, native summoner/vehicle revocation and fresh re-entry pass. Native spell effects, actual teleport/credit/persistence/client not instantiated.\n";
#endif
}
