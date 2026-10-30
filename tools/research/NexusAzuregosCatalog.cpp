#include "DB2Store.h"
#include "DB2Structure.h"
#include "DB2LoadInfo.h"
#include "Config.h"
#include "DatabaseEnv.h"
#include "MySQLThreading.h"
#include <cstdlib>
#include <iostream>
#include <set>
int main(int argc,char**argv)
{
 if(argc!=2)return 1;std::string error;auto config=std::getenv("LEGION_DIALOGUE_CONFIG");
 if(!config||!sConfigMgr->LoadInitial(config,{},error))return 2;
 MySQL::Library_Init();HotfixDatabase.SetConnectionInfo(sConfigMgr->GetStringDefault("HotfixDatabaseInfo",""),1,1);
 if(HotfixDatabase.Open()||!HotfixDatabase.PrepareStatements())return 3;
 HotfixDatabase.DirectExecute("SET SESSION TRANSACTION READ ONLY");
 DB2Storage<SpellEntry> spells("Spell.db2",SpellLoadInfo::Instance());
 DB2Storage<SpellEffectEntry> effects("SpellEffect.db2",SpellEffectLoadInfo::Instance());
 DB2Storage<ScenarioStepEntry> steps("ScenarioStep.db2",ScenarioStepLoadInfo::Instance());
 DB2Storage<CriteriaTreeEntry> trees("CriteriaTree.db2",CriteriaTreeLoadInfo::Instance());
 DB2Storage<CriteriaEntry> criteria("Criteria.db2",CriteriaLoadInfo::Instance());
 DB2Storage<ConversationLineEntry> lines("ConversationLine.db2",ConversationLineLoadInfo::Instance());
 DB2Storage<BroadcastTextEntry> texts("BroadcastText.db2",BroadcastTextLoadInfo::Instance());
 if(!spells.Load(argv[1],0)||!effects.Load(argv[1],0)||!steps.Load(argv[1],0)||!trees.Load(argv[1],0)||!criteria.Load(argv[1],0)||!lines.Load(argv[1],0)||!texts.Load(argv[1],0))return 4;
 spells.LoadFromDB();effects.LoadFromDB();steps.LoadFromDB();trees.LoadFromDB();criteria.LoadFromDB();lines.LoadFromDB();texts.LoadFromDB();
 for(uint32 id:{207752u,209811u,211768u,207797u,211763u,211725u}){auto s=spells.LookupEntry(id);if(!s)return 5;std::cout<<"SPELL\t"<<id<<'\t'<<s->Name->Str[0]<<'\n';for(auto e:effects)if(e->SpellID==id&&!e->DifficultyID)std::cout<<"EFFECT\t"<<id<<'\t'<<e->EffectIndex<<'\t'<<e->Effect<<'\t'<<e->EffectAura<<'\t'<<e->EffectMiscValue[0]<<'\t'<<e->EffectTriggerSpell<<'\t'<<e->ImplicitTarget[0]<<'\t'<<e->ImplicitTarget[1]<<'\n';}
 unsigned matches=0;
 for(auto s:steps)if(s->ScenarioID==1101||s->ScenarioID==1065)
  for(auto t:trees)if(t->CriteriaID){auto c=criteria.LookupEntry(t->CriteriaID);if(!c)return 6;if(c->Type!=92||!std::set<int32>{49864,50369,49807,49808,49809}.count(c->Asset))continue;std::set<uint32>seen;uint32 id=t->ID;while(id&&seen.insert(id).second){if(id==s->Criteriatreeid){++matches;std::cout<<"CRITERION\t"<<s->ScenarioID<<'\t'<<unsigned(s->OrderIndex)<<'\t'<<c->Asset<<'\t'<<t->Amount<<'\n';std::cout<<"CRITERION_TREE\t"<<s->ScenarioID<<'\t'<<unsigned(s->OrderIndex)<<'\t'<<c->Asset<<'\t'<<t->ID<<'\t'<<s->Criteriatreeid<<'\n';break;}auto p=trees.LookupEntry(id);if(!p)break;id=p->Parent;}}
 if(matches<5)return 7;
 for(uint32 id:{3610u,3611u,3612u,3613u,4545u,4548u,4549u}){auto l=lines.LookupEntry(id);if(!l)return 8;auto t=texts.LookupEntry(l->BroadcastTextID);if(!t)return 9;std::cout<<"LINE\t"<<id<<'\t'<<l->BroadcastTextID<<'\t'<<l->NextConversationLineID<<'\t'<<t->Text->Str[0]<<'\n';}
 HotfixDatabase.Close();MySQL::Library_End();return 0;
}
