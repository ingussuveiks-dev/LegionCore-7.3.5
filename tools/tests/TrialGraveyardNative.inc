#include "Corpse.h"
static void StopGraveyardFixtureZones(MapInstanced& base)
{
    // A real ghost ACK can create another zone worker at the destination.
    // All maps in this disposable process belong to this synchronous fixture.
    for(auto const& entry:base.m_InstancedMaps)entry.second->SetMapStop();
    for(auto& zone:base._zoneThreads)if(zone.second->joinable())zone.second->join();
}
// Unlike ControlTravelSession, this guard must survive actual logout deleting
// its Player. The socketless WorldSession itself remains fixture-owned.
struct GraveyardSession
{
    WorldSession* session;
    GraveyardSession(GarmPlayer& player, InstanceMap& raid):session(player.GetSession())
    {
        auto base=sMapMgr->CreateBaseMap(1220);base->SetMapStop();auto& worker=sMapMgr->_mapThreads[1220];if(worker&&worker->joinable())worker->join();
        auto target=sMapMgr->CreateMap(1220,&player);target->SetMapStop();
        StopGraveyardFixtureZones(*static_cast<MapInstanced*>(base));
        auto& sessions=const_cast<SessionMap&>(sWorld->GetAllSessions());Require(!sessions.count(session->GetAccountId()),"Graveyard fixture would replace another session");
        sessions[session->GetAccountId()]=WorldSessionPtr(session,[](WorldSession*){});session->SetMap(&raid);
    }
    ~GraveyardSession(){const_cast<SessionMap&>(sWorld->GetAllSessions()).erase(session->GetAccountId());session->SetMap(nullptr);}
};
static void TrialGraveyardCases(GarmPlayer& p,InstanceMap& map,Creature& helya,Group& group)
{
    sObjectMgr->LoadGraveyardZones();auto destination=sWorldSafeLocsStore.LookupEntry(5779);auto home=sWorldSafeLocsStore.LookupEntry(5041);Require(destination&&destination->MapID==1220&&home&&home->MapID==1220,"Native raid exterior/fixture homebind missing");
    for(uint32 id:{5775u,5839u,5827u}){
        auto checkpoint=sWorldSafeLocsStore.LookupEntry(id);Require(checkpoint&&checkpoint->MapID==1648,"Native internal checkpoint missing");
        auto selected=sObjectMgr->GetClosestGraveYard(checkpoint->Loc.X,checkpoint->Loc.Y,checkpoint->Loc.Z,1648,p.GetTeam());
        std::cout<<"GRAVE_SELECT team="<<p.GetTeam()<<" checkpoint="<<id<<" selected="<<(selected?selected->ID:0)<<std::endl;
        Require(selected==destination,"Trial of Valor external graveyard missing/wrong");
    }
    auto internal=map.GetInstanceScript()->GetClosestGraveYard(helya.GetPositionX(),helya.GetPositionY(),helya.GetPositionZ());auto helheim=sWorldSafeLocsStore.LookupEntry(5827);
    Require(internal&&internal->GetMapId()==1648&&internal->GetExactDist(helheim->Loc.X,helheim->Loc.Y,helheim->Loc.Z)<0.01f,"Earned internal Helheim checkpoint changed");
    auto staging=sMapMgr->CreateMap(1220,&p);StopGraveyardFixtureZones(*static_cast<MapInstanced*>(sMapMgr->FindBaseMap(1220)));
    for(bool logout:{false,true}){
        Restart(p,map,helya);
        auto owned=std::make_unique<GarmPlayer>(*staging,p.getRace());auto target=owned.get();auto guid=target->GetGUID();auto session=target->GetSession();
        // Login normally initializes these fields. Use a distinct, valid
        // native Valdisdall anchor to expose the existing logout fallback.
        target->SetHomebind(WorldLocation(1220,home->Loc.X,home->Loc.Y,home->Loc.Z,0),7541);
        target->*Access(ControlSocialTag{})=sSocialMgr->LoadFromDB({},guid);
        sWorld->AddCharacterInfo(guid,0,target->GetName(),GENDER_MALE,target->getRace(),target->getClass(),target->getLevel());target->SaveToDB(true);FlushWrites();
        group.ConvertToRaid();Require(group.AddMember(target),"Graveyard native group admission failed");target->SetRaidDifficultyID(map.GetDifficultyID());
        staging->RemovePlayerFromMap(target,false);target->ResetMap();target->SetMap(&map);target->Relocate(helya.GetPosition());Require(map.AddPlayerToMap(target,false),"Graveyard native raid admission failed");
        if(target->HasPendingBind()){WorldPacket wire(CMSG_INSTANCE_LOCK_RESPONSE);wire.WriteBit(true);wire.FlushBits();WorldPackets::Instance::InstanceLockResponse response(std::move(wire));response.Read();session->HandleInstanceLockResponse(response);}
        auto bind=group.GetBoundInstance(map.GetDifficultyID(),1648);Require(bind&&bind->save->GetInstanceId()==map.GetInstanceId()&&!target->HasPendingBind(),"Graveyard group bind/admission missing");
        auto mask=map.GetInstanceScript()->GetCompletedEncounterMask();GraveyardSession registered(*target,map);
        // Real attacker/combat admission, with actor scheduling frozen. Never
        // CombatStop this subject before the actual logout death branch.
        Require(helya.Attack(target,true),"Native Helya attacker setup failed");target->SetInCombatWith(&helya);
        Require(map.GetInstanceScript()->IsEncounterInProgress()&&!target->getAttackers()->empty()&&target->isInCombat(),"Graveyard did not enter active attacked input");
        std::cout<<"GRAVE_INPUT diff="<<uint32(map.GetDifficultyID())<<" race="<<uint32(p.getRace())<<" logout="<<logout<<" attackers="<<target->getAttackers()->size()<<" combat="<<target->isInCombat()<<std::endl;
        if(logout){
            // WorldSession::Update's null-map world dispatcher owns logout.
            session->SetMap(nullptr);owned.release();session->SetCanLogout();session->Update(1);Require(!session->GetPlayer()&&!ObjectAccessor::FindPlayer(guid),"Actual attacked logout did not delete Player");FlushWrites();
            auto corpse=sObjectAccessor->GetCorpseForPlayerGUID(guid);Require(corpse&&corpse->GetMapId()==1648&&corpse->GetInstanceId()==map.GetInstanceId(),"Disconnect death lost its real raid corpse");
            auto row=CharacterDatabase.Query(Trinity::StringFormat("SELECT map,position_x,position_y,position_z,online FROM characters WHERE guid=%u",guid.GetCounter()).c_str());Require(row&&!row->Fetch()[4].GetBool()&&row->Fetch()[0].GetUInt32()==1220&&std::abs(row->Fetch()[1].GetFloat()-home->Loc.X)<0.01f&&std::abs(row->Fetch()[2].GetFloat()-home->Loc.Y)<0.01f&&std::abs(row->Fetch()[3].GetFloat()-home->Loc.Z)<0.01f,"Disconnect death lost native pending-transfer homebind fallback/offline save");
            // Logout has its own existing pending-transfer/homebind fallback;
            // retain the actual saved destination rather than simulating ACK.
            auto f=row->Fetch();std::cout<<"GRAVE_LOGOUT_SAVE map="<<f[0].GetUInt32()<<" pos="<<f[1].GetFloat()<<','<<f[2].GetFloat()<<','<<f[3].GetFloat()<<std::endl;
            sObjectAccessor->ConvertCorpseForPlayer(guid);
        }else{
            target->CombatStop(true);target->KillPlayer();target->BuildPlayerRepop();Require(target->GetCorpse()&&target->isDead(),"Native death/repop did not create a corpse");
            target->RepopAtGraveyard();auto const& loc=target->GetTeleportDest();
            Require(target->IsBeingTeleportedFar()&&loc.GetMapId()==1220&&loc.GetExactDist(destination->Loc.X,destination->Loc.Y,destination->Loc.Z+2)<0.01f&&std::abs(loc.GetOrientation()-destination->Loc.O*float(M_PI)/180.0f)<0.001f,"Actual active-encounter repop missed native exterior");
            session->HandleWorldPortAck();StopGraveyardFixtureZones(*static_cast<MapInstanced*>(sMapMgr->FindBaseMap(1220)));Require(target->IsInWorld()&&target->GetMapId()==1220&&!target->IsBeingTeleportedFar()&&target->isDead(),"Ghost exterior worldport ACK failed");
            target->SaveToDB();FlushWrites();auto row=CharacterDatabase.Query(Trinity::StringFormat("SELECT map,position_x,position_y,position_z FROM characters WHERE guid=%u",guid.GetCounter()).c_str());Require(row&&row->Fetch()[0].GetUInt32()==1220&&std::abs(row->Fetch()[1].GetFloat()-destination->Loc.X)<0.01f&&std::abs(row->Fetch()[2].GetFloat()-destination->Loc.Y)<0.01f,"Ghost save lost exterior destination");
            target->ResurrectPlayer(1,false);target->SpawnCorpseBones();Require(!target->GetCorpse(),"Graveyard corpse cleanup failed");
            target->GetMap()->RemovePlayerFromMap(target,false);
        }
        Require(map.GetInstanceScript()->GetCompletedEncounterMask()==mask,"External repop changed earned raid progress");ClearCombat(p,helya);
        std::cout<<"PASS: native Trial external graveyard/death/repop/logout diff="<<uint32(map.GetDifficultyID())<<" race="<<uint32(p.getRace())<<" logout="<<logout<<std::endl;
    }
}
