#include "DB2Store.h"
#include "DB2Structure.h"
#include "DB2LoadInfo.h"
#include "Config.h"
#include "DatabaseEnv.h"
#include "MySQLThreading.h"
#include <cstdlib>
#include <algorithm>
#include <cctype>
#include <iostream>
#include <string>
int main(int argc,char**argv)
{
    if(argc!=2)return 1;
    std::string error;auto config=std::getenv("LEGION_DIALOGUE_CONFIG");
    if(!config||!sConfigMgr->LoadInitial(config,{},error))return 4;
    MySQL::Library_Init();
    HotfixDatabase.SetConnectionInfo(sConfigMgr->GetStringDefault("HotfixDatabaseInfo",""),1,1);
    if(HotfixDatabase.Open()||!HotfixDatabase.PrepareStatements())return 5;
    HotfixDatabase.DirectExecute("SET SESSION TRANSACTION READ ONLY");
    HotfixDatabase.Execute("SET SESSION TRANSACTION READ ONLY");
    DB2Storage<SceneScriptTextEntry> scenes("SceneScriptText.db2",SceneScriptTextLoadInfo::Instance());
    DB2Storage<BroadcastTextEntry> texts("BroadcastText.db2",BroadcastTextLoadInfo::Instance());
    DB2Storage<SummonPropertiesEntry> props("SummonProperties.db2",SummonPropertiesLoadInfo::Instance());
    DB2Storage<VehicleEntry> vehicles("Vehicle.db2",VehicleLoadInfo::Instance());
    DB2Storage<VehicleSeatEntry> seats("VehicleSeat.db2",VehicleSeatLoadInfo::Instance());
    DB2Storage<SpellEntry> spells("Spell.db2",SpellLoadInfo::Instance());
    if(!scenes.Load(argv[1],0)||!texts.Load(argv[1],0)||!props.Load(argv[1],0)||!vehicles.Load(argv[1],0)||!seats.Load(argv[1],0)||!spells.Load(argv[1],0))return 2;
    texts.LoadFromDB();props.LoadFromDB();vehicles.LoadFromDB();seats.LoadFromDB();spells.LoadFromDB();
    uint32 count=0;
    for(auto scene:scenes)
    {
        ++count;
        std::string name=scene->Name->Str[0],text=scene->Script->Str[0],lower=name+'\n'+text;
        std::transform(lower.begin(),lower.end(),lower.begin(),[](unsigned char c){return char(std::tolower(c));});
        bool match=false;
        for(auto term:{"100475","100481","100972","198964","off to adventure","juicy fish","lili","li li"})
            match|=lower.find(term)!=std::string::npos;
        if(match)std::cout<<"SCENE\t"<<scene->ID<<'\t'<<name<<"\nBEGIN\n"<<text<<"\nEND\n";
    }
    for(uint32 id=103325;id<=103350;++id)
        if(auto t=texts.LookupEntry(id))std::cout<<"BROADCAST\t"<<id<<'\t'<<t->Text->Str[0]<<'\t'<<t->Text1->Str[0]<<'\t'<<t->SoundEntriesID[0]<<'\t'<<t->SoundEntriesID[1]<<'\t'<<t->EmotesID<<'\t'<<t->EmoteID[0]<<'\t'<<t->EmoteDelay[0]<<'\n';
    for(auto t:texts)if(std::string(t->Text1->Str[0]).find("No, you get one")!=std::string::npos)std::cout<<"FINAL_LINE\t"<<t->ID<<'\t'<<t->Text1->Str[0]<<'\t'<<t->SoundEntriesID[0]<<'\t'<<t->SoundEntriesID[1]<<'\n';
    for(auto id:{3545u,3706u})if(auto p=props.LookupEntry(id))std::cout<<"PROPERTIES\t"<<id<<'\t'<<p->Control<<'\t'<<p->Title<<'\t'<<p->Slot<<'\t'<<p->Flags<<'\n';
    if(auto v=vehicles.LookupEntry(4518))for(uint32 i=0;i<8;++i)if(auto s=seats.LookupEntry(v->SeatID[i]))std::cout<<"SEAT\t"<<i<<'\t'<<s->ID<<'\t'<<s->Flags<<'\t'<<s->FlagsB<<'\n';
    for(auto s:spells){std::string name=s->Name->Str[0];if((s->ID>=198000&&s->ID<=203000)&&(name.find("Crane")!=std::string::npos||name.find("Li Li")!=std::string::npos||name.find("Fish")!=std::string::npos||name.find("Adventure")!=std::string::npos))std::cout<<"SPELL_NAME\t"<<s->ID<<'\t'<<name<<'\n';}
    HotfixDatabase.Close();std::cout<<"COMPLETE\t"<<count<<'\n';return count==5969?0:3;
}
