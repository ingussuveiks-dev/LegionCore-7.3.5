// Matching26972/hotfix content only, not a client traversal.
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
 Expect(argc==2,"Expected native dbc directory");std::string error;auto config=std::getenv("LEGION_DIALOGUE_CONFIG");Expect(config&&sConfigMgr->LoadInitial(config,{},error),"Private native config unavailable");
 MySQL::Library_Init();HotfixDatabase.SetConnectionInfo(sConfigMgr->GetStringDefault("HotfixDatabaseInfo",""),1,1);Expect(!HotfixDatabase.Open()&&HotfixDatabase.PrepareStatements(),"Readonly hotfix pool failed");HotfixDatabase.DirectExecute("SET SESSION TRANSACTION READ ONLY");HotfixDatabase.Execute("SET SESSION TRANSACTION READ ONLY");auto readOnly=HotfixDatabase.Query("SELECT @@session.transaction_read_only");Expect(readOnly&&readOnly->Fetch()[0].GetBool(),"Hotfix pool is not readonly");
#define STORE(T,N,F,I) DB2Storage<T> N(F ".db2",I::Instance());Expect(N.Load(argv[1],0),"Matching DB2 missing");N.LoadFromDB()
 STORE(SpellEntry,spells,"Spell",SpellLoadInfo);STORE(SpellEffectEntry,effects,"SpellEffect",SpellEffectLoadInfo);STORE(ItemEffectEntry,items,"ItemEffect",ItemEffectLoadInfo);STORE(SpellMiscEntry,misc,"SpellMisc",SpellMiscLoadInfo);STORE(SpellCastTimesEntry,times,"SpellCastTimes",SpellCastTimesLoadInfo);STORE(CriteriaTreeEntry,trees,"CriteriaTree",CriteriaTreeLoadInfo);STORE(CriteriaEntry,criteria,"Criteria",CriteriaLoadInfo);
#undef STORE
 auto effect=[&](uint32 id,int32 index){SpellEffectEntry const* row=nullptr;for(auto e:effects)if(e->SpellID==id&&!e->DifficultyID&&e->EffectIndex==index){Expect(!row,"Ambiguous native effect");row=e;}Expect(row,"Native effect missing");return row;};
 auto pocket=effect(921,0);Expect(pocket->Effect==71&&pocket->ImplicitTarget[0]==25,"Actual Pick Pocket effect changed");auto root=effect(211684,0);Expect(root->Effect==6&&root->EffectAura==26,"Chains root26 changed");Expect(effect(211686,0)->EffectAura==67&&effect(211686,1)->EffectAura==254,"Native two-slot disarm changed");
 auto key=effect(210097,0);Expect(key->Effect==6&&key->EffectAura==4,"Cell Door Key aura changed");unsigned obtain=0;for(auto e:items)if(e->ItemID==136802&&e->SpellID==210097&&e->TriggerType==5)++obtain;Expect(obtain==1,"Actual Wardstone on-obtain effect not unique");
 auto release=effect(211689,0),event=effect(211689,1),reclaim=effect(211687,0);Expect(release->Effect==164&&release->EffectTriggerSpell==211684&&event->Effect==61&&event->EffectMiscValue[0]==50908,"Native cage producer changed");Expect(reclaim->Effect==164&&reclaim->EffectTriggerSpell==211686,"Native reclaim removal changed");bool twoSeconds=false;for(auto m:misc)if(m->SpellID==211687&&!m->DifficultyID){auto t=times.LookupEntry(m->CastingTimeIndex);twoSeconds=t&&t->Base==2000;}Expect(twoSeconds,"Native two-second reclaim changed");
 auto parent=trees.LookupEntry(47360),loot=trees.LookupEntry(47377),receipt=trees.LookupEntry(54243);Expect(parent&&parent->Operator==8&&parent->Amount==1&&loot&&receipt&&loot->Parent==47360&&receipt->Parent==47360,"Native Wardstone ANY root changed");auto looted=criteria.LookupEntry(loot->CriteriaID),notified=criteria.LookupEntry(receipt->CriteriaID);Expect(looted&&looted->Type==42&&looted->Asset==136802&&notified&&notified->Type==92&&notified->Asset==54824,"Native item/notification alternatives changed");
 for(uint32 id:{921u,1784u,210097u,211684u,211686u}){auto s=spells.LookupEntry(id);Expect(s,"Native spell name missing");std::cout<<"SPELL\t"<<id<<'\t'<<s->Name->Str[0]<<'\n';}
 std::cout<<"ITEM_EFFECT\t136802\t210097\t5\nROOT\t47360\t8\t1\nLEAF\t47377\t42\t136802\nLEAF\t54243\t92\t54824\tflags="<<receipt->Flags<<'\n';
 std::cout<<"PASS native26972/hotfix Wardstone obtain5/Cell Door Key210097, Pick Pocket71, Chains root26/not stun12 and disarm67/254, original211689/211687/two-second cast and actual stage1 ANY loot42/notification92 alternatives. No client/gameplay claim.\n";HotfixDatabase.Close();MySQL::Library_End();return 0;
}catch(std::exception const&e){std::cerr<<e.what()<<'\n';return 1;}}
