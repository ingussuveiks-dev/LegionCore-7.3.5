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
 unsigned count=0;
 for(auto e:effects)if(e->SpellID==205551){++count;std::cout<<"spell205551 index="<<e->EffectIndex<<" effect="<<e->Effect<<" target="<<e->ImplicitTarget[0]<<" misc="<<e->EffectMiscValue[0]<<" trigger="<<e->EffectTriggerSpell<<'\n';
  if(e->EffectIndex==0&&(e->Effect!=45||e->ImplicitTarget[0]!=1||e->EffectMiscValue[0]!=495))return 3;
  if(e->EffectIndex==1&&(e->Effect!=6||e->EffectAura!=4||e->ImplicitTarget[0]!=1))return 3;

 }

 if(count!=2)return 4;
 bool instant=false;for(auto s:misc)if(s->SpellID==205551){auto t=times.LookupEntry(s->CastingTimeIndex);if(!t||t->Base||s->Speed)return 5;instant=true;}if(!instant)return 5;
 auto movie=movies.LookupEntry(495);if(!movie||movie->AudioFileDataID!=1487146||movie->SubtitleFileDataID!=1488499)return 6;
 std::cout<<"movie495 audio="<<movie->AudioFileDataID<<" subtitles="<<movie->SubtitleFileDataID<<'\n';
 for(auto id:{30776}){auto d=displays.LookupEntry(id);if(!d)return 7;auto m=models.LookupEntry(d->ModelID);if(!m||!m->FileDataID)return 7;std::cout<<"actor display="<<id<<" model="<<m->ID<<" file="<<m->FileDataID<<'\n';}
 bool chapter=false;for(auto q:lines)if(q->QuestID==38882){std::cout<<"quest38882 chapter="<<q->QuestLineID<<" order="<<unsigned(q->OrderIndex)<<'\n';if(q->QuestLineID!=137||q->OrderIndex!=6)return 8;chapter=true;}if(!chapter)return 8;
 unsigned rewards=0;for(auto r:packages)if(r->PackageID==751){if(!items.LookupEntry(r->ItemID)||r->ItemQuantity<=0)return 9;++rewards;std::cout<<"reward751 item="<<r->ItemID<<" quantity="<<r->ItemQuantity<<" displayType="<<unsigned(r->DisplayType)<<'\n';}if(!rewards)return 9;
 std::cout<<"PASS: native instant205551/PLAY_MOVIE495 and retained dummy aura, movie audio/subtitle references, Sylvanas models, chapter137/order6 and reward751 item links. Playback/reward selection remain client checks.\n";
}
