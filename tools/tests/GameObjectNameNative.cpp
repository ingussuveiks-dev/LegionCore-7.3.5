// Actual matching ObjectMgr loader, locale fallback and7.3.5 query serializer.
// Response population mirrors HandleQueryGameObject; no socket/auth/client save.
#include <Windows.h>
#include <unordered_set>
#include "Config.h"
#include "DatabaseEnv.h"
#include "MySQLThreading.h"
#include "DB2Stores.h"
#include "SpellMgr.h"
#include "ObjectMgr.h"
#include "GameObjectData.h"
#include "QueryPackets.h"
#include "Log.h"
#include <iostream>
#include <stdexcept>
#include <map>
#include <vector>
#include <cstring>
#include "GameObjectNameRows.inc"
static void Require(bool ok,char const* message){if(!ok)throw std::runtime_error(message);}
static std::string Unhex(char const* text){std::string out;auto value=[](char c){return c<='9'?c-'0':c-'A'+10;};for(size_t i=0;text[i];i+=2){Require(text[i+1]!=0,"Odd name hex");out.push_back(char((value(text[i])<<4)|value(text[i+1])));}return out;}
static WorldPacket Packet(GameObjectTemplate const& info,LocaleConstant locale,bool omitName)
{
    WorldPackets::Query::QueryGameObjectResponse response;response.GameObjectID=info.entry;response.Allow=true;
    auto& stats=response.Stats;stats.DisplayID=info.displayId;stats.IconName=info.IconName;stats.Name[0]=info.name;stats.CastBarCaption=info.castBarCaption;
    for(auto item:info.QuestItems)if(item)stats.QuestItems.push_back(item);
    memcpy(stats.Data,info.raw.data,sizeof(stats.Data));stats.Size=info.size;stats.Type=info.type;stats.UnkString=info.unk1;stats.RequiredLevel=info.RequiredLevel;
    if(auto localized=sObjectMgr->GetGameObjectLocale(info.entry)){ObjectMgr::GetLocaleString(localized->Name,locale,stats.Name[0]);ObjectMgr::GetLocaleString(localized->CastBarCaption,locale,stats.CastBarCaption);ObjectMgr::GetLocaleString(localized->Unk1,locale,stats.UnkString);}
    if(omitName)stats.Name[0].clear();
    WorldPacket wire(*response.Write());wire.rpos(0);
    uint32 id,bytes,type,display;wire>>id;bool allow=wire.ReadBit();wire.ResetBitPos();wire>>bytes>>type>>display;
    Require(id==info.entry&&allow&&bytes==wire.size()-9&&type==info.type&&display==info.displayId,"Native query header/byte length changed");
    for(auto const& expected:stats.Name){std::string name;wire>>name;Require(name==expected,"Native query name bytes differ");}
    for(auto const& expected:{stats.IconName,stats.CastBarCaption,stats.UnkString}){std::string text;wire>>text;Require(text==expected,"Native query secondary text differs");}
    for(auto expected:stats.Data){int32 value;wire>>value;Require(uint32(value)==expected,"Native query gameplay data differs");}
    float scale;uint8 count;wire>>scale>>count;Require(scale==stats.Size&&count==stats.QuestItems.size(),"Native query metadata differs");for(auto expected:stats.QuestItems){int32 item;wire>>item;Require(item==expected,"Native quest item changed");}int32 level;wire>>level;Require(uint32(level)==stats.RequiredLevel&&wire.rpos()==wire.size(),"Native query trailing fields differ");
    return wire;
}
int main(int argc,char** argv){
    SetErrorMode(SEM_FAILCRITICALERRORS|SEM_NOGPFAULTERRORBOX);
    try{
        Require(argc==3,"Expected private config/runtime");std::string error;Require(sConfigMgr->LoadInitial(argv[1],{},error),"Config failed");sLog->Initialize(nullptr);MySQL::Library_Init();
        WorldDatabase.SetConnectionInfo(sConfigMgr->GetStringDefault("WorldDatabaseInfo",""),1,1);HotfixDatabase.SetConnectionInfo(sConfigMgr->GetStringDefault("HotfixDatabaseInfo",""),1,1);
        Require(!WorldDatabase.Open()&&WorldDatabase.PrepareStatements()&&!HotfixDatabase.Open()&&HotfixDatabase.PrepareStatements(),"Native DB pool failed");
        WorldDatabase.DirectExecute("SET SESSION TRANSACTION READ ONLY");WorldDatabase.Execute("SET SESSION TRANSACTION READ ONLY");HotfixDatabase.DirectExecute("SET SESSION TRANSACTION READ ONLY");HotfixDatabase.Execute("SET SESSION TRANSACTION READ ONLY");
        sDB2Manager.LoadStores(argv[2],LOCALE_enUS);sSpellMgr->LoadSpellInfoStore();sObjectMgr->LoadScriptNames();sObjectMgr->LoadPageTexts();sObjectMgr->LoadGameObjectLocales();
#ifndef GO_NAMES_LIVE
        // Single synchronous connection; only this connection-local TEMP table
        // is mutable. Guarded migrations validate real old/new input first.
        WorldDatabase.DirectExecute("SET SESSION TRANSACTION READ WRITE");
        WorldDatabase.DirectExecute("CREATE TEMPORARY TABLE name_seed LIKE gameobject_template");
        WorldDatabase.DirectExecute((std::string("INSERT INTO name_seed SELECT * FROM gameobject_template WHERE entry IN(")+NameIds+")").c_str());
        WorldDatabase.DirectExecute("CREATE TEMPORARY TABLE gameobject_template LIKE name_seed");WorldDatabase.DirectExecute("INSERT INTO gameobject_template SELECT * FROM name_seed");
        for(auto sql:NameMigrations)WorldDatabase.DirectExecute(sql);
        for(auto const& row:NameRows)WorldDatabase.DirectExecute((std::string("UPDATE gameobject_template SET name=CONVERT(0x")+row.before+" USING utf8mb4) WHERE entry="+std::to_string(row.entry)+" AND HEX(name)='"+row.after+"'").c_str());
        WorldDatabase.DirectExecute("SET SESSION TRANSACTION READ ONLY");
#endif
        std::map<std::pair<uint32,uint8>,std::vector<uint8>> original;
        for(bool repaired:{false,true}){
#ifndef GO_NAMES_LIVE
            if(repaired){WorldDatabase.DirectExecute("SET SESSION TRANSACTION READ WRITE");for(auto sql:NameMigrations)WorldDatabase.DirectExecute(sql);WorldDatabase.DirectExecute("SET SESSION TRANSACTION READ ONLY");}
#endif
            sObjectMgr->LoadGameObjectTemplate();
            for(auto const& row:NameRows){auto info=sObjectMgr->GetGameObjectTemplate(row.entry);Require(info,"Native object missing");
#ifdef GO_NAMES_LIVE
                auto expected=Unhex(row.after);
#else
                auto expected=Unhex(repaired?row.after:row.before);
#endif
                Require(info->name==expected,"Native loader name differs from exact declared bytes");
                for(uint8 locale=0;locale<TOTAL_LOCALES;++locale){auto wire=Packet(*info,LocaleConstant(locale),false);auto normalized=Packet(*info,LocaleConstant(locale),true);auto key=std::make_pair(row.entry,locale);std::vector<uint8> bytes(normalized.contents(),normalized.contents()+normalized.size());if(!repaired)original[key]=bytes;else Require(original.at(key)==bytes,"Name repair changed other query fields/locale behavior");}
                if(repaired)std::cout<<"CASE native name="<<row.entry<<" exact UTF8/locale/packet PASS\n";
            }
        }
        auto ro=WorldDatabase.Query("SELECT @@session.transaction_read_only");Require(ro&&ro->Fetch()[0].GetBool(),"Native final session not read-only");HotfixDatabase.Close();WorldDatabase.Close();
#ifdef GO_NAMES_LIVE
        std::cout<<"PASS100 native ObjectMgr names, actual live UTF8 bytes, all locale fallbacks and actual7.3.5 query serializer; no authenticated client/cache rendering claim\n";
#else
        std::cout<<"PASS100 native ObjectMgr names, paired old/repaired input, all locale fallbacks and actual7.3.5 query serializer; no authenticated client/cache rendering claim\n";
#endif
        return 0;
    }catch(std::exception const& e){std::cerr<<e.what()<<'\n';return 1;}
}
