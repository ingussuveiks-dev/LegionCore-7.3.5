#include "DB2Store.h"
#include "DB2Structure.h"
#include "DB2LoadInfo.h"
#include <iostream>
#include <set>
#include "QuestDef.h"
static_assert(QUEST_OBJECTIVE_FLAG_TRACKED_ON_MINIMAP==1);
int main(int argc,char** argv){if(argc!=2)return 1;
 DB2Storage<SpellEntry> spells("Spell.db2",SpellLoadInfo::Instance());DB2Storage<SpellEffectEntry> effects("SpellEffect.db2",SpellEffectLoadInfo::Instance());
 DB2Storage<VehicleEntry> vehicles("Vehicle.db2",VehicleLoadInfo::Instance());DB2Storage<VehicleSeatEntry> seats("VehicleSeat.db2",VehicleSeatLoadInfo::Instance());
 if(!spells.Load(argv[1],0)||!effects.Load(argv[1],0)||!vehicles.Load(argv[1],0)||!seats.Load(argv[1],0))return 1;
 DB2Storage<CreatureDisplayInfoEntry> displays("CreatureDisplayInfo.db2",CreatureDisplayInfoLoadInfo::Instance());DB2Storage<CreatureModelDataEntry> models("CreatureModelData.db2",CreatureModelDataLoadInfo::Instance());DB2Storage<FactionTemplateEntry> factions("FactionTemplate.db2",FactionTemplateLoadInfo::Instance());DB2Storage<QuestLineXQuestEntry> line("QuestLineXQuest.db2",QuestLineXQuestLoadInfo::Instance());
 if(!displays.Load(argv[1],0)||!models.Load(argv[1],0)||!factions.Load(argv[1],0)||!line.Load(argv[1],0))return 3;
 for(unsigned id:{65873u,68213u,76630u}){auto d=displays.LookupEntry(id);if(!d)return 4;auto m=models.LookupEntry(d->ModelID);if(!m||!m->FileDataID)return 5;std::cout<<"display "<<id<<" model "<<d->ModelID<<" file "<<m->FileDataID<<'\n';}
 for(unsigned id:{14u,2780u}){auto enemy=factions.LookupEntry(id);if(!enemy)return 6;for(unsigned player:{1u,2u}){auto p=factions.LookupEntry(player);if(!p||!enemy->IsHostileTo(*p))return 7;std::cout<<"faction "<<id<<" hostile to "<<player<<'\n';}}
 for(unsigned id:{35u,190u}){auto npc=factions.LookupEntry(id);if(!npc)return 6;for(unsigned player:{1u,2u}){auto p=factions.LookupEntry(player);if(!p||npc->IsHostileTo(*p))return 7;std::cout<<"faction "<<id<<" non-hostile to "<<player<<'\n';}}
 for(auto q:line)if(q->QuestID>=40002&&q->QuestID<=40005||q->QuestID==40072)std::cout<<"quest "<<q->QuestID<<" line "<<q->QuestLineID<<" order "<<unsigned(q->OrderIndex)<<'\n';
 std::set<unsigned> selected{182921,191780,191801,191802,193336,193341,193342,193362,193485,193493,193504,209076,214298,193975,201938};
 for(auto e:effects)if((e->Effect==134||e->Effect==90)&&(e->EffectMiscValue[0]==92307||e->EffectMiscValue[0]==103766))selected.insert(e->SpellID);
 for(auto id:selected){auto s=spells.LookupEntry(id);if(!s)return 8;std::cout<<"spell "<<id<<" "<<s->Name->Str[0]<<'\n';for(auto e:effects)if(e->SpellID==id)std::cout<<" effect="<<e->Effect<<" aura="<<e->EffectAura<<" misc="<<e->EffectMiscValue[0]<<","<<e->EffectMiscValue[1]<<" trigger="<<e->EffectTriggerSpell<<" target="<<e->ImplicitTarget[0]<<","<<e->ImplicitTarget[1]<<" base="<<e->EffectBasePoints<<'\n';}
 auto v=vehicles.LookupEntry(4367);if(!v)return 2;for(auto id:v->SeatID)if(id){auto s=seats.LookupEntry(id);if(!s)return 9;std::cout<<"seat "<<id<<" flags "<<s->Flags<<" "<<s->FlagsB<<'\n';}
}
