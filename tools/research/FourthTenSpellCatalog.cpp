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
 std::set<uint32>ids{40121,50461,51052,108968,118594,142723,147655,147656,148385,190833,191463,191464,192761,192762,192763,192764,192765,192766,192767,192768,192769,192770,192771,192772,192773,192774,192775,192776,202862,208843,208844,208845,208846,208847,208848,208849,208850,208851,208852,208853,208854,208855,208856,208857,208858,208859,211309,215404,215405,219567,226829,227225,229174,229175,229176,229186,229187,229188,237795,237796,237797,237798,237799,237800,237801,237802,237803,237820,237821,237822,237823,237824,237825,237826,237827,237828,250143,250144,250145,250146,250147,250148,250149,250150,250152,250153,250155,250156,250157,250158,250159,250160,250162,250163,250164,250165,250166,250167};
 for(auto id:ids){auto s=spells.LookupEntry(id);if(!s){std::cout<<"ABSENT "<<id<<'\n';continue;}std::cout<<"SPELL "<<id<<" "<<s->Name->Str[0]<<" | "<<s->Description->Str[0]<<'\n';for(auto e:effects)if(e->SpellID==id&&e->DifficultyID==0)std::cout<<"EFFECT "<<id<<" "<<e->EffectIndex<<" "<<e->Effect<<" "<<e->EffectAura<<" "<<e->EffectBasePoints<<" "<<e->ImplicitTarget[0]<<" "<<e->ImplicitTarget[1]<<" misc="<<e->EffectMiscValue[0]<<'\n';}
 for(auto row:itemEffects)if(ids.count(row->SpellID)){auto item=items.LookupEntry(row->ItemID);if(!item)return 6;std::cout<<"ITEM_EFFECT "<<item->ID<<" spell="<<row->SpellID<<" classes="<<item->AllowableClass<<'\n';}
 auto rows=WorldDatabase.Query("SELECT spell_id,ScriptName FROM spell_script_names WHERE ScriptName IN('aura_commanders_valor_location','spell_tirathon_beam_dmg','spell_gulp_frog_toxin','spell_pri_void_shift','spell_dru_swift_flight_passive','spell_dk_anti_magic_zone','spell_gen_artificial_stamina','spell_gen_artificial_threat','spell_censer_of_eternal_agony','spell_item_rancid_maw','spell_vantus_rune_antorus','spell_vantus_rune_the_nighthold','spell_vantus_rune_the_emerald_nightmare','spell_vantus_rune_tomb_of_sargeras','spell_vantus_rune_trial_of_valor') ORDER BY ScriptName,spell_id");if(!rows)return 7;do{auto f=rows->Fetch();std::cout<<"BIND "<<f[0].GetUInt32()<<" "<<f[1].GetString()<<'\n';}while(rows->NextRow());
 std::cout<<"EOF fourth-ten-catalog\n";WorldDatabase.Close();HotfixDatabase.Close();MySQL::Library_End();return 0;}
