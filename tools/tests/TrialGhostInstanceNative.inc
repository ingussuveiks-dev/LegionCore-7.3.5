// The corpse remains in earned instance A. Select an actual empty instance B
// through the native group-bind API; never edit the corpse's instance fields.
struct GhostObjectInstanceTag{using Type=uint32 WorldObject::*;friend Type Access(GhostObjectInstanceTag);};
template struct FixtureAccess<GhostObjectInstanceTag,&WorldObject::m_InstanceId>;
struct GhostMapInstanceTag{using Type=uint32 Map::*;friend Type Access(GhostMapInstanceTag);};
template struct FixtureAccess<GhostMapInstanceTag,&Map::i_InstanceId>;
static void TrialGhostOtherInstance(GarmPlayer& player,InstanceMap& raid,Group& group)
{
    auto corpse=player.GetCorpse();Require(corpse&&corpse->GetInstanceId()==raid.GetInstanceId(),"Other-instance corpse input missing");
    auto corpseGuid=corpse->GetGUID();auto health=player.GetHealth();auto anchor=WorldLocation(player.GetMapId(),player.GetPositionX(),player.GetPositionY(),player.GetPositionZ(),player.GetOrientation());
    auto personal=player.GetBoundInstance(1648,raid.GetDifficultyID());auto personalSave=personal?personal->save:nullptr;bool personalPerm=personal&&personal->perm;
    auto corpseOffset=reinterpret_cast<char*>(&(corpse->*Access(GhostObjectInstanceTag{})))-reinterpret_cast<char*>(corpse);auto playerOffset=reinterpret_cast<char*>(&(player.*Access(GhostObjectInstanceTag{})))-reinterpret_cast<char*>(&player);Require(corpseOffset==playerOffset,"Review native WorldObject instance ABI");
    std::cout<<"GHOST_INSTANCE_ABI object="<<corpseOffset<<" map="<<(reinterpret_cast<char*>(&(raid.*Access(GhostMapInstanceTag{})))-reinterpret_cast<char*>(&raid))<<std::endl;
    GarmPlayer selector(*player.GetMap(),player.getRace());GarmGroup foreignGroup(selector,raid.GetDifficultyID());foreignGroup.group.ConvertToRaid();selector.SetRaidDifficultyID(raid.GetDifficultyID());auto foreign=sMapMgr->CreateMap(1648,&selector)->ToInstanceMap();Require(foreign&&foreign!=&raid,"Other-instance factory failed");
    auto bind=group.GetBoundInstance(raid.GetDifficultyID(),1648);Require(bind&&bind->save,"Original group save missing");auto originalSave=bind->save;bool permanent=bind->perm;
    auto rebind=[&](InstanceSave* save,bool perm){group.UnbindInstance(1648,raid.GetDifficultyID());Require(group.BindToInstance(save,perm),"Native group rebind failed");};
    auto foreignSave=sInstanceSaveMgr->AddInstanceSave(1648,foreign->GetInstanceId(),foreign->GetDifficultyID(),0,foreign->GetInstanceScript()->GetSaveData(),GameTime::GetGameTime()+WEEK,true);
    Require(foreignSave&&foreignGroup.group.BindToInstance(foreignSave,false),"Native replacement group binding failed");rebind(foreignSave,false);Require(sMapMgr->CreateMap(1648,&player)==foreign,"Native group selection did not choose the other instance");
    {GarmPlayer living(*player.GetMap(),player.getRace());Require(foreign->CanEnter(&living),"Empty foreign instance rejected an unrelated living player");living.KillPlayer();Require(!living.GetCorpse()&&foreign->CanEnter(&living),"Foreign corpse guard changed missing-corpse admission");}
    bool admitted=sMapMgr->CanPlayerEnter(1648,&player,false);
    std::cout<<"GHOST_OTHER_PREFLIGHT original="<<raid.GetInstanceId()<<" selected="<<foreign->GetInstanceId()<<" admitted="<<admitted<<" alive="<<player.IsAlive()<<" corpse="<<(player.GetCorpse()!=nullptr)<<std::endl;
    // Retain the existing GM entry bypass to exercise ACK independently of
    // ordinary admission. Even a permitted transfer must not reclaim A in B.
    player.SetGameMaster(true);auto dest=sWorldSafeLocsStore.LookupEntry(5775);Require(dest&&player.SafeTeleport(1648,dest->Loc.X,dest->Loc.Y,dest->Loc.Z,0)&&player.GetTeleportMap()==foreign,"GM foreign-instance transfer failed");player.GetSession()->HandleWorldPortAck();
    std::cout<<"GHOST_OTHER_ACK map="<<player.GetMapId()<<" instance="<<player.GetInstanceId()<<" alive="<<player.IsAlive()<<" corpse="<<(player.GetCorpse()!=nullptr)<<std::endl;
    Require(player.IsInWorld()&&player.GetMap()==foreign&&player.isDead()&&player.GetHealth()==health&&player.GetCorpse()==corpse&&corpse->GetGUID()==corpseGuid&&player.HasFlag(PLAYER_FIELD_PLAYER_FLAGS,PLAYER_FLAGS_GHOST),"Foreign-instance ACK resurrected ghost or consumed original corpse");
    player.GetSession()->HandleWorldPortAck();Require(player.isDead()&&player.GetHealth()==health&&player.GetCorpse()==corpse,"Repeated foreign ACK consumed the original corpse");
    Require(!admitted,"Ordinary admission accepted a corpse from another instance");
    FlushWrites();auto row=CharacterDatabase.Query(Trinity::StringFormat("SELECT corpseGuid,instanceId FROM corpse WHERE guid=%u AND corpseType<>0",player.GetGUIDLow()).c_str());Require(row&&row->GetRowCount()==1&&row->Fetch()[0].GetUInt64()==corpseGuid.GetCounter()&&row->Fetch()[1].GetUInt32()==raid.GetInstanceId(),"Foreign ACK changed persisted original corpse");
    rebind(originalSave,permanent);Require(player.SafeTeleport(anchor),"GM exterior return failed");player.GetSession()->HandleWorldPortAck();StopGraveyardFixtureZones(*static_cast<MapInstanced*>(sMapMgr->FindBaseMap(1220)));player.SetGameMaster(false);
    Require(player.IsInWorld()&&player.GetMapId()==1220&&player.isDead()&&player.GetCorpse()==corpse&&!player.IsBeingTeleportedFar(),"Foreign-instance round trip lost exterior ghost/corpse");
    rebind(foreignSave,false);Require(!player.SafeTeleport(1648,dest->Loc.X,dest->Loc.Y,dest->Loc.Z,0)&&!player.IsBeingTeleportedFar()&&player.GetMapId()==1220&&player.GetCorpse()==corpse,"Ordinary foreign transfer was not rejected before departure");rebind(originalSave,permanent);
    Require(raid.GetInstanceScript()->GetCompletedEncounterMask()==3&&!foreign->GetInstanceScript()->GetCompletedEncounterMask(),"Foreign test changed either instance's encounter progress");
    auto restored=player.GetBoundInstance(1648,raid.GetDifficultyID());Require((restored?restored->save:nullptr)==personalSave&&bool(restored&&restored->perm)==personalPerm,"Foreign transfer changed personal binding");
    // LFR's non-permanent BindToInstance does not persist a checkpoint on
    // its own. Save the existing earned script state through the normal API.
    raid.GetInstanceScript()->SaveToDB();FlushWrites();auto savedBind=CharacterDatabase.Query(Trinity::StringFormat("SELECT instance,completedEncounters FROM group_instance WHERE guid=%u AND map=1648 AND difficulty=%u",group.GetDbStoreId(),raid.GetDifficultyID()).c_str());Require(savedBind&&savedBind->GetRowCount()==1&&savedBind->Fetch()[0].GetUInt32()==raid.GetInstanceId()&&savedBind->Fetch()[1].GetUInt32()==3,"Native rebind failed to restore the unique earned SQL checkpoint");
    std::cout<<"PASS: native different-instance admission, living/missing-corpse controls, GM ACK corpse preservation, SQL identity and original bind restore diff="<<uint32(raid.GetDifficultyID())<<" race="<<uint32(player.getRace())<<std::endl;
}
