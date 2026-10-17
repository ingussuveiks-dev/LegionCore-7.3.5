#include "DB2Store.h"
#include "DB2Structure.h"
#include "DB2LoadInfo.h"
#include <iostream>
int main(int argc,char**argv){
 if(argc!=2)return 1;
 DB2Storage<SpellEffectEntry> effects("SpellEffect.db2",SpellEffectLoadInfo::Instance());
 DB2Storage<SpellMiscEntry> misc("SpellMisc.db2",SpellMiscLoadInfo::Instance());
 DB2Storage<SpellCastTimesEntry> times("SpellCastTimes.db2",SpellCastTimesLoadInfo::Instance());
 DB2Storage<MovieEntry> movies("Movie.db2",MovieLoadInfo::Instance());
 DB2Storage<CreatureDisplayInfoEntry> displays("CreatureDisplayInfo.db2",CreatureDisplayInfoLoadInfo::Instance());
 DB2Storage<CreatureModelDataEntry> models("CreatureModelData.db2",CreatureModelDataLoadInfo::Instance());
 DB2Storage<QuestLineXQuestEntry> lines("QuestLineXQuest.db2",QuestLineXQuestLoadInfo::Instance());
 DB2Storage<QuestPackageItemEntry> packages("QuestPackageItem.db2",QuestPackageItemLoadInfo::Instance());
 DB2Storage<ItemEntry> items("Item.db2",ItemLoadInfo::Instance());
 if(!effects.Load(argv[1],0)||!misc.Load(argv[1],0)||!times.Load(argv[1],0)||!movies.Load(argv[1],0)||!displays.Load(argv[1],0)||!models.Load(argv[1],0)||!lines.Load(argv[1],0)||!packages.Load(argv[1],0)||!items.Load(argv[1],0))return 2;
 unsigned count=0,auxiliary=0;
 for(auto e:effects)if(e->SpellID==190073){++count;std::cout<<"spell190073 index="<<e->EffectIndex<<" effect="<<e->Effect<<" target="<<e->ImplicitTarget[0]<<" misc="<<e->EffectMiscValue[0]<<" trigger="<<e->EffectTriggerSpell<<'\n';
  if(e->EffectIndex==0&&(e->Effect!=45||e->ImplicitTarget[0]!=1||e->EffectMiscValue[0]!=494))return 3;
  if(e->EffectIndex==1&&e->Effect!=199)return 3;
  if(e->EffectIndex==2&&(e->Effect!=64||e->EffectTriggerSpell!=229076))return 3;
 }
 for(auto e:effects)if(e->SpellID==229076){if(e->Effect!=77||e->EffectIndex)return 4;++auxiliary;}
 if(count!=3||auxiliary!=1)return 4;
 bool instant=false;for(auto s:misc)if(s->SpellID==190073){auto t=times.LookupEntry(s->CastingTimeIndex);if(!t||t->Base||s->Speed)return 5;instant=true;}if(!instant)return 5;
 auto movie=movies.LookupEntry(494);if(!movie||movie->AudioFileDataID!=1487146||movie->SubtitleFileDataID!=1488499)return 6;
 std::cout<<"movie494 audio="<<movie->AudioFileDataID<<" subtitles="<<movie->SubtitleFileDataID<<'\n';
 for(auto id:{62093,34520}){auto d=displays.LookupEntry(id);if(!d)return 7;auto m=models.LookupEntry(d->ModelID);if(!m||!m->FileDataID)return 7;std::cout<<"actor display="<<id<<" model="<<m->ID<<" file="<<m->FileDataID<<'\n';}
 bool chapter=false;for(auto q:lines)if(q->QuestID==39122){std::cout<<"quest39122 chapter="<<q->QuestLineID<<" order="<<unsigned(q->OrderIndex)<<'\n';if(q->QuestLineID!=136||q->OrderIndex!=6)return 8;chapter=true;}if(!chapter)return 8;
 unsigned rewards=0;for(auto r:packages)if(r->PackageID==754){if(!items.LookupEntry(r->ItemID)||r->ItemQuantity<=0)return 9;++rewards;std::cout<<"reward754 item="<<r->ItemID<<" quantity="<<r->ItemQuantity<<" displayType="<<unsigned(r->DisplayType)<<'\n';}if(!rewards)return 9;
 std::cout<<"PASS: native instant190073/PLAY_MOVIE494 and retained effects199/trigger229076, movie audio/subtitle references, Genn/Lorna models, chapter136/order6 and reward754 item links. Playback/reward selection remain client checks.\n";
}
