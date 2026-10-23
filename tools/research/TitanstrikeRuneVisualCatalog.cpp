#include "DB2Store.h"
#include "DB2Structure.h"
#include "DB2LoadInfo.h"
#include <iostream>
int main(int argc,char** argv)
{
 if(argc!=2)return 1;
 DB2Storage<CreatureDisplayInfoEntry> displays("CreatureDisplayInfo.db2",CreatureDisplayInfoLoadInfo::Instance());
 DB2Storage<AnimKitEntry> kits("AnimKit.db2",AnimKitLoadInfo::Instance());
 DB2Storage<SpellEntry> spells("Spell.db2",SpellLoadInfo::Instance());
 DB2Storage<SpellEffectEntry> effects("SpellEffect.db2",SpellEffectLoadInfo::Instance());
 DB2Storage<SpellXSpellVisualEntry> visuals("SpellXSpellVisual.db2",SpellXSpellVisualLoadInfo::Instance());
 if(!displays.Load(argv[1],0)||!kits.Load(argv[1],0)||!spells.Load(argv[1],0)||!effects.Load(argv[1],0)||!visuals.Load(argv[1],0))return 2;
 for(int id:{69560,69561,69562,11686}){
  auto d=displays.LookupEntry(id);if(!d)return 3;
  std::cout<<"DISPLAY\t"<<id<<'\t'<<d->ModelID<<'\n';
 }
 if(!kits.LookupEntry(10586)||!spells.LookupEntry(191160))return 4;
 std::cout<<"KIT\t10586\nSPELL\t191160\t"<<spells.LookupEntry(191160)->Name->Str[0]<<'\n';
 bool visual=false;
 for(auto v:visuals)if(v->SpellID==191160){visual=true;std::cout<<"VISUAL\t191160\t"<<v->SpellVisualID<<'\n';}
 for(auto e:effects)if(e->SpellID==191160&&!e->DifficultyID)
  std::cout<<"EFFECT\t191160\t"<<e->EffectIndex<<'\t'<<e->Effect<<'\t'<<e->EffectTriggerSpell<<'\t'<<e->ImplicitTarget[0]<<'\n';
 return visual?0:5;
}
