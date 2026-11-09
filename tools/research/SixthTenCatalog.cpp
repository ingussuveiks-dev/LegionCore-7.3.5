#include "DB2Store.h"
#include "DB2Structure.h"
#include "DB2LoadInfo.h"
#include "Config.h"
#include "DatabaseEnv.h"
#include "MySQLThreading.h"
#include <cstdlib>
#include <iostream>
#include <set>
int main(int argc,char**argv){if(argc!=2)return 1;std::string error;auto config=std::getenv("LEGION_DIALOGUE_CONFIG");if(!config||!sConfigMgr->LoadInitial(config,{},error))return 2;MySQL::Library_Init();
 HotfixDatabase.SetConnectionInfo(sConfigMgr->GetStringDefault("HotfixDatabaseInfo",""),1,1);WorldDatabase.SetConnectionInfo(sConfigMgr->GetStringDefault("WorldDatabaseInfo",""),1,1);
 if(HotfixDatabase.Open()||!HotfixDatabase.PrepareStatements()||WorldDatabase.Open()||!WorldDatabase.PrepareStatements())return 3;
 for(bool world:{false,true}){if(world){WorldDatabase.DirectExecute("SET SESSION TRANSACTION READ ONLY");WorldDatabase.Execute("SET SESSION TRANSACTION READ ONLY");}else{HotfixDatabase.DirectExecute("SET SESSION TRANSACTION READ ONLY");HotfixDatabase.Execute("SET SESSION TRANSACTION READ ONLY");}}
 DB2Storage<SpellEntry>spells("Spell.db2",SpellLoadInfo::Instance());DB2Storage<SpellEffectEntry>effects("SpellEffect.db2",SpellEffectLoadInfo::Instance());DB2Storage<ItemSparseEntry>items("ItemSparse.db2",ItemSparseLoadInfo::Instance());DB2Storage<ItemEffectEntry>itemEffects("ItemEffect.db2",ItemEffectLoadInfo::Instance());
 if(!spells.Load(argv[1],0)||!effects.Load(argv[1],0)||!items.Load(argv[1],0)||!itemEffects.Load(argv[1],0))return 4;spells.LoadFromDB();effects.LoadFromDB();items.LoadFromDB();itemEffects.LoadFromDB();
 std::set<uint32>ids{234684,207498,207553,202328,202339,205230,205233,55937,55946};
 for(auto id:ids){auto s=spells.LookupEntry(id);if(!s){std::cout<<"ABSENT "<<id<<'\n';continue;}std::cout<<"SPELL "<<id<<" "<<s->Name->Str[0]<<" | "<<s->Description->Str[0]<<'\n';for(auto e:effects)if(e->SpellID==id&&e->DifficultyID==0)std::cout<<"EFFECT "<<id<<" "<<e->EffectIndex<<" "<<e->Effect<<" "<<e->EffectAura<<" "<<e->EffectBasePoints<<" "<<e->ImplicitTarget[0]<<" "<<e->ImplicitTarget[1]<<" misc="<<e->EffectMiscValue[0]<<'\n';}
 for(auto row:itemEffects)if(ids.count(row->SpellID)){auto item=items.LookupEntry(row->ItemID);if(!item)return 6;std::cout<<"ITEM_EFFECT "<<item->ID<<" spell="<<row->SpellID<<" classes="<<item->AllowableClass<<'\n';}
 auto rows=WorldDatabase.Query("SELECT spell_id,ScriptName FROM spell_script_names WHERE ScriptName IN('spell_q_artifact_trait_unlock','spell_sha_ancestral_protection','spell_betrug_mighty_smash') ORDER BY ScriptName,spell_id");if(!rows)return 7;do{auto f=rows->Fetch();std::cout<<"BIND "<<f[0].GetUInt32()<<" "<<f[1].GetString()<<'\n';}while(rows->NextRow());
 auto creatures=WorldDatabase.Query("SELECT t.entry,w.Name1,t.ScriptName FROM creature_template t JOIN creature_template_wdb w ON w.Entry=t.entry WHERE t.entry IN(30020,30113,102446,92879)");if(!creatures)return 8;do{auto f=creatures->Fetch();std::cout<<"CREATURE "<<f[0].GetUInt32()<<" "<<f[1].GetString()<<" "<<f[2].GetString()<<'\n';}while(creatures->NextRow());
 std::cout<<"EOF sixth-ten-catalog\n";WorldDatabase.Close();HotfixDatabase.Close();MySQL::Library_End();return 0;}
