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
 std::set<uint32>ids{253826,253825,188639,132404,67019,67016,67017,67018,153595,228601,44614,228596,232661,250863,242609,87649,233490,196364};
 for(auto id:ids){auto s=spells.LookupEntry(id);if(!s)return 5;std::cout<<"SPELL "<<id<<" "<<s->Name->Str[0]<<" | "<<s->Description->Str[0]<<'\n';for(auto e:effects)if(e->SpellID==id&&e->DifficultyID==0)std::cout<<"EFFECT "<<id<<" "<<e->EffectIndex<<" "<<e->Effect<<" "<<e->EffectAura<<" "<<e->EffectBasePoints<<" "<<e->ImplicitTarget[0]<<" "<<e->ImplicitTarget[1]<<'\n';}
 for(auto row:itemEffects)if(row->SpellID==67019){auto item=items.LookupEntry(row->ItemID);if(!item)return 6;std::cout<<"FLASK_ITEM "<<item->ID<<" classes="<<item->AllowableClass<<'\n';}
 auto rows=WorldDatabase.Query("SELECT spell_id,ScriptName FROM spell_script_names WHERE ScriptName IN('spell_gen_umbral_shift','spell_warr_shatter_the_bones','spell_item_flask_of_the_north','spell_mage_comet_storm','spell_mage_flurry','spell_storm_comet','spell_gen_reckless_stomp','spell_item_walling_souls','spell_item_chocolate_cookie','spell_warl_unstable_affliction') ORDER BY ScriptName,spell_id");if(!rows)return 7;do{auto f=rows->Fetch();std::cout<<"BIND "<<f[0].GetUInt32()<<" "<<f[1].GetString()<<'\n';}while(rows->NextRow());
 std::cout<<"EOF next-ten-catalog\n";WorldDatabase.Close();HotfixDatabase.Close();MySQL::Library_End();return 0;}
