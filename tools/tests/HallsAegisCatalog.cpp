#include "DB2Store.h"
#include "DB2Structure.h"
#include "DB2LoadInfo.h"
#include "QuestDef.h"
#include <iostream>
static_assert(QUEST_OBJECTIVE_FLAG_TRACKED_ON_MINIMAP==1 && QUEST_OBJECTIVE_FLAG_SEQUENCED==2);
int main(int argc,char** argv)
{
    if(argc!=2)return 1;
    DB2Storage<CreatureDisplayInfoEntry> displays("CreatureDisplayInfo.db2",CreatureDisplayInfoLoadInfo::Instance());
    DB2Storage<CreatureModelDataEntry> models("CreatureModelData.db2",CreatureModelDataLoadInfo::Instance());
    DB2Storage<GameObjectDisplayInfoEntry> objects("GameObjectDisplayInfo.db2",GameobjectDisplayInfoLoadInfo::Instance());
    DB2Storage<QuestLineXQuestEntry> line("QuestLineXQuest.db2",QuestLineXQuestLoadInfo::Instance());
    DB2Storage<MapEntry> maps("Map.db2",MapLoadInfo::Instance());
    DB2Storage<MapDifficultyEntry> difficulties("MapDifficulty.db2",MapDifficultyLoadInfo::Instance());
    DB2Storage<PhaseEntry> phases("Phase.db2",PhaseLoadInfo::Instance());
    DB2Storage<SpellEffectEntry> effects("SpellEffect.db2",SpellEffectLoadInfo::Instance());
    if(!displays.Load(argv[1],0)||!models.Load(argv[1],0)||!objects.Load(argv[1],0)||!line.Load(argv[1],0)||!maps.Load(argv[1],0)||!difficulties.Load(argv[1],0)||!phases.Load(argv[1],0)||!effects.Load(argv[1],0))return 2;
    for(unsigned id:{65873u,67230u,76630u}){
        auto d=displays.LookupEntry(id);if(!d)return 3;auto m=models.LookupEntry(d->ModelID);if(!m||!m->FileDataID)return 4;
        std::cout<<"display "<<id<<" model "<<d->ModelID<<" file "<<m->FileDataID<<'\n';
    }
    auto go=objects.LookupEntry(35704);if(!go||!go->FileDataID)return 5;
    std::cout<<"Aegis display35704 file "<<go->FileDataID<<" minZ "<<go->GeoBoxMin.Z<<" maxZ "<<go->GeoBoxMax.Z<<'\n';
    if(!maps.LookupEntry(1477)||!maps.LookupEntry(1477)->IsDungeon()||!phases.LookupEntry(3554))return 6;
    for(auto d:difficulties)if(d->MapID==1477)std::cout<<"map1477 difficulty "<<unsigned(d->DifficultyID)<<'\n';
    for(auto q:line)if(q->QuestID==40072||q->QuestID==43349)std::cout<<"quest "<<q->QuestID<<" line "<<q->QuestLineID<<" order "<<unsigned(q->OrderIndex)<<'\n';
    unsigned credits=0;for(auto effect:effects)if((effect->Effect==90||effect->Effect==134)&&effect->EffectMiscValue[0]==110425)++credits;
    std::cout<<"native direct/group spell credit effects for110425 "<<credits<<'\n';
    std::cout<<"native flags3=tracked|sequenced; map1477 dungeon and phase3554 exist\n";
}
