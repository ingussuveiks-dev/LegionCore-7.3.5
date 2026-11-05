// Exact matching26972/hotfix fanout; reference metadata is not client playback.
#include "DB2Store.h"
#include "DB2Structure.h"
#include "DB2LoadInfo.h"
#include "Config.h"
#include "DatabaseEnv.h"
#include "MySQLThreading.h"
#include <cstdlib>
#include <iostream>
#include <stdexcept>
static void Expect(bool yes,char const* text){if(!yes)throw std::runtime_error(text);}
int main(int argc,char**argv){try{
 Expect(argc==2,"Expected matching dbc directory");std::string error;auto config=std::getenv("LEGION_DIALOGUE_CONFIG");Expect(config&&sConfigMgr->LoadInitial(config,{},error),"Private native config unavailable");
 MySQL::Library_Init();HotfixDatabase.SetConnectionInfo(sConfigMgr->GetStringDefault("HotfixDatabaseInfo",""),1,1);Expect(!HotfixDatabase.Open()&&HotfixDatabase.PrepareStatements(),"Readonly hotfix pool failed");HotfixDatabase.DirectExecute("SET SESSION TRANSACTION READ ONLY");HotfixDatabase.Execute("SET SESSION TRANSACTION READ ONLY");auto readOnly=HotfixDatabase.Query("SELECT @@session.transaction_read_only");Expect(readOnly&&readOnly->Fetch()[0].GetBool(),"Hotfix pool is not readonly");
#define STORE(T,N,F,I) DB2Storage<T> N(F ".db2",I::Instance());Expect(N.Load(argv[1],0),"Matching DB2 missing");N.LoadFromDB()
 STORE(SpellEffectEntry,effects,"SpellEffect",SpellEffectLoadInfo);STORE(SpellMiscEntry,misc,"SpellMisc",SpellMiscLoadInfo);STORE(SpellDurationEntry,durations,"SpellDuration",SpellDurationLoadInfo);STORE(SummonPropertiesEntry,props,"SummonProperties",SummonPropertiesLoadInfo);
#undef STORE
 auto effect=[&](uint32 id,int32 index){SpellEffectEntry const* row=nullptr;for(auto e:effects)if(e->SpellID==id&&!e->DifficultyID&&e->EffectIndex==index){Expect(!row,"Ambiguous native effect");row=e;}Expect(row,"Native effect absent");return row;};
 Expect(effect(209231,0)->Effect==6&&effect(209231,0)->EffectAura==18&&effect(209231,0)->ImplicitTarget[0]==1,"Native self-stealth changed");Expect(effect(209231,1)->Effect==64&&effect(209231,1)->EffectTriggerSpell==209233,"Native initial clone trigger changed");Expect(effect(209231,2)->EffectAura==260&&effect(209231,2)->EffectMiscValue[0]==81,"Native Vanish screen changed");
 for(int32 index:{1,2}){auto e=effect(209233,index);Expect(e->Effect==64&&e->EffectTriggerSpell==209233+index&&e->ImplicitTarget[0]==1,"Native clone fanout trigger changed");}
 auto property=props.LookupEntry(61);Expect(property&&property->Flags==2&&property->Control==1&&property->Title==2&&!property->Faction&&!property->Slot,"Native clone summon properties61 changed");
 for(uint32 id:{209231u,209233u,209234u,209235u}){unsigned found=0;for(auto m:misc)if(m->SpellID==id&&!m->DifficultyID){auto d=durations.LookupEntry(m->DurationIndex);Expect(d&&d->Duration==10000,"Native10s duration changed");++found;}Expect(found==1,"Native duration must be unique");if(id!=209231){auto e=effect(id,0);Expect(e->Effect==28&&e->EffectMiscValue[0]==105609&&e->EffectMiscValue[1]==61&&e->ImplicitTarget[0]==int32(id-209233+48)&&!e->ImplicitTarget[1]&&e->EffectRadiusIndex[0]==15,"Actual105609 clone producer changed");std::cout<<"SUMMON\t"<<id<<"\t105609\t61\t"<<e->ImplicitTarget[0]<<"\t15\t10000\n";}}
 std::cout<<"PASS native26972/hotfix Vanish209231/self stealth18/screen81 ->209233 ->209234+209235, exactly3 back48/right49/left50 actors105609 via native properties61, original10s durations; no inferred105535 clone or scripted replacement\n";HotfixDatabase.Close();MySQL::Library_End();return 0;
}catch(std::exception const&e){std::cerr<<e.what()<<'\n';return 1;}}
