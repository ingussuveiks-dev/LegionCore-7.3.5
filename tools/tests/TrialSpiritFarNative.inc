// Cross-continent spirit resurrection, real far ACK and session-owned logout.
struct SpiritFarSession
{
    WorldSession* session;
    explicit SpiritFarSession(GarmPlayer& player):session(player.GetSession())
    {
        auto& sessions=const_cast<SessionMap&>(sWorld->GetAllSessions());Require(!sessions.count(session->GetAccountId()),"Far spirit fixture would replace another session");
        sessions[session->GetAccountId()]=WorldSessionPtr(session,[](WorldSession*){});session->SetMap(player.GetMap());
    }
    ~SpiritFarSession(){const_cast<SessionMap&>(sWorld->GetAllSessions()).erase(session->GetAccountId());session->SetMap(nullptr);}
};
static void SpiritFarFreeze(uint32 mapId)
{
    auto base=sMapMgr->FindBaseMap(mapId);Require(base,"Far spirit map missing");base->SetMapStop();
    auto& worker=sMapMgr->_mapThreads[mapId];if(worker&&worker->joinable())worker->join();
    if(base->Instanceable()||base->CanCreatedZone())StopGraveyardFixtureZones(*static_cast<MapInstanced*>(base));
}
static void SpiritFarAck(GarmPlayer& player,WorldSafeLocsEntry const& destination)
{
    auto session=player.GetSession();Require(player.IsBeingTeleportedFar()&&!player.IsChangeMap(),"Far spirit ACK input missing");
    session->HandleWorldPortAck();SpiritFarFreeze(destination.MapID);
    // WorldPortAck's initial packets set m_zoneForce. Execute the same native
    // UpdateZone call as the next Player::Update, without ticking health regen.
    // The initial map factory uses source coordinates; the zone callback then
    // places a continent arrival into its actual destination worker.
    player.UpdateZone(player.GetCurrentZoneID(),player.GetCurrentAreaID());SpiritFarFreeze(destination.MapID);
    if(player.IsBeingTeleportedFar()&&player.IsChangeMap())session->m_Functions.Update(500);
    Require(player.IsAlive()&&player.IsInWorld()&&!player.IsBeingTeleported()&&player.GetMapId()==destination.MapID&&player.GetExactDist(destination.Loc.X,destination.Loc.Y,destination.Loc.Z)<0.01f,"Far spirit ACK did not reach corpse graveyard");
    if(player.GetMap()->CanCreatedZone())Require(player.GetInstanceId()==sMapMgr->GetZoneId(destination.MapID,destination.Loc.X,destination.Loc.Y,destination.Loc.Z),"Far spirit arrival retained the source-coordinate zone worker");
    auto health=player.GetHealth();auto map=player.GetMap();session->HandleWorldPortAck();
    Require(player.GetMap()==map&&player.GetHealth()==health&&!player.GetCorpse(),"Repeated far ACK changed spirit arrival");
}
static void TrialSpiritFarCases(GarmPlayer& leader,InstanceMap& raid)
{
    // Reserve additional native maps before their first construction.
    dynamic_cast<VMAP::VMapManager2*>(VMAP::VMapFactory::createOrGetVMapManager())->InitializeThreadUnsafe({{1648,{}},{1220,{}},{0,{}},{1,{}}});
    MMAP::MMapFactory::createOrGetMMapManager()->InitializeThreadUnsafe({{1648,{}},{1220,{}},{0,{}},{1,{}}});
    auto islands=sWorldSafeLocsStore.LookupEntry(5779),oldWorld=sObjectMgr->GetDefaultGraveYard(leader.GetTeam());
    Require(islands&&oldWorld&&islands->MapID==1220&&(oldWorld->MapID==0||oldWorld->MapID==1),"Native cross-continent graveyard anchors missing");
    auto mask=raid.GetInstanceScript()->GetCompletedEncounterMask();
    for(unsigned direction=0;direction<2;++direction)for(unsigned mode=0;mode<3;++mode)
    {
        auto body=direction?oldWorld:islands;auto ghost=direction?islands:oldWorld;
        auto destination=sObjectMgr->GetClosestGraveYard(body->Loc.X,body->Loc.Y,body->Loc.Z,body->MapID,leader.GetTeam());
        auto source=sObjectMgr->GetClosestGraveYard(ghost->Loc.X,ghost->Loc.Y,ghost->Loc.Z,ghost->MapID,leader.GetTeam());
        Require(destination&&source&&source->MapID!=destination->MapID,"Different native continent graveyards missing");
        auto zone=sMapMgr->GetZoneId(destination->MapID,destination->Loc.X,destination->Loc.Y,destination->Loc.Z);
        std::cout<<"SPIRIT_FAR_SELECT race="<<uint32(leader.getRace())<<" direction="<<direction<<" corpse="<<destination->ID<<" destinationMap="<<destination->MapID<<" ghost="<<source->ID<<" sourceMap="<<source->MapID<<" zone="<<zone<<std::endl;
        Require(zone,"Native far spirit destination zone missing");
        auto owned=std::make_unique<GarmPlayer>(*leader.GetMap(),leader.getRace());auto target=owned.get();auto guid=target->GetGUID();auto session=target->GetSession();
        target->*Access(ControlSocialTag{})=sSocialMgr->LoadFromDB({},guid);
        target->SetHomebind(WorldLocation(destination->MapID,destination->Loc.X,destination->Loc.Y,destination->Loc.Z,1.25f),zone);
        SpiritGravePlace(*target,*body);sWorld->AddCharacterInfo(guid,0,target->GetName(),GENDER_MALE,target->getRace(),target->getClass(),target->getLevel());target->SaveToDB(true);FlushWrites();
        target->SetMaxHealth(1000);target->KillPlayer();target->BuildPlayerRepop();Require(target->GetCorpse(),"Far spirit corpse input missing");
        SpiritGravePlace(*target,*source);SpiritFarSession registered(*target);
        Actor healer(target->GetMap(),6491,30000+guid.GetCounter(),target->GetPosition());Require(target->GetMap()->AddToMap(static_cast<Creature*>(&healer)),"Far spirit healer spawn failed");
        Require(target->GetNPCIfCanInteractWith(healer.GetGUID(),UNIT_NPC_FLAG_SPIRITHEALER)==&healer,"Far spirit healer interaction missing");
        target->SaveToDB();FlushWrites();SpiritGraveRequest(session,healer.GetGUID());SpiritFarFreeze(destination->MapID);
        Require(target->IsAlive()&&target->GetHealth()==500&&!target->GetCorpse()&&!target->HasFlag(PLAYER_FIELD_PLAYER_FLAGS,PLAYER_FLAGS_GHOST)&&target->HasAura(15007),"Far spirit resurrection state failed");
        Require(target->IsBeingTeleportedFar()&&!target->IsBeingTeleportedNear()&&!target->IsInWorld()&&target->GetTeleportMap()&&target->GetTeleportDest().GetMapId()==destination->MapID&&target->GetTeleportDest().GetExactDist(destination->Loc.X,destination->Loc.Y,destination->Loc.Z)<0.01f,"Spirit healer did not start actual corpse-graveyard far transfer");
        auto expected=target->GetTeleportDest();target->SetHealth(321);auto sickness=target->GetAura(15007);sickness->SetDuration(598000);SpiritGraveRequest(session,healer.GetGUID());
        Require(target->GetHealth()==321&&sickness->GetDuration()==598000&&target->IsBeingTeleportedFar()&&target->GetTeleportDest().GetExactDist(expected)<0.01f,"Repeated spirit request changed pending far transfer/penalties");
        if(mode==2){
            // Conversion/resurrection can already have queued a source save.
            // Drain it before comparing the later deferred-save request.
            FlushWrites();
            auto before=CharacterDatabase.Query(Trinity::StringFormat("SELECT map,position_x,position_y,position_z,zone,health FROM characters WHERE guid=%u",guid.GetCounter()).c_str());Require(bool(before),"Far live-save baseline missing");
            Require(before->Fetch()[0].GetUInt32()==source->MapID,"Far live-save baseline is not the source map");
            target->SaveToDB();FlushWrites();auto after=CharacterDatabase.Query(Trinity::StringFormat("SELECT map,position_x,position_y,position_z,zone,health FROM characters WHERE guid=%u",guid.GetCounter()).c_str());Require(bool(after),"Far deferred save row missing");auto a=before->Fetch();auto b=after->Fetch();
            Require(a[0].GetUInt32()==b[0].GetUInt32()&&a[1].GetFloat()==b[1].GetFloat()&&a[2].GetFloat()==b[2].GetFloat()&&a[3].GetFloat()==b[3].GetFloat()&&a[4].GetUInt32()==b[4].GetUInt32()&&a[5].GetUInt64()==b[5].GetUInt64(),"Live far save did not defer until admission");
        }
        if(mode)SpiritFarAck(*target,*destination);
        uint32 instance=mode?target->GetInstanceId():0;
        if(mode==2){target->SaveToDB();FlushWrites();}
        else{session->SetMap(nullptr);owned.release();session->SetCanLogout();session->Update(1);Require(!session->GetPlayer()&&!ObjectAccessor::FindPlayer(guid),"Far spirit logout did not delete Player");FlushWrites();}
        auto saved=CharacterDatabase.Query(Trinity::StringFormat("SELECT map,position_x,position_y,position_z,orientation,zone,health,playerFlags,instance_id,online FROM characters WHERE guid=%u",guid.GetCounter()).c_str());Require(bool(saved),"Far spirit character save missing");auto f=saved->Fetch();
        std::cout<<"SPIRIT_FAR_SAVE race="<<uint32(leader.getRace())<<" direction="<<direction<<" mode="<<mode<<" map="<<f[0].GetUInt32()<<" zone="<<f[5].GetUInt32()<<" instance="<<f[8].GetUInt32()<<" health="<<f[6].GetUInt64()<<std::endl;
        Require(f[0].GetUInt32()==expected.GetMapId()&&std::abs(f[1].GetFloat()-expected.GetPositionX())<0.01f&&std::abs(f[2].GetFloat()-expected.GetPositionY())<0.01f&&std::abs(f[3].GetFloat()-expected.GetPositionZ())<0.01f&&std::abs(f[4].GetFloat()-expected.GetOrientation())<0.001f&&f[5].GetUInt32()==zone&&f[6].GetUInt64()==321&&!(f[7].GetUInt32()&PLAYER_FLAGS_GHOST)&&f[8].GetUInt32()==instance&&(mode==2||!f[9].GetBool()),"Far spirit saved destination/zone/alive/health/instance/offline state changed");
        auto corpses=CharacterDatabase.Query(Trinity::StringFormat("SELECT COUNT(*) FROM corpse WHERE guid=%u AND corpseType<>0",guid.GetCounter()).c_str());Require(corpses&&!corpses->Fetch()[0].GetUInt32(),"Far spirit left resurrectable SQL corpse");
        if(mode<2){
            session->HandleWorldPortAck();SpiritGraveRequest(session,healer.GetGUID());Require(!session->GetPlayer(),"Late far ACK/spirit request recreated logged-out Player");
            GarmPlayer fresh(*leader.GetMap(),leader.getRace(),guid.GetCounter());fresh.SetMaxHealth(1000);fresh.SetHealth(f[6].GetUInt64());fresh.SetUInt32Value(PLAYER_FIELD_PLAYER_FLAGS,f[7].GetUInt32());
            auto stmt=CharacterDatabase.GetPreparedStatement(CHAR_SEL_CHARACTER_AURAS);stmt->setUInt64(0,guid.GetCounter());auto auras=ReadPrepared(stmt);stmt=CharacterDatabase.GetPreparedStatement(CHAR_SEL_CHARACTER_AURAS_EFFECTS);stmt->setUInt64(0,guid.GetCounter());(fresh.*Access(ControlAurasLoadTag{}))(auras,ReadPrepared(stmt),0);
            fresh.LoadCorpse();SpiritGravePlace(fresh,*destination);session->HandleWorldPortAck();SpiritGraveRequest(session,healer.GetGUID());
            Require(fresh.IsAlive()&&fresh.GetHealth()==321&&!fresh.GetCorpse()&&!fresh.HasFlag(PLAYER_FIELD_PLAYER_FLAGS,PLAYER_FLAGS_GHOST)&&fresh.HasAura(15007)&&fresh.GetMapId()==destination->MapID&&fresh.GetExactDist(destination->Loc.X,destination->Loc.Y,destination->Loc.Z)<0.01f,"Far spirit saved-field/aura relog or stale old session changed recovery");
            fresh.GetMap()->RemovePlayerFromMap(&fresh,false);
            std::cout<<"PASS: spirit far saved-field/prepared-aura relog diff="<<uint32(raid.GetDifficultyID())<<" race="<<uint32(leader.getRace())<<" direction="<<direction<<" mode="<<mode<<std::endl;
        }
        if(owned)target->GetMap()->RemovePlayerFromMap(target,false);
        Require(raid.GetInstanceScript()->GetCompletedEncounterMask()==mask,"Far spirit transfer changed raid progress");
        std::cout<<"PASS: spirit far transfer/save/logout diff="<<uint32(raid.GetDifficultyID())<<" race="<<uint32(leader.getRace())<<" direction="<<direction<<" mode="<<mode<<std::endl;
    }
}
