// Real packet handler, corpse persistence and map identity; socketless Players.
#include "MiscPackets.h"

static void ReclaimRequest(LootPlayer& player, ObjectGuid guid)
{
    WorldPacket wire(CMSG_RECLAIM_CORPSE);
    wire << guid;
    WorldPackets::Misc::ReclaimCorpse request(std::move(wire));
    request.Read();
    Require(request.CorpseGUID==guid,"Native reclaim packet decoding changed");
    player.GetSession()->HandleReclaimCorpse(request);
}
static void ReclaimMove(LootPlayer& player, Map& map)
{
    player.Unit::RemoveFromWorld();player.RemoveFromGrid();player.ResetMap();player.SetMap(&map);
    map.LoadGrid(player.GetPositionX(),player.GetPositionY());
    map.AddToGrid(static_cast<Player*>(&player),Cell(Trinity::ComputeCellCoord(player.GetPositionX(),player.GetPositionY())));
    player.Unit::AddToWorld();
}
static void CheckCorpseReclaim()
{
    // Common bootstrap reserves only map1220; register the foreign-map control
    // before constructing any maps, as the real startup catalog does.
    dynamic_cast<VMAP::VMapManager2*>(VMAP::VMapFactory::createOrGetVMapManager())->InitializeThreadUnsafe({{1220,{}},{1,{}}});
    MMAP::MMapFactory::createOrGetMMapManager()->InitializeThreadUnsafe({{1220,{}},{1,{}}});
    bool pve=sWorld->getBoolConfig(CONFIG_DEATH_CORPSE_RECLAIM_DELAY_PVE);
    bool pvp=sWorld->getBoolConfig(CONFIG_DEATH_CORPSE_RECLAIM_DELAY_PVP);
    sWorld->setBoolConfig(CONFIG_DEATH_CORPSE_RECLAIM_DELAY_PVE,true);
    sWorld->setBoolConfig(CONFIG_DEATH_CORPSE_RECLAIM_DELAY_PVP,true);
    unsigned cases=0;
    for(uint8 race : {uint8(RACE_HUMAN),uint8(RACE_ORC)})for(bool pvpDeath : {false,true})
    for(std::string const mode : {"early","exact-delay","after-delay","far-horizontal","far-vertical","radius-inside","radius-outside","other-map","other-instance","unreleased","missing","already-alive"})
    {
        Map map(1220,300,0,DIFFICULTY_NONE), otherInstance(1220,300,71,DIFFICULTY_NONE), otherMap(1,300,0,DIFFICULTY_NONE);
        uint32 owner=12000+cases++;
        sWorld->AddCharacterInfo(ObjectGuid::Create<HighGuid::Player>(owner),0,"Reclaim"+std::to_string(owner),GENDER_MALE,race,CLASS_WARRIOR,110);
        LootPlayer player(Session(),map,owner,{2370.78f,895.832f,252.924f,0});
        player.SetByteValue(UNIT_FIELD_BYTES_0,0,race);player.SetMaxHealth(1000);player.SetHealth(1000);
        player.SetPvPDeath(pvpDeath);player.KillPlayer();player.BuildPlayerRepop();FlushWrites();
        Require(player.isDead()&&player.HasAura(8326)&&player.HasFlag(PLAYER_FIELD_PLAYER_FLAGS,PLAYER_FLAGS_GHOST),"Native reclaim ghost input missing");
        auto corpse=player.GetCorpse();Require(corpse&&corpse->IsInMap(&player),"Native reclaim corpse input missing");
        ObjectGuid corpseGuid=corpse->GetGUID();
        uint32 delay=player.GetCorpseReclaimDelay(pvpDeath);Require(delay>=30,"Enabled native reclaim delay missing");
        // Only timestamp input is controlled. Hydrate through real SQL loader;
        // no mutation of private corpse time or modeled delay admission.
        auto now=GameTime::GetGameTime();
        time_t ghostTime=now-delay+(mode=="early"?1:(mode=="after-delay"?-1:0));
        ForgetCorpse(corpse);
        CharacterDatabase.DirectExecute(Trinity::StringFormat("UPDATE corpse SET time=%u WHERE guid=%u",uint32(ghostTime),owner).c_str());
        sObjectMgr->LoadCorpses();corpse=player.GetCorpse();
        Require(corpse&&corpse->GetGUID()==corpseGuid&&corpse->GetGhostTime()==ghostTime&&corpse->GetMapId()==map.GetId()&&corpse->GetInstanceId()==map.GetInstanceId(),"Native reclaim timestamp hydration failed");
        // AddToMap requires a map pointer already attached by its caller.
        // SQL hydration restores map IDs; normal grid loading attaches pointers.
        corpse->SetMap(&map);Require(map.AddToMap(corpse)&&corpse->IsInMap(&player),"Hydrated corpse map attachment failed");
        float edge=CORPSE_RECLAIM_RADIUS+corpse->GetObjectSize()+player.GetObjectSize();
        if(mode=="far-horizontal")player.Relocate(corpse->GetPositionX()+100,corpse->GetPositionY(),corpse->GetPositionZ());
        if(mode=="far-vertical")player.Relocate(corpse->GetPositionX(),corpse->GetPositionY(),corpse->GetPositionZ()+100);
        if(mode=="radius-inside"||mode=="radius-outside")player.Relocate(corpse->GetPositionX()+edge+(mode=="radius-inside"?-0.25f:0.25f),corpse->GetPositionY(),corpse->GetPositionZ());
        if(mode=="other-map")ReclaimMove(player,otherMap);
        if(mode=="other-instance")ReclaimMove(player,otherInstance);
        if(mode=="other-map"||mode=="other-instance")Require(!corpse->IsInMap(&player)&&corpse->IsWithinDist(&player,CORPSE_RECLAIM_RADIUS,true),"Map mismatch must be within distance but outside native map identity");
        if(mode=="unreleased"){player.RemoveAurasDueToSpell(8326);Require(player.isDead()&&!player.HasFlag(PLAYER_FIELD_PLAYER_FLAGS,PLAYER_FLAGS_GHOST),"Unreleased dead input missing");}
        if(mode=="already-alive")player.ResurrectPlayer(0.5f);
        if(mode=="missing"){sObjectAccessor->ConvertCorpseForPlayer(player.GetGUID());corpse=nullptr;FlushWrites();Require(!player.GetCorpse(),"Missing corpse input failed");}
        bool allowed=mode=="exact-delay"||mode=="after-delay"||mode=="radius-inside";
        uint64 health=player.GetHealth();bool alive=player.IsAlive();
        Require(GameTime::GetGameTime()==now,"Fixture clock advanced across controlled delay boundary");
        ReclaimRequest(player,corpseGuid);FlushWrites();
        if(allowed)
        {
            Require(player.IsAlive()&&player.GetHealth()==500&&!player.GetCorpse()&&!player.HasAura(8326)&&!player.HasFlag(PLAYER_FIELD_PLAYER_FLAGS,PLAYER_FLAGS_GHOST),"Eligible native reclaim failed half-health resurrection and corpse cleanup");
            player.SetHealth(321);ReclaimRequest(player,corpseGuid);FlushWrites();
            Require(player.IsAlive()&&player.GetHealth()==321&&!player.GetCorpse(),"Duplicate reclaim changed health or recreated corpse");
        }
        else Require(player.IsAlive()==alive&&player.GetHealth()==health&&(mode=="missing"?!player.GetCorpse():player.GetCorpse()==corpse),"Rejected native reclaim changed death/health/corpse");
        auto saved=CharacterDatabase.Query(Trinity::StringFormat("SELECT COUNT(*),MIN(corpseGuid) FROM corpse WHERE guid=%u AND corpseType<>0",owner).c_str());
        Require(saved&&saved->Fetch()[0].GetUInt32()==uint32(!allowed&&mode!="missing"),"Reclaim SQL deletion differs from admission");
        if(!allowed&&mode!="missing")Require(saved->Fetch()[1].GetUInt64()==corpseGuid.GetCounter(),"Rejected reclaim changed persisted corpse identity");
        std::cout<<"CORPSE_RECLAIM mode="<<mode<<" race="<<unsigned(race)<<" pvp="<<pvpDeath<<" delay="<<delay<<" allowed="<<allowed<<std::endl;
        if(player.GetMap()!=&map)ReclaimMove(player,map);
        if(player.GetCorpse())sObjectAccessor->ConvertCorpseForPlayer(player.GetGUID());
        FlushWrites();
    }
    sWorld->setBoolConfig(CONFIG_DEATH_CORPSE_RECLAIM_DELAY_PVE,pve);
    sWorld->setBoolConfig(CONFIG_DEATH_CORPSE_RECLAIM_DELAY_PVP,pvp);
    auto rows=CharacterDatabase.Query("SELECT COUNT(*) FROM corpse");Require(rows&&!rows->Fetch()[0].GetUInt32(),"Reclaim cases left persisted corpses");
    std::cout<<"PASS: "<<cases<<" native reclaim cases cover delay, 3D radius, map/instance, dead/ghost state, missing corpse, SQL preservation/deletion and duplicate packets."<<std::endl;
}
