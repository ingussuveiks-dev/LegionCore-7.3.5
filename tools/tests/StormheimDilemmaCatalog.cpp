#include "DB2Store.h"
#include "DB2Structure.h"
#include "DB2LoadInfo.h"
#include <iostream>
#include <map>
int main(int argc,char**argv){
 if(argc!=2)return 1;
 DB2Storage<CreatureDisplayInfoEntry> displays("CreatureDisplayInfo.db2",CreatureDisplayInfoLoadInfo::Instance());
 DB2Storage<CreatureModelDataEntry> models("CreatureModelData.db2",CreatureModelDataLoadInfo::Instance());
 DB2Storage<FactionTemplateEntry> factions("FactionTemplate.db2",FactionTemplateLoadInfo::Instance());
 DB2Storage<QuestLineXQuestEntry> lines("QuestLineXQuest.db2",QuestLineXQuestLoadInfo::Instance());
 DB2Storage<SpellEntry> spells("Spell.db2",SpellLoadInfo::Instance());
 DB2Storage<SpellEffectEntry> effects("SpellEffect.db2",SpellEffectLoadInfo::Instance());
 if(!displays.Load(argv[1],0)||!models.Load(argv[1],0)||!factions.Load(argv[1],0)||!lines.Load(argv[1],0))return 2;
 if(!spells.Load(argv[1],0)||!effects.Load(argv[1],0))return 2;
 for(unsigned id:{63721u,63722u,63723u,63724u,63726u,63727u,63728u,63729u,66644u}){auto d=displays.LookupEntry(id);if(!d)return 3;auto m=models.LookupEntry(d->ModelID);if(!m||!m->FileDataID)return 3;std::cout<<"display "<<id<<" model "<<m->ID<<" file "<<m->FileDataID<<'\n';}
 auto horde=factions.LookupEntry(2),alliance=factions.LookupEntry(1),giver=factions.LookupEntry(118);if(!horde||!alliance||!giver||giver->IsHostileTo(*horde))return 4;
 for(unsigned id:{2166u,2618u}){auto f=factions.LookupEntry(id);if(!f||!f->IsHostileTo(*horde))return 4;std::cout<<"faction "<<id<<" hostileHorde="<<f->IsHostileTo(*horde)<<" hostileAlliance="<<f->IsHostileTo(*alliance)<<'\n';}
 std::map<unsigned,unsigned> expected{{39153,0},{38873,1},{39385,2},{39154,3}};unsigned found=0;
 for(auto q:lines)if(expected.count(q->QuestID)){if(q->QuestLineID!=137||q->OrderIndex!=expected.at(q->QuestID))return 5;std::cout<<"quest "<<q->QuestID<<" chapter137 order"<<unsigned(q->OrderIndex)<<'\n';++found;}if(found!=4)return 5;
 auto spell=spells.LookupEntry(168875);if(!spell)return 6;std::cout<<"OOC spell168875 "<<spell->Name->Str[0]<<'\n';unsigned cosmetic=0;for(auto e:effects)if(e->SpellID==168875){if(e->Effect!=3||e->EffectAura||e->EffectMiscValue[0]||e->EffectTriggerSpell||e->ImplicitTarget[0]!=46||e->ImplicitTarget[1]!=91)return 6;++cosmetic;std::cout<<" effect="<<e->Effect<<" aura="<<e->EffectAura<<" misc="<<e->EffectMiscValue[0]<<" trigger="<<e->EffectTriggerSpell<<" target="<<e->ImplicitTarget[0]<<","<<e->ImplicitTarget[1]<<'\n';}if(cosmetic!=1)return 6;
 std::cout<<"PASS: native opponent/giver models, hostile Horde faction templates2166/2618, friendly giver118 and all three chapter137 branches0/1/2 before delivery3. Native table order is not an AND prerequisite.\n";
}
