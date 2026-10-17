#include "DB2Store.h"
#include "DB2Structure.h"
#include "DB2LoadInfo.h"
#include <iostream>
#include <map>
#include <set>
int main(int argc,char** argv)
{
 if(argc!=2)return 1;
 DB2Storage<CreatureDisplayInfoEntry> displays("CreatureDisplayInfo.db2",CreatureDisplayInfoLoadInfo::Instance());
 DB2Storage<CreatureModelDataEntry> models("CreatureModelData.db2",CreatureModelDataLoadInfo::Instance());
 DB2Storage<FactionTemplateEntry> factions("FactionTemplate.db2",FactionTemplateLoadInfo::Instance());
 DB2Storage<SpellEntry> spells("Spell.db2",SpellLoadInfo::Instance());
 DB2Storage<SpellEffectEntry> effects("SpellEffect.db2",SpellEffectLoadInfo::Instance());
 DB2Storage<SpellMiscEntry> miscs("SpellMisc.db2",SpellMiscLoadInfo::Instance());
 DB2Storage<SpellDurationEntry> durations("SpellDuration.db2",SpellDurationLoadInfo::Instance());
 DB2Storage<SpellCooldownsEntry> cooldowns("SpellCooldowns.db2",SpellCooldownsLoadInfo::Instance());
 DB2Storage<QuestPackageItemEntry> packages("QuestPackageItem.db2",QuestPackageItemLoadInfo::Instance());
 DB2Storage<ItemSparseEntry> items("ItemSparse.db2",ItemSparseLoadInfo::Instance());
 if(!displays.Load(argv[1],0)||!models.Load(argv[1],0)||!factions.Load(argv[1],0)||!spells.Load(argv[1],0)||!effects.Load(argv[1],0)||!miscs.Load(argv[1],0)||!durations.Load(argv[1],0)||!cooldowns.Load(argv[1],0)||!packages.Load(argv[1],0)||!items.Load(argv[1],0))return 2;
 for(unsigned id:{65144u,70529u,25668u,23553u,70163u}){auto d=displays.LookupEntry(id);if(!d)return 3;auto m=models.LookupEntry(d->ModelID);if(!m||!m->FileDataID)return 3;std::cout<<"display "<<id<<" model "<<m->ID<<" file "<<m->FileDataID<<'\n';}
 auto enemy=factions.LookupEntry(2817),horde=factions.LookupEntry(2),alliance=factions.LookupEntry(1),giver=factions.LookupEntry(35);
 if(!enemy||!enemy->Faction||enemy->IsNeutralToAll()||!horde||!alliance||!giver||!enemy->IsHostileTo(*horde)||!enemy->IsHostileTo(*alliance)||giver->IsHostileTo(*horde)||giver->IsHostileTo(*alliance))return 4;
 std::cout<<"faction2817 hostile to both player factions; Havi35 nonhostile to both\n";
 std::set<uint32> selected{186553,213738,215377,220399,220428,220431,220570,220622,220636,220638,220585,220409,220436};
 std::set<uint32> triggers;
 for(auto id:selected){auto s=spells.LookupEntry(id);if(!s)return 5;unsigned count=0,miscCount=0;std::cout<<"spell "<<id<<" "<<s->Name->Str[0]<<'\n';
  for(auto e:effects)if(e->SpellID==id){++count;if(e->EffectTriggerSpell)triggers.insert(e->EffectTriggerSpell);std::cout<<" effect "<<e->Effect<<" aura "<<e->EffectAura<<" trigger "<<e->EffectTriggerSpell<<" target "<<e->ImplicitTarget[0]<<","<<e->ImplicitTarget[1]<<'\n';}
  for(auto m:miscs)if(m->SpellID==id&&!m->DifficultyID){auto d=durations.LookupEntry(m->DurationIndex);uint32 recovery=0;for(auto c:cooldowns)if(c->SpellID==id&&!c->DifficultyID)recovery=c->RecoveryTime;std::cout<<"profile "<<id<<" "<<uint32(m->Attributes[0])<<" "<<(d?d->Duration:0)<<" "<<recovery<<'\n';++miscCount;}
  if(!count||miscCount!=1)return 5;
 }
 for(auto id:triggers){auto s=spells.LookupEntry(id);if(!s)return 6;std::cout<<"trigger-spell "<<id<<" "<<s->Name->Str[0]<<'\n';unsigned n=0;for(auto e:effects)if(e->SpellID==id){++n;std::cout<<" trigger-effect "<<e->Effect<<" misc "<<e->EffectMiscValue[0]<<" target "<<e->ImplicitTarget[0]<<","<<e->ImplicitTarget[1]<<'\n';}if(!n)return 6;}
 unsigned n=0;for(auto p:packages)if(p->PackageID==9687){auto i=items.LookupEntry(p->ItemID);if(!i||p->ItemQuantity!=1)return 6;++n;std::cout<<"package9687 item "<<p->ItemID<<" display "<<unsigned(p->DisplayType)<<" "<<i->Display->Str[0]<<'\n';}if(n!=5)return 6;
 std::cout<<"PASS: four target and summoned crew displays/models, hostile factions,13 unique spells/native effect and AI-condition input fields, first-hop triggers, quest package9687. This does not prove spell execution or client visuals.\n";
}
