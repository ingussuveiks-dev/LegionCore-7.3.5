#include "DB2Store.h"
#include "DB2Structure.h"
#include "DB2LoadInfo.h"
#include "MovementPackets.h"
#include "SpellAuraDefines.h"
#include "SpellInfo.h"
#include <iostream>
int main(int argc,char** argv)
{
 if(argc!=2)return 1;
 DB2Storage<ItemSparseEntry> items("ItemSparse.db2",ItemSparseLoadInfo::Instance());
 DB2Storage<ItemEffectEntry> uses("ItemEffect.db2",ItemEffectLoadInfo::Instance());
 DB2Storage<SpellEntry> spells("Spell.db2",SpellLoadInfo::Instance());
 DB2Storage<SpellEffectEntry> effects("SpellEffect.db2",SpellEffectLoadInfo::Instance());
 DB2Storage<SpellMiscEntry> misc("SpellMisc.db2",SpellMiscLoadInfo::Instance());
 DB2Storage<SpellCastTimesEntry> times("SpellCastTimes.db2",SpellCastTimesLoadInfo::Instance());
 DB2Storage<SpellRangeEntry> ranges("SpellRange.db2",SpellRangeLoadInfo::Instance());
 DB2Storage<CreatureDisplayInfoEntry> actors("CreatureDisplayInfo.db2",CreatureDisplayInfoLoadInfo::Instance());
 DB2Storage<CreatureModelDataEntry> models("CreatureModelData.db2",CreatureModelDataLoadInfo::Instance());
 DB2Storage<GameObjectDisplayInfoEntry> objects("GameObjectDisplayInfo.db2",GameobjectDisplayInfoLoadInfo::Instance());
 DB2Storage<QuestLineXQuestEntry> line("QuestLineXQuest.db2",QuestLineXQuestLoadInfo::Instance());
 if(!items.Load(argv[1],0)||!uses.Load(argv[1],0)||!spells.Load(argv[1],0)||!effects.Load(argv[1],0)||!misc.Load(argv[1],0)||!times.Load(argv[1],0)||!ranges.Load(argv[1],0)||!actors.Load(argv[1],0)||!models.Load(argv[1],0)||!objects.Load(argv[1],0)||!line.Load(argv[1],0))return 2;
 auto item=items.LookupEntry(129161);if(!item||item->Stackable!=1)return 3;
 std::cout<<"horn129161 "<<item->Display->Str[0]<<" stack="<<item->Stackable<<" max="<<item->MaxCount<<'\n';
 unsigned found=0;for(auto u:uses)if(u->ItemID==129161){if(u->SpellID!=193327||u->TriggerType!=0||u->Charges)return 4;++found;}
 if(found!=1)return 5;
 for(unsigned id:{193327u,193328u,193329u,193330u,196084u}){
  auto s=spells.LookupEntry(id);if(!s)return 6;std::cout<<"spell "<<id<<" "<<s->Name->Str[0]<<'\n';
  for(auto e:effects)if(e->SpellID==id)std::cout<<" effect="<<e->Effect<<" misc="<<e->EffectMiscValue[0]<<" target="<<e->ImplicitTarget[0]<<","<<e->ImplicitTarget[1]<<'\n';
 }
 bool src=false,dst=false;SpellImplicitTargetInfo target(TARGET_UNIT_NEARBY_ENTRY);
 if(target.GetCheckType()!=TARGET_CHECK_ENTRY||target.GetExplicitTargetMask(src,dst))return 7;
 for(auto m:misc)if(m->SpellID==193327){auto t=times.LookupEntry(m->CastingTimeIndex);auto r=ranges.LookupEntry(m->RangeIndex);if(!t||t->Base||!r||r->RangeMax[0]!=20)return 8;std::cout<<"horn cast0ms range20 native nearby-entry/conditions; explicit mask0\n";}
 auto actor=actors.LookupEntry(68213);if(!actor)return 9;auto model=models.LookupEntry(actor->ModelID);if(!model||!model->FileDataID)return 10;
 std::cout<<"Vethir68213 model="<<model->ID<<" file="<<model->FileDataID<<'\n';
 auto portal=objects.LookupEntry(26244);if(!portal||!portal->FileDataID)return 11;
 std::cout<<"portal26244 file="<<portal->FileDataID<<'\n';
 for(auto q:line)if(q->QuestID==40003)std::cout<<"quest40003 line="<<q->QuestLineID<<" order="<<unsigned(q->OrderIndex)<<'\n';
}
