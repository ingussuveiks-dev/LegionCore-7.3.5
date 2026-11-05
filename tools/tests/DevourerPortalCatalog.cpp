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
 STORE(SpellEffectEntry,effects,"SpellEffect",SpellEffectLoadInfo);STORE(SpellEntry,spells,"Spell",SpellLoadInfo);STORE(SpellMiscEntry,misc,"SpellMisc",SpellMiscLoadInfo);STORE(SpellDurationEntry,durations,"SpellDuration",SpellDurationLoadInfo);STORE(SpellCastTimesEntry,times,"SpellCastTimes",SpellCastTimesLoadInfo);STORE(BroadcastTextEntry,texts,"BroadcastText",BroadcastTextLoadInfo);STORE(PlayerConditionEntry,conditions,"PlayerCondition",PlayerConditionLoadInfo);STORE(SoundKitEntryEntry,sounds,"SoundKitEntry",SoundKitEntryLoadInfo);
#undef STORE

 for(uint32 id:{209056u,209055u,209065u,209066u}){auto n=spells.LookupEntry(id);Expect(n,"Native spell missing");std::cout<<"SPELL\t"<<id<<'\t'<<n->Name->Str[0]<<'\n';for(auto e:effects)if(e->SpellID==id&&!e->DifficultyID)std::cout<<"EFFECT\t"<<id<<'\t'<<e->EffectIndex<<'\t'<<e->Effect<<'\t'<<e->EffectAura<<'\t'<<e->EffectMiscValue[0]<<'\t'<<e->EffectTriggerSpell<<'\t'<<e->ImplicitTarget[0]<<'\t'<<e->ImplicitTarget[1]<<'\n';for(auto m:misc)if(m->SpellID==id&&!m->DifficultyID){auto d=durations.LookupEntry(m->DurationIndex);auto c=times.LookupEntry(m->CastingTimeIndex);std::cout<<"TIMING\t"<<id<<'\t'<<(d?d->Duration:0)<<'\t'<<(c?c->Base:0)<<'\n';}}
 auto c=conditions.LookupEntry(39597);if(c){std::cout<<"CONDITION\t39597\t"<<c->ClassMask<<'\t'<<c->MinLevel<<'\t'<<c->CurrQuestLogic<<'\t'<<c->PrevQuestLogic<<'\t'<<c->CurrentCompletedQuestLogic<<'\n';for(unsigned i=0;i<4;++i)std::cout<<"QUESTGATE\t"<<i<<'\t'<<c->CurrQuestID[i]<<'\t'<<c->PrevQuestID[i]<<'\t'<<c->CurrentCompletedQuestID[i]<<'\n';}else std::cout<<"MISSING_CONDITION\t39597\n";
 for(auto t:texts)if(t->ID>=108240&&t->ID<=108265)std::cout<<"TEXT\t"<<t->ID<<'\t'<<t->Text->Str[0]<<'\t'<<t->Text1->Str[0]<<'\t'<<t->SoundEntriesID[0]<<'\t'<<t->SoundEntriesID[1]<<'\n';
 for(auto c:conditions){bool match=false;for(unsigned i=0;i<4;++i)if(c->CurrQuestID[i]==41924||c->PrevQuestID[i]==41924||c->CurrentCompletedQuestID[i]==41924)match=true;if(match)std::cout<<"CANDIDATE_CONDITION\t"<<c->ID<<'\t'<<c->ClassMask<<'\t'<<c->CurrQuestLogic<<'\t'<<c->PrevQuestLogic<<'\t'<<c->CurrentCompletedQuestLogic<<'\t'<<c->CurrQuestID[0]<<'\t'<<c->PrevQuestID[0]<<'\t'<<c->CurrentCompletedQuestID[0]<<'\n';}
 for(auto e:sounds)if(e->SoundKitID>=63731&&e->SoundKitID<=63733)std::cout<<"AUDIO_FILE\t"<<e->SoundKitID<<'\t'<<e->FileDataID<<'\n';
 auto effect=[&](uint32 spell,uint32 index){SpellEffectEntry const* result=nullptr;for(auto e:effects)if(e->SpellID==spell&&!e->DifficultyID&&e->EffectIndex==index){Expect(!result,"Duplicate native effect");result=e;}Expect(result,"Native effect missing");return result;};
 auto opening=effect(209056,0);Expect(opening->Effect==90&&opening->EffectMiscValue[0]==105435&&opening->ImplicitTarget[0]==92,"Opening personal credit/selected recipient changed");auto camera=effect(209056,1);Expect(camera->Effect==140&&camera->EffectTriggerSpell==209055&&camera->ImplicitTarget[0]==92,"Opening camera target changed");auto visual=effect(209056,2);Expect(visual->Effect==64&&visual->EffectTriggerSpell==209065,"Opening destination presentation changed");
 bool cast=false;for(auto m:misc)if(m->SpellID==209056&&!m->DifficultyID){auto time=times.LookupEntry(m->CastingTimeIndex);Expect(time&&time->Base==5000,"Native opening cast duration changed");cast=true;}Expect(cast,"Opening timing missing");
 Expect(effect(209066,0)->Effect==90&&effect(209066,0)->EffectMiscValue[0]==116918&&effect(209066,1)->Effect==227,"Native portal credit/teleport changed");Expect(!conditions.LookupEntry(39597),"Legacy39597 now exists: review actual data before adapting gate");
 for(uint32 index=0;index<3;++index){uint32 id=index==0?108249:index==1?108250:108257;auto text=texts.LookupEntry(id);Expect(text&&text->SoundEntriesID[0]==63731+index,"Native Valzuun BroadcastText/SoundKit changed");bool file=false;for(auto e:sounds)if(e->SoundKitID==63731+index){Expect(e->FileDataID==1392450+index,"Native voice file identity changed");file=true;}Expect(file,"Native voice mapping absent");}
 std::cout<<"PASS matching26972/hotfix209056 selected personal105435 credit/camera/5s cast, native209066 transfer, missing legacy39597 explicitly retained, three authored BroadcastText/SoundKit/FileData identities. Metadata, not client playback\n";
 HotfixDatabase.Close();MySQL::Library_End();return 0;
}catch(std::exception const&e){std::cerr<<e.what()<<'\n';return 1;}}
