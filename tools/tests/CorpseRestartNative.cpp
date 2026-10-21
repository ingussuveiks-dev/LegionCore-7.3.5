// Two independent native processes share only disposable SQL. The first uses
// real death/repop/save; the second starts with an empty ObjectAccessor and
// calls the production startup loader. Player construction is socketless;
// LoadCorpse below is a login component check, not aggregate LoadFromDB.
#define LOST_ARMY_TURNIN_NATIVE_SUPPORT_ONLY
#include "LostArmyTurnInNative.cpp"
#include "Corpse.h"
#include "PlayerDefines.h"

static ObjectGuid Owner(unsigned i) { return ObjectGuid::Create<HighGuid::Player>(10001+i); }
static void ForgetCorpse(Corpse* corpse)
{
    sObjectAccessor->RemoveCorpse(corpse);
    delete corpse; // Unload only: retain the SQL row for the next process.
}
#include "CorpseReclaimNative.inc"
#include "SpiritHealerNative.inc"
static int RestartCorpses(int argc,char** argv)
{
    InitializeNative(argc,argv);opcodeTable.Initialize();OpenScratch();
    sObjectAccessor->SetGuidSize(HighGuid::Player,20000);
    auto rows=CharacterDatabase.Query("SELECT COUNT(*) FROM corpse");
    Require(bool(rows),"Scratch corpse count failed");bool read=rows->Fetch()[0].GetUInt32()!=0;
    std::cout<<"CORPSE_PROCESS phase="<<(read?"read":"write")<<" pid="<<GetCurrentProcessId()<<std::endl;
    std::cout<<"CORPSE_ABI field="<<sizeof(Field)<<std::endl;
    for(unsigned i=0;i<7;++i)sWorld->AddCharacterInfo(Owner(i),0,"Ghost"+std::to_string(i),GENDER_MALE,i==4?RACE_NIGHTELF:(i%2?RACE_ORC:RACE_HUMAN),CLASS_WARRIOR,110);
    if(!read){
        CheckCorpseReclaim();
        CheckSpiritHealer();
        Map map(1220,300,0,DIFFICULTY_NONE);
        for(unsigned i=0;i<6;++i){
            LootPlayer player(Session(),map,Owner(i).GetCounter(),{2370.78f+float(i),895.832f,252.924f,1.25f});
            player.SetByteValue(UNIT_FIELD_BYTES_0,0,i==4?RACE_NIGHTELF:(i%2?RACE_ORC:RACE_HUMAN));
            if(i%2)player.SetPvPDeath(true);
            player.KillPlayer();player.BuildPlayerRepop();auto corpse=player.GetCorpse();
            Require(corpse&&corpse->GetGUIDLow()!=Owner(i).GetCounter()&&corpse->GetType()==(i%2?CORPSE_RESURRECTABLE_PVP:CORPSE_RESURRECTABLE_PVE),"Distinct corpse/owner or PvE/PvP input missing");
            Require(!corpse->IsExpired(corpse->GetGhostTime()+3*DAY)&&corpse->IsExpired(corpse->GetGhostTime()+3*DAY+1),"Native resurrectable expiry boundary changed");
            FlushWrites();ForgetCorpse(corpse);
        }
        CharacterDatabase.DirectExecute("INSERT INTO corpse(corpseGuid,guid,itemCache,corpseType) VALUES(9999,19999,'',0)");
        // Controlled age input only; production startup SQL decides deletion.
        CharacterDatabase.DirectExecute("UPDATE corpse SET time=UNIX_TIMESTAMP(NOW())-259260 WHERE guid IN (10003,10005)");
        FlushWrites();std::cout<<"PASS: native death/repop saved six PvE/PvP corpses and exact three-day expiry boundaries; two aged for independent startup cleanup."<<std::endl;
    }else{
        Require(!sObjectAccessor->GetCorpseForPlayerGUID(Owner(0))&&!sObjectAccessor->GetCorpseForPlayerGUID(Owner(1)),"Cold process already has corpses");
        Require(rows->Fetch()[0].GetUInt32()==7,"Persisted expiry/bones inputs missing");
        auto selected=CharacterDatabase.Query(CharacterDatabase.GetPreparedStatement(CHAR_SEL_CORPSES));Require(selected&&selected->GetRowCount()==6,"Prepared startup query did not exclude bones");
        auto cleanup=CharacterDatabase.GetPreparedStatement(CHAR_DEL_OLD_CORPSES);cleanup->setUInt32(0,3*DAY);CharacterDatabase.Execute(cleanup);FlushWrites();
        auto retained=CharacterDatabase.Query("SELECT COUNT(*) FROM corpse");Require(retained&&retained->Fetch()[0].GetUInt32()==4,"Startup cleanup did not remove only two expired corpses and bones");
        // Simulate aging after startup cleanup, before native cache cleanup.
        CharacterDatabase.DirectExecute("UPDATE corpse SET time=UNIX_TIMESTAMP(NOW())-259260 WHERE guid IN (10004,10006)");
        sObjectMgr->LoadCorpses();
        Require(!sObjectAccessor->GetCorpseForPlayerGUID(Owner(2))&&!sObjectAccessor->GetCorpseForPlayerGUID(Owner(4)),"Startup reloaded expired corpses");
        for(unsigned i : {3u,5u}){auto corpse=sObjectAccessor->GetCorpseForPlayerGUID(Owner(i));Require(corpse&&corpse->IsExpired(GameTime::GetGameTime()),"Runtime expiry input missing");}
        Corpse bones;Require(!bones.IsExpired(bones.GetGhostTime()+5*MINUTE)&&bones.IsExpired(bones.GetGhostTime()+5*MINUTE+1),"Native bones expiry boundary changed");
        sObjectAccessor->RemoveOldCorpses();FlushWrites();
        for(unsigned i : {3u,5u})Require(!sObjectAccessor->GetCorpseForPlayerGUID(Owner(i)),"Runtime cleanup retained expired cache entry");
        auto liveRows=CharacterDatabase.Query("SELECT COUNT(*) FROM corpse");Require(liveRows&&liveRows->Fetch()[0].GetUInt32()==2,"Runtime cleanup did not preserve exactly two fresh corpses");
        Map map(1220,300,0,DIFFICULTY_NONE);
        for(unsigned i=2;i<7;++i){
            LootPlayer player(Session(),map,Owner(i).GetCounter(),{2370.78f,895.832f,252.924f,0});
            player.SetByteValue(UNIT_FIELD_BYTES_0,0,i==4?RACE_NIGHTELF:(i%2?RACE_ORC:RACE_HUMAN));
            player.SetMaxHealth(1000);player.KillPlayer();player.CastSpell(&player,8326,true);
            if(i==4){player.CastSpell(&player,20584,true);Require(player.HasAura(20584),"Night Elf ghost speed input missing");}
            Require(player.isDead()&&player.HasAura(8326)&&player.HasFlag(PLAYER_FIELD_PLAYER_FLAGS,PLAYER_FLAGS_GHOST)&&!player.GetCorpse(),"Missing-corpse ghost input missing");
            player.LoadCorpse();
            Require(player.IsAlive()&&player.GetHealth()==500&&!player.GetCorpse()&&!player.HasAura(8326)&&!player.HasAura(20584)&&!player.HasFlag(PLAYER_FIELD_PLAYER_FLAGS,PLAYER_FLAGS_GHOST)&&!player.HasAura(15007),"Missing-corpse login failed native half-health ghost cleanup");
            player.SetHealth(321);player.LoadCorpse();Require(player.IsAlive()&&player.GetHealth()==321&&!player.GetCorpse(),"Repeated living LoadCorpse healed or recreated corpse");
            std::cout<<"CORPSE_RECOVERY owner="<<Owner(i).GetCounter()<<" reason="<<(i==6?"missing":(i%2?"runtime-expired":"startup-expired"))<<" alive=1 ghost=0 initialHealth=500 repeatHealth=321"<<std::endl;
        }
        for(unsigned i=0;i<2;++i){
            auto corpse=sObjectAccessor->GetCorpseForPlayerGUID(Owner(i));Require(corpse,"Startup did not index saved owner");
            auto saved=CharacterDatabase.Query(Trinity::StringFormat("SELECT corpseGuid,guid,posX,posY,posZ,orientation,mapId,instanceId,phaseMask,corpseType,time,displayId,bytes1,bytes2,flags,dynFlags FROM corpse WHERE guid=%u AND corpseType<>0",Owner(i).GetCounter()).c_str());Require(bool(saved),"Saved corpse missing");auto f=saved->Fetch();
            std::cout<<"CORPSE_ID owner="<<Owner(i).GetCounter()<<" saved="<<f[0].GetUInt64()<<" loaded="<<corpse->GetGUIDLow()<<std::endl;
            Require(corpse->GetGUIDLow()==f[0].GetUInt64()&&corpse->GetOwnerGUID()==Owner(i),"Startup replaced corpse GUID with its owner's GUID");
            Require(corpse->GetExactDist(f[2].GetFloat(),f[3].GetFloat(),f[4].GetFloat())<0.001f&&std::abs(corpse->GetOrientation()-f[5].GetFloat())<0.001f&&corpse->GetMapId()==f[6].GetUInt16()&&corpse->GetInstanceId()==f[7].GetUInt32()&&corpse->GetPhaseMask()==f[8].GetUInt16()&&corpse->GetType()==f[9].GetUInt8()&&corpse->GetGhostTime()==f[10].GetUInt32(),"Cold corpse location/type/time changed");
            Require(corpse->GetUInt32Value(CORPSE_FIELD_DISPLAY_ID)==f[11].GetUInt32()&&corpse->GetUInt32Value(CORPSE_FIELD_BYTES_1)==f[12].GetUInt32()&&corpse->GetUInt32Value(CORPSE_FIELD_BYTES_2)==f[13].GetUInt32()&&corpse->GetUInt32Value(CORPSE_FIELD_FLAGS)==f[14].GetUInt8()&&corpse->GetUInt32Value(CORPSE_FIELD_DYNAMIC_FLAGS)==f[15].GetUInt8(),"Cold corpse appearance changed");
            LootPlayer player(Session(),map,Owner(i).GetCounter(),{2370.78f,895.832f,252.924f,0});
            player.KillPlayer();player.CastSpell(&player,8326,true);
            Require(player.isDead()&&player.HasFlag(PLAYER_FIELD_PLAYER_FLAGS,PLAYER_FLAGS_GHOST),"Login component ghost input missing");
            player.LoadCorpse();Require(player.isDead()&&player.GetCorpse()==corpse&&player.HasFlag(PLAYER_FIELD_LOCAL_FLAGS,PLAYER_LOCAL_FLAG_RELEASE_TIMER),"LoadCorpse lost cold corpse or resurrected ghost");
            corpse->SaveToDB();FlushWrites();
            auto count=CharacterDatabase.Query(Trinity::StringFormat("SELECT COUNT(*),MIN(corpseGuid) FROM corpse WHERE guid=%u AND corpseType<>0",Owner(i).GetCounter()).c_str());Require(count&&count->Fetch()[0].GetUInt32()==1&&count->Fetch()[1].GetUInt64()==f[0].GetUInt64(),"Reload/save changed identity or duplicated corpse");
            player.ResurrectPlayer(0.5f,false);player.LoadCorpse();FlushWrites();Require(!player.GetCorpse(),"Living login failed to convert stale corpse");
        }
        auto remaining=CharacterDatabase.Query("SELECT COUNT(*) FROM corpse WHERE corpseType<>0");Require(remaining&&!remaining->Fetch()[0].GetUInt32(),"Converted saved corpses remain in SQL");
        std::cout<<"PASS: independent cold startup preserves corpse GUID/owner/PvE/PvP/location/appearance/time; native ghost/living LoadCorpse and repeat-save conversion checked."<<std::endl;
    }
    CharacterDatabase.Close();LoginDatabase.Close();HotfixDatabase.Close();WorldDatabase.Close();return 0;
}
int main(int argc,char** argv)
{
    SetErrorMode(SEM_FAILCRITICALERRORS|SEM_NOGPFAULTERRORBOX);SetUnhandledExceptionFilter(CrashTrace);
    try{return RestartCorpses(argc,argv);}catch(std::exception const& e){std::cerr<<e.what()<<std::endl;return 1;}
}
