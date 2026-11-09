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
 std::set<uint32>ids{49357,52845,43900,43899,49379,49378,74589,70764,71084,242459,242460,242461,242462,242463,242464,242465,232698,185908,185909,185910,185911,194913,195975,115181,213183,227681,202272,202274,195558,207472};
 for(auto id:ids){auto s=spells.LookupEntry(id);if(!s)return 5;std::cout<<"SPELL "<<id<<" "<<s->Name->Str[0]<<" | "<<s->Description->Str[0]<<'\n';for(auto e:effects)if(e->SpellID==id&&e->DifficultyID==0)std::cout<<"EFFECT "<<id<<" "<<e->EffectIndex<<" "<<e->Effect<<" "<<e->EffectAura<<" "<<e->EffectBasePoints<<" "<<e->ImplicitTarget[0]<<" "<<e->ImplicitTarget[1]<<" misc="<<e->EffectMiscValue[0]<<'\n';}
 for(auto row:itemEffects)if(ids.count(row->SpellID)){auto item=items.LookupEntry(row->ItemID);if(!item)return 6;std::cout<<"FLASK_ITEM "<<item->ID<<" classes="<<item->AllowableClass<<'\n';}
 auto rows=WorldDatabase.Query("SELECT spell_id,ScriptName FROM spell_script_names WHERE ScriptName IN('spell_item_brewfest_mount_transformation','spell_item_demon_hunters_aspect','spell_item_faded_wizard_hat','spell_gen_citizen_costume_alliance_darkmoon','spell_gen_citizen_costume_horde_darkmoon','spell_item_ocean_embrace','spell_pri_shadowform','spell_dk_glacial_advance','spell_monk_breath_of_fire','spell_gen_hammer_of_supreme_justice','spell_item_prydaz_abs') ORDER BY ScriptName,spell_id");if(!rows)return 7;do{auto f=rows->Fetch();std::cout<<"BIND "<<f[0].GetUInt32()<<" "<<f[1].GetString()<<'\n';}while(rows->NextRow());
 std::cout<<"EOF third-ten-catalog\n";WorldDatabase.Close();HotfixDatabase.Close();MySQL::Library_End();return 0;}
