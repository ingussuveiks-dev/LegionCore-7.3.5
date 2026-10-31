// Actual Map/InstanceMap, Player destruction, ScriptMgr routing and aura sinks.
#define MASTER_LOOT_NATIVE_SUPPORT_ONLY
#include "MasterLootNative.cpp"
#include "InstanceScript.h"
#include "InstanceSaveMgr.h"

namespace DarkRidersRevil { void RetireFollower(Player* player); }
struct LeaveFrame
{
    ObjectGuid player;
    unsigned instance=0, map=0, destroyed=0;
    bool instanceLive=false, mapLive=false, publishedDestroyed=false;
};
static LeaveFrame* activeLeave;
static unsigned leaveCases;

struct LeavePlayer : LootPlayer
{
    LeaveFrame& frame;
    LeavePlayer(WorldSession* session,InstanceMap& map,uint64 guid,Position const& pos,LeaveFrame& f)
        : LootPlayer(session,map,guid,pos),frame(f) { frame.player=GetGUID(); }
    ~LeavePlayer() { ++frame.destroyed; }
};

struct LeaveInstance : InstanceScript
{
    LeaveFrame& frame;
    LeaveInstance(InstanceMap* map,LeaveFrame& f):InstanceScript(map),frame(f) { }
    void OnPlayerLeave(Player* player) override
    {
        ++frame.instance;
        frame.instanceLive=!frame.destroyed && player->IsInWorld() && player->GetMap()==instance && player->GetGUID()==frame.player;
        Require(frame.instanceLive,"Instance callback lost its original live player/map");
        // Native sinks used by the real Dark Riders callback. Its full scenario
        // admission/owned-summon body is tested independently, not modeled here.
        player->RemoveAurasDueToSpell(198654);
        DarkRidersRevil::RetireFollower(player);
    }
};

struct LeaveMapScript : InstanceMapScript
{
    LeaveMapScript():InstanceMapScript("instance_dark_riders_artifact",1533) { }
    void OnPlayerLeave(InstanceMap* map,Player* player) override
    {
        Require(activeLeave,"Missing isolated native callback observer");
        auto& frame=*activeLeave; ++frame.map;
        frame.publishedDestroyed=frame.destroyed!=0;
        // The baseline intentionally records late pointer publication without
        // dereferencing the already-destroyed Player. Do not manufacture a crash.
        if(!frame.destroyed)
            frame.mapLive=player->IsInWorld() && player->GetMap()==map && player->GetGUID()==frame.player;
    }
};
struct LeaveDataTag {using Type=InstanceScript* InstanceMap::*;friend Type Access(LeaveDataTag);};
template struct FixtureAccess<LeaveDataTag,&InstanceMap::i_data>;

static void CheckLeave(uint8 race,bool destroy,uint64 number)
{
    LeaveFrame frame; activeLeave=&frame;
    Map parent(1533,300,0,DIFFICULTY_N_SCENARIO);
    InstanceMap map(1533,300,uint32(9999900+number),DIFFICULTY_N_SCENARIO,&parent);
    // Real admission normally establishes this in-memory save. Keep the native
    // final-player reset scheduling prerequisite without opening CharacterDB.
    Require(sInstanceSaveMgr->AddInstanceSave(1533,map.GetInstanceId(),DIFFICULTY_N_SCENARIO,0,"",time(nullptr)+7200,true,true),
        "Native instance save admission prerequisite missing");
    map.*Access(LeaveDataTag{})=new LeaveInstance(&map,frame);
    auto session=new WorldSession(0,std::string("Native leave observer"),{},SEC_PLAYER,6,0,"Win",LOCALE_enUS,0,false,AT_AUTH_FLAG_NONE,{});
    auto player=new LeavePlayer(session,map,100+number,{-11036.0f,-1998.0f,94.0f,0.0f},frame);
    player->SetByteValue(UNIT_FIELD_BYTES_0,0,race); player->setFactionForRace(race);
    player->SetByteValue(UNIT_FIELD_BYTES_0,1,CLASS_DEATH_KNIGHT);
    Require(player->GetInstanceScript()==map.GetInstanceScript(),"Native instance lookup prerequisite missing");
    Require(player->AddAura(198654,player) && player->HasAura(198654),"Native ghost aura fixture failed");
    Require(player->IsInWorld(),"Native player not admitted before removal");
    map.RemovePlayerFromMap(player,destroy);
#ifdef INSTANCE_LEAVE_BASELINE
    Require(frame.instance==0,"Pinned old instance callback unexpectedly dispatched");
    Require(frame.map==(destroy?1u:0u),"Pinned ordinary transfer callback omission changed");
    Require(frame.publishedDestroyed==destroy,"Pinned post-destruction pointer publication changed");
#else
    Require(frame.instance==1 && frame.instanceLive && frame.map==1 && frame.mapLive && !frame.publishedDestroyed,
        "Native leave callbacks must run once while original player/map are live");
#endif
    Require(frame.destroyed==(destroy?1u:0u),"Native remove flag changed Player destruction");
    if(!destroy)
    {
#ifndef INSTANCE_LEAVE_BASELINE
        Require(!player->HasAura(198654),"Actual native aura sink did not run on ordinary transfer");
#endif
        Require(!player->IsInWorld() && !player->IsInGrid(),"Native world/grid removal changed");
        delete player;
    }
    session->SetPlayer(nullptr);
    Require(frame.destroyed==1,"Native Player destructor not executed exactly once");
    ++leaveCases;
    std::cout<<"CASE native leave race="<<unsigned(race)<<" remove="<<destroy<<" instance="<<frame.instance
        <<" map="<<frame.map<<" publishedDestroyed="<<frame.publishedDestroyed<<std::endl;
}

static int LeaveRun(int argc,char** argv)
{
    InitializeNative(argc,argv);opcodeTable.Initialize();LoadLootInputs();
    LoginDatabase.SetConnectionInfo(sConfigMgr->GetStringDefault("LoginDatabaseInfo",""),1,1);
    Require(!LoginDatabase.Open()&&LoginDatabase.PrepareStatements(),"Native login pool failed");
    LoginDatabase.DirectExecute("SET SESSION TRANSACTION READ ONLY");LoginDatabase.Execute("SET SESSION TRANSACTION READ ONLY");
    auto ro=LoginDatabase.Query("SELECT @@session.transaction_read_only");Require(ro&&ro->Fetch()[0].GetBool(),"Login is not read-only");
    sScriptMgr->SetScriptContext(sScriptMgr->GetNameOfStaticContext());new LeaveMapScript;sScriptMgr->SwapScriptContext(true);
    HashMapHolder<Player>::SetSize(32);
    dynamic_cast<VMAP::VMapManager2*>(VMAP::VMapFactory::createOrGetVMapManager())->InitializeThreadUnsafe({{1533,{}}});
    MMAP::MMapFactory::createOrGetMMapManager()->InitializeThreadUnsafe({{1533,{}}});
    uint64 number=0;
    for(uint8 race:{uint8(RACE_HUMAN),uint8(RACE_ORC)}) for(bool destroy:{false,true}) CheckLeave(race,destroy,++number);
    activeLeave=nullptr;
    LoginDatabase.Close();HotfixDatabase.Close();WorldDatabase.Close();
#ifdef INSTANCE_LEAVE_BASELINE
    std::cout<<"EXPECTED_BROKEN: "<<leaveCases<<" actual native InstanceScript callback omissions, ordinary transfer MapScript omission and post-Player-destruction callback pointer publication; no dangling pointer dereferenced by this observer.\n";
#else
    std::cout<<"PASS: "<<leaveCases<<" actual native removal/Player destruction/instance+MapScript routing and ghost aura sinks, both factions and remove flags. Full script effects/client traversal are separate checks.\n";
#endif
    return 0;
}
int main(int argc,char** argv)
{
    SetErrorMode(SEM_FAILCRITICALERRORS|SEM_NOGPFAULTERRORBOX);SetUnhandledExceptionFilter(CrashTrace);
    try{return LeaveRun(argc,argv);}catch(std::exception const& e){std::cerr<<e.what()<<'\n';return 1;}
}
