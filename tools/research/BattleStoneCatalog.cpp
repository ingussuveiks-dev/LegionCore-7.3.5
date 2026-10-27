#include "DB2Store.h"
#include "DB2Structure.h"
#include "DB2LoadInfo.h"
#include "Config.h"
#include "DatabaseEnv.h"
#include "MySQLThreading.h"
#include <cstdlib>
#include <iostream>
#include <set>
int main(int argc,char**argv){if(argc!=2)return 1;std::string error;auto config=std::getenv("LEGION_DIALOGUE_CONFIG");if(!config||!sConfigMgr->LoadInitial(config,{},error))return 2;
MySQL::Library_Init();HotfixDatabase.SetConnectionInfo(sConfigMgr->GetStringDefault("HotfixDatabaseInfo",""),1,1);if(HotfixDatabase.Open()||!HotfixDatabase.PrepareStatements())return 3;
HotfixDatabase.DirectExecute("SET SESSION TRANSACTION READ ONLY");HotfixDatabase.Execute("SET SESSION TRANSACTION READ ONLY");
DB2Storage<ItemEffectEntry> items("ItemEffect.db2",ItemEffectLoadInfo::Instance());DB2Storage<SpellEffectEntry> effects("SpellEffect.db2",SpellEffectLoadInfo::Instance());DB2Storage<SpellEntry> spells("Spell.db2",SpellLoadInfo::Instance());
if(!items.Load(argv[1],0)||!effects.Load(argv[1],0)||!spells.Load(argv[1],0))return 4;items.LoadFromDB();effects.LoadFromDB();spells.LoadFromDB();std::set<uint32> selected;
DB2Storage<ItemSparseEntry> sparse("ItemSparse.db2",ItemSparseLoadInfo::Instance());
DB2Storage<BattlePetSpeciesEntry> species("BattlePetSpecies.db2",BattlePetSpeciesLoadInfo::Instance());
if(!sparse.Load(argv[1],0)||!species.Load(argv[1],0))return 6;sparse.LoadFromDB();species.LoadFromDB();
if(auto i=sparse.LookupEntry(98715))std::cout<<"ITEM\t"<<i->ID<<'\t'<<i->Display->Str[0]<<'\t'<<i->Stackable<<'\n';
if(auto p=species.LookupEntry(1155))std::cout<<"SPECIES\t"<<p->ID<<'\t'<<p->CreatureID<<'\t'<<unsigned(p->PetTypeEnum)<<'\n';
for(auto i:items)if(i->ItemID==98715){selected.insert(i->SpellID);std::cout<<"ITEM_EFFECT\t"<<i->ItemID<<'\t'<<i->SpellID<<'\t'<<unsigned(i->TriggerType)<<'\t'<<i->Charges<<'\n';}
for(auto s:spells)if(selected.count(s->ID))std::cout<<"SPELL\t"<<s->ID<<'\t'<<s->Name->Str[0]<<'\n';
for(auto e:effects)if(selected.count(e->SpellID))std::cout<<"EFFECT\t"<<e->SpellID<<'\t'<<unsigned(e->EffectIndex)<<'\t'<<e->Effect<<'\t'<<e->EffectBasePoints<<'\t'<<e->EffectMiscValue[0]<<'\t'<<unsigned(e->ImplicitTarget[0])<<'\t'<<unsigned(e->ImplicitTarget[1])<<'\n';
HotfixDatabase.Close();MySQL::Library_End();return selected.size()==1?0:5;}
