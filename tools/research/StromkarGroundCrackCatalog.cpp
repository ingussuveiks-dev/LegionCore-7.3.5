#include "DB2Store.h"
#include "DB2LoadInfo.h"
#include "DB2Meta.h"
#include "StromkarGroundCrackData.h"
#include "ByteBuffer.h"
#include "Config.h"
#include "DatabaseEnv.h"
#include "MySQLThreading.h"
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <map>
#include <vector>

template<class T>std::map<uint32,std::vector<uint8>> Snapshot(DB2Storage<T>& store)
{
    std::map<uint32,std::vector<uint8>> rows;
    for(auto p:store){auto b=reinterpret_cast<uint8 const*>(p);rows[p->ID]={b,b+sizeof(T)};}
    return rows;
}
template<class T>bool Unchanged(DB2Storage<T>& store,std::map<uint32,std::vector<uint8>> const& before)
{
    for(auto const& r:before){auto p=store.LookupEntry(r.first);if(!p||std::memcmp(p,r.second.data(),sizeof(T)))return false;}
    return true;
}
int main(int argc,char**argv)
{
    if(argc!=2)return 1;
    std::string error;auto config=std::getenv("LEGION_DIALOGUE_CONFIG");
    if(!config||!sConfigMgr->LoadInitial(config,{},error))return 2;
    MySQL::Library_Init();HotfixDatabase.SetConnectionInfo(sConfigMgr->GetStringDefault("HotfixDatabaseInfo",""),1,1);
    if(HotfixDatabase.Open()||!HotfixDatabase.PrepareStatements())return 3;
    HotfixDatabase.DirectExecute("SET SESSION TRANSACTION READ ONLY");
    DB2Storage<DecalPropertiesEntry> decals("DecalProperties.db2",DecalPropertiesLoadInfo::Instance());
    DB2Storage<TextureBlendSetEntry> blends("TextureBlendSet.db2",TextureBlendSetLoadInfo::Instance());
    if(sizeof(DecalPropertiesEntry)!=62||sizeof(TextureBlendSetEntry)!=84||
       sizeof(DecalPropertiesEntry)!=DecalPropertiesMeta::Instance()->GetRecordSize()||
       sizeof(TextureBlendSetEntry)!=TextureBlendSetMeta::Instance()->GetRecordSize()||
       !decals.Load(argv[1],0)||!blends.Load(argv[1],0))return 4;
    if(decals.GetTableHash()!=3350717493u||blends.GetTableHash()!=2512539351u)return 5;
    auto originalDecals=Snapshot(decals);auto originalBlends=Snapshot(blends);
    auto nativeDecal=decals.LookupEntry(24);auto nativeBlend=blends.LookupEntry(21);
    if(!nativeDecal||!nativeBlend)return 6;
    auto d=*nativeDecal;auto t=*nativeBlend;
    d.ID=t.ID=10000;d.TopTextureBlendSetID=d.BotTextureBlendSetID=10000;t.TextureFileDataID[0]=797909;
    if(!StromkarGroundCrackData::Supported(&d,&t))return 7;
    decals.LoadFromDB();blends.LoadFromDB();
    if(!Unchanged(decals,originalDecals)||!Unchanged(blends,originalBlends))return 8;
    auto customDecal=decals.LookupEntry(10000);auto customBlend=blends.LookupEntry(10000);
    if(bool(customDecal)!=bool(customBlend))return 9;
    if(customDecal){
        if(!StromkarGroundCrackData::Supported(customDecal,customBlend)||std::memcmp(customDecal,&d,sizeof d)||std::memcmp(customBlend,&t,sizeof t))return 10;
        ByteBuffer a,b;decals.WriteRecord(10000,0,a);blends.WriteRecord(10000,0,b);
        // Decal ID is in data; TextureBlendSet's ID is external in native26972.
        // The native serializer preserves byte fields and omits that external ID.
        if(a.size()!=62||b.size()!=80)return 11;
        if(std::memcmp(a.contents(),&d,62)||std::memcmp(b.contents(),reinterpret_cast<uint8 const*>(&t)+4,80))return 12;
        std::cout<<"CUSTOM_PAYLOAD\t"<<a.size()<<'\t'<<b.size()<<"\t10000\n";
    }else if(std::getenv("LEGION_REQUIRE_GROUND_HOTFIX"))return 13;
    std::cout<<"PASS native26972 typed62/84 stores, hashes, complete native24/21 adaptation, all "<<originalDecals.size()<<'/'<<originalBlends.size()<<" original records byte unchanged; custom="<<bool(customDecal)<<". No rendering claim.\n";
    HotfixDatabase.Close();MySQL::Library_End();return 0;
}
