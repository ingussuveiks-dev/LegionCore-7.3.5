#include "DB2Store.h"
#include "DB2Structure.h"
#include "DB2LoadInfo.h"
#include "Config.h"
#include "DatabaseEnv.h"
#include "MySQLThreading.h"
#include <cstdlib>
#include <iostream>
int main(int argc,char**argv)
{
 if(argc!=2)return 1;
 auto config=std::getenv("LEGION_DIALOGUE_CONFIG");std::string error;
 if(!config||!sConfigMgr->LoadInitial(config,{},error))return 2;
 MySQL::Library_Init();
 HotfixDatabase.SetConnectionInfo(sConfigMgr->GetStringDefault("HotfixDatabaseInfo",""),1,1);
 WorldDatabase.SetConnectionInfo(sConfigMgr->GetStringDefault("WorldDatabaseInfo",""),1,1);
 if(HotfixDatabase.Open()||!HotfixDatabase.PrepareStatements()||WorldDatabase.Open()||!WorldDatabase.PrepareStatements())return 3;
 HotfixDatabase.DirectExecute("SET SESSION TRANSACTION READ ONLY");HotfixDatabase.Execute("SET SESSION TRANSACTION READ ONLY");
 WorldDatabase.DirectExecute("SET SESSION TRANSACTION READ ONLY");WorldDatabase.Execute("SET SESSION TRANSACTION READ ONLY");
 DB2Storage<AnimKitEntry> kits("AnimKit.db2",AnimKitLoadInfo::Instance());
 DB2Storage<CharTitlesEntry> titles("CharTitles.db2",CharTitlesLoadInfo::Instance());
 DB2Storage<SpellEffectEntry> effects("SpellEffect.db2",SpellEffectLoadInfo::Instance());
 if(!kits.Load(argv[1],0)||!titles.Load(argv[1],0)||!effects.Load(argv[1],0))return 4;
 kits.LoadFromDB();titles.LoadFromDB();effects.LoadFromDB();
 for(uint32 id:{9038u,9039u,9176u}){auto kit=kits.LookupEntry(id);if(!kit)return 5;std::cout<<"KIT\t"<<id<<'\t'<<kit->OneShotDuration<<'\n';}
 uint32 high=0,negative=0;
 for(auto title:titles){if(title->MaskID>=64)++high;if(title->MaskID<0)++negative;}
 if(!high)return 6;
 std::cout<<"TITLE_MASKS\t"<<high<<'\t'<<negative<<'\n';
 uint32 clone=0;
 for(auto effect:effects)if(effect->SpellID==194981){++clone;std::cout<<"CLONE_EFFECT\t"<<effect->EffectIndex<<'\t'<<effect->Effect<<'\n';}
 if(!clone)return 7;
 auto binding=WorldDatabase.Query("SELECT ScriptName,AIName FROM creature_template WHERE entry=99090");
 if(!binding||binding->GetRowCount()!=1)return 8;
 auto fields=binding->Fetch();std::cout<<"BINDING\t99090\t"<<fields[0].GetString()<<'\t'<<fields[1].GetString()<<'\n';
 if(fields[0].GetString()!="npc_soul_echoes_stalker")return 9;
 WorldDatabase.Close();HotfixDatabase.Close();return 0;
}
