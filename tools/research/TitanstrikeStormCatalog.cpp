#include "DB2Store.h"
#include "DB2Structure.h"
#include "DB2LoadInfo.h"
#include <iostream>
#include <set>
int main(int argc, char** argv)
{
 if(argc!=2)return 1;
 DB2Storage<SpellEntry> names("Spell.db2",SpellLoadInfo::Instance());
 DB2Storage<SpellEffectEntry> effects("SpellEffect.db2",SpellEffectLoadInfo::Instance());
 DB2Storage<SpellMiscEntry> misc("SpellMisc.db2",SpellMiscLoadInfo::Instance());
 DB2Storage<SpellDurationEntry> durations("SpellDuration.db2",SpellDurationLoadInfo::Instance());
 DB2Storage<SummonPropertiesEntry> summons("SummonProperties.db2",SummonPropertiesLoadInfo::Instance());
 DB2Storage<ConversationLineEntry> lines("ConversationLine.db2",ConversationLineLoadInfo::Instance());
 DB2Storage<BroadcastTextEntry> texts("BroadcastText.db2",BroadcastTextLoadInfo::Instance());
 DB2Storage<SpellRadiusEntry> radii("SpellRadius.db2",SpellRadiusLoadInfo::Instance());
 DB2Storage<SpellXSpellVisualEntry> visuals("SpellXSpellVisual.db2",SpellXSpellVisualLoadInfo::Instance());
 if(!names.Load(argv[1],0)||!effects.Load(argv[1],0)||!misc.Load(argv[1],0)||!durations.Load(argv[1],0)||!summons.Load(argv[1],0)||!lines.Load(argv[1],0)||!texts.Load(argv[1],0))return 2;
 if(!radii.Load(argv[1],0)||!visuals.Load(argv[1],0))return 3;
 std::set<int32> spells={187207,191038,201239,210880,210942,210975,210976,210977,210978,46598,210825,210831,210836,210845,210848,211733,211736,211016,211097};
 for(size_t old=0;old!=spells.size();){old=spells.size();for(auto e:effects)if(!e->DifficultyID&&spells.count(e->SpellID)&&e->EffectTriggerSpell)spells.insert(e->EffectTriggerSpell);}
 for(auto id:spells)if(auto s=names.LookupEntry(id))std::cout<<"SPELL\t"<<id<<'\t'<<s->Name->Str[0]<<'\t'<<s->Description->Str[0]<<'\n';
 for(auto e:effects)if(!e->DifficultyID&&spells.count(e->SpellID)) {
  std::cout<<"EFFECT\t"<<e->SpellID<<'\t'<<e->EffectIndex<<'\t'<<e->Effect<<'\t'<<e->EffectAura<<'\t'<<e->EffectMiscValue[0]<<'\t'<<e->EffectMiscValue[1]<<'\t'<<e->EffectTriggerSpell<<'\t'<<e->ImplicitTarget[0]<<'\t'<<e->ImplicitTarget[1]<<'\t'<<e->EffectRadiusIndex[0]<<'\t'<<e->EffectRadiusIndex[1]<<'\n';
  if(e->Effect==28)if(auto p=summons.LookupEntry(e->EffectMiscValue[1]))std::cout<<"SUMMON\t"<<e->SpellID<<'\t'<<p->ID<<'\t'<<p->Control<<'\t'<<p->Title<<'\t'<<p->Slot<<'\t'<<p->Flags<<'\n';
 }
 for(auto m:misc)if(!m->DifficultyID&&spells.count(m->SpellID)){auto d=durations.LookupEntry(m->DurationIndex);std::cout<<"MISC\t"<<m->SpellID<<'\t'<<m->RangeIndex<<'\t'<<(d?d->Duration:0)<<'\t'<<m->Speed<<'\n';}
 for(int id:{18,27,108})if(auto r=radii.LookupEntry(id))std::cout<<"RADIUS\t"<<id<<'\t'<<r->Radius<<'\t'<<r->RadiusMax<<'\n';
 for(auto v:visuals)if(spells.count(v->SpellID))std::cout<<"VISUAL\t"<<v->SpellID<<'\t'<<v->SpellVisualID<<'\n';
 for(auto l:lines)if(l->ID>=4250&&l->ID<=4450){auto t=texts.LookupEntry(l->BroadcastTextID);std::cout<<"LINE\t"<<l->ID<<'\t'<<l->BroadcastTextID<<'\t'<<l->SpellVisualKitID<<'\t'<<l->AnimKitID<<'\t'<<l->AdditionalDuration<<'\t'<<(t?t->SoundEntriesID[0]:0)<<'\t'<<(t?t->Text->Str[0]:"")<<'\n';}
}
