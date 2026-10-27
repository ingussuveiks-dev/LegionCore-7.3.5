#include "DB2Store.h"
#include "DB2Structure.h"
#include "DB2LoadInfo.h"
#include "Config.h"
#include "DatabaseEnv.h"
#include "MySQLThreading.h"
#include <cstdlib>
#include <iostream>
#include <set>
#include <string>
int main(int argc,char** argv)
{
    if(argc!=2)return 1;
    std::string error;auto config=std::getenv("LEGION_DIALOGUE_CONFIG");
    if(!config||!sConfigMgr->LoadInitial(config,{},error))return 2;
    MySQL::Library_Init();HotfixDatabase.SetConnectionInfo(sConfigMgr->GetStringDefault("HotfixDatabaseInfo",""),1,1);
    if(HotfixDatabase.Open()||!HotfixDatabase.PrepareStatements())return 3;
    HotfixDatabase.DirectExecute("SET SESSION TRANSACTION READ ONLY");HotfixDatabase.Execute("SET SESSION TRANSACTION READ ONLY");
    DB2Storage<SpellEntry> spells("Spell.db2",SpellLoadInfo::Instance());
    DB2Storage<SpellEffectEntry> effects("SpellEffect.db2",SpellEffectLoadInfo::Instance());
    DB2Storage<SpellMiscEntry> misc("SpellMisc.db2",SpellMiscLoadInfo::Instance());
    DB2Storage<SpellRadiusEntry> radii("SpellRadius.db2",SpellRadiusLoadInfo::Instance());
    DB2Storage<TalentEntry> talents("Talent.db2",TalentLoadInfo::Instance());
    DB2Storage<PvpTalentEntry> pvp("PvpTalent.db2",PvpTalentLoadInfo::Instance());
    DB2Storage<SpellDurationEntry> durations("SpellDuration.db2",SpellDurationLoadInfo::Instance());
    DB2Storage<SpellCategoriesEntry> categories("SpellCategories.db2",SpellCategoriesLoadInfo::Instance());
    DB2Storage<SpellCategoryEntry> category("SpellCategory.db2",SpellCategoryLoadInfo::Instance());
    DB2Storage<SpellClassOptionsEntry> classes("SpellClassOptions.db2",SpellClassOptionsLoadInfo::Instance());
    if(!spells.Load(argv[1],0)||!effects.Load(argv[1],0)||!misc.Load(argv[1],0)||!radii.Load(argv[1],0)||!talents.Load(argv[1],0)||!pvp.Load(argv[1],0)||!durations.Load(argv[1],0)||!categories.Load(argv[1],0)||!category.Load(argv[1],0))return 4;
    if(!classes.Load(argv[1],0))return 6;
    spells.LoadFromDB();effects.LoadFromDB();misc.LoadFromDB();radii.LoadFromDB();talents.LoadFromDB();pvp.LoadFromDB();durations.LoadFromDB();categories.LoadFromDB();category.LoadFromDB();classes.LoadFromDB();
    std::set<uint32> wanted{6544,94954,52174,202163,202164,199085,199086,209483,209484,208177,208086};
    for(auto t:talents)if(t->ClassID==1) {
        auto s=spells.LookupEntry(t->SpellID);if(!s)continue;
        std::string text=s->Description->Str[0],name=s->Name->Str[0];
        if(text.find("Heroic Leap")!=std::string::npos||name.find("Bounding")!=std::string::npos) {
            std::cout<<"TALENT "<<t->ID<<" spell "<<t->SpellID<<" spec "<<t->SpecID<<'\n';
            wanted.insert(t->SpellID);
        }
    }
    for(auto t:pvp)if(t->ClassID==1)if(auto s=spells.LookupEntry(t->SpellID))
        if(std::string(s->Description->Str[0]).find("Heroic Leap")!=std::string::npos) {
            std::cout<<"PVP_TALENT "<<t->ID<<" spell "<<t->SpellID<<'\n';wanted.insert(t->SpellID);
        }
    for(uint32 id:wanted) {
        auto spell=spells.LookupEntry(id);if(!spell)return 5;
        std::cout<<"SPELL "<<id<<' '<<spell->Name->Str[0]<<" DESCRIPTION "<<spell->Description->Str[0]<<'\n';
        for(auto e:effects)if(e->SpellID==id) {
            std::cout<<"EFFECT "<<id<<' '<<e->EffectIndex<<" diff "<<e->DifficultyID<<" type "<<e->Effect<<" aura "<<e->EffectAura<<" misc "<<e->EffectMiscValue[0]<<','<<e->EffectMiscValue[1]<<" trigger "<<e->EffectTriggerSpell<<" target "<<e->ImplicitTarget[0]<<','<<e->ImplicitTarget[1]<<" base "<<e->EffectBasePoints<<" AP "<<e->BonusCoefficientFromAP<<" coeff "<<e->Coefficient<<" radius "<<e->EffectRadiusIndex[0]<<','<<e->EffectRadiusIndex[1]<<'\n';
            std::cout<<"MASK "<<id<<' '<<e->EffectIndex;for(unsigned j=0;j<4;++j)std::cout<<' '<<e->EffectSpellClassMask[j];std::cout<<'\n';
        }
        for(auto m:misc)if(m->SpellID==id) {
            std::cout<<"MISC "<<id<<" speed "<<m->Speed<<" launch "<<m->LaunchDelay<<" school "<<unsigned(m->SchoolMask)<<'\n';
            if(auto d=durations.LookupEntry(m->DurationIndex))std::cout<<"DURATION "<<id<<' '<<d->Duration<<'\n';
        }
        for(auto c:categories)if(c->SpellID==id)if(auto cat=category.LookupEntry(c->ChargeCategory))std::cout<<"CHARGE "<<id<<' '<<cat->ID<<' '<<cat->ChargeRecoveryTime<<' '<<unsigned(cat->MaxCharges)<<'\n';
        for(auto c:classes)if(c->SpellID==int32(id)){std::cout<<"CLASS "<<id<<' '<<unsigned(c->SpellClassSet);for(unsigned j=0;j<4;++j)std::cout<<' '<<c->SpellClassMask[j];std::cout<<'\n';}
    }
    for(auto r:radii)if(r->ID==14)std::cout<<"RADIUS "<<r->ID<<' '<<r->Radius<<' '<<r->RadiusMax<<'\n';
}
