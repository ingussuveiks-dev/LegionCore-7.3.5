// Actual DB chest lifecycle, real instance victory/save-load and native
// Opening cast -> EffectOpenLock -> SendLoot. No synthetic spawned chest.
#define TRIAL_RAID_INSTANCE_SUPPORT_ONLY
#include "TrialRaidInstanceNative.cpp"
#include "GameObject.h"
#include "GameObjectPackets.h"
#include "ItemEnchantmentMgr.h"

static void LoadChestInputs()
{
    LoadInstanceInputs();LoadRandomEnchantmentsTable();sObjectMgr->LoadPersonalLootTemplate();
    auto& name=LootTemplates_Gameobject.*Access(LootTableNameTag{});auto original=name;name+=" WHERE Entry=70585";
    Require((LootTemplates_Gameobject.*Access(LoadLootTableTag{}))()>0,"Native chest loot loader empty");name=original;
    auto conditions=WorldDatabase.Query("SELECT COUNT(*) FROM conditions WHERE (SourceTypeOrReferenceId=4 AND SourceGroup=70585) OR (SourceTypeOrReferenceId=13 AND SourceEntry IN(3365,6247))");
    Require(conditions&&!conditions->Fetch()[0].GetUInt32(),"Load new native chest/opening conditions before testing");
    auto rows=WorldDatabase.Query("SELECT guid,id,map,zoneId,areaId,spawnMask,phaseMask,PhaseId,position_x,position_y,position_z,orientation,rotation0,rotation1,rotation2,rotation3,spawntimesecs,animprogress,AiID,state,isActive,personal_size FROM gameobject WHERE guid=545038 AND id=260526");
    Require(bool(rows),"Native Odyn DB chest missing");auto f=rows->Fetch();auto& d=sObjectMgr->NewGOData(f[0].GetUInt64());
    d.guid=f[0].GetUInt64();d.id=f[1].GetUInt32();d.mapid=f[2].GetUInt16();d.zoneId=f[3].GetUInt16();d.areaId=f[4].GetUInt16();d.spawnMask=f[5].GetUInt64();d.phaseMask=f[6].GetUInt32();Require(f[7].GetString().empty(),"Load new chest phase IDs before testing");
    d.posX=f[8].GetFloat();d.posY=f[9].GetFloat();d.posZ=f[10].GetFloat();d.orientation=f[11].GetFloat();d.rotation=G3D::Quat(f[12].GetFloat(),f[13].GetFloat(),f[14].GetFloat(),f[15].GetFloat());d.spawntimesecs=f[16].GetInt32();d.animprogress=f[17].GetUInt8();d.AiID=f[18].GetUInt32();d.go_state=GOState(f[19].GetUInt8());d.isActive=f[20].GetBool();d.personalSize=f[21].GetFloat();
    Require(d.spawntimesecs==-1&&d.mapid==1648&&d.spawnMask==245760&&!rows->NextRow(),"Native hidden chest spawn changed");
    auto go=sObjectMgr->GetGameObjectTemplate(260526);Require(go,"Native chest template missing");auto lock=sLockStore.LookupEntry(go->GetLockId());auto opening=sSpellMgr->GetSpellInfo(6247);
    Require(go&&go->GetLootId()==70585&&go->GetLockId()==1634&&lock&&opening,"Native chest/lock/opening catalog missing");
    bool fits=false;for(unsigned i=0;i<MAX_LOCK_CASE;++i){std::cout<<"LOCK slot="<<i<<" type="<<uint32(lock->Type[i])<<" index="<<lock->Index[i]<<" skill="<<lock->Skill[i]<<std::endl;if(lock->Type[i]==LOCK_KEY_SKILL&&lock->Index[i]==uint32(opening->GetEffect(EFFECT_0,DIFFICULTY_NONE)->MiscValue))fits=true;}
    Require(fits&&opening->GetEffect(EFFECT_0,DIFFICULTY_NONE)->Effect==SPELL_EFFECT_OPEN_LOCK,"Opening6247 does not match native chest lock");
}
static GameObject* LoadChest(InstanceMap& map)
{
    auto chest=new GameObject();Require(chest->LoadGameObjectFromDB(545038,&map,true),"Actual DB chest initialization failed");chest->Update(1);
    Require(!chest->isSpawnedByDefault()&&chest->GetDBTableGUIDLow()==545038,"Actual negative-spawn semantics missing");return chest;
}
static void ReportUse(TrialPlayer& p,GameObject& chest)
{
    WorldPacket wire(CMSG_GAME_OBJ_REPORT_USE);wire<<chest.GetGUID();WorldPackets::GameObject::GameObjReportUse packet(std::move(wire));packet.Read();p.GetSession()->HandleGameobjectReportUse(packet);
}
static void Open(TrialPlayer& p,GameObject& chest)
{
    Require(p.CastSpell(&chest,6247,false)==SPELL_CAST_OK,"Native Opening6247 prepare failed");
    Tick(p,10000);
}
static void CheckOpening(InstanceMap& map,GameObject& chest)
{
    for(auto race:{RACE_HUMAN,RACE_ORC})
    {
        TrialPlayer p(map,race);p.Relocate(chest.GetPosition());p.Begin(45088);
        ReportUse(p,chest);Require(!p.personalLoot.count(chest.GetGUID())&&!p.GetItemCount(142487),"Report-use awarded chest loot before opening");
        Require(p.CastSpell(&chest,3365,false)!=SPELL_CAST_OK,"Wrong native lock type accepted");Tick(p,10000);Require(!p.personalLoot.count(chest.GetGUID())&&!p.IsPlayerLootCooldown(260526,TYPE_GO,map.GetDifficultyID()),"Wrong-lock opening awarded loot/cooldown");
        Open(p,chest);auto it=p.personalLoot.find(chest.GetGUID());Require(it!=p.personalLoot.end(),"Actual Opening did not create personal chest loot");
        it->second.AutoStoreItems();auto count=p.GetItemCount(142487);Require(count>=30&&count<=60&&p.IsPlayerLootCooldown(260526,TYPE_GO,map.GetDifficultyID()),"Actual Opening lost fragment reward/cooldown");
        p.GetSession()->DoLootRelease(chest.GetGUID());Open(p,chest);Require(p.GetItemCount(142487)==count,"Repeated Opening duplicated earned fragments");
        std::cout<<"PASS: actual DB chest report-use/wrong-lock/complete/repeated Opening; diff="<<uint32(map.GetDifficultyID())<<" race="<<uint32(race)<<std::endl;
    }
}
static CompletedInstance WinChest(Map& parent,Difficulty diff,bool early)
{
    auto id=++nextGuid;InstanceMap map(1648,300,id,diff,&parent);InstanceSave partial(1648,id,diff,0,"T O V 0 3 0 0 0",GameTime::GetGameTime()+WEEK,false);map.CreateInstanceData(&partial);
    auto odyn=LoadBoss(map,505062);GameObject* chest=nullptr;
    if(early){chest=LoadChest(map);Require(!chest->isSpawned(),"DB chest visible before Odyn victory");}
    auto instance=map.GetInstanceScript();instance->SetData(Data::DataMisc::OdynEventComplete1,DONE);instance->SetData(Data::DataMisc::OdynEventComplete2,DONE);instance->SetBossState(0,IN_PROGRESS);
    odyn->AI()->DoAction(ACTION_1);odyn->AI()->UpdateAI(7000);odyn->CastStop();odyn->AI()->UpdateAI(2000);odyn->CastStop();
    Require(odyn->AI()->GetData(Data::BossIDs::OdynID)==2,"Native Odyn phase2 setup failed");odyn->SetHealth(odyn->CountPctFromMaxHealth(9));uint32 damage=1;odyn->AI()->DamageTakenAfterScaling(odyn,damage,DIRECT_DAMAGE);
    Require(instance->GetBossState(0)==DONE,"Native Odyn surrender did not finish encounter");if(!early)chest=LoadChest(map);
#ifdef TRIAL_CHEST_BASELINE
    Require(chest->isSpawned()==early,"Pinned late chest failure changed");if(early)CheckOpening(map,*chest);
    if(!early)std::cout<<"BASELINE: late DB chest remains hidden after actual Odyn victory"<<std::endl;
#else
    Require(chest->isSpawned(),"Odyn victory lost early/late DB chest availability");CheckOpening(map,*chest);
#endif
    auto data=instance->GetSaveData();map.UnloadAll();FlushWrites();return{id,data};
}
static void SavedChest(Map& parent,Difficulty diff,CompletedInstance const& completed,bool bossFirst)
{
    InstanceMap map(1648,300,completed.id,diff,&parent);InstanceSave save(1648,completed.id,diff,0,completed.data,GameTime::GetGameTime()+WEEK,false);map.CreateInstanceData(&save);
    if(bossFirst)LoadBoss(map,505062);auto chest=LoadChest(map);if(!bossFirst)LoadBoss(map,505062);
#ifdef TRIAL_CHEST_BASELINE
    Require(!chest->isSpawned(),"Pinned saved chest failure changed");std::cout<<"BASELINE: saved-DONE DB chest remains hidden"<<std::endl;
#else
    Require(chest->isSpawned(),"Saved-DONE chest not restored");CheckOpening(map,*chest);
    // A distinct shorter native timer detects an accidental24h reset even
    // when the deterministic game clock has not advanced between callbacks.
    chest->SetRespawnTime(123);auto expiry=chest->GetRespawnTime();chest->RemoveFromWorld();chest->AddToWorld();Require(chest->isSpawned()&&chest->GetRespawnTime()==expiry,"Repeated chest admission reset active reward lifetime");
#endif
    std::cout<<"PASS: actual saved Trial factory/DB chest admission; diff="<<uint32(diff)<<" bossFirst="<<bossFirst<<std::endl;map.UnloadAll();
}
static int RunChest(int argc,char** argv)
{
    InitializeNative(argc,argv);opcodeTable.Initialize();LoadChestInputs();OpenScratch();sAchievementMgr->LoadRewards();sAchievementMgr->LoadCompletedAchievements();
    HashMapHolder<GameObject>::SetSize(20000);
    sScriptMgr->SetScriptContext(sScriptMgr->GetNameOfStaticContext());AddSC_tov_boss_odyn();AddSC_boss_tov_helya();AddSC_trial_of_valor();AddSC_instance_trial_of_valor();sScriptMgr->SwapScriptContext(true);
    dynamic_cast<VMAP::VMapManager2*>(VMAP::VMapFactory::createOrGetVMapManager())->InitializeThreadUnsafe({{1648,{}}});MMAP::MMapFactory::createOrGetMMapManager()->InitializeThreadUnsafe({{1648,{}}});
    {MapInstanced parent(1648,300);for(auto diff:{DIFFICULTY_LFR_RAID,DIFFICULTY_NORMAL_RAID,DIFFICULTY_HEROIC_RAID,DIFFICULTY_MYTHIC_RAID}){auto completed=WinChest(parent,diff,true);WinChest(parent,diff,false);for(bool bossFirst:{false,true})SavedChest(parent,diff,completed,bossFirst);}}
    FlushWrites();CharacterDatabase.Close();LoginDatabase.Close();HotfixDatabase.Close();WorldDatabase.Close();return 0;
}
int main(int argc,char** argv)
{
    SetErrorMode(SEM_FAILCRITICALERRORS|SEM_NOGPFAULTERRORBOX);SetUnhandledExceptionFilter(CrashTrace);
    try{return RunChest(argc,argv);}catch(std::exception const& e){std::cerr<<e.what()<<'\n';return 1;}
}
