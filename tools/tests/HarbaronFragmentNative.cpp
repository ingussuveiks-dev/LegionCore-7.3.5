// Real native boss/instance/Spell/Aura/summon/kill paths. No client login or persistent save.
#define ROCKAWAY_NATIVE_SUPPORT_ONLY
#include "RockawayOffensiveNative.cpp"
#include "MapInstanced.h"
#include "Pet.h"
#include "GameObject.h"
#include "GameObjectModel.h"
#include "InstanceSaveMgr.h"
#include <filesystem>
#ifdef HARBARON_FRAGMENT_BASELINE
#include "HarbaronBaseline.inc"
#else
#include "../../src/server/scripts/Legion/MawofSouls/boss_harbaron.cpp"
#endif
#include "../../src/server/scripts/Legion/MawofSouls/instance_maw_of_souls.cpp"

static unsigned checked;
static void Check(bool condition,char const* message){Require(condition,message);++checked;}
void AddLegionScripts();
static void InitializeDungeon(int argc,char**argv,bool allDungeons=false)
{
    Require(argc==2,"Expected native dbc path");SetUnhandledExceptionFilter(CrashTrace);
    auto config=std::getenv("LEGION_DIALOGUE_CONFIG");std::string error;
    Require(config&&sConfigMgr->LoadInitial(config,{},error),"Private native configuration unavailable");
    sLog->Initialize(nullptr);MySQL::Library_Init();
    WorldDatabase.SetConnectionInfo(sConfigMgr->GetStringDefault("WorldDatabaseInfo",""),1,1);
    HotfixDatabase.SetConnectionInfo(sConfigMgr->GetStringDefault("HotfixDatabaseInfo",""),1,1);
    Require(!WorldDatabase.Open()&&WorldDatabase.PrepareStatements()&&!HotfixDatabase.Open()&&HotfixDatabase.PrepareStatements(),"Native readonly data pools failed");
    HotfixDatabase.DirectExecute("SET SESSION TRANSACTION READ ONLY");HotfixDatabase.Execute("SET SESSION TRANSACTION READ ONLY");
    auto directory=std::filesystem::path(argv[1]).lexically_normal();if(directory.filename().empty())directory=directory.parent_path();auto runtime=directory.parent_path().parent_path().string()+'/';
    sDB2Manager.LoadStores(runtime,LOCALE_enUS);LoadGameTables(runtime);
    if(allDungeons){
        std::set<uint32> selected;for(auto lfg:sLfgDungeonsStore){auto map=sMapStore.LookupEntry(lfg->MapID);if(map&&map->ExpansionID==6&&map->InstanceType==MAP_INSTANCE&&lfg->MinLevel<=110&&lfg->MaxLevel>=110)selected.insert(map->ID);}
        Require(selected.size()==13,"Review changed native dungeon scope before shadow setup");
        std::string entries;for(auto row:sDungeonEncounterStore)if(selected.count(row->MapID)){if(!entries.empty())entries+=',';entries+=std::to_string(row->ID);}
        Require(!entries.empty(),"Empty native encounter shadow scope");
        WorldDatabase.DirectExecute(("CREATE TEMPORARY TABLE legion_encounters_seed AS SELECT * FROM instance_encounters WHERE entry IN("+entries+")").c_str());
        WorldDatabase.DirectExecute("CREATE TEMPORARY TABLE instance_encounters LIKE legion_encounters_seed");
        WorldDatabase.DirectExecute("INSERT INTO instance_encounters SELECT * FROM legion_encounters_seed");
        auto scoped=WorldDatabase.Query("SELECT COUNT(*),MAX(lastEncounterDungeon) FROM instance_encounters");Require(scoped&&scoped->Fetch()[0].GetUInt32()==66&&!scoped->Fetch()[1].GetUInt32(),"Scoped native encounter shadows unavailable or changed");
    }
    WorldDatabase.DirectExecute("SET SESSION TRANSACTION READ ONLY");WorldDatabase.Execute("SET SESSION TRANSACTION READ ONLY");
    sSpellMgr->LoadSpellInfoStore();sSpellMgr->LoadSpellRanks();sSpellMgr->LoadTalentSpellInfo();sSpellMgr->LoadSkillLineAbilityMap();sSpellMgr->LoadSpellCustomAttr();sSpellMgr->LoadSpellInfoSpellSpecificAndAuraState();
    sObjectMgr->LoadScriptNames();sObjectMgr->LoadCreatureModelInfo();sObjectMgr->LoadWDBCreatureTemplates();sObjectMgr->LoadCreatureTemplates();sObjectMgr->LoadCreatureClassLevelStats();sObjectMgr->LoadCreatureScalingData();sObjectMgr->LoadCreatureDifficultyStat();sObjectMgr->LoadInstanceTemplate();
    sObjectMgr->LoadPageTexts();sObjectMgr->LoadGameObjectTemplate();
    sSpellMgr->LoadSpellTargetPositions();sSpellMgr->LoadSpellLinked();sSpellMgr->LoadSpellTriggered();
    auto conditions=WorldDatabase.Query("SELECT COUNT(*) FROM conditions WHERE SourceTypeOrReferenceId IN(13,17) AND SourceEntry IN(194325,194327,194344,194345,194381,198551)");
    Require(conditions&&!conditions->Fetch()[0].GetUInt32(),"Load changed scoped spell conditions before native test");
    sScriptMgr->SetScriptContext(sScriptMgr->GetNameOfStaticContext());
    if(allDungeons)AddLegionScripts();else{AddSC_boss_harbaron();new instance_maw_of_souls();}
    sScriptMgr->SwapScriptContext(true);
    AIRegistry::Initialize();UnitAI::FillAISpellInfo();sWorld->LoadConfigSettings();
    LoadGameObjectModelList(sWorld->GetDataPath());
    std::unordered_map<uint32,std::vector<uint32>> mapTiles;
    if(allDungeons){for(auto map:sMapStore)if(map->ExpansionID==6&&map->InstanceType==MAP_INSTANCE)mapTiles.emplace(map->ID,std::vector<uint32>{});}else mapTiles.emplace(1492,std::vector<uint32>{});
    dynamic_cast<VMAP::VMapManager2*>(VMAP::VMapFactory::createOrGetVMapManager())->InitializeThreadUnsafe(mapTiles);MMAP::MMapFactory::createOrGetMMapManager()->InitializeThreadUnsafe(mapTiles);
    HashMapHolder<Creature>::SetSize(200000);HashMapHolder<GameObject>::SetSize(200000);HashMapHolder<Player>::SetSize(1024);HashMapHolder<Pet>::SetSize(1024);opcodeTable.Initialize();
    LoginDatabase.SetConnectionInfo(sConfigMgr->GetStringDefault("LoginDatabaseInfo",""),1,1);CharacterDatabase.SetConnectionInfo(sConfigMgr->GetStringDefault("CharacterDatabaseInfo",""),1,1);
    Require(!LoginDatabase.Open()&&LoginDatabase.PrepareStatements()&&!CharacterDatabase.Open()&&CharacterDatabase.PrepareStatements(),"Native session metadata pools failed");
    for(auto sql:{"CREATE TEMPORARY TABLE dungeon_account_seed LIKE account","CREATE TEMPORARY TABLE account LIKE dungeon_account_seed"}){LoginDatabase.DirectExecute(sql);LoginDatabase.Execute(sql);}
    // Native boss kills create bindings. Keep those writes in empty connection-local
    // shadow tables on both native connections; persistent character rows stay untouched.
    for(auto sql:{"CREATE TEMPORARY TABLE dungeon_instance_seed LIKE character_instance","CREATE TEMPORARY TABLE character_instance LIKE dungeon_instance_seed"}){CharacterDatabase.DirectExecute(sql);CharacterDatabase.Execute(sql);}
    LoginDatabase.DirectExecute("SET SESSION TRANSACTION READ ONLY");LoginDatabase.Execute("SET SESSION TRANSACTION READ ONLY");CharacterDatabase.DirectExecute("SET SESSION TRANSACTION READ ONLY");CharacterDatabase.Execute("SET SESSION TRANSACTION READ ONLY");
}
struct DungeonPlayer:Player
{
    DungeonPlayer(Map& map,uint8 race,uint32 low=101):Player(new WorldSession(0,"Isolated dungeon fixture",{},SEC_PLAYER,6,0,"Win",LOCALE_enUS,0,false,AT_AUTH_FLAG_NONE,{}))
    {
        _Create(ObjectGuid::Create<HighGuid::Player>(low));SetByteValue(UNIT_FIELD_BYTES_0,0,race);SetByteValue(UNIT_FIELD_BYTES_0,1,CLASS_HUNTER);SetLevel(110);setFactionForRace(race);auto definition=sChrRacesStore.LookupEntry(race);Require(definition,"Native race missing");SetDisplayId(definition->MaleDisplayID);SetNativeDisplayId(definition->MaleDisplayID);SetObjectScale(1.f);SetFloatValue(UNIT_FIELD_BOUNDING_RADIUS,DEFAULT_WORLD_OBJECT_SIZE);SetFloatValue(UNIT_FIELD_COMBAT_REACH,DEFAULT_COMBAT_REACH);SetMaxHealth(1000000);SetHealth(1000000);
        SetMap(&map);Relocate(2934.49f,711.757f,554.286f,1.55255f);GetZoneAndAreaId(m_zoneId,m_areaId);GetSession()->SetPlayer(this);sWorld->IncreasePlayerCount();sObjectAccessor->AddObject(static_cast<Player*>(this));map.LoadGrid(GetPositionX(),GetPositionY());map.AddToGrid(static_cast<Player*>(this),Cell(Trinity::ComputeCellCoord(GetPositionX(),GetPositionY())));Unit::AddToWorld();GetMotionMaster()->Initialize();GetMapRef().link(&map,this);
    }
    ~DungeonPlayer(){CombatStop(true);CastStop();RemoveAllAuras();m_Events.KillAllEvents(true);UnbindInstance(1492,GetMap()->GetDifficultyID(),true);GetMapRef().unlink();Unit::RemoveFromWorld();RemoveFromGrid();sObjectAccessor->RemoveObject(static_cast<Player*>(this));GetSession()->SetPlayer(nullptr);ResetMap();}
};
struct DungeonShip:GameObject
{
    explicit DungeonShip(Map& map){Position pose{2933.33f,800.f,500.f,3.14159f};Require(Create(180100,246919,&map,1,pose,G3D::Quat(2.5924e-43f,0,0,1),255,GO_STATE_READY),"Actual native ship creation failed");map.LoadGrid(pose.GetPositionX(),pose.GetPositionY());Require(map.AddToMap(static_cast<GameObject*>(this)),"Actual native ship publication failed");Require(m_model&&m_model->isCollisionEnabled(),"Actual native ship collision unavailable");}
    ~DungeonShip(){if(IsInWorld())GetMap()->RemoveFromMap(static_cast<GameObject*>(this),false);ResetMap();}
};
struct DungeonActor:Creature
{
    explicit DungeonActor(Map& map){Require(Create(180001,&map,1,96754,0,ALLIANCE,2934.49f,715.757f,554.286f,1.55255f),"Native boss create failed");SetHomePosition(*this);}
    ~DungeonActor(){CombatStop(true);CastStop();RemoveAllAuras();m_Events.KillAllEvents(true);if(IsInWorld())GetMap()->RemoveFromMap(static_cast<Creature*>(this),false);ResetMap();}
};
struct DungeonPet:Pet
{
    DungeonPet(Player& owner,Map& map):Pet(&owner,HUNTER_PET){Require(Create(200,&map,1,299,1),"Native pet create failed");setPetType(HUNTER_PET);SetOwnerGUID(owner.GetGUID());SetCreatorGUID(owner.GetGUID());Relocate(owner.GetPosition());setFaction(owner.getFaction());SetLevel(110);SetMaxHealth(1000000);SetHealth(1000000);Require(map.AddToMap(static_cast<Creature*>(this)),"Native pet admission failed");}
    void AddToWorld()override{sObjectAccessor->AddObject(static_cast<Pet*>(this));Unit::AddToWorld();Require(AIM_Initialize(new NullCreatureAI(this)),"Native pet AI failed");}
    ~DungeonPet(){CombatStop(true);CastStop();RemoveAllAuras();m_Events.KillAllEvents(true);if(IsInWorld())GetMap()->RemoveFromMap(static_cast<Creature*>(this),false);ResetMap();}
};
struct DungeonGuardian:Guardian
{
    DungeonGuardian(Player& owner,Map& map):Guardian(nullptr,&owner,false){auto pose=owner.GetPosition();Require(Create(201,&map,1,299,0,ALLIANCE,pose.GetPositionX(),pose.GetPositionY(),pose.GetPositionZ(),0),"Native guardian create failed");SetOwnerGUID(owner.GetGUID());SetCreatorGUID(owner.GetGUID());setFaction(owner.getFaction());SetLevel(110);SetMaxHealth(1000000);SetHealth(1000000);Require(map.AddToMap(static_cast<Creature*>(this)),"Native guardian admission failed");}
    void AddToWorld()override{sObjectAccessor->AddObject(static_cast<Creature*>(this));Unit::AddToWorld();Require(AIM_Initialize(new NullCreatureAI(this)),"Native guardian AI failed");}
    ~DungeonGuardian(){CombatStop(true);CastStop();RemoveAllAuras();m_Events.KillAllEvents(true);if(IsInWorld())GetMap()->RemoveFromMap(static_cast<Creature*>(this),false);ResetMap();}
};
static void Pump(DungeonPlayer& player,boss_harbaron& boss,uint32 duration)
{
    for(uint32 elapsed=0;elapsed<duration;elapsed+=10){player.Unit::Update(10);for(auto guid:boss.trashGUID)if(auto actor=player.GetMap()->GetCreature(guid))actor->Update(10);player.GetMap()->MoveAllCreaturesInMoveList();player.GetMap()->RemoveAllObjectsInRemoveList();}
}
#ifndef HARBARON_NATIVE_SUPPORT_ONLY
int main(int argc,char**argv){try{
    InitializeDungeon(argc,argv);unsigned scenario=0;
    for(uint8 race:{uint8(RACE_HUMAN),uint8(RACE_ORC)})for(Difficulty difficulty:{DIFFICULTY_NORMAL,DIFFICULTY_HEROIC,DIFFICULTY_MYTHIC_DUNGEON,DIFFICULTY_MYTHIC_KEYSTONE})for(unsigned killerKind=0;killerKind<3;++killerKind)for(unsigned ending=0;ending<2;++ending)
    {
        MapInstanced parent(1492,300);InstanceMap map(1492,300,9996000+scenario++,difficulty,&parent);map.CreateInstanceData(nullptr);Check(dynamic_cast<instance_maw_of_souls::instance_maw_of_souls_InstanceMapScript*>(map.GetInstanceScript()),"Actual registered Maw instance missing");DungeonShip ship(map);DungeonPlayer player(map,race);DungeonPet pet(player,map);DungeonGuardian guardian(player,map);
        Require(sInstanceSaveMgr->AddInstanceSave(1492,map.GetInstanceId(),difficulty,0,"",0,true,true),"Native ephemeral instance save missing");
        DungeonActor boss(map);Require(map.AddToMap(static_cast<Creature*>(&boss)),"Native boss publication failed");auto ai=dynamic_cast<boss_harbaron*>(boss.AI());Check(ai,"Actual native Harbaron factory missing");
        auto result=boss.CastSpell(&player,194325,true);Pump(player,*ai,100);unsigned expected=(difficulty==23||difficulty==8)?4:2;
#ifdef HARBARON_FRAGMENT_BASELINE
        if(difficulty==8)expected=2;
#endif
        std::cout<<"STATE race="<<unsigned(race)<<" mode="<<unsigned(difficulty)<<" killer="<<killerKind<<" result="<<unsigned(result)<<" fragments="<<ai->trashGUID.size()<<" parent="<<player.HasAura(194327)<<'\n';
        Check(result==SPELL_CAST_OK&&ai->trashGUID.size()==expected&&player.HasAura(194327),"Real Fragment cast/count/native parent aura differs");
        Unit* killer=killerKind==0?static_cast<Unit*>(&player):killerKind==1?static_cast<Unit*>(&pet):static_cast<Unit*>(&guardian);unsigned dead=0;
        for(auto guid:ai->trashGUID){auto fragment=map.GetCreature(guid);std::cout<<"OWNER exists="<<bool(fragment)<<" entry="<<(fragment?fragment->GetEntry():0)<<" owner="<<(fragment&&fragment->GetAnyOwner()==&player)<<" drain="<<player.HasAura(194344,guid)<<" ai="<<(fragment?typeid(*fragment->AI()).name():"missing")<<'\n';Check(fragment&&fragment->GetEntry()==98761&&fragment->GetAnyOwner()==&player&&player.HasAura(194344,guid),"Actual native fragment ownership/per-caster drain missing");killer->Kill(fragment);++dead;Check(!player.HasAura(194344,guid)&&player.HasAura(194327)==(dead<expected),"Actual player/pet/guardian last-fragment cleanup failed");}
        ai->DespawnSouls();Pump(player,*ai,100);Check(!player.HasAura(194344)&&!player.HasAura(194327),"Finished native fragments left target auras");
        ai->trashGUID.clear();Check(boss.CastSpell(&player,194325,true)==SPELL_CAST_OK,"Repeated real Fragment cast failed");Pump(player,*ai,100);Check(ai->trashGUID.size()==expected,"Repeated cast lost difficulty count");if(ending)killer->Kill(&boss);else ai->EnterEvadeMode();Pump(player,*ai,100);
        std::cout<<"CLEANUP drain="<<player.HasAura(194344)<<" parent="<<player.HasAura(194327)<<'\n';
#ifdef HARBARON_FRAGMENT_BASELINE
        Check(player.HasAura(194344)&&player.HasAura(194327),"Expected original encounter-end orphaned fragment auras not reproduced");
#else
        Check(!player.HasAura(194344)&&!player.HasAura(194327),"Native encounter end left fragment damage/parent aura");
#endif
    }
#ifdef HARBARON_FRAGMENT_BASELINE
    std::cout<<"COUNTERFACTUAL "<<checked<<" native original Mythic+2 fragments and encounter-end orphaned auras; expected broken mechanics,not ordinary successful gameplay\n";
#else
    std::cout<<"PASS "<<checked<<" actual native Maw/Harbaron Fragment cases across4 modes,2 races,player/pet/guardian kills; real194325 dummy/194327/98761/194344 and cleanup. Controlled accepted player/actor publication;no network/client/full-dungeon claim\n";
#endif
    CharacterDatabase.Close();LoginDatabase.Close();WorldDatabase.Close();HotfixDatabase.Close();MySQL::Library_End();return 0;
}catch(std::exception const& error){std::cerr<<error.what()<<'\n';return 1;}}
#endif
