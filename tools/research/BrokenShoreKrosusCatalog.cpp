#include "DB2Store.h"
#include "DB2Structure.h"
#include "DB2LoadInfo.h"
#include "Config.h"
#include "DatabaseEnv.h"
#include "MySQLThreading.h"
#include "../../src/server/scripts/Scenario/BrokenIslands/KrosusReach.h"
#include <cstdlib>
#include <iostream>
int main(int argc, char** argv)
{
    if (argc != 2) return 1;
    std::string error; auto config=std::getenv("LEGION_DIALOGUE_CONFIG");
    if(!config || !sConfigMgr->LoadInitial(config,{},error)) return 5;
    MySQL::Library_Init(); HotfixDatabase.SetConnectionInfo(sConfigMgr->GetStringDefault("HotfixDatabaseInfo",""),1,1);
    if(HotfixDatabase.Open() || !HotfixDatabase.PrepareStatements()) return 6;
    HotfixDatabase.DirectExecute("SET SESSION TRANSACTION READ ONLY");
    HotfixDatabase.Execute("SET SESSION TRANSACTION READ ONLY");
    DB2Storage<CreatureDisplayInfoEntry> displays("CreatureDisplayInfo.db2", CreatureDisplayInfoLoadInfo::Instance());
    DB2Storage<CreatureModelDataEntry> models("CreatureModelData.db2", CreatureModelDataLoadInfo::Instance());
    DB2Storage<SpellEffectEntry> effects("SpellEffect.db2", SpellEffectLoadInfo::Instance());
    if (!displays.Load(argv[1],0) || !models.Load(argv[1],0) || !effects.Load(argv[1],0)) return 2;
    displays.LoadFromDB(); models.LoadFromDB(); effects.LoadFromDB();
    for (uint32 id : {71701,68323}) {
        auto d=displays.LookupEntry(id); if(!d) return 3;
        auto m=models.LookupEntry(d->ModelID); if(!m) return 4;
        std::cout<<"MODEL "<<id<<" displayScale "<<d->CreatureModelScale<<" model "<<m->ID<<" scale "<<m->ModelScale<<" file "<<m->FileDataID<<" collision "<<m->CollisionWidth<<' '<<m->CollisionHeight<<" hover "<<m->HoverHeight<<" box";
        for(float f:m->GeoBox) std::cout<<' '<<f;
        std::cout<<'\n';
        if(id==71701) {
            float reach=BrokenShoreKrosus::CombatReach(30.0f,m->GeoBox,d->CreatureModelScale*m->ModelScale);
            std::cout<<"REPAIRED_REACH "<<reach<<'\n';
            if(reach<42.90f || reach>42.92f) return 7;
        }
    }
    for(auto e:effects) if(e->SpellID==225099||e->SpellID==225098||e->SpellID==208495)
        std::cout<<"EFFECT "<<e->SpellID<<' '<<unsigned(e->EffectIndex)<<" type "<<e->Effect<<" aura "<<e->EffectAura<<" points "<<e->EffectBasePoints<<" misc "<<e->EffectMiscValue[0]<<' '<<e->EffectMiscValue[1]<<'\n';
}
