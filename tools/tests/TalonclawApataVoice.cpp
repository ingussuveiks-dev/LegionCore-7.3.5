#include <unordered_set>
#include <Windows.h>
#include "Config.h"
#include "DatabaseEnv.h"
#include "MySQLThreading.h"
#include "DB2Stores.h"
#include "DB2LoadInfo.h"
#include "SharedDefines.h"
#include <fstream>
#include <iostream>
#include <map>
#include <set>
#include <stdexcept>
#include "CreatureTextEntry.inc"

void Require(bool value,char const* message)
{
    if(!value)throw std::runtime_error(message);
}
struct Speaker
{
    uint32 display=66135;
    uint8 gender=GENDER_FEMALE;
    uint32 GetDisplayId() const{return display;}
    uint8 getGender() const{return gender;}
};
uint32 SelectedSound(Speaker* source,CreatureTextEntry const* text,BroadcastTextEntry const* bct,uint32 sound=0)
{
#include "SoundSelection.inc"
    return fSound;
}
int Run(int argc,char** argv)
{
    Require(argc==4,"Expected native data, scoped SQL rows and private configuration");
    std::string error;
    Require(sConfigMgr->LoadInitial(argv[3],{},error),"Private configuration failed");
    MySQL::Library_Init();
    HotfixDatabase.SetConnectionInfo(sConfigMgr->GetStringDefault("HotfixDatabaseInfo",""),1,1);
    Require(!HotfixDatabase.Open()&&HotfixDatabase.PrepareStatements(),"Hotfix pool failed");
    HotfixDatabase.DirectExecute("SET SESSION TRANSACTION READ ONLY");
    HotfixDatabase.Execute("SET SESSION TRANSACTION READ ONLY");
    auto readOnly=HotfixDatabase.Query("SELECT @@session.transaction_read_only");
    Require(readOnly&&readOnly->Fetch()[0].GetBool(),"Hotfix session not read-only");
    Require(sBroadcastTextStore.Load(argv[1],0)&&sCreatureDisplayInfoStore.Load(argv[1],0),"Native text/display data missing");
    sBroadcastTextStore.LoadFromDB();sCreatureDisplayInfoStore.LoadFromDB();
    DB2Storage<SoundKitEntryEntry> files("SoundKitEntry.db2",SoundKitEntryLoadInfo::Instance());
    Require(files.Load(argv[1],0),"Native voice files missing");files.LoadFromDB();
    auto display=sCreatureDisplayInfoStore.LookupEntry(66135);
    Require(display&&display->Gender==GENDER_FEMALE,"Actual Apata display is not female");
    std::map<uint32,uint32> expected{{11,102113},{12,102114},{13,103426},{14,103430}};
    std::ifstream snapshot(argv[2]);Require(bool(snapshot),"Missing scoped SQL rows");
    uint32 count=0,group=0,sqlSound=0,broadcast=0,type=0,hold=0;
    while(snapshot>>group>>sqlSound>>broadcast>>type>>hold){
        Require(expected.count(group)==1&&sqlSound==0&&type==(group==11?14u:12u),"Unexpected speech scope/metadata");
        auto wanted=sBroadcastTextStore.LookupEntry(expected.at(group));
        Require(wanted&&wanted->SoundEntriesID[0]==0&&wanted->SoundEntriesID[1]==62683+group,"Wrong native voice identity");
        auto current=sBroadcastTextStore.LookupEntry(broadcast);
        Speaker actor;
        CreatureTextEntry text{};text.sound=sqlSound;
        auto selected=SelectedSound(&actor,&text,current);
        std::cout<<"SELECTED\t"<<group<<'\t'<<broadcast<<'\t'<<selected<<'\n';
        Require(selected==uint32(wanted->SoundEntriesID[1]),"Native SendText cannot select the original Apata voice");
        Require(SelectedSound(&actor,&text,nullptr)==0&&SelectedSound(&actor,&text,current,123)==123,"Fallback/explicit sound override changed");
        BroadcastTextEntry silent=*current;silent.SoundEntriesID[0]=silent.SoundEntriesID[1]=0;
        text.sound=321;Require(SelectedSound(&actor,&text,&silent)==321,"Silent native broadcast lost SQL sound fallback");text.sound=0;
        Speaker missingDisplay{0,GENDER_FEMALE};Require(SelectedSound(&missingDisplay,&text,current)==selected,"Missing display lost native actor gender fallback");
        auto value=DB2Manager::GetBroadcastTextValue(current,LOCALE_enUS,GENDER_FEMALE);
        Require(value&&value[0],"Native localized female text is empty");
        std::cout<<"TEXT\t"<<group<<'\t'<<value<<'\n';
        uint32 linked=0;
        for(auto file:files)if(file->SoundKitID==selected){
            Require(file->FileDataID&&file->Frequency&&file->Volume>0,"Invalid/inaudible native voice file");
            std::cout<<"FILE\t"<<group<<'\t'<<selected<<'\t'<<file->FileDataID<<'\t'<<uint32(file->Frequency)<<'\t'<<file->Volume<<'\t'<<hold<<'\n';++linked;
        }
        Require(linked==1,"Expected one actual native voice file per line");
        expected.erase(group);++count;
    }
    Require(count==4&&expected.empty(),"Incomplete native voice snapshot");
    HotfixDatabase.Close();
    std::cout<<"PASS: actual typed native text/display/sound data and production SendText sound selector resolve four female Apata voices; localized text accessor, explicit sound override and native fallbacks preserved. Speaker getters controlled; no client playback claim.\n";
    return 0;
}
int main(int argc,char** argv)
{
    SetErrorMode(SEM_FAILCRITICALERRORS|SEM_NOGPFAULTERRORBOX);
    try{return Run(argc,argv);}catch(std::exception const& error){std::cerr<<error.what()<<'\n';return 1;}
}
