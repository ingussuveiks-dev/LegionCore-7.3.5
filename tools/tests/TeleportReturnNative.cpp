// Real linked Player::TeleportTo/SafeTeleport and near acknowledgement.
#define ALTHEA_INTRO_NATIVE_SUPPORT_ONLY
#include "AltheaIntroNative.cpp"
#include "MovementPackets.h"

struct TeleportExpansionTag { using Type=uint8 WorldSession::*; friend Type Access(TeleportExpansionTag); };
template struct FixtureAccess<TeleportExpansionTag,&WorldSession::m_expansion>;
struct TeleportDelayTag { using Type=bool Player::*; friend Type Access(TeleportDelayTag); };
struct TeleportPendingTag { using Type=bool Player::*; friend Type Access(TeleportPendingTag); };
template struct FixtureAccess<TeleportDelayTag,&Player::m_bCanDelayTeleport>;
template struct FixtureAccess<TeleportPendingTag,&Player::m_bHasDelayedTeleport>;

int main(int argc,char** argv)
{
    SetErrorMode(SEM_FAILCRITICALERRORS|SEM_NOGPFAULTERRORBOX);SetUnhandledExceptionFilter(CrashTrace);
    try
    {
        InitializeAltheaGuardsNative(argc,argv);
        Require(sObjectMgr->LoadTrinityStrings(),"Native server strings missing");
        sSpellMgr->LoadSpellProcEvents();sSpellMgr->LoadSpellProcs();sSpellMgr->LoadSpellScene();
        auto safe=const_cast<WorldSafeLocsEntry*>(sWorldSafeLocsStore.LookupEntry(5779));
        Require(safe,"Matching safe location missing");auto original=*safe;
        // Scope one real DB2 record to this controlled instance destination;
        // restore it before exit. Scalar/Position/WorldLocation use same pose.
        Position target{-8321.93f,287.852f,156.916f,0.7101f};
        safe->MapID=1620;safe->Loc.X=target.GetPositionX();safe->Loc.Y=target.GetPositionY();safe->Loc.Z=target.GetPositionZ();safe->Loc.O=target.GetOrientation();
        unsigned cases=0,refusals=0;
        for(auto race:{RACE_HUMAN,RACE_ORC})for(unsigned overload=0;overload<4;++overload)for(unsigned mode=0;mode<4;++mode)
        {
            MapInstanced parent(1620,300);InstanceMap map(1620,300,9997000+cases,DIFFICULTY_N_SCENARIO,&parent);
            map.CreateInstanceData(nullptr);auto session=Session();
            {
                NativePlayer p(session,map,101,{-8311.93f,287.852f,156.916f,0});
                p.SetByteValue(UNIT_FIELD_BYTES_0,0,race);p.setFactionForRace(p.getRace());
                if(mode==0)session->*Access(TeleportExpansionTag{})=0;
                if(mode==2)p.*Access(TeleportDelayTag{})=true;
                // Mode3 takes the real far path but deliberately has no World
                // session registration: SafeTeleport must reject this fixture.
                uint32 options=mode==3?TELE_TO_ZONE_MAP:0;
                bool result=false;
                if(overload==0)result=p.TeleportTo(1620,target.GetPositionX(),target.GetPositionY(),target.GetPositionZ(),target.GetOrientation(),options);
                if(overload==1)result=p.TeleportTo(1620,&target,options,0);
                if(overload==2)result=p.TeleportTo(WorldLocation(1620,target.GetPositionX(),target.GetPositionY(),target.GetPositionZ(),target.GetOrientation()),options);
                if(overload==3)result=p.TeleportTo(5779,options);
                bool refused=mode==0||mode==3;
                if(refused)
                {
#ifdef TELEPORT_RETURN_BASELINE
                    Require(result,"Pinned wrapper no longer reproduces false success");
#else
                    Require(!result,"Rejected native SafeTeleport returned success");
#endif
                    Require(!p.HasPendingTeleport()&&p.GetMap()==&map&&p.IsInWorld(),"Refused native transfer consumed position/world state");
                    Require(p.GetExactDist(target.GetPositionX()+10,target.GetPositionY(),target.GetPositionZ())<0.01f,"Refused transfer moved player");
                    ++refusals;
                }
                else
                {
                    Require(result&&p.HasPendingTeleport(),"Accepted/deferred native transfer returned failure");
                    Require(p.GetTeleportDest().GetMapId()==1620&&p.GetTeleportDest().GetExactDist(&target)<0.01f,"Native pending destination differs");
                    if(mode==2)
                    {
                        Require(p.*Access(TeleportPendingTag{})&&!p.IsBeingTeleportedNear(),"Native delay bypassed scheduling");
                        p.*Access(TeleportDelayTag{})=false;p.*Access(TeleportPendingTag{})=false;
                        Require(p.TeleportTo(1620,&target,options,0),"Deferred native destination could not be dispatched");
                    }
                    WorldPacket wire(CMSG_MOVE_TELEPORT_ACK);wire<<p.GetGUID()<<int32(0)<<int32(0);
                    WorldPackets::Movement::MoveTeleportAck ack(std::move(wire));ack.Read();session->HandleMoveTeleportAck(ack);
                    Require(!p.HasPendingTeleport()&&p.GetExactDist(&target)<0.01f,"Native near ACK failed");
                }
                Require(!p.TeleportTo(0u,0u),"Missing WorldSafeLocs record accepted");
                std::cout<<"CASE race="<<uint32(race)<<" overload="<<overload<<" mode="<<mode<<" result="<<result<<std::endl;
            }
            session->SetPlayer(nullptr);delete session;++cases;
        }
        *safe=original;
#ifdef TELEPORT_RETURN_BASELINE
        std::cout<<"COUNTERFACTUAL expected-broken linked wrapper: "<<refusals<<" genuine native expansion/missing-world-session refusals reported success; not a passing repaired run\n";
#else
        std::cout<<"PASS "<<cases<<" actual linked native wrapper/overload cases: "<<refusals<<" refused, accepted near/ACK and delayed scheduling; both factions. SafeLoc target and clock controlled; no client/far accepted traversal claim\n";
#endif
        CharacterDatabase.Close();LoginDatabase.Close();HotfixDatabase.Close();WorldDatabase.Close();return 0;
    }
    catch(std::exception const& e){std::cerr<<e.what()<<'\n';return 1;}
}
