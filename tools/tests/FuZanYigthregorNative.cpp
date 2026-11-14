#define FUZAN_YIGTHREGOR_NATIVE
#define FUZAN_DEFENSE_NATIVE_SUPPORT_ONLY
#include "FuZanDefenseNative.cpp"
#include "WaypointManager.h"
#include "AreaTrigger.h"
#include "AreaTriggerData.h"
#include "../../src/server/scripts/Scenario/Artifacts/Monk/TempleOfTheJadeSerpent/fuzan_yigthregor.h"
struct DbSpawnActor : Creature
{
    ~DbSpawnActor()
    {
        CombatStop(true);CastStop();RemoveAllAuras();m_Events.KillAllEvents(true);
        if(IsInWorld())GetMap()->RemoveFromMap(static_cast<Creature*>(this),false);ResetMap();
    }
};
static void CacheActualCorruptedSpawn()
{
    auto row=WorldDatabase.Query("SELECT guid,id,map,zoneId,areaId,spawnMask,phaseMask,PhaseId,modelid,equipment_id,position_x,position_y,position_z,orientation,spawntimesecs,spawndist,currentwaypoint,curhealth,curmana,MovementType,npcflag,unit_flags,unit_flags3,dynamicflags,isActive,skipClone,personal_size,isTeemingSpawn,AiID,MovementID,MeleeID FROM creature WHERE guid=14670030 AND id=109820 AND map=1625");
    Require(row&&row->GetRowCount()==1&&!sObjectMgr->GetCreatureData(14670030),"Actual native cached spawn source missing/occupied");
    auto f=row->Fetch();auto& data=sObjectMgr->NewOrExistCreatureData(14670030);
    data.guid=f[0].GetUInt64();data.id=f[1].GetUInt32();data.mapid=f[2].GetUInt16();data.zoneId=f[3].GetUInt16();data.areaId=f[4].GetUInt16();
    data.spawnMask=f[5].GetUInt64();data.phaseMask=f[6].GetUInt32();
    Tokenizer phases(f[7].GetString(),' ',100);for(auto phase:phases){uint32 id=uint32(strtoul(phase,nullptr,10));Require(bool(sPhaseStore.LookupEntry(id)),"Actual spawn phase absent in native DB2");data.PhaseID.insert(id);}
    data.displayid=f[8].GetUInt32();data.equipmentId=f[9].GetInt8();data.posX=f[10].GetFloat();data.posY=f[11].GetFloat();data.posZ=f[12].GetFloat();data.orientation=f[13].GetFloat();
    data.spawntimesecs=f[14].GetUInt32();data.spawndist=f[15].GetFloat();data.currentwaypoint=f[16].GetUInt32();data.curhealth=f[17].GetUInt64();data.curmana=f[18].GetUInt32();data.movementType=f[19].GetUInt8();
    data.npcflag=f[20].GetUInt32();data.unit_flags=f[21].GetUInt32();data.unit_flags3=f[22].GetUInt32();data.dynamicflags=f[23].GetUInt32();data.isActive=f[24].GetBool();data.skipClone=f[25].GetBool();data.personalSize=f[26].GetFloat();data.isTeemingSpawn=f[27].GetBool();data.AiID=f[28].GetUInt32();data.MovementID=f[29].GetUInt32();data.MeleeID=f[30].GetUInt32();
    // Exactly the World record, explicitly nonpersistent test ownership. Never
    // run global LoadCreatures/clone maintenance or save a respawn/character.
    data.dbData=false;
#ifdef YIGTHREGOR_LIVE
    Require(data.movementType==IDLE_MOTION_TYPE,"Actual live Yigthregor spawn still has invalid waypoint mode");
#else
    Require(data.movementType==WAYPOINT_MOTION_TYPE||data.movementType==IDLE_MOTION_TYPE,"Private movement projection refuses unknown custom mode");
    data.movementType=IDLE_MOTION_TYPE; // Exact staged SQL delta; no persistent write.
#endif
    auto paths=WorldDatabase.Query("SELECT COUNT(*) FROM waypoint_data WHERE id=0");Require(paths&&!paths->Fetch()[0].GetUInt32()&&!sWaypointMgr->GetPath(0),"Unknown real path0 must be loaded first");
    std::cout<<"OBSERVATION actual cached DB movement="<<unsigned(data.movementType)<<" phase6906="<<data.PhaseID.count(6906)<<" pose="<<data.posX<<','<<data.posY<<','<<data.posZ<<'\n';
}

static void ShadowAt(std::string const& table,std::string const& where)
{
    WorldDatabase.DirectExecute(("CREATE TEMPORARY TABLE yig_seed_"+table+" LIKE "+table).c_str());
    WorldDatabase.DirectExecute(("INSERT INTO yig_seed_"+table+" SELECT * FROM "+table+" WHERE "+where).c_str());
    WorldDatabase.DirectExecute(("CREATE TEMPORARY TABLE "+table+" LIKE yig_seed_"+table).c_str());
    WorldDatabase.DirectExecute(("INSERT INTO "+table+" SELECT * FROM yig_seed_"+table).c_str());
}
static void CheckYigthregor(uint8 race,uint32 killer,uint32 number)
{
    DefenseFixture d(race,killer,number);auto& f=d.wave;d.corrupted.reset();
    DbSpawnActor actual;Require(actual.LoadCreatureFromDB(14670030,&f.map),"Native Yigthregor DB load failed");
    Require(actual.GetEntry()==109820&&actual.GetDBTableGUIDLow()==14670030&&actual.GetDefaultMovementType()==IDLE_MOTION_TYPE&&actual.GetWaypointPath()==0&&dynamic_cast<SmartAI*>(actual.AI()),"Actual source/default/SmartAI identity differs");
    float z=actual.GetPositionZ();unsigned fire=0;bool sawBomb=false;
    auto pump=[&](uint32 duration){for(uint32 time=0;time<duration;time+=10){
        d.Pump(10);actual.Update(10);
        std::list<AreaTrigger*> areas;actual.GetAreaObjectList(areas,217787);
        for(auto area:areas){Require(area->GetCasterGUID()==actual.GetGUID()&&area->GetSpellId()==217787,"Native bomb origin changed");sawBomb=true;area->Update(10);}
        fire=std::max(fire,unsigned(areas.size()));f.map.MoveAllCreaturesInMoveList();f.map.RemoveAllObjectsInRemoveList();
    }};
    for(unsigned wave=1;wave<=4;++wave){auto actors=DefenseActors(*f.yulon);actors.remove_if([](Creature* actor){return !actor->IsAlive();});Require(actors.size()==8,"Native defense wave changed");for(auto actor:actors)d.Kill(*actor,true);pump(100);}
    pump(60000);
    f.map.PlayerRelocation(f.player.get(),885.f,-2710.f,192.165f,0.f);f.player->SetMaxHealth(1000000000);f.player->SetHealth(1000000000);
    pump(15000);
    Require(actual.IsAlive()&&std::abs(actual.GetPositionZ()-z)<.02f&&actual.GetMovementTemplate().IsFlightAllowed()&&actual.IsLevitating()&&actual.GetMotionMaster()->GetCurrentMovementGeneratorType()==IDLE_MOTION_TYPE,"Airborne stationary bombardier moved/fell or installed invalid waypoint movement");
    auto ai=dynamic_cast<SmartAI*>(actual.AI());Require(ai->GetScript()->HasRunEvent(0,SMART_EVENT_UPDATE)&&sawBomb&&fire>0,"Native timed217786/217787 attack did not actually create owned7604 areas");
    Require(f.scenario->GetCurrentStep()==6&&BelphiarCounter(f.scenario,49177)==1&&BelphiarCounter(f.scenario,49351)==1,"Native confrontation path/criteria changed");
    f.yulon->CastSpell(f.player.get(),217787,true);std::list<AreaTrigger*> foreign;f.yulon->GetAreaObjectList(foreign,217787);Require(foreign.size()==1,"Foreign caster Felfire control missing");
    actual.AI()->Reset();std::list<AreaTrigger*> afterReset;actual.GetAreaObjectList(afterReset,217787);Require(afterReset.empty(),"Native reset retained old owned Felfire fields");
    pump(10000);actual.GetAreaObjectList(afterReset,217787);Require(!afterReset.empty(),"Reset lost original timed bombardment/retry");
    d.Kill(*d.korithis,true);pump(100);
    Require(!actual.IsAlive()&&BelphiarCounter(f.scenario,49179)==1&&f.scenario->GetCurrentStep()==7,"Real Korithis death/109820 DATA1/1 cleanup or victory receipt changed");
    actual.AI()->SetData(1,1);actual.CastSpell(f.player.get(),217787,true);pump(20000);
    Require(!actual.IsAlive()&&BelphiarCounter(f.scenario,49179)==1,"Repeated cleanup revived actor or repeated victory");
    std::list<AreaTrigger*> remaining;actual.GetAreaObjectList(remaining,217787);Require(remaining.empty(),"Owned Felfire areas did not finish native death/expiry cleanup");
    foreign.clear();f.yulon->GetAreaObjectList(foreign,217787);Require(foreign.size()==1,"Yigthregor cleanup removed another caster's field");f.yulon->RemoveAreaObject(217787);
    std::list<GameObject*> weapons;d.korithis->GetGameObjectListWithEntryInGrid(weapons,251605,100);Require(weapons.size()==1,"Native finale manual source missing/duplicated");
    for(auto weapon:weapons)weapon->Delete();std::list<Creature*> spirits;d.korithis->GetCreatureListWithEntryInGrid(spirits,109432,100);for(auto spirit:spirits)spirit->DespawnOrUnsummon();f.map.RemoveAllObjectsInRemoveList();
    std::cout<<"CASE native Yigthregor race="<<unsigned(race)<<" killer="<<killer<<" bomb_peak="<<fire<<" actual DB movement/attack/Korithis cleanup PASS\n";
}
int main(int argc,char** argv)
{
    SetErrorMode(SEM_FAILCRITICALERRORS|SEM_NOGPFAULTERRORBOX);SetUnhandledExceptionFilter(CrashTrace);
    try{
        InitializeIntro(argc,argv,true,true,true,true,true,true);HashMapHolder<EventObject>::SetSize(20000);HashMapHolder<GameObject>::SetSize(20000);LoadBelphiarLoot();
        auto& lootTable=LootTemplates_Creature.*Access(LootTableNameTag{});auto originalTable=lootTable;lootTable+=" WHERE Entry IN(109395,109397,109438,109933,109821) ORDER BY Entry,Item";
        auto loaded=(LootTemplates_Creature.*Access(LoadLootTableTag{}))();lootTable=originalTable;Require(loaded==12,"Actual defense loot context missing");
        CacheActualCorruptedSpawn();
        WorldDatabase.DirectExecute("SET SESSION TRANSACTION READ WRITE");
        for(auto name:{"areatrigger_template","areatrigger_data","areatrigger_actions"})ShadowAt(name,"entry=7604 OR customEntry=12004");
        for(auto name:{"areatrigger_template_circle","areatrigger_template_sequence","areatrigger_polygon","areatrigger_scripts"})ShadowAt(name,"entry IN(7604,12004)");
        ShadowAt("creature_movement_override","SpawnId=14670030");
#ifdef YIGTHREGOR_LIVE
        auto overrideRow=WorldDatabase.Query("SELECT Ground,Flight FROM creature_movement_override WHERE SpawnId=14670030");Require(overrideRow&&overrideRow->GetRowCount()==1&&overrideRow->Fetch()[0].GetUInt8()==0&&overrideRow->Fetch()[1].GetUInt8()==1,"Actual live per-spawn airborne profile missing");
#else
        WorldDatabase.DirectExecute("INSERT INTO creature_movement_override(SpawnId,Ground,Flight) VALUES(14670030,0,1) ON DUPLICATE KEY UPDATE Ground=VALUES(Ground),Flight=VALUES(Flight)");
#endif
        WorldDatabase.DirectExecute("SET SESSION TRANSACTION READ ONLY");sObjectMgr->LoadCreatureMovementOverrides();
        sObjectAccessor->SetGuidSize(HighGuid::AreaTrigger,100000);sAreaTriggerDataStore->LoadAreaTriggerScripts();sAreaTriggerDataStore->LoadAreaTriggerActionsAndData();
        auto at=sAreaTriggerDataStore->GetAreaTriggerInfo(7604);Require(at&&at->spellId==217787&&at->customEntry==12004&&at->actions.size()==2,"Actual bomb template/actions changed");
        auto actorTemplate=const_cast<CreatureTemplate*>(sObjectMgr->GetCreatureTemplate(109820));auto actorScript=sObjectMgr->GetScriptId("npc_fuzan_yigthregor");
        auto& bindings=sScriptDataStore->*Access(IntroBindingsTag{});auto range=bindings.equal_range(217787);auto bombScript=sObjectMgr->GetScriptId("spell_fuzan_felfire_bomb");
#ifdef YIGTHREGOR_LIVE
        Require(actorTemplate->ScriptID==actorScript&&std::distance(range.first,range.second)==1&&range.first->second.first==bombScript,"Actual live Yigthregor creature/spell bindings missing");
#else
        Require(!actorTemplate->ScriptID&&range.first==range.second,"Pending Yigthregor fixture refuses custom bindings");actorTemplate->ScriptID=actorScript;bindings.emplace(217787,std::make_pair(bombScript,true));
#endif
        sScriptMgr->SetScriptContext(sScriptMgr->GetNameOfStaticContext());RegisterCreatureAI(npc_fuzan_yigthregor);RegisterSpellScript(spell_fuzan_felfire_bomb);RegisterCreatureAI(npc_fuzan_belphiar);RegisterGameObjectAI(go_fuzan_temple_sanctuary_door);sScriptMgr->SwapScriptContext(true);
        uint32 number=7000;for(uint8 race:{uint8(RACE_HUMAN),uint8(RACE_ORC)})for(uint32 killer=0;killer<5;++killer)CheckYigthregor(race,killer,++number);
        CharacterDatabase.Close();LoginDatabase.Close();HotfixDatabase.Close();WorldDatabase.Close();
        std::cout<<"PASS10 actual DB-spawn Yigthregor stationary-air/native bombardment and Korithis finale contexts; actual player/pet/guardian/owned ally killing inputs; no character saves/client motion fidelity claim\n";return 0;
    }catch(std::exception const& error){std::cerr<<error.what()<<'\n';return 1;}
}
