// Real corpse/repop and DB entrance720, admission and WorldPortAck. No
// modeled corpse or manual resurrection after the exterior transfer.
#include "TrialGhostInstanceNative.inc"
static void TrialGhostReturn(GarmPlayer& player,GarmPlayer& leader,InstanceMap& raid,Creature& helya,Group& group)
{
    auto corpse=player.GetCorpse();Require(corpse&&corpse->GetMapId()==1648&&corpse->GetInstanceId()==raid.GetInstanceId(),"Ghost return lost original raid corpse");
    auto corpseGuid=corpse->GetGUID();auto instance=raid.GetInstanceId();auto mask=raid.GetInstanceScript()->GetCompletedEncounterMask();auto outdoor=player.GetMap();auto session=player.GetSession();
    // Component reload in the active raid fixture. The separate two-process
    // CorpseRestart test establishes an actually empty process/cache boundary.
    FlushWrites();sObjectAccessor->RemoveCorpse(corpse);delete corpse;
    Require(!player.GetCorpse(),"Unloaded raid corpse remains cached");sObjectMgr->LoadCorpses();corpse=player.GetCorpse();
    Require(corpse&&corpse->GetGUID()==corpseGuid&&corpse->GetMapId()==1648&&corpse->GetInstanceId()==instance,"Startup reload changed raid corpse identity or instance");
    player.LoadCorpse();Require(player.isDead()&&player.GetCorpse()==corpse&&!player.HasFlag(PLAYER_FIELD_LOCAL_FLAGS,PLAYER_LOCAL_FLAG_RELEASE_TIMER),"Raid corpse login component resurrected ghost or enabled outdoor release timer");
    FlushWrites();auto before=CharacterDatabase.Query(Trinity::StringFormat("SELECT COUNT(*) FROM corpse WHERE corpseGuid=%u AND guid=%u",corpseGuid.GetCounter(),player.GetGUIDLow()).c_str());Require(before&&before->Fetch()[0].GetUInt32()==1,"Ghost input has no actual saved corpse row");
    Require(!sMapMgr->CanPlayerEnter(1648,&player,false),"Active raid admitted a returning ghost");
    std::cout<<"GHOST_DENIED alive="<<player.IsAlive()<<" corpse="<<(player.GetCorpse()!=nullptr)<<" ghost="<<player.HasFlag(PLAYER_FIELD_PLAYER_FLAGS,PLAYER_FLAGS_GHOST)<<std::endl;
    Require(player.isDead()&&player.GetCorpse()==corpse&&player.HasFlag(PLAYER_FIELD_PLAYER_FLAGS,PLAYER_FLAGS_GHOST),"Denied raid admission resurrected ghost or consumed corpse");
    // Login access checks also must not mutate the saved ghost/corpse.
    Require(sMapMgr->CanPlayerEnter(1648,&player,true)&&player.isDead()&&player.GetCorpse()==corpse,"Login admission check mutated a ghost");
    ClearCombat(leader,helya);
    TrialGhostOtherInstance(player,raid,group);outdoor=player.GetMap();
    Require(sMapMgr->CanPlayerEnter(1648,&player,false)&&player.isDead()&&player.GetCorpse()==corpse,"Successful preflight resurrected ghost before map admission");
    Require(sMapMgr->CreateMap(1648,&player)==&raid,"Ghost selected a different saved raid instance");
    auto row=WorldDatabase.Query("SELECT guid,id,map,zoneId,areaId,position_x,position_y,position_z,orientation,spawnMask,phaseMask,PhaseId FROM eventobject WHERE guid=410220");Require(bool(row),"Trial entrance DB spawn missing");auto f=row->Fetch();
    auto& data=sEventObjectDataStore->NewOrExistEventObjectData(f[0].GetUInt64());data.guid=f[0].GetUInt64();data.id=f[1].GetUInt32();data.mapid=f[2].GetUInt16();data.zoneId=f[3].GetUInt16();data.areaId=f[4].GetUInt16();data.Pos.Relocate(f[5].GetFloat(),f[6].GetFloat(),f[7].GetFloat(),f[8].GetFloat());data.spawnMask=f[9].GetUInt64();data.phaseMask=f[10].GetUInt32();Require(data.id==720&&data.mapid==1220&&data.phaseMask==1&&f[11].GetString().empty(),"Native Trial entrance data changed");
    auto entrance=new EventObject();Require(entrance->LoadEventObjectFromDB(data.guid,outdoor),"Actual Trial entrance DB initialization failed");outdoor->LoadGrid(data.Pos.GetPositionX(),data.Pos.GetPositionY());outdoor->AddToGrid(entrance,Cell(Trinity::ComputeCellCoord(data.Pos.GetPositionX(),data.Pos.GetPositionY())));entrance->AddToWorld();
    auto dest=sWorldSafeLocsStore.LookupEntry(5775);Require(entrance->eventTemplate&&entrance->eventTemplate->WorldSafeLocID==5775&&dest&&dest->MapID==1648,"Native Trial entrance destination changed");
    outdoor->PlayerRelocation(&player,entrance->GetPositionX()+20,entrance->GetPositionY(),entrance->GetPositionZ(),entrance->GetOrientation());
    {GarmPlayer body(*outdoor,player.getRace());outdoor->PlayerRelocation(&body,entrance->GetPositionX(),entrance->GetPositionY(),entrance->GetPositionZ(),0);body.KillPlayer();Require(body.isDead()&&!body.GetCorpse()&&!body.HasFlag(PLAYER_FIELD_PLAYER_FLAGS,PLAYER_FLAGS_GHOST),"Unreleased body input missing");entrance->Update(1);Require(!entrance->ActivatedForPlayer(&body)&&!body.IsBeingTeleportedFar()&&body.isDead(),"Pure portal activated an unreleased body");}
    outdoor->PlayerRelocation(&player,entrance->GetPositionX(),entrance->GetPositionY(),entrance->GetPositionZ(),entrance->GetOrientation());
    // Synthetic negative template inputs on the actual dispatcher, distinct
    // from the unchanged DB720 template used for the real transfer below.
    auto nativeTemplate=entrance->eventTemplate;EventObjectTemplate spells=*nativeTemplate;spells.SpellID=17;spells.WorldSafeLocID=0;entrance->eventTemplate=&spells;entrance->Update(1);Require(!entrance->ActivatedForPlayer(&player)&&!player.HasAura(17)&&!player.IsBeingTeleportedFar(),"Spell-only event admitted a ghost");
    spells.WorldSafeLocID=5775;entrance->Update(1);Require(!entrance->ActivatedForPlayer(&player)&&!player.HasAura(17)&&!player.IsBeingTeleportedFar(),"Mixed spell/teleport event admitted a ghost");entrance->eventTemplate=nativeTemplate;entrance->Update(1);
    std::cout<<"GHOST_PORTAL far="<<player.IsBeingTeleportedFar()<<" dead="<<player.isDead()<<" affected="<<entrance->ActivatedForPlayer(&player)<<" world="<<player.IsInWorld()<<std::endl;
    Require(player.IsBeingTeleportedFar()&&player.isDead()&&player.GetCorpse()==corpse&&player.GetTeleportMap()==&raid,"Actual entrance did not transfer ghost to its saved instance");
    Require(player.GetTeleportDest().GetExactDist(dest->Loc.X,dest->Loc.Y,dest->Loc.Z)<0.01f,"Ghost entrance destination differs from native5775");
    session->HandleWorldPortAck();Require(player.IsInWorld()&&player.GetMap()==&raid&&!player.IsBeingTeleportedFar()&&player.IsAlive()&&!player.GetCorpse()&&!player.HasFlag(PLAYER_FIELD_PLAYER_FLAGS,PLAYER_FLAGS_GHOST),"Actual raid ACK did not resurrect ghost and convert corpse");
    Require(player.GetHealth()==player.GetMaxHealth()/2&&player.GetExactDist(dest->Loc.X,dest->Loc.Y,dest->Loc.Z)<0.01f,"Ghost return health or native entrance position changed");
    auto health=player.GetHealth();session->HandleWorldPortAck();Require(player.GetHealth()==health&&!player.GetCorpse(),"Repeated ACK resurrected again");
    player.SaveToDB();FlushWrites();auto saved=CharacterDatabase.Query(Trinity::StringFormat("SELECT map,instance_id,health,playerFlags FROM characters WHERE guid=%u",player.GetGUIDLow()).c_str());Require(saved&&saved->Fetch()[0].GetUInt32()==1648&&saved->Fetch()[1].GetUInt32()==instance&&saved->Fetch()[2].GetUInt32()==health&&!(saved->Fetch()[3].GetUInt32()&PLAYER_FLAGS_GHOST),"Recovered Player save lost map/instance/health/ghost state");
    auto bind=group.GetBoundInstance(raid.GetDifficultyID(),1648);Require(bind&&bind->save->GetInstanceId()==instance&&raid.GetInstanceScript()->GetCompletedEncounterMask()==mask,"Ghost return changed earned raid binding/progress");
    auto remaining=CharacterDatabase.Query(Trinity::StringFormat("SELECT COUNT(*) FROM corpse WHERE corpseGuid=%u AND guid=%u",corpseGuid.GetCounter(),player.GetGUIDLow()).c_str());Require(remaining&&!remaining->Fetch()[0].GetUInt32(),"Recovered corpse remained in SQL");
    outdoor->RemoveFromMap(entrance,true);
    std::cout<<"PASS: native Trial ghost denial/login preflight, DB entrance720, original instance, ACK resurrection/corpse conversion/save diff="<<uint32(raid.GetDifficultyID())<<" race="<<uint32(player.getRace())<<std::endl;
}
