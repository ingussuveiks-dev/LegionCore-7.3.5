// Actual prepared bind reload -> MapManager selection -> instance factory.
// Socket authentication and automatic map workers are outside this probe.
#define TRIAL_RAID_INSTANCE_SUPPORT_ONLY
#include "TrialRaidInstanceNative.cpp"
#include "MapManager.h"
#include "CalendarPackets.h"

struct BindLoadTag {using Type=void(Player::*)(PreparedQueryResult);friend Type Access(BindLoadTag);};
template struct FixtureAccess<BindLoadTag,&Player::_LoadBoundInstances>;
struct LfgDungeonTag {using Type=void(lfg::LFGMgr::*)(ObjectGuid,uint32);friend Type Access(LfgDungeonTag);};
template struct FixtureAccess<LfgDungeonTag,&lfg::LFGMgr::SetDungeon>;
struct LfgStateTag {using Type=void(lfg::LFGMgr::*)(ObjectGuid,lfg::LfgState,uint32);friend Type Access(LfgStateTag);};
template struct FixtureAccess<LfgStateTag,&lfg::LFGMgr::SetState>;
struct LfgSaveTag {using Type=void(lfg::LFGMgr::*)(ObjectGuid,uint32);friend Type Access(LfgSaveTag);};
template struct FixtureAccess<LfgSaveTag,&lfg::LFGMgr::_SaveToDB>;
static Difficulty const modes[]={DIFFICULTY_NORMAL_RAID,DIFFICULTY_HEROIC_RAID,DIFFICULTY_MYTHIC_RAID,DIFFICULTY_LFR_RAID};
struct BoundPlayer:TrialPlayer
{
    using TrialPlayer::TrialPlayer;
    ~BoundPlayer()
    {
        // Native unload cleanup preserves the SQL checkpoint for the next
        // fresh Player while detaching the save manager's live references.
        for(auto diff:modes)while(!GetBoundInstances(diff).empty())
        {auto bind=GetBoundInstances(diff).begin();UnbindInstance(bind->first,bind->second.save->GetDifficultyID(),true);}
    }
};
static void DetachGroup(Group& group)
{
    for(auto diff:modes)while(!group.GetBoundInstances(diff).empty())
    {auto bind=group.GetBoundInstances(diff).begin();group.UnbindInstance(bind->first,bind->second.save->GetDifficultyID(),true);}
    while(auto ref=group.GetFirstMember())ref->getSource()->SetGroup(nullptr);
    sGroupMgr->RegisterGroupDbStoreId(group.GetDbStoreId(),nullptr);sGroupMgr->RemoveGroup(&group);
}
static void CacheCharacter(BoundPlayer& p)
{sWorld->AddCharacterInfo(p.GetGUID(),0,p.GetName(),GENDER_MALE,p.getRace(),p.getClass(),p.getLevel());}
struct BoundGroup
{
    Group group;
    BoundGroup(BoundPlayer& leader,BoundPlayer& member)
    {
        CacheCharacter(leader);CacheCharacter(member);Require(group.Create(&leader,0,false),"Native LFG group creation failed");sGroupMgr->AddGroup(&group);
        Require(group.AddMember(&member),"Native LFG group member creation failed");
        auto dungeon=sLFGMgr->GetLFGDungeon(1648,DIFFICULTY_LFR_RAID,leader.GetTeam());Require(dungeon&&dungeon->dbc,"Native Trial LFR dungeon missing");
        group.ConvertToLFG(dungeon);group.SetRaidDifficultyID(DIFFICULTY_LFR_RAID);
        (sLFGMgr->*Access(LfgDungeonTag{}))(group.GetGUID(),dungeon->dbc->ID);(sLFGMgr->*Access(LfgStateTag{}))(group.GetGUID(),lfg::LFG_STATE_DUNGEON,0);(sLFGMgr->*Access(LfgSaveTag{}))(group.GetGUID(),group.GetDbStoreId());
    }
    ~BoundGroup(){DetachGroup(group);}
};
static void MoveTo(BoundPlayer& p,Map& map)
{
    p.GetMap()->RemovePlayerFromMap(&p,false);p.ResetMap();p.SetMap(&map);p.Relocate(pos);
    Require(map.AddPlayerToMap(&p,false),"Native map admission failed");
}
static InstanceMap* Select(BoundPlayer& p,Difficulty diff)
{
    p.SetRaidDifficultyID(diff);auto map=sMapMgr->CreateMap(1648,&p);Require(map&&map->ToInstanceMap(),"Native instance selection/factory failed");return map->ToInstanceMap();
}
static void ReloadBinds(BoundPlayer& p)
{
    auto stmt=CharacterDatabase.GetPreparedStatement(CHAR_SEL_CHARACTER_INSTANCE);stmt->setUInt64(0,p.GetGUIDLow());(p.*Access(BindLoadTag{}))(ReadPrepared(stmt));
}
static void CheckCalendar(BoundPlayer& p,std::array<uint32,4> const& ids)
{
    Player* _player=&p;time_t currTime=GameTime::GetGameTime();
    WorldPackets::Calendar::CalendarSendCalendar packet;
#include "NativeCalendarBindings.inc"
    std::array<unsigned,3> counts{};
    for(auto const& lockout:packet.RaidLockouts)
    {
        bool matched=false;
        for(unsigned i=0;i<3;++i)if(lockout.InstanceID==ids[i])
        {Require(lockout.MapID==1648&&lockout.DifficultyID==modes[i]&&lockout.ExpireTime>0,"Calendar changed saved lockout fields");++counts[i];matched=true;}
        Require(matched,"Calendar emitted unrelated lockout");
    }
#ifdef TRIAL_REENTRY_BASELINE
    Require(counts==std::array<unsigned,3>{{2,1,0}},"Pinned calendar duplicate Normal/missing Mythic changed");
    std::cout<<"BASELINE: calendar repeats Normal and omits Mythic"<<std::endl;
#else
    Require(counts==std::array<unsigned,3>{{1,1,1}},"Calendar must enumerate each actual bind slot once");
    std::cout<<"PASS: native calendar enumeration/serialization preserves each saved raid once"<<std::endl;
#endif
    Require(packet.Write()!=nullptr,"Native calendar packet serialization failed");
}
static uint32 EarnOdyn(BoundPlayer& p,Difficulty diff)
{
    auto map=Select(p,diff);MoveTo(p,*map);auto odyn=LoadBoss(*map,505062);auto instance=map->GetInstanceScript();
    instance->SetData(Data::DataMisc::OdynEventComplete1,DONE);instance->SetData(Data::DataMisc::OdynEventComplete2,DONE);instance->SetBossState(0,IN_PROGRESS);
    odyn->AI()->DoAction(ACTION_1);odyn->AI()->UpdateAI(7000);odyn->CastStop();odyn->AI()->UpdateAI(2000);odyn->CastStop();
    Require(odyn->AI()->GetData(Data::BossIDs::OdynID)==2,"Native managed Odyn phase setup failed");odyn->SetHealth(odyn->CountPctFromMaxHealth(9));uint32 damage=1;odyn->AI()->DamageTakenAfterScaling(&p,damage,DIRECT_DAMAGE);
    Require(instance->GetBossState(0)==DONE,"Native managed Odyn victory failed");FlushWrites();
    auto table=diff==DIFFICULTY_LFR_RAID?"group_instance":"character_instance";auto owner=diff==DIFFICULTY_LFR_RAID?p.GetGroup()->GetDbStoreId():p.GetGUIDLow();
    auto row=CharacterDatabase.Query(Trinity::StringFormat("SELECT difficulty,data,permanent,completedEncounters FROM %s WHERE guid=%u AND instance=%u",table,owner,map->GetInstanceId()).c_str());
    Require(row&&row->Fetch()[0].GetUInt8()==diff&&row->Fetch()[1].GetString()==instance->GetSaveData()&&row->Fetch()[2].GetBool()==(diff!=DIFFICULTY_LFR_RAID),"Native victory bind/data SQL checkpoint missing");
    Require(row->Fetch()[3].GetUInt32()&&row->Fetch()[3].GetUInt32()==instance->GetCompletedEncounterMask(),"Native victory checkpoint lost encounter mask");
    std::cout<<"EARN diff="<<uint32(diff)<<" instance="<<map->GetInstanceId()<<" mask="<<row->Fetch()[3].GetUInt32()<<std::endl;return map->GetInstanceId();
}
static void RoundTrip(Map& staging,MapInstanced& parent,uint8 race)
{
    auto guid=++nextGuid,memberGuid=++nextGuid;uint32 groupId=0;std::array<uint32,4> ids{};
    {
        BoundPlayer p(staging,race,guid),member(staging,race,memberGuid);p.SaveToDB(true);member.SaveToDB(true);FlushWrites();
        for(unsigned i=0;i<3;++i){ids[i]=EarnOdyn(p,modes[i]);MoveTo(p,staging);}
        CheckCalendar(p,ids);
        BoundGroup finder(p,member);groupId=finder.group.GetDbStoreId();
        // A retained Normal group save must not satisfy a Raid Finder request.
        auto lfr=Select(p,DIFFICULTY_LFR_RAID);
#ifdef TRIAL_REENTRY_BASELINE
        Require(lfr->GetInstanceId()==ids[0]&&lfr->GetDifficultyID()==DIFFICULTY_NORMAL_RAID,"Pinned Normal-to-LFR selection collision changed");
        std::cout<<"BASELINE: requested LFR17 selected saved Normal14 instance="<<ids[0]<<std::endl;
#else
        Require(lfr->GetDifficultyID()==DIFFICULTY_LFR_RAID&&lfr->GetInstanceId()!=ids[0],"LFR selected saved Normal instance");
        ids[3]=EarnOdyn(p,DIFFICULTY_LFR_RAID);MoveTo(p,staging);
        for(unsigned i=0;i<3;++i){auto bind=p.GetBoundInstance(1648,modes[i]);Require(bind&&bind->save->GetInstanceId()==ids[i],"LFR displaced another player bind");}
        Require(!p.GetBoundInstance(1648,DIFFICULTY_LFR_RAID),"LFR created an individual permanent/solo bind");
        Require(finder.group.GetBoundInstance(DIFFICULTY_NORMAL_RAID,1648)->save->GetInstanceId()==ids[0]&&finder.group.GetBoundInstance(DIFFICULTY_LFR_RAID,1648)->save->GetInstanceId()==ids[3],"LFR displaced the Normal group bind");
#endif
        FlushWrites();
    }
    for(bool cold:{false,true})
    {
        if(cold)parent.UnloadAll();
        // Full production group loader reads the native Create/AddMember,
        // LFG checkpoint and group_instance rows, including its cleanup SQL.
        sGroupMgr->LoadGroups();auto group=sGroupMgr->GetGroupByDbStoreId(groupId);
        Require(group&&group->isLFGGroup()&&group->GetRaidDifficultyID()==DIFFICULTY_LFR_RAID&&group->GetMembersCount()==2,"Native persisted LFG group/roster/difficulty reload failed");
        {
            // The leader has no live Player here: exercise native group-save
            // selection for a member while the leader is offline.
            BoundPlayer member(staging,race,memberGuid);member.SetGroup(group,0);ReloadBinds(member);auto selected=Select(member,DIFFICULTY_LFR_RAID);
#ifdef TRIAL_REENTRY_BASELINE
            Require(selected->GetInstanceId()==ids[0]&&selected->GetDifficultyID()==DIFFICULTY_NORMAL_RAID,"Pinned reloaded group collision changed");
            std::cout<<"BASELINE: SQL-reloaded LFR group selects Normal while leader offline"<<std::endl;
#else
            Require(selected->GetInstanceId()==ids[3]&&selected->GetDifficultyID()==DIFFICULTY_LFR_RAID&&selected->GetInstanceScript()->GetBossState(0)==DONE,"Reloaded group/offline leader lost LFR progress");MoveTo(member,*selected);MoveTo(member,staging);
#endif
            member.SetGroup(nullptr);
        }
        {
            BoundPlayer p(staging,race,guid);ReloadBinds(p);
            CheckCalendar(p,ids);
            for(unsigned i=0;i<3;++i)
            {
                auto selected=Select(p,modes[i]);Require(selected->GetInstanceId()==ids[i]&&selected->GetDifficultyID()==modes[i]&&selected->GetInstanceScript()->GetBossState(0)==DONE&&selected->GetInstanceScript()->GetCompletedEncounterMask(),"Prepared player bind reload chose wrong instance/difficulty/progress");MoveTo(p,*selected);MoveTo(p,staging);
            }
#ifndef TRIAL_REENTRY_BASELINE
            p.SetGroup(group,0);auto selected=Select(p,DIFFICULTY_LFR_RAID);Require(selected->GetInstanceId()==ids[3]&&selected->GetDifficultyID()==DIFFICULTY_LFR_RAID&&selected->GetInstanceScript()->GetBossState(0)==DONE,"Reloaded LFR leader lost independent progress");MoveTo(p,*selected);MoveTo(p,staging);p.SetGroup(nullptr);
#endif
        }
        DetachGroup(*group);delete group;
        std::cout<<"PASS: native player/group SQL reload, managed instance selection/entry and offline leader; race="<<uint32(race)<<" cold="<<cold<<std::endl;
    }
    parent.UnloadAll();
    for(auto table:{"group_instance","group_member","lfg_data","groups"})CharacterDatabase.DirectExecute(Trinity::StringFormat("DELETE FROM `%s` WHERE guid=%u",table,groupId).c_str());
}
static int RunReentry(int argc,char** argv)
{
    InitializeNative(argc,argv);opcodeTable.Initialize();LoadInstanceInputs();OpenScratch();sAchievementMgr->LoadRewards();sAchievementMgr->LoadCompletedAchievements();
    for(auto state:{WS_INSTANCE_DAILY_RESET_TIME,WS_INSTANCE_WEEKLY_RESET_TIME,WS_WEEKLY_RESET_TIME})sWorld->setWorldState(state,uint32(GameTime::GetGameTime()+WEEK));
    sScriptMgr->SetScriptContext(sScriptMgr->GetNameOfStaticContext());AddSC_tov_boss_odyn();AddSC_boss_tov_helya();AddSC_trial_of_valor();AddSC_instance_trial_of_valor();sScriptMgr->SwapScriptContext(true);
    dynamic_cast<VMAP::VMapManager2*>(VMAP::VMapFactory::createOrGetVMapManager())->InitializeThreadUnsafe({{1648,{}},{1220,{}}});MMAP::MMapFactory::createOrGetMMapManager()->InitializeThreadUnsafe({{1648,{}},{1220,{}}});
    sMapMgr->InitInstanceIds();auto parent=static_cast<MapInstanced*>(sMapMgr->CreateBaseMap(1648));parent->SetMapStop();sMapMgr->_mapThreads[1648]->join();
    {Map staging(1220,300,0,DIFFICULTY_NONE);for(auto race:{RACE_HUMAN,RACE_ORC})RoundTrip(staging,*parent,race);}
    FlushWrites();sInstanceSaveMgr->UnloadAll();CharacterDatabase.Close();LoginDatabase.Close();HotfixDatabase.Close();WorldDatabase.Close();return 0;
}
int main(int argc,char** argv)
{
    SetErrorMode(SEM_FAILCRITICALERRORS|SEM_NOGPFAULTERRORBOX);SetUnhandledExceptionFilter(CrashTrace);
    try{return RunReentry(argc,argv);}catch(std::exception const& e){std::cerr<<e.what()<<'\n';return 1;}
}
