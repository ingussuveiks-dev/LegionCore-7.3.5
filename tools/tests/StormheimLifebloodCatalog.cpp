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
    DB2Storage<SpellEntry> spells("Spell.db2",SpellLoadInfo::Instance());
    DB2Storage<SpellEffectEntry> effects("SpellEffect.db2",SpellEffectLoadInfo::Instance());
    DB2Storage<SoundKitEntry> sounds("SoundKit.db2",SoundKitLoadInfo::Instance());
    DB2Storage<SummonPropertiesEntry> summons("SummonProperties.db2",SummonPropertiesLoadInfo::Instance());
    DB2Storage<QuestPackageItemEntry> packages("QuestPackageItem.db2",QuestPackageItemLoadInfo::Instance());
    DB2Storage<ItemSparseEntry> items("ItemSparse.db2",ItemSparseLoadInfo::Instance());
    if(!lines.Load(argv[1],0)||!actors.Load(argv[1],0)||!models.Load(argv[1],0)||!spells.Load(argv[1],0)||!effects.Load(argv[1],0)||!sounds.Load(argv[1],0)||!summons.Load(argv[1],0)||!packages.Load(argv[1],0)||!items.Load(argv[1],0))return 2;
    uint32 broadcasts[]={98498,98499,98501};
    for(uint32 i=0;i<3;++i){auto l=lines.LookupEntry(4753+i);if(!l||l->BroadcastTextID!=broadcasts[i]||l->NextConversationLineID)return 3;
        std::cout<<"line"<<l->ID<<" broadcast="<<l->BroadcastTextID<<" next="<<l->NextConversationLineID<<" additional="<<l->AdditionalDuration<<" visualKit="<<l->SpellVisualKitID<<" animKit="<<l->AnimKitID<<'\n';}
    for(uint32 id:{65043u,68635u,76630u}){auto a=actors.LookupEntry(id);if(!a)return 4;auto m=models.LookupEntry(a->ModelID);if(!m||!m->FileDataID)return 5;std::cout<<"display"<<id<<" model="<<a->ModelID<<" file="<<m->FileDataID<<'\n';}
    for(uint32 id:{53005u,52988u,52989u,52991u}){if(!sounds.LookupEntry(id))return 6;std::cout<<"sound"<<id<<" exists=1\n";}
    for(uint32 id:{192470u,192471u,212916u,212917u,212918u,216818u,192670u,192671u,192672u,241140u}){
        auto s=spells.LookupEntry(id);if(!s)return 7;std::cout<<"spell"<<id<<" name="<<s->Name->Str[0]<<'\n';uint32 count=0;
        for(auto e:effects)if(e->SpellID==id){++count;std::cout<<" effect="<<e->Effect<<" aura="<<e->EffectAura<<" trigger="<<e->EffectTriggerSpell<<" misc="<<e->EffectMiscValue[0]<<","<<e->EffectMiscValue[1]<<" target="<<e->ImplicitTarget[0]<<","<<e->ImplicitTarget[1]<<'\n';}
        if(!count)return 8;
    }
    auto summon=summons.LookupEntry(3281);if(!summon||summon->Slot!=6||spells.LookupEntry(81040))return 9;
    std::cout<<"native3281 slot="<<summon->Slot<<" control="<<summon->Control<<" missing81040=1\n";
    uint32 count=0;for(auto p:packages)if(p->PackageID==9690){auto item=items.LookupEntry(p->ItemID);if(!item||p->ItemQuantity!=1)return 10;++count;std::cout<<"package9690 item="<<p->ItemID<<" quantity="<<p->ItemQuantity<<" display="<<uint32(p->DisplayType)<<'\n';}
    if(!count)return 11;
    std::cout<<"PASS: native sparse final conversations, Ashildir/Helya/Havi model chains and voices, exit scene/root effects, absent81040/guardian slot, and real package9690 items. Data references do not verify client playback, exact retail choreography, item eligibility or turn-in.\n";
}
