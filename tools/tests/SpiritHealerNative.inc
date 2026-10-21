#include "NPCPackets.h"
static_assert(ALIVE==0,"Release ALIVE guard comparison must be reviewed");
struct SpiritDeathTag{using Type=DeathState Unit::*;friend Type Access(SpiritDeathTag);};
template struct FixtureAccess<SpiritDeathTag,&Unit::m_deathState>;

static void SpiritRequest(LootPlayer& player,ObjectGuid healer)
{
    WorldPacket wire(CMSG_SPIRIT_HEALER_ACTIVATE);wire<<healer;
    WorldPackets::NPC::SpiritHealerActivate request(std::move(wire));request.Read();
    Require(request.Healer==healer,"Spirit healer packet decoding changed");
    player.GetSession()->HandleSpiritHealerActivate(request);
}
static void CheckSpiritHealer()
{
    sObjectMgr->LoadGraveyardZones();
    sObjectMgr->LoadItemTemplates();sObjectMgr->LoadItemTemplateAddon();
    int32 sicknessLevel=sWorld->getIntConfig(CONFIG_DEATH_SICKNESS_LEVEL);
    sWorld->setIntConfig(CONFIG_DEATH_SICKNESS_LEVEL,11);
    unsigned cases=0;
    for(uint8 race : {uint8(RACE_HUMAN),uint8(RACE_ORC)})for(uint8 level : {uint8(10),uint8(15),uint8(20)})
    for(std::string const mode : {"alive","valid","far-horizontal","far-vertical","wrong-flag","despawned","other-map","other-instance","other-phase","missing-corpse","unreleased","unknown-guid","direct-alive"})
    {
        Map map(1220,300,0,DIFFICULTY_NONE),otherInstance(1220,300,71,DIFFICULTY_NONE),otherMap(1,300,0,DIFFICULTY_NONE);
        uint32 owner=13000+cases++;
        sWorld->AddCharacterInfo(ObjectGuid::Create<HighGuid::Player>(owner),0,"Spirit"+std::to_string(owner),GENDER_MALE,race,CLASS_WARRIOR,level);
        LootPlayer player(Session(),map,owner,{2370.78f,895.832f,252.924f,0});
        if(cases==1)std::cout<<"SPIRIT_ABI death="<<(reinterpret_cast<char*>(&(player.*Access(SpiritDeathTag{})))-reinterpret_cast<char*>(&player))<<std::endl;
        player.SetByteValue(UNIT_FIELD_BYTES_0,0,race);player.SetLevel(level);player.SetMaxHealth(1000);player.SetHealth(777);
        player.SetInventorySlotCount(16);
        ItemPosCountVec dest;Require(player.CanStoreNewItem(NULL_BAG,NULL_SLOT,dest,25,1)==EQUIP_ERR_OK,"Durable backpack item admission failed");
        Item* item=player.StoreNewItem(dest,25,true);Require(item&&item->GetUInt32Value(ITEM_FIELD_MAX_DURABILITY)>=4,"Native durable item25 missing");
        uint32 initialDurability=item->GetUInt32Value(ITEM_FIELD_DURABILITY),maxDurability=item->GetUInt32Value(ITEM_FIELD_MAX_DURABILITY);
        Actor healer(&map,6491,17000+cases,player.GetPosition());Require(map.AddToMap(static_cast<Creature*>(&healer)),"Native spirit healer spawn failed");
        Require(healer.HasFlag(UNIT_FIELD_NPC_FLAGS,UNIT_NPC_FLAG_SPIRITHEALER)&&player.GetNPCIfCanInteractWith(healer.GetGUID(),UNIT_NPC_FLAG_SPIRITHEALER)==&healer,"Spirit healer template/interaction positive control missing");
        bool initiallyAlive=mode=="alive"||mode=="direct-alive";
        if(!initiallyAlive){player.KillPlayer();player.BuildPlayerRepop();Require(player.isDead()&&player.GetCorpse()&&player.HasAura(8326),"Spirit healer ghost input missing");}
        FlushWrites();auto corpse=player.GetCorpse();ObjectGuid corpseGuid=corpse?corpse->GetGUID():ObjectGuid::Empty;
        if(mode=="far-horizontal")player.Relocate(healer.GetPositionX()+100,healer.GetPositionY(),healer.GetPositionZ());
        if(mode=="far-vertical")player.Relocate(healer.GetPositionX(),healer.GetPositionY(),healer.GetPositionZ()+100);
        if(mode=="wrong-flag")healer.RemoveFlag(UNIT_FIELD_NPC_FLAGS,UNIT_NPC_FLAG_SPIRITHEALER);
        if(mode=="despawned")map.RemoveFromMap(static_cast<Creature*>(&healer),false);
        if(mode=="other-map")ReclaimMove(player,otherMap);
        if(mode=="other-instance")ReclaimMove(player,otherInstance);
        if(mode=="other-phase")healer.SetPhaseMask(2,false);
        if(mode=="missing-corpse"){sObjectAccessor->ConvertCorpseForPlayer(player.GetGUID());corpse=nullptr;FlushWrites();Require(!player.GetCorpse(),"Missing spirit corpse input failed");}
        if(mode=="unreleased"){player.RemoveAurasDueToSpell(8326);Require(player.isDead()&&!player.HasFlag(PLAYER_FIELD_PLAYER_FLAGS,PLAYER_FLAGS_GHOST),"Unreleased spirit input missing");}
        bool allowed=mode=="valid"||mode=="missing-corpse"||mode=="unreleased";
        uint64 health=player.GetHealth();bool alive=player.IsAlive();
        if(mode=="direct-alive")player.GetSession()->SendSpiritResurrect();
        else SpiritRequest(player,mode=="unknown-guid"?ObjectGuid::Create<HighGuid::Creature>(1220,6491,19999):healer.GetGUID());
        FlushWrites();
        std::cout<<"SPIRIT_RESULT mode="<<mode<<" race="<<unsigned(race)<<" level="<<unsigned(level)<<" alive="<<player.IsAlive()<<" health="<<player.GetHealth()<<" durability="<<item->GetUInt32Value(ITEM_FIELD_DURABILITY)<<" initial="<<initialDurability<<" sickness="<<player.HasAura(15007)<<std::endl;
        if(allowed)
        {
            Require(player.IsAlive()&&player.GetHealth()==500&&!player.GetCorpse()&&!player.HasAura(8326)&&!player.HasFlag(PLAYER_FIELD_PLAYER_FLAGS,PLAYER_FLAGS_GHOST),"Spirit healer resurrection/ghost cleanup failed");
            uint32 expected=initialDurability-uint32(maxDurability*0.25f);
            Require(item->GetUInt32Value(ITEM_FIELD_DURABILITY)==expected,"Spirit healer did not apply one native25 percent durability loss");
            Aura* sickness=player.GetAura(15007);
            Require(bool(sickness)==(level>=11),"Spirit healer sickness level boundary failed");
            if(sickness){Require(sickness->GetDuration()==(level<20?(level-10)*MINUTE:10*MINUTE)*IN_MILLISECONDS,"Native sickness duration failed");sickness->SetDuration(sickness->GetDuration()-2000);}
            int32 remaining=sickness?sickness->GetDuration():0;
            player.SetHealth(321);SpiritRequest(player,healer.GetGUID());FlushWrites();
            Require(player.GetHealth()==321&&item->GetUInt32Value(ITEM_FIELD_DURABILITY)==expected&&!player.GetCorpse()&&(!sickness||sickness->GetDuration()==remaining),"Duplicate spirit request reapplied health/durability/sickness");
        }
        else Require(player.IsAlive()==alive&&player.GetHealth()==health&&player.GetCorpse()==corpse&&item->GetUInt32Value(ITEM_FIELD_DURABILITY)==initialDurability&&!player.HasAura(15007),"Rejected spirit request changed health/death/corpse/durability/sickness");
        auto saved=CharacterDatabase.Query(Trinity::StringFormat("SELECT COUNT(*),MIN(corpseGuid) FROM corpse WHERE guid=%u AND corpseType<>0",owner).c_str());
        Require(saved&&saved->Fetch()[0].GetUInt32()==uint32(!allowed&&corpse!=nullptr),"Spirit healer corpse SQL count differs from admission");
        if(!allowed&&corpse)Require(saved->Fetch()[1].GetUInt64()==corpseGuid.GetCounter(),"Rejected spirit request changed saved corpse GUID");
        std::cout<<"SPIRIT_HEALER mode="<<mode<<" race="<<unsigned(race)<<" level="<<unsigned(level)<<" allowed="<<allowed<<std::endl;
        if(player.GetMap()!=&map)ReclaimMove(player,map);
        if(player.GetCorpse())sObjectAccessor->ConvertCorpseForPlayer(player.GetGUID());FlushWrites();
    }
    sWorld->setIntConfig(CONFIG_DEATH_SICKNESS_LEVEL,sicknessLevel);
    std::cout<<"PASS: "<<cases<<" native spirit healer cases cover interaction admission, resurrection, sickness levels, backpack durability and duplicate requests."<<std::endl;
}
