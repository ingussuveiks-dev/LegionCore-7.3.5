#include "DB2Store.h"
#include "DB2Structure.h"
#include "DB2LoadInfo.h"
#include <iostream>
int main(int argc,char** argv)
{
 if(argc!=2)return 1;
 std::cout<<std::unitbuf;
 DB2Storage<SpellEntry> spells("Spell.db2",SpellLoadInfo::Instance());
 DB2Storage<SpellEffectEntry> effects("SpellEffect.db2",SpellEffectLoadInfo::Instance());
 DB2Storage<ItemSparseEntry> items("ItemSparse.db2",ItemSparseLoadInfo::Instance());
 DB2Storage<ItemEffectEntry> uses("ItemEffect.db2",ItemEffectLoadInfo::Instance());
 DB2Storage<SpellAuraRestrictionsEntry> auras("SpellAuraRestrictions.db2",SpellAuraRestrictionsLoadInfo::Instance());
 DB2Storage<SpellTargetRestrictionsEntry> targets("SpellTargetRestrictions.db2",SpellTargetRestrictionsLoadInfo::Instance());
 DB2Storage<SpellMiscEntry> misc("SpellMisc.db2",SpellMiscLoadInfo::Instance());
 DB2Storage<SpellDurationEntry> durations("SpellDuration.db2",SpellDurationLoadInfo::Instance());
 DB2Storage<SpellRangeEntry> ranges("SpellRange.db2",SpellRangeLoadInfo::Instance());
 DB2Storage<SpellRadiusEntry> radii("SpellRadius.db2",SpellRadiusLoadInfo::Instance());
 DB2Storage<CreatureDisplayInfoEntry> displays("CreatureDisplayInfo.db2",CreatureDisplayInfoLoadInfo::Instance());
 DB2Storage<CreatureModelDataEntry> models("CreatureModelData.db2",CreatureModelDataLoadInfo::Instance());
 if(!spells.Load(argv[1],0)||!effects.Load(argv[1],0)||!items.Load(argv[1],0)||!uses.Load(argv[1],0)||!auras.Load(argv[1],0)||!targets.Load(argv[1],0)||!misc.Load(argv[1],0)||!durations.Load(argv[1],0)||!ranges.Load(argv[1],0)||!radii.Load(argv[1],0)||!displays.Load(argv[1],0)||!models.Load(argv[1],0))return 2;
 auto i=items.LookupEntry(129725);if(!i||i->Stackable!=1||i->MaxCount||i->Bonding!=4||i->Flags[0]!=67648||i->Flags[1]!=8192||i->StartQuestID)return 3;
 unsigned useCount=0;for(auto u:uses)if(u->ItemID==129725){if(u->SpellID!=194067||u->TriggerType||u->Charges)return 3;++useCount;}
 if(useCount!=1)return 3;std::cout<<"Smoldering Torch129725: one on-use194067, unlimited charges, stack1, quest bonding4, flags67648/8192\n";
 auto dur=durations.LookupEntry(1);auto range=ranges.LookupEntry(291);
 if(!dur||dur->Duration!=10000||dur->MaxDuration!=10000||!range||range->RangeMax[0]!=15||range->RangeMax[1]!=15)return 4;
 unsigned hit=0,burn=0;for(auto e:effects){
  if(e->SpellID==194067){auto r=radii.LookupEntry(e->EffectRadiusIndex[0]);if(e->Effect!=3||e->ImplicitTarget[0]!=8||e->ImplicitTarget[1]||!r||r->Radius!=5||e->EffectTriggerSpell)return 4;++hit;}
  if(e->SpellID==194069){if(e->Effect!=6||e->EffectAura!=4||e->ImplicitTarget[0]!=1||e->ImplicitTarget[1]||e->EffectTriggerSpell)return 4;++burn;}
 }
 if(hit!=1||burn!=1)return 4;
 uint32 expected[14]{696254848,1056,273170437,269681152,8388736,393224,4608,0,0,1048576,0,0,0,131072};unsigned torchMisc=0,burnMisc=0,targetCount=0;
 for(auto m:misc){if(m->SpellID==194067){if(m->RangeIndex!=291||m->DurationIndex)return 5;for(unsigned n=0;n<14;++n)if(uint32(m->Attributes[n])!=expected[n])return 5;++torchMisc;}
  if(m->SpellID==194069){if(m->DurationIndex!=1)return 5;++burnMisc;}}
 for(auto a:auras)if(a->SpellID==194067)return 5;
 for(auto t:targets)if(t->SpellID==194067){if(t->Targets!=64||t->TargetCreatureType)return 5;++targetCount;}
 if(torchMisc!=1||burnMisc!=1||targetCount!=1)return 5;
 std::cout<<"194067: native destination64, area-entry target8, radius5, range15, dummy hit.194069: native self dummy aura, duration10000ms. No native aura restriction on194067.\n";
 for(auto id:{25240u,25241u,25242u,63102u}){auto d=displays.LookupEntry(id);if(!d)return 6;auto m=models.LookupEntry(d->ModelID);if(!m||!m->FileDataID)return 6;std::cout<<"display"<<id<<" model="<<m->ID<<" file="<<m->FileDataID<<'\n';}
 for(auto id:{194067u,194069u,159474u,211179u}){auto s=spells.LookupEntry(id);if(!s)return 7;std::cout<<"spell"<<id<<" "<<s->Name->Str[0]<<'\n';}
 std::cout<<"Native spell IDs retained. Attachment/animation/fire playback and quest item UI still require client verification.\n";
}
