#include "DB2Store.h"
#include "DB2Structure.h"
#include "DB2LoadInfo.h"
#include <iostream>
#include <set>
int main(int argc,char**argv){
 if(argc!=2)return 1;
 DB2Storage<SpellEffectEntry> effects("SpellEffect.db2",SpellEffectLoadInfo::Instance());
 DB2Storage<SpellMiscEntry> misc("SpellMisc.db2",SpellMiscLoadInfo::Instance());
 DB2Storage<SpellCastTimesEntry> times("SpellCastTimes.db2",SpellCastTimesLoadInfo::Instance());
 DB2Storage<SpellDurationEntry> duration("SpellDuration.db2",SpellDurationLoadInfo::Instance());
 DB2Storage<SpellXSpellVisualEntry> visuals("SpellXSpellVisual.db2",SpellXSpellVisualLoadInfo::Instance());
 DB2Storage<LockEntry> locks("Lock.db2",LockLoadInfo::Instance());
 DB2Storage<GameObjectDisplayInfoEntry> objects("GameObjectDisplayInfo.db2",GameobjectDisplayInfoLoadInfo::Instance());
 DB2Storage<CreatureDisplayInfoEntry> displays("CreatureDisplayInfo.db2",CreatureDisplayInfoLoadInfo::Instance());
 DB2Storage<CreatureModelDataEntry> models("CreatureModelData.db2",CreatureModelDataLoadInfo::Instance());
 DB2Storage<QuestLineXQuestEntry> lines("QuestLineXQuest.db2",QuestLineXQuestLoadInfo::Instance());
 if(!effects.Load(argv[1],0)||!misc.Load(argv[1],0)||!times.Load(argv[1],0)||!duration.Load(argv[1],0)||!visuals.Load(argv[1],0)||!locks.Load(argv[1],0)||!objects.Load(argv[1],0)||!displays.Load(argv[1],0)||!models.Load(argv[1],0)||!lines.Load(argv[1],0))return 2;
 auto lock=locks.LookupEntry(43);if(!lock||lock->Type[1]!=2||lock->Index[1]!=13||lock->Skill[1])return 3;
 unsigned credit=0,opening=0;std::set<unsigned> buffs;
 for(auto e:effects){
  if(e->SpellID==186982){std::cout<<"communion effect"<<e->EffectIndex<<" type="<<e->Effect<<" target="<<e->ImplicitTarget[0]<<" misc="<<e->EffectMiscValue[0]<<'\n';if(e->EffectIndex||e->Effect!=134||e->ImplicitTarget[0]!=1||e->EffectMiscValue[0]!=94477)return 4;++credit;}
  if(e->SpellID==6478&&e->Effect==33&&e->EffectMiscValue[0]==13&&e->ImplicitTarget[0]==23)++opening;
  if(e->SpellID==186965||e->SpellID==187780||e->SpellID==186964||e->SpellID==186972){if(e->ImplicitTarget[0]!=1)return 5;buffs.insert(e->SpellID);std::cout<<"blessing"<<e->SpellID<<" aura="<<e->EffectAura<<" trigger="<<e->EffectTriggerSpell<<'\n';}
 }
 if(credit!=1||!opening||buffs.size()!=4)return 6;
 unsigned cast=0;std::set<unsigned> durations;
 for(auto s:misc){if(s->SpellID==186982){auto t=times.LookupEntry(s->CastingTimeIndex);if(!t||t->Base!=2000||s->Speed)return 7;++cast;std::cout<<"Communing castMs="<<t->Base<<" range="<<s->RangeIndex<<'\n';}
  if(buffs.count(s->SpellID)){auto d=duration.LookupEntry(s->DurationIndex);if(!d||d->Duration!=600000)return 8;durations.insert(s->SpellID);}}
 if(!cast||durations.size()!=4)return 8;
 bool visual=false;for(auto v:visuals)if(v->SpellID==186982&&v->SpellVisualID==41037)visual=true;if(!visual)return 9;
 for(auto id:{26892,27227,35263,35264}){auto d=objects.LookupEntry(id);if(!d||!d->FileDataID)return 10;std::cout<<"shrine display="<<id<<" file="<<d->FileDataID<<'\n';}
 auto d=displays.LookupEntry(70980);if(!d)return 11;auto m=models.LookupEntry(d->ModelID);if(!m||!m->FileDataID)return 11;std::cout<<"statue model="<<m->ID<<" file="<<m->FileDataID<<'\n';
 std::set<unsigned> chapters;for(auto q:lines)if(q->QuestID==39092||q->QuestID==39155){if(q->QuestLineID!=(q->QuestID==39092?136:137)||q->OrderIndex!=5)return 12;chapters.insert(q->QuestID);}if(chapters.size()!=2)return 12;
 std::cout<<"PASS: native Lock43/open6478, four self blessings/ten-minute durations, two-second self communion134/94477 and visual41037, actual shrine/statue models and both faction chapters136/137 order5. Visual playback remains client pending.\n";
}
