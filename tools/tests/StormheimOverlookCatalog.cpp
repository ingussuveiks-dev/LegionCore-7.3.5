#include "DB2Store.h"
#include "DB2Structure.h"
#include "DB2LoadInfo.h"
#include <iostream>
#include <map>
int main(int argc,char**argv){
 if(argc!=2)return 1;
 DB2Storage<SpellEntry> spells("Spell.db2",SpellLoadInfo::Instance());
 DB2Storage<SpellEffectEntry> effects("SpellEffect.db2",SpellEffectLoadInfo::Instance());
 DB2Storage<ItemSparseEntry> items("ItemSparse.db2",ItemSparseLoadInfo::Instance());
 DB2Storage<CreatureDisplayInfoEntry> displays("CreatureDisplayInfo.db2",CreatureDisplayInfoLoadInfo::Instance());
 DB2Storage<CreatureModelDataEntry> models("CreatureModelData.db2",CreatureModelDataLoadInfo::Instance());
 DB2Storage<FactionTemplateEntry> factions("FactionTemplate.db2",FactionTemplateLoadInfo::Instance());
 DB2Storage<SpellVisualKitEntry> kits("SpellVisualKit.db2",SpellVisualKitLoadInfo::Instance());
 if(!spells.Load(argv[1],0)||!effects.Load(argv[1],0)||!items.Load(argv[1],0)||!displays.Load(argv[1],0)||!models.Load(argv[1],0)||!factions.Load(argv[1],0)||!kits.Load(argv[1],0))return 2;
 std::map<unsigned,unsigned> counts{{181532,1},{145363,3},{207420,1},{182517,1},{154795,1}};
 for(auto const& row:counts){unsigned count=0;auto s=spells.LookupEntry(row.first);if(!s)return 3;std::cout<<"spell "<<row.first<<" "<<s->Name->Str[0]<<'\n';
  for(auto e:effects)if(e->SpellID==row.first){if(e->ImplicitTarget[0]!=1||e->ImplicitTarget[1]||e->EffectTriggerSpell)return 3;
   if(row.first==181532&&(e->Effect!=157||e->EffectMiscValue[0]!=60012||e->EffectAura||!kits.LookupEntry(60012)))return 3;
   if(row.first!=181532&&e->Effect!=6)return 3;
   if(row.first==145363&&e->EffectAura!=4&&e->EffectAura!=12&&e->EffectAura!=120)return 3;
   if(row.first==207420&&(e->EffectAura!=56||e->EffectMiscValue[0]!=104778))return 3;
   if(row.first==182517&&e->EffectAura!=4)return 3;
   if(row.first==154795&&(e->EffectAura!=312||e->EffectMiscValue[0]!=61))return 3;
   ++count;std::cout<<" effect="<<e->Effect<<" aura="<<e->EffectAura<<" misc="<<e->EffectMiscValue[0]<<" trigger="<<e->EffectTriggerSpell<<" target="<<e->ImplicitTarget[0]<<","<<e->ImplicitTarget[1]<<'\n';}
  if(count!=row.second)return 3;
 }
 auto i=items.LookupEntry(122581);if(!i||i->MaxCount||i->Stackable!=1||i->StartQuestID||i->Flags[0]!=67584||i->Flags[1]!=8192)return 4;
 std::cout<<"item122581 "<<i->Display->Str[0]<<" max="<<i->MaxCount<<" stack="<<i->Stackable<<" start="<<i->StartQuestID<<" flags="<<i->Flags[0]<<","<<i->Flags[1]<<'\n';
 for(unsigned id:{62465u,29084u,57965u,62917u,62918u}){auto d=displays.LookupEntry(id);if(!d)return 5;auto m=models.LookupEntry(d->ModelID);if(!m||!m->FileDataID)return 5;std::cout<<"display "<<id<<" model="<<d->ModelID<<" file="<<m->FileDataID<<'\n';}
 auto horde=factions.LookupEntry(2);if(!horde)return 6;
 for(unsigned id:{83u,84u}){auto f=factions.LookupEntry(id);if(!f||f->IsHostileTo(*horde)!=(id==84))return 6;std::cout<<"faction "<<id<<" hostileHorde="<<f->IsHostileTo(*horde)<<'\n';}
 std::cout<<"PASS: native intel item, clicker-self visual-only181532/kit60012, Knockwhistle feign-death and installed leader aura effects, models and Horde hostility. Aura installation and catalog existence do not verify disguise/visual playback.\n";
}
