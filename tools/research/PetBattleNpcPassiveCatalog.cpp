// Matching Legion26972 stores and read-only native NPC team references.
// Metadata only; no gameplay dispatch, character writes or server process.
#include <unordered_set>
#include <array>
#include "Common.h"
#include "SharedDefines.h"
#include "UnitDefines.h"
#include "DB2Stores.h"
#include "DB2LoadInfo.h"
#include "Config.h"
#include "DatabaseEnv.h"
#include "MySQLThreading.h"
#include "ObjectGuid.h"
#include "Position.h"
#include "Timer.h"
#include "PetBattle.h"
#include "PetBattleAbilityEffect.h"
#include "GameTables.h"
#include "Log.h"
#include <cstdlib>
#include <iostream>
#include <stdexcept>
static_assert(sizeof(PetBattleAbilityEffect)==0x50,"Bend Time owner context must reuse existing execution padding");
static void Require(bool value,char const*message){if(!value)throw std::runtime_error(message);}
template<class T>static void Load(T&store,char const*path){Require(store.Load(path,0),"Matching native DB2 missing");store.LoadFromDB();}
static void Cases(std::string const&){
 auto state=sBattlePetStateStore.LookupEntry(183);std::cout<<"NATIVE183 present="<<(state!=nullptr);if(state)std::cout<<" name="<<state->LuaName->Str[0]<<" flags="<<state->Flags;std::cout<<"\n"; for(auto st:sBattlePetSpeciesStateStore)if(st->BattlePetStateID==183){auto passive=sBattlePetAbilityStore.LookupEntry(st->Value);std::cout<<"NATIVE_PASSIVE183 species="<<st->SpeciesID<<" ability="<<st->Value<<" name="<<(passive?passive->Name->Str[0]:"MISSING")<<" flags="<<(passive?uint32(passive->Flags):0)<<"\n";} for(uint32 id:{1389u,1595u}){
  auto a=sBattlePetAbilityStore.LookupEntry(id);Require(a,"Matching remaining type ability missing");std::cout<<"ABILITY "<<id<<" name="<<a->Name->Str[0]<<" flags="<<uint32(a->Flags)<<"\n";
  for(auto e:sBattlePetAbilityEffectStore){bool used=e->AuraBattlePetAbilityID==id;for(auto p:e->Param)used=used||p==id;if(used){auto t=sBattlePetAbilityTurnStore.LookupEntry(e->BattlePetAbilityTurnID);std::cout<<"EFFECT_REFERENCE "<<id<<" effect="<<e->ID<<" root="<<(t?t->BattlePetAbilityID:0)<<" property="<<e->BattlePetEffectPropertiesID<<" aura="<<e->AuraBattlePetAbilityID<<" params=";for(auto p:e->Param)std::cout<<p<<",";std::cout<<"\n";}}
  for(auto e:sBattlePetAbilityStateStore)if(uint32(e->Value)==id)std::cout<<"ABILITY_STATE_REFERENCE "<<id<<" aura="<<e->BattlePetAbilityID<<" state="<<uint32(e->BattlePetStateID)<<"\n";
  for(auto e:sBattlePetSpeciesStateStore)if(e->Value==id)std::cout<<"SPECIES_STATE_REFERENCE "<<id<<" species="<<e->SpeciesID<<" state="<<uint32(e->BattlePetStateID)<<"\n";
 }
 Require(!WorldDatabase.Open()&&WorldDatabase.PrepareStatements(),"Read-only native world connection failed");WorldDatabase.DirectExecute("SET SESSION TRANSACTION READ ONLY");
 auto teams=WorldDatabase.Query("SELECT t.NpcID,c.Name1,t.Specie,t.Ability1,t.Ability2,t.Ability3 FROM battlepet_npc_team_member t LEFT JOIN creature_template_wdb c ON c.Entry=t.NpcID WHERE t.Ability1 IN(1389,1595) OR t.Ability2 IN(1389,1595) OR t.Ability3 IN(1389,1595) OR t.Specie=1504 OR t.NpcID=99878");uint32 count=0;if(teams)do{auto f=teams->Fetch();std::cout<<"WORLD_TEAM npc="<<f[0].GetUInt32()<<" name="<<f[1].GetString()<<" species="<<f[2].GetUInt32()<<" abilities="<<f[3].GetUInt32()<<","<<f[4].GetUInt32()<<","<<f[5].GetUInt32()<<"\n";++count;}while(teams->NextRow());std::cout<<"WORLD_TEAM_COUNT "<<count<<"\n";
 auto actors=WorldDatabase.Query("SELECT w.Entry,w.Name1,c.ScriptName FROM creature_template_wdb w LEFT JOIN creature_template c ON c.entry=w.Entry WHERE w.Name1 LIKE '%Ominitron%' OR w.Name1 LIKE '%Omnitron%' OR w.Name1 LIKE '%Roboflauge%'");if(actors)do{auto f=actors->Fetch();std::cout<<"WORLD_ACTOR "<<f[0].GetUInt32()<<" name="<<f[1].GetString()<<" script="<<f[2].GetString()<<"\n";}while(actors->NextRow());
 WorldDatabase.Close();std::cout<<"WORLD_READ_EOF\n";
}
int main(int argc,char**argv){try{
 Require(argc==2,"Native26972 data path required");std::string error;auto config=std::getenv("LEGION_DIALOGUE_CONFIG");Require(config&&sConfigMgr->LoadInitial(config,{},error),"Native test configuration unavailable");
 sLog->Initialize(nullptr);std::cout<<std::unitbuf;std::cout<<"STAGE hotfix open\n";
 MySQL::Library_Init();HotfixDatabase.SetConnectionInfo(sConfigMgr->GetStringDefault("HotfixDatabaseInfo",""),1,1);Require(!HotfixDatabase.Open()&&HotfixDatabase.PrepareStatements(),"Native readonly hotfix connection failed");HotfixDatabase.DirectExecute("SET SESSION TRANSACTION READ ONLY");
 DB2Storage<BattlePetVisualEntry> visuals("BattlePetVisual.db2",BattlePetVisualLoadInfo::Instance());
 WorldDatabase.SetConnectionInfo(sConfigMgr->GetStringDefault("WorldDatabaseInfo",""),1,1);std::cout<<"STAGE species states\n";Load(sBattlePetSpeciesStateStore,argv[1]);std::cout<<"STAGE ordinary stores\n";Load(sBattlePetAbilityStore,argv[1]);Load(sBattlePetAbilityTurnStore,argv[1]);Load(sBattlePetAbilityEffectStore,argv[1]);Load(sBattlePetAbilityStateStore,argv[1]);Load(sBattlePetStateStore,argv[1]);Load(sBattlePetSpeciesStore,argv[1]);Load(visuals,argv[1]);
 Cases("");
 HotfixDatabase.Close();MySQL::Library_End();std::cout<<"PASS actual native remaining NPC producer research: "<<"read-only metadata and world records; not repaired gameplay"<<"; matching26972 stores and actual read-only World queries; no gameplay or character changes\n";return 0;
 }catch(std::exception const&e){std::cerr<<e.what()<<"\n";return 1;}}
