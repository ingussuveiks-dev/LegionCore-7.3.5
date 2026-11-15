// Actual native loot-generation branches, preserving personal/ordinary routing.
// Controlled in-memory actors; no login, inventory grant or character save.
#include "NativeLootLink.inc"
#include "Opcodes.h"
template<class Tag,typename Tag::Type member>struct Accessor{friend typename Tag::Type Access(Tag){return member;}};
struct NameTag{using Type=std::string LootStore::*;friend Type Access(NameTag);};template struct Accessor<NameTag,&LootStore::m_name>;
struct LoadTag{using Type=uint32(LootStore::*)();friend Type Access(LoadTag);};template struct Accessor<LoadTag,&LootStore::LoadLootTable>;
static void Init(int argc,char** argv){
 Require(argc==3,"Expected config/runtime");std::string error;Require(sConfigMgr->LoadInitial(argv[1],{},error),"Config failed");sLog->Initialize(nullptr);MySQL::Library_Init();
 WorldDatabase.SetConnectionInfo(sConfigMgr->GetStringDefault("WorldDatabaseInfo",""),1,1);HotfixDatabase.SetConnectionInfo(sConfigMgr->GetStringDefault("HotfixDatabaseInfo",""),1,1);LoginDatabase.SetConnectionInfo(sConfigMgr->GetStringDefault("LoginDatabaseInfo",""),1,1);Require(!WorldDatabase.Open()&&WorldDatabase.PrepareStatements()&&!HotfixDatabase.Open()&&HotfixDatabase.PrepareStatements()&&!LoginDatabase.Open()&&LoginDatabase.PrepareStatements(),"Pools failed");
 for(auto pool:{&WorldDatabase})pool->Execute("SET SESSION TRANSACTION READ ONLY");for(auto pool:{&HotfixDatabase}){pool->DirectExecute("SET SESSION TRANSACTION READ ONLY");pool->Execute("SET SESSION TRANSACTION READ ONLY");}LoginDatabase.DirectExecute("SET SESSION TRANSACTION READ ONLY");LoginDatabase.Execute("SET SESSION TRANSACTION READ ONLY");
 for(auto const& s:SetupSql)WorldDatabase.DirectExecute(s.c_str());
#ifndef LOOT_LINK_LIVE
 for(auto const& s:Migrations)WorldDatabase.DirectExecute(s.c_str());for(auto const& s:ResetSql)WorldDatabase.DirectExecute(s.c_str());
#endif
 WorldDatabase.DirectExecute("SET SESSION TRANSACTION READ ONLY");sDB2Manager.LoadStores(argv[2],LOCALE_enUS);LoadGameTables(argv[2]);sSpellMgr->LoadSpellInfoStore();sObjectMgr->LoadScriptNames();sObjectMgr->LoadCreatureModelInfo();sObjectMgr->LoadWDBCreatureTemplates();sObjectMgr->LoadCreatureClassLevelStats();sObjectMgr->LoadCreatureScalingData();sObjectMgr->LoadCreatureDifficultyStat();sObjectMgr->LoadItemTemplates();sObjectMgr->LoadItemTemplateAddon();
 auto& name=LootTemplates_Creature.*Access(NameTag{});auto old=name;name+=" WHERE Entry IN("+std::string(EntryIds)+")";auto count=(LootTemplates_Creature.*Access(LoadTag{}))();name=old;Require(count==LootRowCount,"Authored native loot rows failed validation");for(auto e:Entries)Require(LootTemplates_Creature.HaveLootFor(e),"Native source missing");
 opcodeTable.Initialize();AIRegistry::Initialize();UnitAI::FillAISpellInfo();sWorld->LoadConfigSettings();for(uint32 i=0;i<MAX_RATES;++i)sWorld->setRate(Rates(i),1.0f);HashMapHolder<Creature>::SetSize(20000);HashMapHolder<Player>::SetSize(1000);HashMapHolder<Pet>::SetSize(1000);
 dynamic_cast<VMAP::VMapManager2*>(VMAP::VMapFactory::createOrGetVMapManager())->InitializeThreadUnsafe({{1220,{}}});MMAP::MMapFactory::createOrGetMMapManager()->InitializeThreadUnsafe({{1220,{}}});
}
int main(int argc,char** argv){SetErrorMode(SEM_FAILCRITICALERRORS|SEM_NOGPFAULTERRORBOX);try{Init(argc,argv);uint32 guid=1000;
 for(bool fixed:{false,true}){
#ifndef LOOT_LINK_LIVE
  if(fixed){WorldDatabase.DirectExecute("SET SESSION TRANSACTION READ WRITE");for(auto const& s:Migrations)WorldDatabase.DirectExecute(s.c_str());WorldDatabase.DirectExecute("SET SESSION TRANSACTION READ ONLY");}
#endif
  sObjectMgr->LoadCreatureTemplates();
  // WorldSession destructor performs an account UPDATE. These two socketless
  // sessions remain unowned until process exit, as in LostArmyLootNative.
  auto session=new WorldSession(0,"Loot link readonly probe",{},SEC_PLAYER,6,0,"Win",LOCALE_enUS,0,false,AT_AUTH_FLAG_NONE,{});auto other=new WorldSession(0,"Loot link bystander",{},SEC_PLAYER,6,0,"Win",LOCALE_enUS,0,false,AT_AUTH_FLAG_NONE,{});
  {Map map(1220,300,9999970,DIFFICULTY_NONE);NativePlayer owner(session,map,101,{5478.0f,470.0f,150.0f,0}),bystander(other,map,102,owner.GetPosition());map.GetZoneAndAreaId(owner.m_zoneId,owner.m_areaId,owner.GetPositionX(),owner.GetPositionY(),owner.GetPositionZ());map.GetZoneAndAreaId(bystander.m_zoneId,bystander.m_areaId,bystander.GetPositionX(),bystander.GetPositionY(),bystander.GetPositionZ());CreditPet pet(owner,map);CreditGuardian guardian(owner,map);
   for(uint8 race:{uint8(RACE_HUMAN),uint8(RACE_ORC)}){owner.SetByteValue(UNIT_FIELD_BYTES_0,0,race);owner.setFactionForRace(race);
    for(Unit* killer:{static_cast<Unit*>(&owner),static_cast<Unit*>(&pet),static_cast<Unit*>(&guardian)})for(auto entry:Entries){Actor victim(&map,entry,guid++,owner.GetPosition());Require(map.AddToMap(static_cast<Creature*>(&victim)),"Victim admission failed");auto looter=killer->GetCharmerOrOwnerPlayerOrPlayerItself();Require(looter==&owner,"Actual pet/guardian owner resolution lost");victim.SetLootRecipient(looter);victim.GetSaveThreatList()->insert(looter->GetGUID());Require(!victim.isWorldBoss()&&!victim.GetTrackingQuestID(),"Unexpected boss/autostore/cooldown branch");
#ifdef LOOT_LINK_LIVE
     bool repaired=true;
#else
     bool repaired=fixed;
#endif
     Require(victim.GetCreatureTemplate()->lootid==(repaired?entry:0),"Native template lootid differs");if(victim.IsPersonal())killer->GeneratePersonalLoot(&victim,looter);else killer->GenerateLoot(&victim,looter);
     Require((victim.loot.GetLootOwner()==looter)==repaired,"Native generation did not change from missing link to authored source");auto it=owner.personalLoot.find(victim.GetGUID());if(!repaired)Require(it==owner.personalLoot.end(),"Baseline unexpectedly generated personal loot");else if(victim.IsPersonal()){if(it==owner.personalLoot.end()||it->second.GetLootOwner()!=looter)std::cerr<<"context entry="<<entry<<" playerzone="<<owner.GetCurrentZoneID()<<" actorzone="<<victim.GetCurrentZoneID()<<" personal="<<victim.IsPersonal()<<" contact="<<looter->CanContact()<<" threat="<<victim.GetSaveThreatList()->size()<<" found="<<(it!=owner.personalLoot.end())<<"\n";Require(it!=owner.personalLoot.end()&&it->second.GetLootOwner()==looter,"Personal native loot owner differs");}else if(it!=owner.personalLoot.end())Require(it->second.GetLootOwner()==looter,"Ordinary personal loot owner differs");if(fixed&&race==RACE_HUMAN&&killer==&owner)std::cout<<"LOOT "<<entry<<" personal="<<victim.IsPersonal()<<" eligible-personal="<<(it!=owner.personalLoot.end())<<" PASS\n";Require(!bystander.personalLoot.count(victim.GetGUID()),"Untapped unrelated player gained loot");owner.personalLoot.erase(victim.GetGUID());
    }
   }
  }
 }
 auto ro=WorldDatabase.Query("SELECT @@session.transaction_read_only");Require(ro&&ro->Fetch()[0].GetBool(),"World not readonly");LoginDatabase.Close();HotfixDatabase.Close();WorldDatabase.Close();std::cout<<"PASS13 native baseline/fixed loot routing,13 authored tables,2 factions/player/pet/guardian owners and unrelated bystander exclusion; no kill/client/drop-rate/inventory claim\n";return 0;
}catch(std::exception const& e){std::cerr<<e.what()<<'\n';return 1;}}
