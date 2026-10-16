#include "DB2Store.h"
#include "DB2Structure.h"
#include "DB2LoadInfo.h"
#include <iostream>
int main(int argc,char** argv)
{
    if(argc!=2)return 1;
    DB2Storage<ConversationLineEntry> lines("ConversationLine.db2",ConversationLineLoadInfo::Instance());
    DB2Storage<CreatureDisplayInfoEntry> actors("CreatureDisplayInfo.db2",CreatureDisplayInfoLoadInfo::Instance());
    DB2Storage<CreatureModelDataEntry> models("CreatureModelData.db2",CreatureModelDataLoadInfo::Instance());
    DB2Storage<FactionTemplateEntry> factions("FactionTemplate.db2",FactionTemplateLoadInfo::Instance());
    DB2Storage<SpellEntry> spells("Spell.db2",SpellLoadInfo::Instance());
    DB2Storage<SpellEffectEntry> effects("SpellEffect.db2",SpellEffectLoadInfo::Instance());
    DB2Storage<SpellMiscEntry> misc("SpellMisc.db2",SpellMiscLoadInfo::Instance());
    DB2Storage<SoundKitEntry> sounds("SoundKit.db2",SoundKitLoadInfo::Instance());
    if(!lines.Load(argv[1],0)||!actors.Load(argv[1],0)||!models.Load(argv[1],0)||!factions.Load(argv[1],0)||!spells.Load(argv[1],0)||!effects.Load(argv[1],0)||!misc.Load(argv[1],0)||!sounds.Load(argv[1],0))return 2;
    uint32 count=0;for(uint32 id=4752;id;){if(++count>16)return 3;auto l=lines.LookupEntry(id);if(!l||!l->BroadcastTextID)return 4;std::cout<<"line"<<id<<" broadcast="<<l->BroadcastTextID<<" next="<<l->NextConversationLineID<<" additional="<<l->AdditionalDuration<<" visualKit="<<l->SpellVisualKitID<<" animKit="<<l->AnimKitID<<'\n';id=l->NextConversationLineID;}
    for(uint32 id:{24301u,65043u}){auto a=actors.LookupEntry(id);if(!a)return 5;auto m=models.LookupEntry(a->ModelID);if(!m||!m->FileDataID)return 6;std::cout<<"display"<<id<<" model="<<a->ModelID<<" file="<<m->FileDataID<<'\n';}
    auto enemy=factions.LookupEntry(1814);if(!enemy)return 7;
    for(uint32 id:{1u,2u}){auto player=factions.LookupEntry(id);if(!player||!enemy->IsHostileTo(*player))return 8;}
    std::cout<<"Geir faction1814 hostileToAlliance=1 hostileToHorde=1\n";
    if(!sounds.LookupEntry(53006))return 12;
    std::cout<<"Helya female sound53006 exists=1\n";
    for(uint32 id:{182866u,182868u,182877u}){auto spell=spells.LookupEntry(id);if(!spell)return 9;std::cout<<"spell"<<id<<" name="<<spell->Name->Str[0]<<'\n';uint32 n=0;for(auto e:effects)if(e->SpellID==id){++n;std::cout<<" effect="<<e->Effect<<" aura="<<e->EffectAura<<" trigger="<<e->EffectTriggerSpell<<" target="<<e->ImplicitTarget[0]<<","<<e->ImplicitTarget[1]<<'\n';if(id==182866&&(e->Effect!=6||e->EffectAura!=23||e->EffectTriggerSpell!=182868))return 10;}if(!n)return 11;}
    std::cout<<"PASS: native Geir/Helya model chains, faction hostile to both players, three combat spell records and sparse2256 line4752 chain. Actual combat, spell timing, sound/animation and client conversation playback remain unverified.\n";
}
