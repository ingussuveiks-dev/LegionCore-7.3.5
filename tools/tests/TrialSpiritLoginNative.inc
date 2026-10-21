#include "LoginQueryHolder.h"
struct SpiritLoginStatsTag{using Type=uint32 Player::*;friend Type Access(SpiritLoginStatsTag);};
template struct FixtureAccess<SpiritLoginStatsTag,&Player::m_updateStatsMask>;
static void SpiritLoginInputs()
{
    static bool loaded=false;if(loaded)return;loaded=true;
    sWorld->setIntConfig(CONFIG_MAX_PLAYER_LEVEL,110);
    // Existing-character loading needs native race/class stats and defaults.
    // Starting quests must already belong to the fixture's scoped catalog.
    // The current database has no custom starting-quest rows.
    auto quests=WorldDatabase.Query("SELECT DISTINCT questid FROM playercreateinfo_quest");
    if(quests)do{Require(sQuestDataStore->GetQuestTemplate(quests->Fetch()[0].GetUInt32()),"Load new native starting quests before full-login testing");}while(quests->NextRow());
    sObjectMgr->LoadPlayerInfo();
    Require(sObjectMgr->GetPlayerInfo(RACE_HUMAN,CLASS_WARRIOR)&&sObjectMgr->GetPlayerInfo(RACE_ORC,CLASS_WARRIOR),"Native login race/class defaults missing");
}
static void SpiritFullLogin(ObjectGuid guid,WorldLocation const& expected,uint64 itemGuid,uint32 durability,WorldSession* oldSession,uint32 difficulty,uint8 race,unsigned direction,unsigned mode)
{
    int32 expectedSickness=598000;
    for(uint32 offline:{0u,20u,700u})
    {
        CharacterDatabase.DirectPExecute("UPDATE characters SET logout_time=%u WHERE guid=%u",uint32(GameTime::GetGameTime())-offline,guid.GetCounter());
        auto holder=std::make_shared<LoginQueryHolder>(0,guid);Require(holder->Initialize(),"Native login query holder initialization failed");
        auto query=CharacterDatabase.DelayQueryHolder(holder);
        Require(query.m_future.wait_for(std::chrono::seconds(5))==std::future_status::ready,"Native login query holder timed out");query.m_future.get();
        Require(holder->GetPreparedResult(PLAYER_LOGIN_QUERY_LOADFROM)&&holder->GetPreparedResult(PLAYER_LOGIN_QUERY_LOADINVENTORY),"Native login character/inventory results missing");
        auto session=Session();session->*Access(ControlLoadingTag{})=guid;
        auto player=new Player(session);
        static bool abi=false;if(!abi){abi=true;std::cout<<"SPIRIT_LOGIN_ABI stats="<<(reinterpret_cast<char*>(&(player->*Access(SpiritLoginStatsTag{})))-reinterpret_cast<char*>(player))<<" stamina="<<uint32(STAT_STAMINA)<<" mask="<<uint32(USM_STAMINA)<<std::endl;}
        Require(player->LoadFromDB(guid,*holder),"Aggregate native Player::LoadFromDB rejected saved spirit recovery");
        SpiritFarFreeze(player->GetMapId());
        std::cout<<"SPIRIT_FULL_LOADED guid="<<guid.GetCounter()<<" offline="<<offline<<" alive="<<player->IsAlive()<<" hp="<<player->GetHealth()<<" max="<<player->GetMaxHealth()<<" map="<<player->GetMapId()<<" distance="<<player->GetExactDist(expected)<<" orientation="<<player->GetOrientation()<<" expectedO="<<expected.GetOrientation()<<" teleport="<<player->IsBeingTeleported()<<std::endl;
        Require(session->GetPlayer()==player&&player->IsAlive()&&player->GetHealth()==321&&!player->HasFlag(PLAYER_FIELD_PLAYER_FLAGS,PLAYER_FLAGS_GHOST)&&!player->IsBeingTeleported()&&player->GetMapId()==expected.GetMapId()&&player->GetExactDist(expected)<0.01f&&std::abs(player->GetOrientation()-expected.GetOrientation())<0.001f,"Aggregate login lost alive/health/position/orientation/transfer state");
        auto item=player->GetItemByGuid(ObjectGuid::Create<HighGuid::Item>(itemGuid));
        Require(item&&item->GetEntry()==25&&item->GetUInt32Value(ITEM_FIELD_DURABILITY)==durability,"Aggregate login lost original inventory item/durability");
        expectedSickness-=int32(offline*1000);
        auto aura=player->GetAura(15007);
        Require(expectedSickness>0?(aura&&aura->GetDuration()==expectedSickness):!aura,"Aggregate login did not restore/count down/expire resurrection sickness");
        player->UpdateStatsByMask();
        Require(player->IsAlive()&&player->GetHealth()==321,"First deferred stat update changed loaded health or killed the player");
        auto map=player->GetMap();session->SetMap(map);
        auto& sessions=const_cast<SessionMap&>(sWorld->GetAllSessions());auto found=sessions.find(0);WorldSessionPtr previous;
        if(found!=sessions.end()){previous=found->second;Require(previous.get()==oldSession,"Aggregate login would replace unrelated session");}
        sessions[0]=WorldSessionPtr(session,[](WorldSession*){});
        Require(map->AddPlayerToMap(player)&&player->CheckInstanceLoginValid(),"Native full-login map admission failed");sObjectAccessor->AddObject(player);
        player->LoadCorpse();uint32 zone,area;player->GetZoneAndAreaId(zone,area);player->UpdateZone(zone,area);SpiritFarFreeze(player->GetMapId());
        Require(player->IsInWorld()&&!player->GetCorpse()&&!player->IsBeingTeleported()&&(!map->CanCreatedZone()||player->GetInstanceId()==sMapMgr->GetZoneId(expected.GetMapId(),expected.GetPositionX(),expected.GetPositionY(),expected.GetPositionZ())),"Full-login map/corpse/zone completion failed");
        oldSession->HandleWorldPortAck();oldSession->SendSpiritResurrect();session->SendSpiritResurrect();
        Require(player->GetHealth()==321&&item->GetUInt32Value(ITEM_FIELD_DURABILITY)==durability&&player->IsAlive()&&!player->GetCorpse(),"Stale/new-session resurrection requests changed fully loaded player");
        session->SetMap(nullptr);session->LogoutPlayer(true);Require(!session->GetPlayer()&&!ObjectAccessor::FindPlayer(guid),"Full-login logout did not delete Player");FlushWrites();if(previous)sessions[0]=previous;else sessions.erase(0);
        auto saved=CharacterDatabase.Query(Trinity::StringFormat("SELECT map,position_x,position_y,position_z,orientation,zone,health,playerFlags,online FROM characters WHERE guid=%u",guid.GetCounter()).c_str());Require(bool(saved),"Full-login subsequent save missing");auto f=saved->Fetch();
        Require(f[0].GetUInt32()==expected.GetMapId()&&std::abs(f[1].GetFloat()-expected.GetPositionX())<0.01f&&std::abs(f[2].GetFloat()-expected.GetPositionY())<0.01f&&std::abs(f[3].GetFloat()-expected.GetPositionZ())<0.01f&&std::abs(f[4].GetFloat()-expected.GetOrientation())<0.001f&&f[5].GetUInt32()==zone&&f[6].GetUInt64()==321&&!(f[7].GetUInt32()&PLAYER_FLAGS_GHOST)&&!f[8].GetBool(),"Full-login subsequent save lost recovery state");
        std::cout<<"PASS: spirit aggregate login diff="<<difficulty<<" race="<<uint32(race)<<" direction="<<direction<<" mode="<<mode<<" offline="<<offline<<" map="<<expected.GetMapId()<<" health="<<f[6].GetUInt64()<<" sickness="<<(expectedSickness>0?expectedSickness:0)<<" durability="<<durability<<std::endl;
    }
}
