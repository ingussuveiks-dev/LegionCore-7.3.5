#include "DB2Store.h"
#include "DB2Structure.h"
#include "DB2LoadInfo.h"
#include <iostream>
#include <set>
int main(int argc,char**argv){
 if(argc!=2)return 1;std::cout<<std::unitbuf<<"Loading native Greywatch catalog\n";
 DB2Storage<ItemSparseEntry> items("ItemSparse.db2",ItemSparseLoadInfo::Instance());
 DB2Storage<ItemEffectEntry> uses("ItemEffect.db2",ItemEffectLoadInfo::Instance());
 DB2Storage<SpellEntry> spells("Spell.db2",SpellLoadInfo::Instance());
 DB2Storage<SpellEffectEntry> effects("SpellEffect.db2",SpellEffectLoadInfo::Instance());
 DB2Storage<SpellMiscEntry> misc("SpellMisc.db2",SpellMiscLoadInfo::Instance());
 DB2Storage<SpellTargetRestrictionsEntry> targets("SpellTargetRestrictions.db2",SpellTargetRestrictionsLoadInfo::Instance());
 DB2Storage<SpellRadiusEntry> radii("SpellRadius.db2",SpellRadiusLoadInfo::Instance());
 DB2Storage<SpellDurationEntry> durations("SpellDuration.db2",SpellDurationLoadInfo::Instance());
 DB2Storage<SpellXSpellVisualEntry> links("SpellXSpellVisual.db2",SpellXSpellVisualLoadInfo::Instance());
 DB2Storage<GameObjectDisplayInfoEntry> objects("GameObjectDisplayInfo.db2",GameobjectDisplayInfoLoadInfo::Instance());
 DB2Storage<CreatureDisplayInfoEntry> displays("CreatureDisplayInfo.db2",CreatureDisplayInfoLoadInfo::Instance());
 DB2Storage<CreatureModelDataEntry> models("CreatureModelData.db2",CreatureModelDataLoadInfo::Instance());
 DB2Storage<QuestLineXQuestEntry> lines("QuestLineXQuest.db2",QuestLineXQuestLoadInfo::Instance());
 #define LOAD(store) std::cout<<"read "<<#store<<'\n';if(!store.Load(argv[1],0))return 2;
 LOAD(items) LOAD(uses) LOAD(spells) LOAD(effects) LOAD(misc) LOAD(targets) LOAD(radii) LOAD(durations) LOAD(links) LOAD(objects) LOAD(displays) LOAD(models) LOAD(lines)
 #undef LOAD
 for(auto id:{127285,127295}){auto p=items.LookupEntry(id);if(!p||p->MaxCount||p->Stackable!=(id==127285?10:1)||p->StartQuestID||p->Bonding!=4)return 3;std::cout<<"item "<<id<<" "<<p->Display->Str[0]<<" stack="<<p->Stackable<<" flags="<<p->Flags[0]<<","<<p->Flags[1]<<'\n';}
 unsigned useCount=0;for(auto p:uses)if(p->ItemID==127285||p->ItemID==127295){if(p->ItemID!=127295||p->SpellID!=186745||p->TriggerType)return 4;++useCount;}if(useCount!=1)return 4;
 auto spell=spells.LookupEntry(186745);if(!spell)return 5;std::cout<<"spell186745 "<<spell->Name->Str[0]<<'\n';
 unsigned count=0;for(auto p:effects)if(p->SpellID==186745){if(p->Effect!=3||p->ImplicitTarget[0]!=8||p->ImplicitTarget[1]||p->EffectRadiusIndex[0]!=8||p->EffectTriggerSpell)return 5;++count;}if(count!=1)return 5;
 auto radius=radii.LookupEntry(8);if(!radius||radius->Radius!=5||radius->RadiusMax!=5)return 5;
 count=0;for(auto p:targets)if(p->SpellID==186745){if(p->Targets!=64||p->TargetCreatureType||p->MaxTargets)return 6;++count;}if(count!=1)return 6;
 count=0;for(auto p:misc)if(p->SpellID==186745){if(p->RangeIndex!=291||p->CastingTimeIndex!=1||p->Attributes[2]!=273170437)return 6;++count;std::cout<<"torch native radius5 dest-area-entry8, dummy3, range291; attrs=";for(auto a:p->Attributes)std::cout<<a<<',';std::cout<<'\n';}if(count!=1)return 6;
 count=0;for(auto p:links)if(p->SpellID==186745)++count;if(count)return 7;
 for(auto id:{186746,186747}){count=0;for(auto p:links)if(p->SpellID==id){if(p->SpellVisualID!=(id==186746?48032:48033))return 7;++count;std::cout<<"native spell"<<id<<" visual="<<p->SpellVisualID<<'\n';}if(count!=1)return 7;}
 unsigned throwEffects=0,burnEffects=0;for(auto p:effects){if(p->SpellID==186746){if(p->Effect!=3||p->ImplicitTarget[0]!=38)return 7;++throwEffects;}if(p->SpellID==186747){if(p->Effect!=6||p->EffectAura!=4||p->ImplicitTarget[0]!=38)return 7;++burnEffects;}}if(throwEffects!=1||burnEffects!=1)return 7;
 auto duration=durations.LookupEntry(6);if(!duration||duration->Duration!=600000)return 7;
 for(auto m:misc){if(m->SpellID==186746&&m->Speed!=7)return 7;if(m->SpellID==186747&&m->DurationIndex!=6)return 7;}
 for(auto id:{8618,9676}){auto p=objects.LookupEntry(id);if(!p||!p->FileDataID)return 8;std::cout<<"GO display"<<id<<" file="<<p->FileDataID<<'\n';}
 for(auto id:{63804,63938,63939,64271,64268,328}){auto d=displays.LookupEntry(id);if(!d)return 8;auto m=models.LookupEntry(d->ModelID);if(!m||!m->FileDataID)return 8;std::cout<<"actor display"<<id<<" model="<<m->ID<<" file="<<m->FileDataID<<'\n';}
 std::set<unsigned> found;for(auto p:lines)if(p->QuestID==39060||p->QuestID==39061||p->QuestID==39472||p->QuestID==39062){if(p->QuestLineID!=136)return 9;found.insert(p->QuestID);std::cout<<"quest"<<p->QuestID<<" native line="<<p->QuestLineID<<" order="<<unsigned(p->OrderIndex)<<'\n';}if(found.size()!=4)return 9;
 std::cout<<"PASS: native packed item/spell/target/radius/duration/visual-link/model/quest-line records. SpellVisual.db2 loading is disabled in DB2Stores.cpp; audit_greywatch_torch_visual.py checks its raw WDC1 dependencies separately. Client rendering is unverified.\n";
}
