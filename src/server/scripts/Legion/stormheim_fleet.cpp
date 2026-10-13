#include "stormheim_fleet.h"
#include "stormheim_fleet_state.h"
#include "ScriptMgr.h"
#include "SmartAI.h"
#include "InstanceScript.h"
#include "ScenarioMgr.h"
#include "Scenario.h"
#include "AchievementMgr.h"
#include "Player.h"
#include "QuestData.h"
#include "DatabaseEnv.h"
#include "ObjectAccessor.h"
#include "Transport.h"
#include "TransportMgr.h"
#include "Vehicle.h"
#include "MotionMaster.h"
#include "TemporarySummon.h"
#include "SpellScript.h"
#include "SpellInfo.h"
#include "GossipDef.h"
#include "LFGMgr.h"
#include "Group.h"
#include "Log.h"
#include <atomic>
#include <map>
#include <set>
#include <cmath>

namespace StormheimFleet
{
uint32 QuestId(Player* p) { return p->GetTeam() == ALLIANCE ? 39800 : 39801; }
struct Checkpoint { State state; uint64 serial = 0; };
std::map<ObjectGuid,Checkpoint> checkpoints;
std::map<ObjectGuid,std::weak_ptr<uint8>> leases;
std::map<ObjectGuid,uint32> finaleTimers;
struct FlareCast { ObjectGuid object;uint64 serial=0,token=0;std::weak_ptr<uint8> lifetime; };
std::map<ObjectGuid,FlareCast> flareCasts;
uint64 nextFlareToken=0;
std::atomic<uint32> acceptanceSequence{0};

void Save(Player* p, Checkpoint const& c)
{
    auto stmt = CharacterDatabase.GetPreparedStatement(CHAR_UPD_STORMHEIM_MISSION);
    stmt->setUInt32(0,c.state.stage); stmt->setUInt32(1,c.state.air); stmt->setUInt32(2,c.state.captains);
    stmt->setUInt32(3,c.state.flares); stmt->setUInt32(4,c.state.frost); stmt->setUInt8(5,c.state.final);
    stmt->setUInt64(6,p->GetGUID().GetCounter()); stmt->setUInt32(7,QuestId(p)); stmt->setUInt64(8,c.serial);
    CharacterDatabase.Execute(stmt);
}
Checkpoint* ReadCheckpoint(Player* p)
{
    auto found = checkpoints.find(p->GetGUID());
    if (found != checkpoints.end()) return &found->second;
    auto stmt = CharacterDatabase.GetPreparedStatement(CHAR_SEL_STORMHEIM_MISSION);
    stmt->setUInt64(0,p->GetGUID().GetCounter()); stmt->setUInt32(1,QuestId(p));
    PreparedQueryResult result = CharacterDatabase.Query(stmt);
    if (!result) return nullptr;
    Field* f = result->Fetch(); Checkpoint c;
    c.serial=f[0].GetUInt64(); c.state.alliance=p->GetTeam()==ALLIANCE;
    c.state.stage=f[1].GetUInt32(); c.state.air=f[2].GetUInt32(); c.state.captains=f[3].GetUInt32();
    c.state.flares=f[4].GetUInt32(); c.state.frost=f[5].GetUInt32(); c.state.final=f[6].GetUInt8()!=0;
    if (c.state.air>30 || c.state.captains>7 || c.state.flares>127 || c.state.stage>(c.state.alliance?5u:6u)) return nullptr;
    return &checkpoints.emplace(p->GetGUID(),c).first->second;
}
void Accepted(Player* p)
{
    if(!p)return;
    uint32 quest=QuestId(p);
    if(p->GetQuestStatus(quest)!=QUEST_STATUS_INCOMPLETE)return;
    // Each acceptance has its own durable serial. Stale asynchronous saves
    // and cleanup cannot overwrite/delete a newly accepted mission.
    Checkpoint c; c.state.alliance=p->GetTeam()==ALLIANCE;
    c.serial=(uint64(time(nullptr))<<32) | ++acceptanceSequence;
    auto stmt=CharacterDatabase.GetPreparedStatement(CHAR_REP_STORMHEIM_MISSION);
    stmt->setUInt64(0,p->GetGUID().GetCounter()); stmt->setUInt32(1,quest); stmt->setUInt64(2,c.serial);
    CharacterDatabase.DirectExecute(stmt);
    checkpoints[p->GetGUID()]=c; finaleTimers.erase(p->GetGUID());
    p->AddQuestDelayedEvent(quest,0,[]{}); leases[p->GetGUID()]=p->getQuestStatus(quest)->ScriptLifetime;
}
bool Active(Player* p)
{
    return p && p->IsInWorld() && p->IsAlive() && p->GetQuestStatus(QuestId(p))==QUEST_STATUS_INCOMPLETE;
}
Position const ArrivalA={3209.48f,3072.73f,440.212f,1.29438f};
Position const ArrivalH={4110.7f,2944.49f,28.1858f,4.38078f};
Position const LocalDeck={-16.0f,5.0f,20.65f,0.0f};
Position const LocalGun={-13.30f,0.0f,25.531f,3.17f};
Position const LocalParachute={-40.0f,12.0f,21.0f,3.14f};
Position const LocalFlares[]={
    {-34,12,20.80f,0},{-31,16,20.65f,0},{-28,12,20.65f,0},
    {-25,16,20.60f,0},{-22,12,20.65f,0},{-19,16,20.75f,0},{-16,12,20.60f,0}
};
Position const Boats[]={
    {4841.76f,3013.46f,7.95f,3.77f}, // Banshee's Wail
    {4800.38f,2873.15f,7.95f,3.32f}, // Black Rose
    {4623.04f,2853.60f,7.95f,1.58f}, // Windrunner: retained report spawn
    {4549.81f,2943.12f,7.95f,3.67f}, // Eternity
    {4730.20f,2938.19f,7.95f,3.51f}  // Blightcaller
};
struct Actor { Kind kind; uint32 slot=0; bool done=false; uint32 entry=0; Position position; bool deck=false; uint32 grace=2000; };
struct ObjectAction { Kind kind; uint32 slot; Position local; };
enum { DATA_GUN=1475001, DATA_LAND=1475002, DATA_TRAVEL=1475003 };

struct Mission : InstanceScript
{
    explicit Mission(InstanceMap* map) : InstanceScript(map) { }
    ObjectGuid owner, ship, flight;
    std::weak_ptr<uint8> lifetime;
    uint64 serial=0;
    State state;
    std::map<ObjectGuid,Actor> actors;
    std::map<ObjectGuid,ObjectAction> objects;
    bool initialized=false, landed=false, frozen=false, spawning=false,reposition=false;
    uint32 timer=0, layoutStage=99, groundKills=0, groundRequired=0, travelTarget=0;
    Position landing;
    std::vector<ObjectGuid> openingActors;
    Player* Owner() const
    {
        Player* p=ObjectAccessor::FindPlayer(owner);
        return Active(p) && p->GetInstanceId()==instance->GetInstanceId() && p->GetMapId()==1475 && !lifetime.expired()?p:nullptr;
    }
    Scenario* Progress() const { return sScenarioMgr->GetScenario(instance->GetInstanceId()); }
    Transport* Ship() const
    {
        GameObject* go=instance->GetGameObject(ship);
        return go?go->ToTransport():nullptr;
    }
    Position DeckWorld(Position local=LocalDeck) const
    {
        if (Transport* t=Ship())
        {
            float x=local.GetPositionX(),y=local.GetPositionY(),z=local.GetPositionZ(),o=local.GetOrientation();
            t->CalculatePassengerPosition(x,y,z,&o);local.Relocate(x,y,z,o);
        }
        return local;
    }
    Position CheckpointPlace() const
    {
        if(state.alliance)
        {
            if(state.stage<=1 || state.stage>=4)return DeckWorld();
            if(state.stage==2){uint32 i=CaptainIndex();return i?Boats[i-1]:DeckWorld();}
            return Boats[2];
        }
        if(state.stage<=1)return Boats[2];
        if(state.stage==2)return Boats[3];
        if(state.stage==3)return Boats[4];
        if(state.stage==4)return Boats[1];
        return DeckWorld();
    }
    WorldLocation graveyard;
    WorldLocation* GetClosestGraveYard(float,float,float) override
    {
        Position pos=CheckpointPlace();graveyard.WorldRelocate(1475,pos.GetPositionX(),pos.GetPositionY(),pos.GetPositionZ(),pos.GetOrientation());return &graveyard;
    }
    void PutOnDeck(Player* p)
    {
        Transport* t=Ship();if(!t)return;
        if (p->GetTransport() && p->GetTransport()!=t) p->GetTransport()->RemovePassenger(p);
        t->AddPassenger(p);p->m_movementInfo.transport.Pos.Relocate(LocalDeck);p->m_movementInfo.transport.VehicleSeatIndex=-1;
        Position world=DeckWorld();p->NearTeleportTo(world.GetPositionX(),world.GetPositionY(),world.GetPositionZ(),world.GetOrientation());
    }
    void Remember()
    {
        if (Player* p=Owner())
        {
            auto found=checkpoints.find(owner);
            if(found!=checkpoints.end() && found->second.serial==serial){found->second.state=state;Save(p,found->second);}
        }
    }
    void Event(uint32 event, uint32 count=1)
    {
        if(Player* p=Owner())for(uint32 i=0;i<count;++i)p->UpdateAchievementCriteria(CRITERIA_TYPE_SCRIPT_EVENT_2,event);
    }
    void Cleanup()
    {
        for(auto const& pair:objects)if(GameObject* go=instance->GetGameObject(pair.first))go->Delete();
        objects.clear();
        auto old=std::move(actors);actors.clear();
        for(auto const& pair:old)if(Creature* c=instance->GetCreature(pair.first)){c->CombatStop(true);c->RemoveAllAuras();c->DespawnOrUnsummon();}
        flight.Clear(); frozen=false; groundKills=groundRequired=0;
    }
    void OnGameObjectCreate(GameObject* go) override
    {
        InstanceScript::OnGameObjectCreate(go);
        if(go->GetEntry()==241630){ship=go->GetGUID();if(Transport* t=go->ToTransport())t->EnableMovement(false);}
    }
    void OnCreatureCreate(Creature* c) override
    {
        InstanceScript::OnCreatureCreate(c);
        // Installed actors belong to the Horde opening only; summoned actors
        // below have their own owner/role and must not inherit that visibility.
        if(c->GetDBTableGUIDLow())
        {
            openingActors.push_back(c->GetGUID());
            if(initialized && state.alliance)c->SetVisible(false);
        }
    }
    void RestoreCriteria()
    {
        Scenario* progress=Progress();if(!progress)return;
        progress->GetAchievementMgr().ClearMap();progress->SetCurrentStep(0);
        if(state.alliance)
        {
            if(state.stage>0)Event(44799);
            if(state.stage>1)Event(44893);
            for(uint32 i=0;i<3;++i)if(state.captains&(1u<<i))Event(i==0?44892:i==1?45033:45034);
            Event(53493,Bits(state.flares));
        }
        else
        {
            if(state.stage>0)Event(45340);
            Event(45416,state.air);
            if(state.stage>2)Event(45365);
            if(state.stage>3)Event(45366);
            Event(53445,Bits(state.flares));
        }
        if(state.final)Event(state.alliance?45041:45368);
        // Restore the completed scenario display from earned finale proof.
        // finale directly and is credited only at the destination.
    }
    void OnPlayerEnter(Player* p) override
    {
        if(!Active(p))return;
        if(!owner.IsEmpty() && owner!=p->GetGUID())return;
        Checkpoint* c=ReadCheckpoint(p);if(!c){Accepted(p);c=ReadCheckpoint(p);}if(!c)return;
        bool reset=!initialized || serial!=c->serial;
        if(reset)
        {
            Cleanup();owner=p->GetGUID();serial=c->serial;state=c->state;
            initialized=true;layoutStage=99;landed=false;
        }
        // Logout destroys the old Player/quest lifetime even when this map
        // instance survives. Rebind a returning owner on every entry.
        p->AddQuestDelayedEvent(QuestId(p),0,[]{});lifetime=p->getQuestStatus(QuestId(p))->ScriptLifetime;leases[owner]=lifetime;
        if(reset)RestoreCriteria();
        timer=0;reposition=true;
        for(ObjectGuid guid:openingActors)if(Creature* actor=instance->GetCreature(guid))
        {
            actor->SetVisible(!state.alliance);actor->SetPhaseMask(p->GetPhaseMask(),false);actor->SetPhaseId(p->GetPhases(),false);actor->UpdateObjectVisibility();
        }
    }
    Creature* Spawn(uint32 entry,Kind kind,uint32 slot,Position pos,bool deck=false)
    {
        Player* p=Owner();if(!p)return nullptr;
        for(auto const& a:actors)if(a.second.kind==kind && a.second.slot==slot)
        {
            if(a.second.done)return nullptr;
            if(a.second.grace || instance->GetCreature(a.first))return instance->GetCreature(a.first);
        }
        TempSummon* c=nullptr;
        if(deck && Ship())
        {
            c=Ship()->SummonPassenger(entry,pos,TEMPSUMMON_MANUAL_DESPAWN,nullptr,0,p);
            if(c){Ship()->RemovePassenger(c);Ship()->AddPassenger(c);c->m_movementInfo.transport.Pos.Relocate(pos);c->m_movementInfo.transport.VehicleSeatIndex=-1;}
        }
        else c=p->SummonCreature(entry,pos,TEMPSUMMON_MANUAL_DESPAWN,0,0,owner);
        if(!c)return nullptr;
        actors.emplace(c->GetGUID(),Actor{kind,slot,false,entry,pos,deck});
        c->SetPhaseMask(p->GetPhaseMask(),false);c->SetPhaseId(p->GetPhases(),false);c->setIgnorePhaseIdCheck(true);c->UpdateObjectVisibility();
        if(kind==Kind::Gun || kind==Kind::Flight || kind==Kind::Guide)
        {c->setFaction(35);c->SetReactState(REACT_PASSIVE);if(kind!=Kind::Guide)c->SetFlag(UNIT_FIELD_NPC_FLAGS,UNIT_NPC_FLAG_SPELLCLICK);}
        else
        {
            c->setFaction(14);c->RemoveFlag(UNIT_FIELD_FLAGS,UNIT_FLAG_NON_ATTACKABLE|UNIT_FLAG_IMMUNE_TO_PC|UNIT_FLAG_NOT_SELECTABLE);
            c->RemoveFlag(UNIT_FIELD_NPC_FLAGS,UNIT_NPC_FLAG_QUESTGIVER|UNIT_NPC_FLAG_GOSSIP);
            c->SetReactState(kind==Kind::Air?REACT_PASSIVE:REACT_AGGRESSIVE);
            if(kind==Kind::Air){c->SetCanFly(true);c->SetDisableGravity(true);c->SetMaxHealth(1);c->SetHealth(1);}
        }
        return c;
    }
    bool AddObject(uint32 entry,Kind kind,uint32 slot,Position local)
    {
        Player* p=Owner();if(!p)return false;
        for(auto const& o:objects)if(o.second.kind==kind && o.second.slot==slot && instance->GetGameObject(o.first))return true;
        Position pos=DeckWorld(local);
        GameObject* go=p->SummonGameObject(entry,pos,0,0,0,0,1,0,owner);
        if(!go)return false;
        go->RemoveFlag(GAMEOBJECT_FIELD_FLAGS,GO_FLAG_NOT_SELECTABLE|GO_FLAG_IN_USE);
        objects.emplace(go->GetGUID(),ObjectAction{kind,slot,local});return true;
    }
    uint32 CaptainIndex() const { for(uint32 i=0;i<3;++i)if(!(state.captains&(1u<<i)))return i;return 3; }
    Position StageDestination() const
    {
        if(state.alliance && state.stage==2)return Boats[CaptainIndex()<3?CaptainIndex():2];
        if(!state.alliance && state.stage==1)return Boats[3];
        if(!state.alliance && state.stage==2)return Boats[4];
        if(!state.alliance && state.stage==3)return Boats[1];
        return DeckWorld();
    }
    bool NeedTravel() const { return !landed && state.stage>= (state.alliance?2u:1u) && !state.final; }
    void TravelActor()
    {
        if(!Owner() || !flight.IsEmpty())return;
        for(auto const& a:actors)if(a.second.kind==Kind::Flight && !a.second.done && (a.second.grace || instance->GetCreature(a.first)))return;
        bool deck=Owner()->GetTransport()==Ship();Position pos=deck?LocalDeck:CheckpointPlace();
        pos.m_positionY+=6;
        uint32 entry=state.alliance?94576:94517;
        if(Creature* c=Spawn(entry,Kind::Flight,0,pos,deck))c->SetFlag(UNIT_FIELD_NPC_FLAGS,UNIT_NPC_FLAG_SPELLCLICK);
    }
    void Build()
    {
        if(state.final || !Owner())return;
        if(NeedTravel())
        {
            if(state.alliance && state.stage==2 && !state.captains)
                AddObject(240210,Kind::Parachute,0,LocalParachute);
            else TravelActor();
            return;
        }
        if(state.stage>=2)
        {
            Position guide=state.alliance && state.stage==2?Boats[CaptainIndex()<3?CaptainIndex():2]:state.stage==(state.alliance?3u:4u)?LocalDeck:StageDestination();
            bool onDeck=state.stage>=(state.alliance?3u:4u);
            if(onDeck)guide=LocalDeck;
            guide.m_positionY+=5;Spawn(state.alliance?93960:94420,Kind::Guide,0,guide,onDeck);
        }
        if(state.alliance && state.stage==0)
        {
            Spawn(92930,Kind::Guide,0,LocalDeck,true);Spawn(92957,Kind::Gun,0,LocalGun,true);
        }
        else if(state.stage==1)
        {
            if(!state.alliance){Position gun=Boats[3];gun.m_positionX+=5;Spawn(94550,Kind::Gun,0,gun);}
            else Spawn(92957,Kind::Gun,0,LocalGun,true);
            if(state.frost==1)
            {
                Position pos=state.alliance?LocalDeck:Boats[3];pos.m_positionX+=6;
                Spawn(state.alliance?93173:94737,Kind::Frost,0,pos,state.alliance);frozen=true;
                for(auto const& a:actors)if(a.second.kind==Kind::Gun)if(Creature* c=instance->GetCreature(a.first))
                {c->RemoveFlag(UNIT_FIELD_NPC_FLAGS,UNIT_NPC_FLAG_SPELLCLICK);c->AddAura(state.alliance?184641:187776,c);}
            }
        }
        else if(state.alliance && state.stage==2)
        {
            uint32 i=CaptainIndex();if(i>=3)return;
            Position center=Boats[i];Spawn(i==0?93560:i==1?93563:93486,Kind::Captain,i,center);
            for(uint32 n=0;n<2;++n){Position guard=center;guard.m_positionY+=n?4:-4;Spawn(i==1?93496:93490,Kind::Guard,n,guard);}
        }
        else if(!state.alliance && (state.stage==2 || state.stage==3))
        {
            Position center=state.stage==2?Boats[4]:Boats[1];groundRequired=state.stage==2?9:8;
            for(uint32 i=0;i<groundRequired;++i){Position pos=center;pos.m_positionX+=float(i%3)*3-3;pos.m_positionY+=float(i/3)*3-3;Spawn(state.stage==2?(i==8?94809:93479):(i%2?93961:93962),Kind::Guard,i,pos);}
        }
        else if(state.stage==(state.alliance?3u:4u))
        {
            for(uint32 i=0;i<7;++i)if(!(state.flares&(1u<<i)))AddObject(state.alliance?242380:243244,Kind::Flare,i,LocalFlares[i]);
        }
        else if(state.stage==(state.alliance?4u:5u))
        {
            Position pos={-26,12,20.65f,0};Spawn(state.alliance?93603:94318,Kind::Final,0,pos,true);
        }
    }
    void Update(uint32 diff) override
    {
        if(timer>diff){timer-=diff;return;}timer=500;
        if(initialized && lifetime.expired())
        {
            Cleanup();initialized=false;layoutStage=99;
            Player* returning=ObjectAccessor::FindPlayer(owner);
            if(Active(returning) && returning->GetMapId()==1475 && returning->GetInstanceId()==instance->GetInstanceId())OnPlayerEnter(returning);
            else return;
        }
        Player* p=Owner();if(!p)return;
        Checkpoint* c=ReadCheckpoint(p);if(!c || c->serial!=serial){OnPlayerEnter(p);return;}
        if(!Ship())
        {
            if(GameObject* go=GetGameObjectByEntry(241630))ship=go->GetGUID();
            else if(Transport* t=sTransportMgr->CreateTransport(241630,0,instance))ship=t->GetGUID();
            if(!Ship())return;Ship()->EnableMovement(false);
        }
        if(reposition)
        {
            p->ExitVehicle();p->SetClientControl(p,true);
            if(p->GetTransport())p->GetTransport()->RemovePassenger(p);
            Position pos=landed?StageDestination():CheckpointPlace();
            bool deck=state.alliance?(state.stage<=1 || state.stage>=4 || (state.stage==2 && !state.captains)):state.stage>=5;
            if(landed && state.stage>=(state.alliance?3u:4u))deck=true;
            if(deck)PutOnDeck(p);else p->NearTeleportTo(pos.GetPositionX(),pos.GetPositionY(),pos.GetPositionZ(),pos.GetOrientation());
            reposition=false;
        }
        if(state.final)return;
        if(state.stage==0 && !state.alliance)
        {
            Scenario* progress=Progress();if(progress && progress->GetCurrentStep()>0){state.stage=1;Remember();}
            return;
        }
        if(layoutStage!=state.stage)
        {
            bool keepGun=state.alliance && layoutStage==0 && state.stage==1 && p->GetVehicleBase();
            if(!keepGun){p->ExitVehicle();p->SetClientControl(p,true);Cleanup();}layoutStage=state.stage;
            landed=(state.stage<=1 && state.alliance) || state.stage==(state.alliance?4u:5u);
            if(state.alliance && state.stage<=1 && !p->GetTransport())PutOnDeck(p);
            if(!keepGun)Build();
        }
        if(NeedTravel()){Build();return;}
        // Failed spawns are retried without credit. A whole wave is accounted
        // for by its required slots, never by the count of successful summons.
        std::vector<Actor> missing;
        for(auto a=actors.begin();a!=actors.end();)
        {
            if(a->second.grace)a->second.grace=a->second.grace>500?a->second.grace-500:0;
            Creature* actor=instance->GetCreature(a->first);
            if(!a->second.done && !a->second.grace && (!actor || !actor->IsAlive()))
            {if(a->second.kind!=Kind::Flight)missing.push_back(a->second);a=actors.erase(a);}
            else ++a;
        }
        for(Actor const& actor:missing)Spawn(actor.entry,actor.kind,actor.slot,actor.position,actor.deck);
        Build();
        if(state.stage==1)
        {
            uint32 live=0;for(auto const& a:actors)if(a.second.kind==Kind::Air && !a.second.done)++live;
            if(live<6 && state.air+live<30)
            {
                Position pos=state.alliance?DeckWorld(LocalGun):Boats[3];
                float angle=pos.GetOrientation()+float(live)*0.12f-0.3f;
                pos.m_positionX+=std::cos(angle)*35;pos.m_positionY+=std::sin(angle)*35;pos.m_positionZ+=12;
                Spawn(state.alliance?93113:93506,Kind::Air,state.air+live,pos);
            }
            if(state.air>=10 && !state.frost)
            {
                Position pos=state.alliance?LocalDeck:Boats[3];pos.m_positionX+=6;
                if(Spawn(state.alliance?93173:94737,Kind::Frost,0,pos,state.alliance))
                {
                    frozen=true;state.frost=1;Remember();p->ExitVehicle();
                    for(auto const& a:actors)if(a.second.kind==Kind::Gun)if(Creature* gun=instance->GetCreature(a.first))
                    {gun->RemoveFlag(UNIT_FIELD_NPC_FLAGS,UNIT_NPC_FLAG_SPELLCLICK);gun->AddAura(state.alliance?184641:187776,gun);}
                }
            }
        }
    }
    bool Interact(Player* p,WorldObject* object,float range=6) const
    {
        return p==Owner() && object && p->InSamePhase(object) && p->canSeeOrDetect(object) && p->IsWithinLOSInMap(object) && p->GetExactDist(object)<=range && !p->isInCombat() && !p->IsBeingTeleported();
    }
    bool Board(Player* p,Creature* c)
    {
        auto a=actors.find(c->GetGUID());if(a==actors.end() || !Interact(p,c,a->second.kind==Kind::Gun?8.0f:6.0f) || a->second.done)return false;
        if(a->second.kind==Kind::Gun)return state.stage<=1 && !frozen;
        return a->second.kind==Kind::Flight && NeedTravel() && flight.IsEmpty();
    }
    void Boarded(Player* p,Creature* c)
    {
        auto a=actors.find(c->GetGUID());if(a==actors.end() || p!=Owner() || c->GetVehicleKit()->GetPassenger(0)!=p)return;
        if(a->second.kind==Kind::Gun)
        {
            if(state.BoardGun()){Remember();Event(44799);}
        }
        else if(a->second.kind==Kind::Flight && NeedTravel())
        {
            flight=c->GetGUID();landing=StageDestination();
            if(c->GetTransport())c->GetTransport()->RemovePassenger(c);
            if(p->GetTransport())p->GetTransport()->RemovePassenger(p);
        }
    }
    void Land(Player* p,Creature* c)
    {
        if(p!=Owner() || flight!=c->GetGUID() || !NeedTravel() || c->GetExactDist(&landing)>3)return;
        p->ExitVehicle();p->SetClientControl(p,true);
        if(state.alliance?state.stage>=3:state.stage>=4)PutOnDeck(p);
        else p->NearTeleportTo(landing.GetPositionX(),landing.GetPositionY(),landing.GetPositionZ(),landing.GetOrientation());
        flight.Clear();Cleanup();landed=true;Build();
    }
    bool Authorized(Unit* attacker) const
    {
        if(!attacker || !Owner())return false;
        auto a=actors.find(attacker->GetGUID());
        if(a!=actors.end())return a->second.kind==Kind::Gun && attacker->GetVehicleKit() && attacker->GetVehicleKit()->GetPassenger(0)==Owner() && !frozen;
        return attacker->GetCharmerOrOwnerPlayerOrPlayerItself()==Owner();
    }
    void OnCreatureDamageTaken(Creature* c,Unit* attacker,uint32& damage) override
    {
        auto a=actors.find(c->GetGUID());if(a==actors.end())return;
        if(a->second.done || !Authorized(attacker)){damage=0;return;}
        Kind kind=a->second.kind;
        if(kind==Kind::Gun || kind==Kind::Flight || kind==Kind::Guide){damage=0;return;}
        if(kind==Kind::Final && c->GetEntry()==94318 && !c->HasAura(189626))
        {
            uint64 health=c->GetHealth(attacker),half=c->GetMaxHealth(attacker)/2;
            if(health<=half || damage>=health-half)
            {
                // Native TRANSFORM aura references creature 45382 (display
                // 72903), followed by the native Greymane's Frenzy chain.
                if(c->CastSpell(c,189626,true)==SPELL_CAST_OK)c->CastSpell(c,189270,true);
            }
        }
        if((kind==Kind::Final || (kind==Kind::Captain && a->second.slot==2)) && ShouldYield(c->GetHealth(attacker),c->GetMaxHealth(attacker),damage))
        {
            damage=0;c->CombatStop(true);c->SetReactState(REACT_PASSIVE);c->setFaction(35);c->SetFlag(UNIT_FIELD_FLAGS,UNIT_FLAG_NON_ATTACKABLE);
            CompleteActor(c,attacker);
        }
    }
    void CompleteActor(Creature* c,Unit* killer)
    {
        auto found=actors.find(c->GetGUID());if(found==actors.end() || found->second.done || !Authorized(killer))return;
        Actor actor=found->second;found->second.done=true;
        if(actor.kind==Kind::Air && state.AirKill()){Remember();if(state.alliance){if(state.air==30)Event(44893);}else Event(45416);}
        else if(actor.kind==Kind::Captain && state.Captain(actor.slot))
        {Remember();Event(actor.slot==0?44892:actor.slot==1?45033:45034);landed=false;layoutStage=99;}
        else if(actor.kind==Kind::Frost)
        {
            frozen=false;state.frost=2;Remember();
            for(auto const& a:actors)if(a.second.kind==Kind::Gun)if(Creature* gun=instance->GetCreature(a.first))
            {gun->RemoveAurasDueToSpell(state.alliance?184641:187776);gun->SetFlag(UNIT_FIELD_NPC_FLAGS,UNIT_NPC_FLAG_SPELLCLICK);}
        }
        else if(actor.kind==Kind::Guard && !state.alliance && (state.stage==2 || state.stage==3))
        {
            if(++groundKills==groundRequired && state.WaveDone()){Remember();Event(state.stage==3?45365:45366);}
        }
        else if(actor.kind==Kind::Final && state.Final()){Remember();Event(state.alliance?45041:45368);}
    }
    void CreatureDies(Creature* c,Unit* killer) override { CompleteActor(c,killer); }
    bool ObjectClick(Player* p,GameObject* go)
    {
        auto a=objects.find(go->GetGUID());if(a==objects.end() || !Interact(p,go,5))return false;
        if(a->second.kind!=Kind::Flare || !state.Flare(a->second.slot))return false;
        Remember();Event(state.alliance?53493:53445);objects.erase(a);go->Delete();return true;
    }
    bool Parachute(Player* p,GameObject* go)
    {
        auto found=objects.find(go->GetGUID());
        if(found==objects.end() || found->second.kind!=Kind::Parachute || !Interact(p,go,5) ||
            !state.alliance || state.stage!=2 || state.captains || !NeedTravel() || p->GetVehicle() || !flight.IsEmpty())return false;
        Position pos=LocalParachute;pos.m_positionZ+=0.3f;
        Creature* body=Spawn(93433,Kind::Flight,0,pos,true);if(!body)return false;
        return p->CastSpell(body,43671,true)==SPELL_CAST_OK;
    }
};
Mission* Get(WorldObject* object)
{
    if(!object || object->GetMapId()!=1475)return nullptr;
    return dynamic_cast<Mission*>(object->GetInstanceScript());
}
}

class instance_stormheim_fleet : public InstanceMapScript
{
public:
    instance_stormheim_fleet():InstanceMapScript("instance_stormheim_fleet",1475){}
    InstanceScript* GetInstanceScript(InstanceMap* map) const override { return new StormheimFleet::Mission(map); }
};

struct npc_stormheim_fleet_actor : SmartAI
{
    explicit npc_stormheim_fleet_actor(Creature* c):SmartAI(c){}
    ObjectGuid rider;
    uint32 point=0,flightTimer=0;
    uint32 attackTimer=3000;
    uint64 flightSerial=0;
    Position destination;
    void Restore(Player* p)
    {
        if(!p)return;
        if(p->GetVehicleBase() && p->GetVehicleBase()!=me)return;
        if(p->GetVehicleBase()==me)p->ExitVehicle();
        p->SetClientControl(p,true);
    }
    void Abort(StormheimFleet::Mission* mission,Player* p)
    {
        rider.Clear();point=flightTimer=0;Restore(p);
        if(mission)
        {
            if(mission->flight==me->GetGUID())mission->flight.Clear();
            mission->actors.erase(me->GetGUID());
        }
        me->DespawnOrUnsummon();
    }
    void PassengerBoarded(Unit* unit,int8 seat,bool apply) override
    {
        auto mission=StormheimFleet::Get(me);
        if(!mission){SmartAI::PassengerBoarded(unit,seat,apply);return;}
        Player* p=unit?unit->ToPlayer():nullptr;
        if(seat!=0 || !p)return;
        if(!apply)
        {
            // Vehicle::RemovePassenger invokes this before Unit::_ExitVehicle
            // clears m_vehicle. Do not recursively request another exit here.
            if(!p->GetVehicleBase() || p->GetVehicleBase()==me)p->SetClientControl(p,true);
            rider.Clear();point=flightTimer=0;
            if(mission->flight==me->GetGUID())
            {
                mission->flight.Clear();mission->actors.erase(me->GetGUID());
                me->DespawnOrUnsummon(1000);
            }
            return;
        }
        mission->Boarded(p,me);
        if(mission->flight!=me->GetGUID())return;
        rider=p->GetGUID();destination=mission->landing;flightSerial=mission->serial;
        me->SetCanFly(true);me->SetDisableGravity(true);me->SetControlled(false,UNIT_STATE_ROOT);
        me->SetSpeed(MOVE_FLIGHT,2.5f,true);me->SetReactState(REACT_PASSIVE);
        p->SetClientControl(me,false); // Seat 0 is boarded before active-mover handoff.
        point=1;flightTimer=120000;
        Position exit=*me;
        exit.m_positionZ+=35;
        me->GetMotionMaster()->MovePoint(147501,exit.GetPositionX(),exit.GetPositionY(),exit.GetPositionZ(),false);
    }
    void MovementInform(uint32 type,uint32 id) override
    {
        if(type!=POINT_MOTION_TYPE || rider.IsEmpty() || id!=147500+point){SmartAI::MovementInform(type,id);return;}
        Player* p=ObjectAccessor::FindPlayer(rider);auto mission=StormheimFleet::Get(me);
        if(!mission || flightSerial!=mission->serial || p!=mission->Owner() || me->GetVehicleKit()->GetPassenger(0)!=p){Abort(mission,p);return;}
        if(point==1){point=2;me->GetMotionMaster()->MovePoint(147502,destination.GetPositionX(),destination.GetPositionY(),destination.GetPositionZ()+35,false);}
        else if(point==2){point=3;me->GetMotionMaster()->MovePoint(147503,destination.GetPositionX(),destination.GetPositionY(),destination.GetPositionZ()+0.5f,false);}
        else
        {
            // Clear our own callback state before ExitVehicle calls us again.
            rider.Clear();point=flightTimer=0;mission->Land(p,me);Restore(p);me->DespawnOrUnsummon();
        }
    }
    void UpdateAI(uint32 diff) override
    {
        auto mission=StormheimFleet::Get(me);
        if(!mission || !mission->actors.count(me->GetGUID())){SmartAI::UpdateAI(diff);return;}
        if(!rider.IsEmpty())
        {
            Player* p=ObjectAccessor::FindPlayer(rider);
            if(flightSerial!=mission->serial || p!=mission->Owner() || flightTimer<=diff)
            {
                Abort(mission,p);return;
            }
            flightTimer-=diff;return;
        }
        auto kind=mission->actors[me->GetGUID()].kind;
        if(kind==StormheimFleet::Kind::Air || kind==StormheimFleet::Kind::Gun || kind==StormheimFleet::Kind::Guide || kind==StormheimFleet::Kind::Flight)return;
        if(!mission->Owner()){me->CombatStop(true);return;}
        if(UpdateVictim())
        {
            if(attackTimer>diff)attackTimer-=diff;
            else
            {
                attackTimer=6000;
                if(SpellInfo const* spell=me->reachWithSpellAttack(me->getVictim()))DoCastVictim(spell->Id);
            }
            DoMeleeAttackIfReady();
        }
    }
};

class spell_stormheim_fleet_board : public SpellScript
{
    PrepareSpellScript(spell_stormheim_fleet_board);
    SpellCastResult Check()
    {
        Creature* target=GetExplTargetUnit()?GetExplTargetUnit()->ToCreature():nullptr;
        auto mission=StormheimFleet::Get(target);if(!mission)return SPELL_CAST_OK;
        return mission->Board(GetCaster()->ToPlayer(),target)?SPELL_CAST_OK:SPELL_FAILED_BAD_TARGETS;
    }
    void Register() override { OnCheckCast+=SpellCheckCastFn(spell_stormheim_fleet_board::Check); }
};

class go_stormheim_fleet_flare : public GameObjectScript
{
public:
    go_stormheim_fleet_flare():GameObjectScript("go_stormheim_fleet_flare"){}
    bool OnGossipHello(Player* p,GameObject* go) override
    {
        auto mission=StormheimFleet::Get(go);if(!mission)return false;
        if(!mission->Interact(p,go,5))return true;
        if(p->IsNonMeleeSpellCast(false))return true;
        StormheimFleet::flareCasts[p->GetGUID()]={go->GetGUID(),mission->serial,++StormheimFleet::nextFlareToken,mission->lifetime};
        if(p->CastSpell(p,186010,false)!=SPELL_CAST_OK)StormheimFleet::flareCasts.erase(p->GetGUID());
        return true;
    }
};
class go_stormheim_fleet_parachute : public GameObjectScript
{
public:
    go_stormheim_fleet_parachute():GameObjectScript("go_stormheim_fleet_parachute"){}
    bool OnGossipHello(Player* p,GameObject* go) override
    {
        auto mission=StormheimFleet::Get(go);if(!mission)return false;
        mission->Parachute(p,go);return true;
    }
};

class spell_stormheim_fleet_flare : public SpellScript
{
    PrepareSpellScript(spell_stormheim_fleet_flare);
    StormheimFleet::FlareCast attempt;
    bool captured=false;
    SpellCastResult Check()
    {
        Player* p=GetCaster()->ToPlayer();auto mission=StormheimFleet::Get(p);if(!mission)return SPELL_CAST_OK;
        auto found=StormheimFleet::flareCasts.find(p->GetGUID());
        if(found==StormheimFleet::flareCasts.end())return SPELL_FAILED_BAD_TARGETS;
        if(!captured){attempt=found->second;captured=true;}
        GameObject* go=ObjectAccessor::GetGameObject(*p,attempt.object);
        return !attempt.lifetime.expired() && mission->serial==attempt.serial && mission->objects.count(attempt.object) && mission->Interact(p,go,5)?SPELL_CAST_OK:SPELL_FAILED_BAD_TARGETS;
    }
    void Complete()
    {
        Player* p=GetCaster()->ToPlayer();auto mission=StormheimFleet::Get(p);
        if(!captured || !mission || attempt.lifetime.expired() || mission->serial!=attempt.serial)return;
        if(GameObject* go=ObjectAccessor::GetGameObject(*p,attempt.object))mission->ObjectClick(p,go);
        auto found=StormheimFleet::flareCasts.find(p->GetGUID());
        if(found!=StormheimFleet::flareCasts.end() && found->second.token==attempt.token)StormheimFleet::flareCasts.erase(found);
    }
    void Register() override { OnCheckCast+=SpellCheckCastFn(spell_stormheim_fleet_flare::Check);AfterCast+=SpellCastFn(spell_stormheim_fleet_flare::Complete); }
};

class spell_stormheim_fleet_finale : public SpellScript
{
    PrepareSpellScript(spell_stormheim_fleet_finale);
    SpellCastResult Check()
    {
        Player* p=GetCaster()->ToPlayer();if(!StormheimFleet::Active(p))return SPELL_FAILED_BAD_TARGETS;
        auto c=StormheimFleet::ReadCheckpoint(p);
        uint32 spell=p->GetTeam()==ALLIANCE?185614:185616;
        return c && c->state.final && GetSpellInfo()->Id==spell?SPELL_CAST_OK:SPELL_FAILED_BAD_TARGETS;
    }
    void Credit(SpellEffIndex index)
    {
        PreventHitDefaultEffect(index); // Native effect 90 runs before teleport acknowledgement.
    }
    void Register() override
    {
        OnCheckCast+=SpellCheckCastFn(spell_stormheim_fleet_finale::Check);
        OnEffectHitTarget+=SpellEffectFn(spell_stormheim_fleet_finale::Credit,EFFECT_2,SPELL_EFFECT_KILL_CREDIT);
    }
};

class player_stormheim_fleet : public PlayerScript
{
public:
    player_stormheim_fleet():PlayerScript("player_stormheim_fleet"){}
    std::map<ObjectGuid,uint32> polls;
    void OnLogin(Player* p) override
    {
        if(p->GetQuestStatus(StormheimFleet::QuestId(p))!=QUEST_STATUS_INCOMPLETE)return;
        if(!StormheimFleet::ReadCheckpoint(p))StormheimFleet::Accepted(p);
        p->AddQuestDelayedEvent(StormheimFleet::QuestId(p),0,[]{});
        StormheimFleet::leases[p->GetGUID()]=p->getQuestStatus(StormheimFleet::QuestId(p))->ScriptLifetime;
    }
    void OnUpdate(Player* p,uint32 diff) override
    {
        uint32& timer=polls[p->GetGUID()];if(timer>diff){timer-=diff;return;}timer=500;
        auto found=StormheimFleet::checkpoints.find(p->GetGUID());if(found==StormheimFleet::checkpoints.end())return;
        auto lease=StormheimFleet::leases.find(p->GetGUID());
        uint32 quest=StormheimFleet::QuestId(p);
        if(lease!=StormheimFleet::leases.end() && lease->second.expired())
        {
            auto stmt=CharacterDatabase.GetPreparedStatement(CHAR_DEL_STORMHEIM_MISSION);
            stmt->setUInt64(0,p->GetGUID().GetCounter());stmt->setUInt32(1,quest);stmt->setUInt64(2,found->second.serial);CharacterDatabase.Execute(stmt);
            StormheimFleet::checkpoints.erase(found);StormheimFleet::leases.erase(lease);StormheimFleet::finaleTimers.erase(p->GetGUID());return;
        }
        if(!StormheimFleet::Active(p) || !found->second.state.final)return;
        Position const& destination=p->GetTeam()==ALLIANCE?StormheimFleet::ArrivalA:StormheimFleet::ArrivalH;
        if(p->GetMapId()==1220 && !p->IsBeingTeleported() && !p->GetVehicle() && !p->GetTransport() && p->GetExactDist(&destination)<12)
        {
            if(Quest const* q=sQuestDataStore->GetQuestTemplate(quest))for(QuestObjective const& objective:q->GetObjectives())
                if(objective.Type==QUEST_OBJECTIVE_MONSTER && objective.ObjectID==113320 && !p->GetQuestObjectiveData(quest,113320))
                {
                    p->SetQuestObjectiveData(q,&objective,1);p->SendQuestUpdateAddCredit(q,ObjectGuid::Empty,objective,1);
                    if(p->CanCompleteQuest(quest))p->CompleteQuest(quest);
                }
            p->RemoveAurasDueToSpell(p->GetTeam()==ALLIANCE?185613:185615);return;
        }
        if(p->GetVehicle())p->ExitVehicle();
        p->SetClientControl(p,true);
        if(p->GetTransport())p->GetTransport()->RemovePassenger(p);
        uint32& elapsed=StormheimFleet::finaleTimers[p->GetGUID()];elapsed+=500;
        uint32 spell=p->GetTeam()==ALLIANCE?185614:185616;
        if(!p->HasAura(spell) && !p->isWatchingMovie() && !p->IsBeingTeleported())p->CastSpell(p,spell,true);
        if(elapsed>=180000)
        {
            // Native film/periodic teleport callback loss is recoverable from
            // the durable earned finale proof, including across login/restart.
            p->setWatchinMovie(false);
            if(!p->IsBeingTeleported())p->TeleportTo(1220,destination.GetPositionX(),destination.GetPositionY(),destination.GetPositionZ(),destination.GetOrientation());
        }
    }
    void OnLogout(Player* p) override
    {
        p->SetClientControl(p,true);
        polls.erase(p->GetGUID());StormheimFleet::checkpoints.erase(p->GetGUID());StormheimFleet::leases.erase(p->GetGUID());StormheimFleet::finaleTimers.erase(p->GetGUID());
        StormheimFleet::flareCasts.erase(p->GetGUID());
    }
};

void AddSC_stormheim_fleet()
{
    new instance_stormheim_fleet();RegisterCreatureAI(npc_stormheim_fleet_actor);
    RegisterSpellScript(spell_stormheim_fleet_board);RegisterSpellScript(spell_stormheim_fleet_finale);
    RegisterSpellScript(spell_stormheim_fleet_flare);
    new go_stormheim_fleet_flare();new go_stormheim_fleet_parachute();new player_stormheim_fleet();
}
