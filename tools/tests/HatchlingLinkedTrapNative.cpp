// Native cold/warm template loader and actual chest contracts. No Player,
// character session, spawned object or daily-quest traversal is simulated.
#include <unordered_set>
#include <Windows.h>
#include "DB2Stores.h"
#include "Config.h"
#include "DatabaseEnv.h"
#include "MySQLThreading.h"
#include "SpellMgr.h"
#include "ObjectMgr.h"
#include "GameObject.h"
#include "Log.h"
#include <cstdlib>
#include <filesystem>
#include <iostream>
#include <stdexcept>
static void Require(bool value,char const* message){if(!value)throw std::runtime_error(message);}
int main(int argc,char**argv){try{
 SetErrorMode(SEM_FAILCRITICALERRORS|SEM_NOGPFAULTERRORBOX);
 Require(argc==2,"Expected matching dbc directory");auto config=std::getenv("LEGION_DIALOGUE_CONFIG");std::string error;Require(config&&sConfigMgr->LoadInitial(config,{},error),"Private native config unavailable");sLog->Initialize(nullptr);MySQL::Library_Init();
 WorldDatabase.SetConnectionInfo(sConfigMgr->GetStringDefault("WorldDatabaseInfo",""),1,1);HotfixDatabase.SetConnectionInfo(sConfigMgr->GetStringDefault("HotfixDatabaseInfo",""),1,1);Require(!WorldDatabase.Open()&&WorldDatabase.PrepareStatements()&&!HotfixDatabase.Open()&&HotfixDatabase.PrepareStatements(),"Native pools unavailable");
 // Private synchronous TEMP shadow only; verify it before any UPDATE. MariaDB
 // disallows its creation in read-only mode; execution becomes read-only below.
#if !defined(HATCHLING_LINKED_LIVE) || defined(HATCHLING_LINKED_BASELINE)
 WorldDatabase.DirectExecute("CREATE TEMPORARY TABLE hatchling_seed LIKE gameobject_template");WorldDatabase.DirectExecute("INSERT INTO hatchling_seed SELECT * FROM gameobject_template");WorldDatabase.DirectExecute("CREATE TEMPORARY TABLE gameobject_template LIKE hatchling_seed");
 auto shadow=WorldDatabase.Query("SHOW CREATE TABLE gameobject_template");Require(shadow&&shadow->Fetch()[1].GetString().find("CREATE TEMPORARY TABLE")!=std::string::npos,"Refuse persistent template writes");WorldDatabase.DirectExecute("INSERT INTO gameobject_template SELECT * FROM hatchling_seed");
#ifdef HATCHLING_LINKED_BASELINE
 WorldDatabase.DirectExecute("UPDATE gameobject_template SET Data7=214863 WHERE entry=214862 AND type=3");
#else
 for(uint32 pass=0;pass<2;++pass)for(auto command:{
#include "HatchlingMigration.inc"
 })WorldDatabase.DirectExecute(command);
#endif
#endif
 WorldDatabase.DirectExecute("SET SESSION TRANSACTION READ ONLY");WorldDatabase.Execute("SET SESSION TRANSACTION READ ONLY");HotfixDatabase.DirectExecute("SET SESSION TRANSACTION READ ONLY");HotfixDatabase.Execute("SET SESSION TRANSACTION READ ONLY");auto readonly=WorldDatabase.Query("SELECT @@session.transaction_read_only");Require(readonly&&readonly->Fetch()[0].GetBool(),"Native runtime is not readonly");
 auto dbc=std::filesystem::path(argv[1]).lexically_normal();if(dbc.filename().empty())dbc=dbc.parent_path();auto runtime=dbc.parent_path().parent_path().string()+'/';sDB2Manager.LoadStores(runtime,LOCALE_enUS);sSpellMgr->LoadSpellInfoStore();sObjectMgr->LoadScriptNames();sObjectMgr->LoadPageTexts();
 Require(!sGameObjectsStore.LookupEntry(214862)&&!sGameObjectsStore.LookupEntry(214863),"New matching client template data requires review");
 for(uint32 pass=0;pass<2;++pass){sObjectMgr->LoadGameObjectTemplate();auto small=sObjectMgr->GetGameObjectTemplate(214862),large=sObjectMgr->GetGameObjectTemplate(214863);Require(small&&large&&small->type==GAMEOBJECT_TYPE_CHEST&&large->type==GAMEOBJECT_TYPE_CHEST&&small->displayId==9572&&large->displayId==9572,"Actual chest variants changed");Require(small->size==0.5f&&large->size==1.f&&small->chest.open==1852&&large->chest.open==1852&&small->chest.chestLoot==43369&&large->chest.chestLoot==43370&&small->chest.consumable==1&&large->chest.consumable==1&&small->chest.chestRestockTime==30&&large->chest.chestRestockTime==30&&small->QuestItems[0]==89052&&small->chest.openTextID==66863&&!large->chest.linkedTrap,"Native loot/lock/size/quest/open-text contracts changed");
#ifdef HATCHLING_LINKED_BASELINE
 Require(small->chest.linkedTrap==214863,"Pinned invalid link missing");
#else
 Require(!small->chest.linkedTrap,"Invalid non-trap link retained");
#endif
 std::cout<<"CASE "<<pass<<" native cold/warm template loader; actual two chest types/loot/size/lock/consumption/quest/open-text retained\n";}
 WorldDatabase.Close();HotfixDatabase.Close();MySQL::Library_End();
#ifdef HATCHLING_LINKED_BASELINE
 std::cout<<"COUNTERFACTUAL old Data7=214863: second native loader resolves the preexisting chest target and must emit the exact non-trap error; first cold forward reference was not proof of valid data. Pair repaired clean run\n";
#else
 std::cout<<"PASS native cold/warm loaders: corrected214862 non-trap relation, both real chest/loot/quest contracts retained. No check bypass/type conversion/loot grant. Actual31704/31716 client collection remains separate\n";
#endif
 return 0;
}catch(std::exception const&e){std::cerr<<e.what()<<'\n';return 1;}}
