#include "DB2Store.h"
#include "DB2Structure.h"
#include "DB2LoadInfo.h"
#include "MovementPackets.h"
#include "SpellAuraDefines.h"
#include "SpellInfo.h"
#include <iostream>
int main(int argc,char**argv){
 if(argc!=2)return 1;
 DB2Storage<SpellEffectEntry> effects("SpellEffect.db2",SpellEffectLoadInfo::Instance());
 DB2Storage<SpellMiscEntry> misc("SpellMisc.db2",SpellMiscLoadInfo::Instance());
 DB2Storage<SpellCastTimesEntry> times("SpellCastTimes.db2",SpellCastTimesLoadInfo::Instance());
 DB2Storage<SpellRangeEntry> ranges("SpellRange.db2",SpellRangeLoadInfo::Instance());
 DB2Storage<SpellDurationEntry> durations("SpellDuration.db2",SpellDurationLoadInfo::Instance());
 DB2Storage<SpellXSpellVisualEntry> visuals("SpellXSpellVisual.db2",SpellXSpellVisualLoadInfo::Instance());
 DB2Storage<SpellTargetRestrictionsEntry> targets("SpellTargetRestrictions.db2",SpellTargetRestrictionsLoadInfo::Instance());
 DB2Storage<ItemSparseEntry> items("ItemSparse.db2",ItemSparseLoadInfo::Instance());
 DB2Storage<ItemEffectEntry> itemEffects("ItemEffect.db2",ItemEffectLoadInfo::Instance());
 DB2Storage<SummonPropertiesEntry> summons("SummonProperties.db2",SummonPropertiesLoadInfo::Instance());
 DB2Storage<GameObjectDisplayInfoEntry> objects("GameObjectDisplayInfo.db2",GameobjectDisplayInfoLoadInfo::Instance());
 DB2Storage<QuestLineXQuestEntry> lines("QuestLineXQuest.db2",QuestLineXQuestLoadInfo::Instance());
 if(!effects.Load(argv[1],0)||!misc.Load(argv[1],0)||!times.Load(argv[1],0)||!ranges.Load(argv[1],0)||!durations.Load(argv[1],0)||!visuals.Load(argv[1],0)||!targets.Load(argv[1],0)||!items.Load(argv[1],0)||!itemEffects.Load(argv[1],0)||!summons.Load(argv[1],0)||!objects.Load(argv[1],0)||!lines.Load(argv[1],0))return 2;
 unsigned summon=0,create=0,itemLink=0;
 for(auto e:effects){if(e->SpellID==189134){if(e->EffectIndex||e->Effect!=28||e->ImplicitTarget[0]!=46||e->EffectMiscValue[0]!=95409||e->EffectMiscValue[1]!=719)return 3;++summon;}
  if(e->SpellID==189152){if(e->EffectIndex||e->Effect!=24||e->EffectItemType!=128287||e->ImplicitTarget[0]!=1)return 3;++create;}}
 for(auto e:itemEffects)if(e->ItemID==128287){if(e->SpellID!=189134||e->TriggerType)return 4;++itemLink;}
 if(summon!=1||create!=1||itemLink!=1)return 4;
 auto item=items.LookupEntry(128287);if(!item)return 5;std::cout<<"detonator128287 maxCount="<<item->MaxCount<<" stack="<<item->Stackable<<'\n';
 for(auto s:misc)if(s->SpellID==189134){auto t=times.LookupEntry(s->CastingTimeIndex);if(!t||t->Base!=1500)return 6;std::cout<<"detonator cast="<<t->Base<<"ms range="<<s->RangeIndex<<" duration="<<s->DurationIndex<<'\n';}
 auto range=ranges.LookupEntry(245);auto duration=durations.LookupEntry(3);if(!range||!duration||range->RangeMax[0]!=500||range->RangeMax[1]!=500||duration->Duration!=60000)return 6;
 auto visual=visuals.LookupEntry(86440);if(!visual||visual->SpellID!=189134||visual->SpellVisualID!=48812)return 6;
 std::cout<<"native range500yd duration60000ms visual48812; visual playback is not verified.\n";
 for(auto t:targets)if(t->SpellID==189134&&t->Targets)return 7;
 bool src=false,dst=false;SpellImplicitTargetInfo target(46);auto mask=target.GetExplicitTargetMask(src,dst);if(!(mask&TARGET_FLAG_UNIT)||!(mask&TARGET_FLAG_DEST_LOCATION))return 8;
 std::cout<<"target46 native explicitMask="<<mask<<" objectType="<<target.GetObjectType()<<" reference="<<target.GetReferenceType()<<" check="<<target.GetCheckType()<<'\n';
 auto prop=summons.LookupEntry(719);if(!prop)return 9;std::cout<<"summon719 control="<<prop->Control<<" title="<<prop->Title<<" slot="<<prop->Slot<<" flags="<<prop->Flags<<'\n';
 auto model=objects.LookupEntry(8586);if(!model||!model->FileDataID)return 10;std::cout<<"explosives model file="<<model->FileDataID<<'\n';
 unsigned quests=0;for(auto q:lines)if(q->QuestID==39385||q->QuestID==39154){if(q->QuestLineID!=137||q->OrderIndex!=(q->QuestID==39385?2:3))return 11;++quests;}if(quests!=2)return 11;
 std::cout<<"PASS: native item128287 ->1500ms189134 -> target46/object-and-destination/summon95409/properties719;189152 creates that item; explosive model and Horde chapter137 orders2/3.\n";
}
