// Read actual Legion DB2 structures with the production native reader.
// Optional argv[2]: one spell ID per line. Named candidates are a research
// index, not evidence that every same-named spell belongs to this dungeon.
// S=name/description; E=effect; M=duration/cast; A=attributes; T=target cap; V=visual.
#include "DB2Store.h"
#include "DB2Structure.h"
#include "DB2LoadInfo.h"
#include <algorithm>
#include <fstream>
#include <iostream>
#include <set>
#include <regex>
#include <string>
std::string Clean(char const* text){std::string s=text?text:"";for(auto& c:s)if(c=='\t'||c=='\r'||c=='\n')c=' ';return s;}
int main(int argc,char**argv)
{
 if(argc<2)return 1;
 DB2Storage<SpellEntry> names("Spell.db2",SpellLoadInfo::Instance());
 DB2Storage<SpellEffectEntry> effects("SpellEffect.db2",SpellEffectLoadInfo::Instance());
 DB2Storage<SpellMiscEntry> misc("SpellMisc.db2",SpellMiscLoadInfo::Instance());
 DB2Storage<SpellXSpellVisualEntry> visuals("SpellXSpellVisual.db2",SpellXSpellVisualLoadInfo::Instance());
 DB2Storage<SpellDurationEntry> durations("SpellDuration.db2",SpellDurationLoadInfo::Instance());
 DB2Storage<SpellCastTimesEntry> casts("SpellCastTimes.db2",SpellCastTimesLoadInfo::Instance());
 DB2Storage<SpellTargetRestrictionsEntry> targets("SpellTargetRestrictions.db2",SpellTargetRestrictionsLoadInfo::Instance());
 if(!names.Load(argv[1],0)||!effects.Load(argv[1],0)||!misc.Load(argv[1],0)||!visuals.Load(argv[1],0)||!durations.Load(argv[1],0)||!casts.Load(argv[1],0)||!targets.Load(argv[1],0))return 2;
 std::set<int> wanted;
 if(argc>2){std::ifstream file(argv[2]);int id;while(file>>id)wanted.insert(id);}
 std::set<std::string> key{"Vile Mushroom","Vile Burst","Root Burst","Corrupted Infusion","Throw Spear","Poison Spear","Poisoned Spear","Nightfall","Nightmare Toxin","Blood Assault","Dreadfire","Dreadburst","Hatespawn","Tormenting Eye","Tormenting Fear","Scorching Shot","Shadow Decoy","Shoot","Propelling Charge","Disengage","Nightmare Eruption","Poisonous Blood","Noxious Spit","Rotting Earth","Blood Metamorphosis"};
 for(auto s:names)if(s->ID>=190000&&s->ID<230000&&key.count(Clean(s->Name->Str[0])))wanted.insert(s->ID);
 std::regex reference("\\$(?:@spelldesc)?([0-9]{6})");
 bool changed=true;while(changed){changed=false;for(auto e:effects)if(wanted.count(e->SpellID)){if(e->EffectTriggerSpell&&wanted.insert(e->EffectTriggerSpell).second)changed=true;if(e->Effect==3&&e->EffectBasePoints>100000&&e->EffectBasePoints<300000&&names.LookupEntry(e->EffectBasePoints)&&wanted.insert(e->EffectBasePoints).second)changed=true;}for(auto id:std::set<int>(wanted))if(auto s=names.LookupEntry(id)){auto desc=Clean(s->Description->Str[0]);for(std::sregex_iterator i(desc.begin(),desc.end(),reference),end;i!=end;++i)if(wanted.insert(std::stoi((*i)[1])).second)changed=true;}}
 for(auto id:wanted){auto s=names.LookupEntry(id);std::cout<<"S\t"<<id<<'\t'<<(s?Clean(s->Name->Str[0]):"MISSING")<<'\t'<<(s?Clean(s->Description->Str[0]):"")<<'\n';}
 for(auto e:effects)if(wanted.count(e->SpellID))std::cout<<"E\t"<<e->SpellID<<'\t'<<unsigned(e->DifficultyID)<<'\t'<<unsigned(e->EffectIndex)<<'\t'<<e->Effect<<'\t'<<e->EffectAura<<'\t'<<e->EffectMiscValue[0]<<'\t'<<e->EffectMiscValue[1]<<'\t'<<e->EffectBasePoints<<'\t'<<e->EffectTriggerSpell<<'\t'<<unsigned(e->ImplicitTarget[0])<<'\t'<<unsigned(e->ImplicitTarget[1])<<'\t'<<e->EffectAmplitude<<'\t'<<e->EffectAuraPeriod<<'\n';
 for(auto m:misc)if(wanted.count(m->SpellID)){auto d=durations.LookupEntry(m->DurationIndex);auto c=casts.LookupEntry(m->CastingTimeIndex);std::cout<<"M\t"<<m->SpellID<<'\t'<<unsigned(m->DifficultyID)<<'\t'<<m->CastingTimeIndex<<'\t'<<m->DurationIndex<<'\t'<<(d?d->Duration:0)<<'\t'<<m->RangeIndex<<'\t'<<(c?c->Base:0)<<'\n';}
 for(auto m:misc)if(wanted.count(m->SpellID)){std::cout<<"A\t"<<m->SpellID<<'\t'<<unsigned(m->DifficultyID);for(auto a:m->Attributes)std::cout<<'\t'<<a;std::cout<<'\n';}
 for(auto t:targets)if(wanted.count(t->SpellID))std::cout<<"T\t"<<t->SpellID<<'\t'<<unsigned(t->DifficultyID)<<'\t'<<unsigned(t->MaxTargets)<<'\t'<<t->Targets<<'\n';
 for(auto v:visuals)if(wanted.count(v->SpellID))std::cout<<"V\t"<<v->SpellID<<'\t'<<unsigned(v->DifficultyID)<<'\t'<<v->SpellVisualID<<'\n';
}
