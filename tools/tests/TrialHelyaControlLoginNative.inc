#include "SocialMgr.h"
#include "InstancePackets.h"
struct ControlSocialTag{using Type=PlayerSocial* Player::*;friend Type Access(ControlSocialTag);};
template struct FixtureAccess<ControlSocialTag,&Player::m_social>;
struct ControlAurasLoadTag{using Type=void(Player::*)(PreparedQueryResult,PreparedQueryResult,uint32);friend Type Access(ControlAurasLoadTag);};
template struct FixtureAccess<ControlAurasLoadTag,&Player::_LoadAuras>;
struct ControlGroupLoadTag{using Type=void(Player::*)(PreparedQueryResult);friend Type Access(ControlGroupLoadTag);};
template struct FixtureAccess<ControlGroupLoadTag,&Player::_LoadGroup>;
struct ControlBindLoadTag{using Type=void(Player::*)(PreparedQueryResult);friend Type Access(ControlBindLoadTag);};
template struct FixtureAccess<ControlBindLoadTag,&Player::_LoadBoundInstances>;
struct ControlLoadingTag{using Type=ObjectGuid WorldSession::*;friend Type Access(ControlLoadingTag);};
template struct FixtureAccess<ControlLoadingTag,&WorldSession::m_playerLoading>;
static void ControlLoginCases(GarmPlayer& p,InstanceMap& map,Creature& helya,Group& group)
{
    static bool gravesLoaded=false;if(!gravesLoaded){sObjectMgr->LoadGraveyardZones();gravesLoaded=true;}
    auto stagingMap=sMapMgr->CreateMap(1220,&p);stagingMap->SetMapStop();for(auto& zone:static_cast<MapInstanced*>(sMapMgr->FindBaseMap(1220))->_zoneThreads)if(zone.second->joinable())zone.second->join();Map& staging=*stagingMap;
    for(unsigned mode=0;mode<4;++mode){
    bool pending=mode==1||mode==2,legacy=mode==3;uint32 offline=mode==2?20:0;
    auto owned=std::make_unique<GarmPlayer>(staging,p.getRace());auto target=owned.get();auto guid=target->GetGUID();auto session=target->GetSession();
    target->*Access(ControlSocialTag{})=sSocialMgr->LoadFromDB({},guid);
    sWorld->AddCharacterInfo(guid,0,target->GetName(),GENDER_MALE,target->getRace(),target->getClass(),target->getLevel());
    target->SaveToDB(true);FlushWrites();group.ConvertToRaid();Require(group.AddMember(target),"Native logout fixture group admission failed");target->SetRaidDifficultyID(map.GetDifficultyID());
    staging.RemovePlayerFromMap(target,false);target->ResetMap();target->SetMap(&map);target->Relocate(helya.GetPosition());Require(map.AddPlayerToMap(target,false),"Native logout fixture raid admission failed");
    if(target->HasPendingBind()){
        WorldPacket wire(CMSG_INSTANCE_LOCK_RESPONSE);wire.WriteBit(true);wire.FlushBits();WorldPackets::Instance::InstanceLockResponse response(std::move(wire));response.Read();session->HandleInstanceLockResponse(response);
    }
    auto bind=target->GetBoundInstance(1648,map.GetDifficultyID());Require(bind&&bind->perm&&bind->save->GetInstanceId()==map.GetInstanceId()&&!target->HasPendingBind(),"Native lock acceptance failed");
    if(pending){
        // Start with an ordinary ranged raid member, not an artificial
        // charm/release cycle that makes Helya auto-attack the former puppet.
        Restart(p,map,helya);map.PlayerRelocation(target,helya.GetPositionX()+35,helya.GetPositionY(),helya.GetPositionZ(),0);target->SetHealth(target->GetMaxHealth());EnterPhaseThree(p,map,helya);helya.CastSpell(target,232450,true);Require(target->HasAura(232450),"Logout pending-Axion input missing");
    }else ControlBegin(p,*target,map,helya);
    target->RemoveAura(228054);helya.SetPower(POWER_ALTERNATE,10);ControlWatch before(map,*target,helya);before.Advance(1500);Require(target->HasUnitState(UNIT_STATE_CASTING)!=pending,"Logout missed real Infusion/pending absorb");
    // Isolate persistence for a living absorb target. A disconnect with a
    // melee attacker instead executes the separate death/repop path, whose
    // missing outdoor graveyard link is retained in the audit. Do not stop
    // combat/casting on the controlled cases: their live Infusion must reach
    // the actual logout dispatcher unchanged.
    if(pending)target->CombatStop();
    Require(target->getAttackers()->empty()&&(!pending||target->HasAura(232450)),"Alive-aura logout fixture lost isolation/absorb");
    for(uint32 id:{232350u,232358u,231862u,232450u})if(auto aura=target->GetAura(id))std::cout<<"CONTROL_SAVE_INPUT spell="<<id<<" savable="<<aura->CanBeSaved()<<" duration="<<aura->GetDuration()<<std::endl;
    Require(!target->GetAura(pending?232450:231862)->CanBeSaved(),"Encounter aura unexpectedly became persistable");
    std::cout<<"CONTROL_LOGOUT mode="<<mode<<" attackers="<<target->getAttackers()->size()<<" combat="<<target->isInCombat()<<" pendingBind="<<target->HasPendingBind()<<std::endl;
    target->SaveToDB();FlushWrites();owned.release();session->SetCanLogout();session->Update(1);
    Require(!session->GetPlayer()&&!ObjectAccessor::FindPlayer(guid),"Actual logout did not delete Player/session lookup");FlushWrites();
    Require(helya.GetPower(POWER_ALTERNATE)==10,"Logout completed an old Infusion before encounter cleanup");
    auto counts=CharacterDatabase.Query(Trinity::StringFormat("SELECT COALESCE(SUM(spell IN(232350,232358,231862)),0),COALESCE(SUM(spell=232450),0) FROM character_aura WHERE guid=%u",guid.GetCounter()).c_str());Require(counts&&counts->Fetch()[0].GetUInt32()==0&&counts->Fetch()[1].GetUInt32()==0,"Actual save/logout persisted an encounter control/absorb aura");
    if(legacy){
        // Explicit old-version input, separate from current native save:
        // the reference saved a permanent231862 row with its native effect0.
        auto trans=CharacterDatabase.BeginTransaction();auto stmt=CharacterDatabase.GetPreparedStatement(CHAR_INS_AURA);stmt->setUInt64(0,guid.GetCounter());stmt->setUInt8(1,0);stmt->setBinary(2,helya.GetGUID().GetRawValue());stmt->setBinary(3,ObjectGuid::Empty.GetRawValue());stmt->setUInt32(4,231862);stmt->setUInt16(5,1);stmt->setUInt8(6,0);stmt->setUInt8(7,1);stmt->setInt32(8,-1);stmt->setInt32(9,-1);stmt->setUInt8(10,0);trans->Append(stmt);
        stmt=CharacterDatabase.GetPreparedStatement(CHAR_INS_AURA_EFFECT);stmt->setUInt64(0,guid.GetCounter());stmt->setUInt8(1,0);stmt->setUInt8(2,0);stmt->setInt32(3,0);stmt->setInt32(4,0);trans->Append(stmt);CharacterDatabase.CommitTransaction(trans);FlushWrites();
    }
    auto saved=CharacterDatabase.Query(Trinity::StringFormat("SELECT map,position_x,position_y,position_z,orientation,online FROM characters WHERE guid=%u",guid.GetCounter()).c_str());Require(saved&&saved->Fetch()[0].GetUInt32()==1648&&!saved->Fetch()[5].GetBool(),"Logout did not save offline raid location");
    GarmPlayer fresh(staging,p.getRace(),guid.GetCounter());fresh.GetSession()->*Access(ControlLoadingTag{})=guid;staging.RemovePlayerFromMap(&fresh,false);fresh.ResetMap();
    auto stmt=CharacterDatabase.GetPreparedStatement(CHAR_SEL_GROUP_MEMBER);stmt->setUInt64(0,guid.GetCounter());(fresh.*Access(ControlGroupLoadTag{}))(ReadPrepared(stmt));Require(fresh.GetGroup()==&group,"Prepared group reload lost persistent membership");
    stmt=CharacterDatabase.GetPreparedStatement(CHAR_SEL_CHARACTER_INSTANCE);stmt->setUInt64(0,guid.GetCounter());(fresh.*Access(ControlBindLoadTag{}))(ReadPrepared(stmt));
    Require(sMapMgr->CanPlayerEnter(1648,&fresh,true),"Login access check rejected retained raid member");auto selected=sMapMgr->CreateMap(1648,&fresh);Require(selected==&map,"Prepared bind reload selected another raid");fresh.SetMap(selected);auto fields=saved->Fetch();fresh.Relocate(fields[1].GetFloat(),fields[2].GetFloat(),fields[3].GetFloat(),fields[4].GetFloat());
    stmt=CharacterDatabase.GetPreparedStatement(CHAR_SEL_CHARACTER_AURAS);stmt->setUInt64(0,guid.GetCounter());auto auras=ReadPrepared(stmt);stmt=CharacterDatabase.GetPreparedStatement(CHAR_SEL_CHARACTER_AURAS_EFFECTS);stmt->setUInt64(0,guid.GetCounter());
    if(legacy)Require(auras&&auras->GetRowCount()==1,"Legacy rejection did not receive its explicit saved aura row");
    (fresh.*Access(ControlAurasLoadTag{}))(auras,ReadPrepared(stmt),offline);
    std::cout<<"CONTROL_LOGIN_LOAD mode="<<mode<<" offline="<<offline<<" control="<<fresh.HasAura(232350)<<" transform="<<fresh.HasAura(232358)<<" bleak="<<fresh.HasAura(231862)<<" axion="<<fresh.HasAura(232450)<<" charmer="<<fresh.GetCharmerGUID().GetCounter()<<" casting="<<fresh.HasUnitState(UNIT_STATE_CASTING)<<std::endl;
    Require(!fresh.HasAura(232350)&&!fresh.HasAura(232358)&&!fresh.HasAura(231862)&&fresh.GetCharmerGUID().IsEmpty()&&!fresh.HasUnitState(UNIT_STATE_CASTING),"Prepared aura reload restored stale Corrupted state");
    Require(!fresh.HasAura(232450),"Unpersisted Axion reappeared at login");
    Require(helya.GetPower(POWER_ALTERNATE)==10,"Aura reload energized Helya before the wipe reset");
    if(legacy){fresh.SaveToDB();FlushWrites();auto old=CharacterDatabase.Query(Trinity::StringFormat("SELECT COUNT(*) FROM character_aura WHERE guid=%u AND spell=231862",guid.GetCounter()).c_str());Require(old&&!old->Fetch()[0].GetUInt32(),"Next native save retained rejected legacy aura row");}
    ClearCombat(p,helya);Require(map.AddPlayerToMap(&fresh,false),"Reloaded Player raid admission failed");fresh.GetSession()->*Access(ControlLoadingTag{})=ObjectGuid::Empty;auto energy=helya.GetPower(POWER_ALTERNATE);ControlWatch after(map,fresh,helya);after.Advance(15000,true);
    bind=fresh.GetBoundInstance(1648,map.GetDifficultyID());Require(bind&&bind->perm&&bind->save->GetInstanceId()==map.GetInstanceId()&&map.GetInstanceScript()->GetCompletedEncounterMask()==3&&after.casts[231863].empty()&&helya.GetPower(POWER_ALTERNATE)==energy,"Reloaded Player lost raid bind/progress or completed old cast");
    std::cout<<"CONTROL_LOGIN_PASS race="<<uint32(p.getRace())<<" mode="<<mode<<" instance="<<map.GetInstanceId()<<" mask="<<map.GetInstanceScript()->GetCompletedEncounterMask()<<" bursts="<<after.casts[231863].size()<<std::endl;
    }
    std::cout<<"PASS: actual Corrupted/Axion logout/save, prepared aura/group/bind reload, short/long offline absence and legacy rejection; race="<<uint32(p.getRace())<<std::endl;
}
