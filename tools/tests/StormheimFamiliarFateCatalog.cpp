#include "DB2Store.h"
#include "DB2Structure.h"
#include "DB2LoadInfo.h"
#include <iostream>
#include <set>
int main(int argc,char** argv)
{
 if(argc!=2)return 1;
 DB2Storage<ItemSparseEntry> items("ItemSparse.db2",ItemSparseLoadInfo::Instance());
 DB2Storage<ItemEffectEntry> use("ItemEffect.db2",ItemEffectLoadInfo::Instance());
 DB2Storage<SpellEntry> spells("Spell.db2",SpellLoadInfo::Instance());
 DB2Storage<SpellEffectEntry> effects("SpellEffect.db2",SpellEffectLoadInfo::Instance());
 DB2Storage<CreatureDisplayInfoEntry> displays("CreatureDisplayInfo.db2",CreatureDisplayInfoLoadInfo::Instance());
 DB2Storage<CreatureModelDataEntry> models("CreatureModelData.db2",CreatureModelDataLoadInfo::Instance());
 DB2Storage<QuestLineXQuestEntry> line("QuestLineXQuest.db2",QuestLineXQuestLoadInfo::Instance());
 if(!items.Load(argv[1],0)||!use.Load(argv[1],0)||!spells.Load(argv[1],0)||!effects.Load(argv[1],0)||!displays.Load(argv[1],0)||!models.Load(argv[1],0)||!line.Load(argv[1],0))return 2;
 for(unsigned id:{129106u,129161u}){auto i=items.LookupEntry(id);if(!i)return 3;std::cout<<"item "<<id<<" "<<i->Display->Str[0]<<" max="<<i->MaxCount<<" stack="<<i->Stackable<<" flags="<<i->Flags[0]<<","<<i->Flags[1]<<"\n";}
 std::set<unsigned> selected;
 for(auto e:use)if(e->ItemID==129106||e->ItemID==129161){std::cout<<"use item="<<e->ItemID<<" spell="<<e->SpellID<<" trigger="<<unsigned(e->TriggerType)<<" charges="<<e->Charges<<'\n';selected.insert(e->SpellID);}
 unsigned credits=0;for(auto e:effects)if((e->Effect==134||e->Effect==90)&&(e->EffectMiscValue[0]==97981||e->EffectMiscValue[0]==97982)){
  if(e->Effect!=134||e->ImplicitTarget[0]!=1||e->ImplicitTarget[1]||!((e->SpellID==193253&&e->EffectMiscValue[0]==97982)||(e->SpellID==193262&&e->EffectMiscValue[0]==97981)))return 4;
  selected.insert(e->SpellID);++credits;
 }
 if(credits!=2)return 5;
 for(unsigned id:selected){auto s=spells.LookupEntry(id);std::cout<<"spell "<<id<<" "<<(s?s->Name->Str[0]:"MISSING")<<'\n';for(auto e:effects)if(e->SpellID==id)std::cout<<" effect="<<e->Effect<<" aura="<<e->EffectAura<<" misc="<<e->EffectMiscValue[0]<<","<<e->EffectMiscValue[1]<<" trigger="<<e->EffectTriggerSpell<<" target="<<e->ImplicitTarget[0]<<","<<e->ImplicitTarget[1]<<" radius="<<e->EffectRadiusIndex[0]<<'\n';}
 for(unsigned id:{62584u,65486u,68213u}){auto d=displays.LookupEntry(id);if(!d)return 6;auto m=models.LookupEntry(d->ModelID);if(!m||!m->FileDataID)return 7;std::cout<<"display "<<id<<" model "<<m->ID<<" file "<<m->FileDataID<<'\n';}
 bool found=false;for(auto q:line)if(q->QuestID==40002){if(q->QuestLineID!=138||q->OrderIndex)return 8;found=true;}
 if(!found)return 9;
 std::cout<<"quest40002 line138 order0; native item/spell/model references only, not client playback\n";
}
