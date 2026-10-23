// Native build-26972 invitation, messenger and playable race/spec catalog.
#include "DB2Store.h"
#include "DB2Structure.h"
#include "DB2LoadInfo.h"
#include <iostream>
#include <set>
#include <string>
std::string CleanOfferText(char const* p){std::string s=p?p:"";for(auto& c:s)if(c=='\t'||c=='\n'||c=='\r')c=' ';return s;}
int main(int argc,char** argv)
{
 if(argc!=2)return 1;
 DB2Storage<CharBaseInfoEntry> pairs("CharBaseInfo.db2",CharBaseInfoLoadInfo::Instance());
 DB2Storage<ChrRacesEntry> races("ChrRaces.db2",ChrRacesLoadInfo::Instance());
 DB2Storage<ChrSpecializationEntry> specs("ChrSpecialization.db2",ChrSpecializationLoadInfo::Instance());
 DB2Storage<SpellEntry> spells("Spell.db2",SpellLoadInfo::Instance());
 DB2Storage<SpellEffectEntry> effects("SpellEffect.db2",SpellEffectLoadInfo::Instance());
 DB2Storage<SpellMiscEntry> misc("SpellMisc.db2",SpellMiscLoadInfo::Instance());
 DB2Storage<SpellDurationEntry> durations("SpellDuration.db2",SpellDurationLoadInfo::Instance());
 if(!pairs.Load(argv[1],0)||!races.Load(argv[1],0)||!specs.Load(argv[1],0)||!spells.Load(argv[1],0)||!effects.Load(argv[1],0)||!misc.Load(argv[1],0)||!durations.Load(argv[1],0))return 2;
 unsigned pairCount=0,specCount=0;
 for(auto p:pairs){auto r=races.LookupEntry(p->RaceID);if(!r)return 3;++pairCount;std::cout<<"PAIR\t"<<unsigned(p->RaceID)<<'\t'<<unsigned(p->ClassID)<<'\t'<<unsigned(r->Alliance)<<'\t'<<CleanOfferText(r->Name->Str[0])<<'\n';}
 for(auto s:specs)if(s->ClassID&&s->ClassID<=12){++specCount;std::cout<<"SPEC\t"<<s->ID<<'\t'<<unsigned(s->ClassID)<<'\t'<<CleanOfferText(s->Name->Str[0])<<'\n';}
 std::set<uint32> wanted{216443,224250,190886,224350,196908,224263,201208,224286,226409,224339,200023,227342,227324,195356,204860,224338,193978,224344,199277,224335,195286,224244};
 // Shaman uses the existing static Thrall instead of an invitation area spell.
 for(auto e:effects)if(e->Effect==28&&e->EffectMiscValue[0]==91731)wanted.insert(e->SpellID);
 for(auto id:wanted){auto s=spells.LookupEntry(id);if(!s)return 4;std::cout<<"SPELL\t"<<id<<'\t'<<CleanOfferText(s->Name->Str[0])<<'\t'<<CleanOfferText(s->Description->Str[0])<<'\n';}
 for(auto e:effects)if(wanted.count(e->SpellID)&&!e->DifficultyID)
  std::cout<<"EFFECT\t"<<e->SpellID<<'\t'<<e->EffectIndex<<'\t'<<e->Effect<<'\t'<<e->EffectAura<<'\t'<<e->EffectMiscValue[0]<<'\t'<<e->EffectMiscValue[1]<<'\t'<<e->EffectTriggerSpell<<'\n';
 for(auto e:effects)if(!e->DifficultyID&&e->Effect==16)std::cout<<"CHOSEN_EFFECT\t"<<e->SpellID<<'\t'<<e->EffectMiscValue[0]<<'\n';
 for(auto m:misc)if(wanted.count(m->SpellID)&&!m->DifficultyID){auto d=durations.LookupEntry(m->DurationIndex);std::cout<<"DURATION\t"<<m->SpellID<<'\t'<<(d?d->Duration:0)<<'\n';}
 if(pairCount!=149||specCount!=36)return 5;
 std::cout<<"PASS\t149 native race/class pairs and 36 player specializations\n";
}
