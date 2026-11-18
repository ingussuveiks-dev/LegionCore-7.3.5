// Actual matching26972 DB2 manager and readonly Hotfix overlay, no World/character writes.
#include <unordered_set>
#include "DB2Stores.h"
#include "DB2Store.h"
#include "DB2Structure.h"
#include "DB2LoadInfo.h"
#include "Config.h"
#include "DatabaseEnv.h"
#include "MySQLThreading.h"
#include "MovementPackets.h"
#include "SpellMgr.h"
#include "SpellInfo.h"
#include <cstdlib>
#include <filesystem>
#include <iostream>
#include <set>

int main(int argc,char** argv)
{
    if(argc!=2)return 1;
    auto config=std::getenv("LEGION_DIALOGUE_CONFIG");std::string error;
    if(!config||!sConfigMgr->LoadInitial(config,{},error))return 2;
    MySQL::Library_Init();
    HotfixDatabase.SetConnectionInfo(sConfigMgr->GetStringDefault("HotfixDatabaseInfo",""),1,1);
    if(HotfixDatabase.Open()||!HotfixDatabase.PrepareStatements())return 3;
    HotfixDatabase.DirectExecute("SET SESSION TRANSACTION READ ONLY");
    HotfixDatabase.Execute("SET SESSION TRANSACTION READ ONLY");
    auto directory=std::filesystem::path(argv[1]).lexically_normal();
    if(directory.filename().empty())directory=directory.parent_path();
    auto runtime=directory.parent_path().parent_path().string()+'/';
    sDB2Manager.LoadStores(runtime,LOCALE_enUS);
    sSpellMgr->LoadSpellInfoStore();
    auto modes=sDB2Manager.GetAllMapsDifficultyes();
    std::set<uint32> maps;
    for(auto map:sMapStore)
        if(map->ExpansionID==6&&map->InstanceType==MAP_INSTANCE)
        {
            maps.insert(map->ID);
            std::cout<<"MAP\t"<<map->ID<<'\t'<<map->MapName->Str[LOCALE_enUS]<<'\t'
                <<unsigned(map->MaxPlayers)<<'\t'<<map->AreaTableID<<'\n';
            for(auto const& pair:modes[map->ID])
            {
                auto row=pair.second;auto difficulty=sDifficultyStore.LookupEntry(row->DifficultyID);
                if(!difficulty)return 4;
                std::cout<<"MODE\t"<<map->ID<<'\t'<<row->ID<<'\t'<<unsigned(row->DifficultyID)
                    <<'\t'<<difficulty->Name->Str[LOCALE_enUS]<<'\t'<<unsigned(row->MaxPlayers)
                    <<'\t'<<unsigned(difficulty->FallbackDifficultyID)<<'\t'<<unsigned(row->Flags)<<'\n';
            }
        }
    for(auto row:sMapChallengeModeStore)if(maps.count(row->MapID))
        std::cout<<"CHALLENGE\t"<<row->MapID<<'\t'<<row->ID<<'\t'<<row->Name->Str[LOCALE_enUS]
            <<'\t'<<row->CriteriaCount[0]<<'\t'<<row->CriteriaCount[1]<<'\t'<<row->CriteriaCount[2]<<'\n';
    for(auto row:sLfgDungeonsStore)if(maps.count(row->MapID))
        std::cout<<"LFG\t"<<row->MapID<<'\t'<<row->ID<<'\t'<<unsigned(row->DifficultyID)
            <<'\t'<<unsigned(row->MinLevel)<<'\t'<<row->MaxLevel<<'\t'<<row->FinalEncounterID
            <<'\t'<<row->Name->Str[LOCALE_enUS]<<'\n';
    for(auto row:sDungeonEncounterStore)if(maps.count(row->MapID))
        std::cout<<"BOSS\t"<<row->MapID<<'\t'<<row->ID<<'\t'<<unsigned(row->DifficultyID)
            <<'\t'<<row->OrderIndex<<'\t'<<row->Name->Str[LOCALE_enUS]<<'\n';
    DB2Storage<JournalEncounterSectionEntry> sections("JournalEncounterSection.db2",JournalEncounterSectionLoadInfo::Instance());
    if(!sections.Load(argv[1],LOCALE_enUS))return 5;sections.LoadFromDB();
    std::set<uint32> harbaron;
    for(auto row:sJournalEncounterStore)
        if(std::string(row->Name->Str[LOCALE_enUS])=="Harbaron")harbaron.insert(row->ID);
    for(auto row:sections)if(row->SpellID==194325||harbaron.count(row->JournalEncounterID))
        std::cout<<"FRAGMENT_JOURNAL\t"<<row->ID<<'\t'<<unsigned(row->DifficultyMask)<<'\t'
            <<row->Title->Str[LOCALE_enUS]<<'\t'<<row->BodyText->Str[LOCALE_enUS]<<'\n';
    for(uint32 id:{194325u,194327u})if(auto entry=sSpellStore.LookupEntry(id))
        std::cout<<"FRAGMENT_TEXT\t"<<id<<'\t'<<entry->Description->Str[LOCALE_enUS]<<'\n';
    auto spell=sSpellMgr->GetSpellInfo(194325);if(!spell)return 6;
    for(uint32 difficulty:{1u,2u,23u,8u})
    {
        auto effect=spell->GetEffect(EFFECT_0,difficulty);
        std::cout<<"FRAGMENT_EFFECT\t"<<difficulty<<'\t'<<effect->Effect<<'\t'<<effect->BasePoints
            <<'\t'<<effect->TriggerSpell<<'\t'<<effect->MiscValue<<'\n';
    }
    std::cout<<"PASS matching native Legion dungeon mode catalog maps="<<maps.size()<<"; no gameplay traversal claim\n";
    HotfixDatabase.Close();MySQL::Library_End();
}
