// Matching Legion DB2 evidence for Snowfeather, riding and cancelling the owl.
#include "DB2Store.h"
#include "DB2Structure.h"
#include "DB2LoadInfo.h"
#include <iostream>
#include <set>
int main(int argc,char** argv)
{
 if(argc!=2)return 1;
 DB2Storage<SpellEntry> spells("Spell.db2",SpellLoadInfo::Instance());
 DB2Storage<SpellEffectEntry> effects("SpellEffect.db2",SpellEffectLoadInfo::Instance());
 DB2Storage<SpellMiscEntry> misc("SpellMisc.db2",SpellMiscLoadInfo::Instance());
 DB2Storage<SpellDurationEntry> durations("SpellDuration.db2",SpellDurationLoadInfo::Instance());
 if(!spells.Load(argv[1],0)||!effects.Load(argv[1],0)||!misc.Load(argv[1],0)||!durations.Load(argv[1],0))return 2;
 std::set<uint32> wanted{196908,242066,242070,90266,90265,34091,34093};
 for(auto id:wanted){auto s=spells.LookupEntry(id);if(!s)return 3;std::cout<<"SPELL\t"<<id<<'\t'<<s->Name->Str[0]<<'\n';}
 bool summon=false,master=false,cancellable=false;
 for(auto e:effects)if(wanted.count(e->SpellID)&&!e->DifficultyID){
  std::cout<<"EFFECT\t"<<e->SpellID<<'\t'<<e->EffectIndex<<'\t'<<e->Effect<<'\t'<<e->EffectAura<<'\t'<<e->EffectMiscValue[0]<<'\t'<<e->EffectTriggerSpell<<'\n';
  if(e->SpellID==196908&&e->Effect==28&&e->EffectMiscValue[0]==100786)summon=true;
  if(e->SpellID==90266&&e->Effect==36&&e->EffectTriggerSpell==90265)master=true;
 }
 for(auto m:misc)if(wanted.count(m->SpellID)&&!m->DifficultyID){
  auto d=durations.LookupEntry(m->DurationIndex);
  std::cout<<"MISC\t"<<m->SpellID<<'\t'<<(d?d->Duration:0)<<'\t'<<uint32(m->Attributes[0])<<'\n';
  if(m->SpellID==242066)cancellable=!(uint32(m->Attributes[0])&0x80000000u);
 }
 if(!summon||!master||!cancellable)return 4;
 std::cout<<"PASS\t196908 summons Snowfeather100786; trainer90266 teaches Master Riding90265; speed buff242066 permits cancellation\n";
 return 0;
}
