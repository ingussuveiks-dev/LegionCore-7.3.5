#include "DB2Store.h"
#include "DB2Structure.h"
#include "DB2LoadInfo.h"
#include "DB2Meta.h"
#include <vector>
#include "MovementPackets.h"
#include "SpellAuraDefines.h"
#include "SpellInfo.h"
#include "Config.h"
#include "DatabaseEnv.h"
#include "MySQLThreading.h"
#include <cstdlib>
#include <iostream>
#include <set>

#pragma pack(push,1)
struct EventRow { SpellVisualEventEntry value; int32 visual; };
struct EffectRow { SpellVisualKitEffectEntry value; int32 kit; };
#pragma pack(pop)
int main(int argc, char** argv)
{
    if (argc != 2) return 1;
    std::string error;
    auto config = std::getenv("LEGION_DIALOGUE_CONFIG");
    if (!config || !sConfigMgr->LoadInitial(config, {}, error)) return 2;
    MySQL::Library_Init();
    HotfixDatabase.SetConnectionInfo(sConfigMgr->GetStringDefault("HotfixDatabaseInfo", ""), 1, 1);
    if (HotfixDatabase.Open() || !HotfixDatabase.PrepareStatements()) return 3;
    HotfixDatabase.DirectExecute("SET SESSION TRANSACTION READ ONLY");
    HotfixDatabase.Execute("SET SESSION TRANSACTION READ ONLY");
    auto check = HotfixDatabase.Query("SELECT @@session.transaction_read_only");
    if (!check || !check->Fetch()[0].GetBool()) return 4;
    DB2Storage<SpellEntry> spells("Spell.db2", SpellLoadInfo::Instance());
    DB2Storage<SpellEffectEntry> effects("SpellEffect.db2", SpellEffectLoadInfo::Instance());
    DB2Storage<SpellMiscEntry> misc("SpellMisc.db2",SpellMiscLoadInfo::Instance());
    DB2Storage<SpellCastTimesEntry> times("SpellCastTimes.db2",SpellCastTimesLoadInfo::Instance());
    DB2Storage<SpellRangeEntry> ranges("SpellRange.db2",SpellRangeLoadInfo::Instance());
    if (!spells.Load(argv[1],0) || !effects.Load(argv[1],0) || !misc.Load(argv[1],0) || !times.Load(argv[1],0) || !ranges.Load(argv[1],0)) return 5;
    spells.LoadFromDB();effects.LoadFromDB();misc.LoadFromDB();times.LoadFromDB();ranges.LoadFromDB();
    DB2Storage<SpellDurationEntry> durations("SpellDuration.db2",SpellDurationLoadInfo::Instance());
    DB2Storage<VehicleEntry> vehicles("Vehicle.db2",VehicleLoadInfo::Instance());
    DB2Storage<VehicleSeatEntry> seats("VehicleSeat.db2",VehicleSeatLoadInfo::Instance());
    if(!durations.Load(argv[1],0)||!vehicles.Load(argv[1],0)||!seats.Load(argv[1],0))return 7;
    durations.LoadFromDB();vehicles.LoadFromDB();seats.LoadFromDB();
    if(auto vehicle=vehicles.LookupEntry(4720)) for(int index=0;index<8;++index)if(vehicle->SeatID[index]) {
        auto seat=seats.LookupEntry(vehicle->SeatID[index]);if(!seat)return 8;
        std::cout<<"SEAT\t"<<4720<<'\t'<<index<<'\t'<<seat->ID<<'\t'<<seat->Flags<<'\t'<<seat->FlagsB<<'\t'<<seat->FlagsC<<'\t'<<unsigned(seat->AttachmentID)<<'\t'<<seat->AttachmentOffset.X<<'\t'<<seat->AttachmentOffset.Y<<'\t'<<seat->AttachmentOffset.Z<<'\t'<<seat->EnterAnimStart<<'\t'<<seat->RideAnimLoop<<'\t'<<seat->ExitAnimStart<<'\t'<<seat->VehicleEnterAnimKitID<<'\t'<<seat->VehicleRideAnimKitID<<'\t'<<seat->VehicleExitAnimKitID<<'\t'<<seat->VehicleEnterAnim<<'\t'<<seat->VehicleRideAnimLoop<<'\t'<<seat->VehicleExitAnim<<'\t'<<unsigned(seat->VehicleEnterAnimBone)<<'\t'<<unsigned(seat->VehicleExitAnimBone)<<'\t'<<unsigned(seat->VehicleRideAnimLoopBone)<<'\n';
    }
    DB2Storage<CreatureDisplayInfoEntry> displays("CreatureDisplayInfo.db2",CreatureDisplayInfoLoadInfo::Instance());
    DB2Storage<CreatureModelDataEntry> models("CreatureModelData.db2",CreatureModelDataLoadInfo::Instance());
    if(!displays.Load(argv[1],0)||!models.Load(argv[1],0))return 14;
    displays.LoadFromDB();models.LoadFromDB();
    auto display=displays.LookupEntry(69084);if(!display)return 15;
    auto model=models.LookupEntry(display->ModelID);if(!model)return 16;
    std::cout<<"MODEL\t"<<display->ID<<'\t'<<display->ModelID<<'\t'<<model->FileDataID<<'\t'<<display->CreatureModelScale<<'\t'<<model->ModelScale<<'\n';
    std::set<uint32> selected{209457,209458,209487,209552,209796,210213,210214};
    for(bool changed=true;changed;)
    {
        changed=false;
        for(auto effect:effects)
            if(selected.count(effect->SpellID) && effect->EffectTriggerSpell)
                changed=selected.insert(effect->EffectTriggerSpell).second || changed;
    }
    for (uint32 id : selected)
    {
        auto spell=spells.LookupEntry(id);
        if (!spell) return 6;
        std::cout << "SPELL\t" << id << '\t' << spell->Name->Str[0] << '\n';
        std::cout<<"DESCRIPTION\t"<<id<<'\t'<<spell->Description->Str[0]<<'\t'<<spell->AuraDescription->Str[0]<<'\n';
        for(auto effect : effects) if(effect->SpellID==id)
        {
            std::cout << "EFFECT\t" << id << '\t' << unsigned(effect->DifficultyID) << '\t' << unsigned(effect->EffectIndex) << '\t' << effect->EffectBasePoints << '\t' << effect->Effect << '\t' << effect->EffectAura << '\t' << effect->EffectMiscValue[0] << '\t' << effect->EffectTriggerSpell << '\t' << effect->ImplicitTarget[0] << '\t' << effect->ImplicitTarget[1] << '\n';
            SpellImplicitTargetInfo target(effect->ImplicitTarget[0]);
            std::cout<<"TARGET\t"<<id<<'\t'<<unsigned(effect->EffectIndex)<<'\t'<<target.GetCheckType()<<'\t'<<target.GetReferenceType()<<'\n';
        }
        for(auto row:misc) if(row->SpellID==id)
        {
            auto time=times.LookupEntry(row->CastingTimeIndex);auto range=ranges.LookupEntry(row->RangeIndex);auto duration=durations.LookupEntry(row->DurationIndex);
            std::cout<<"DURATION\t"<<id<<'\t'<<row->DurationIndex<<'\t'<<(duration?duration->Duration:-2)<<'\n';
            std::cout<<"MISC\t"<<id<<'\t'<<unsigned(row->DifficultyID)<<'\t'<<row->CastingTimeIndex<<'\t'<<(time?time->Base:-1)<<'\t'<<row->RangeIndex<<'\t'<<(range?range->RangeMax[0]:-1)<<'\t'<<(range?range->RangeMax[1]:-1)<<'\n';
        }
    }
    DB2Storage<SpellXSpellVisualEntry> visuals("SpellXSpellVisual.db2",SpellXSpellVisualLoadInfo::Instance());
    DB2Storage<SpellVisualEntry> visualData("SpellVisual.db2",SpellVisualLoadInfo::Instance());
    DB2Storage<SpellVisualAnimEntry> animations("SpellVisualAnim.db2",SpellVisualAnimLoadInfo::Instance());
    DB2Storage<AnimKitEntry> animKits("AnimKit.db2",AnimKitLoadInfo::Instance());
    DB2Storage<AnimKitSegmentEntry> segments("AnimKitSegment.db2",AnimKitSegmentLoadInfo::Instance());
    if(!visuals.Load(argv[1],0)||!visualData.Load(argv[1],0)||!animations.Load(argv[1],0)||!animKits.Load(argv[1],0)||!segments.Load(argv[1],0))return 9;
    visuals.LoadFromDB();visualData.LoadFromDB();animations.LoadFromDB();animKits.LoadFromDB();segments.LoadFromDB();
    DB2Storage<AnimationDataEntry> animationData("AnimationData.db2",AnimationDataLoadInfo::Instance());
    if(!animationData.Load(argv[1],0))return 19;animationData.LoadFromDB();
    for(auto row:animationData)std::cout<<"ANIM_DATA\t"<<row->ID<<'\t'<<row->BehaviorID<<'\t'<<row->Fallback<<'\t'<<unsigned(row->BehaviorTier)<<'\t'<<row->Flags<<'\n';
    DB2Storage<ConversationLineEntry> lines("ConversationLine.db2",ConversationLineLoadInfo::Instance());
    if(!lines.Load(argv[1],0))return 17;lines.LoadFromDB();
    std::set<uint32> visualIds,kits,animationIds;
    for(uint32 id:{4028u,4029u,4030u,4031u,4032u}) {
        auto line=lines.LookupEntry(id);if(!line)return 18;
        std::cout<<"NATIVE_LINE\t"<<id<<'\t'<<line->BroadcastTextID<<'\t'<<line->SpellVisualKitID<<'\t'<<line->AdditionalDuration<<'\t'<<line->NextConversationLineID<<'\t'<<line->AnimKitID<<'\t'<<unsigned(line->SpeechType)<<'\t'<<unsigned(line->StartAnimation)<<'\t'<<unsigned(line->EndAnimation)<<'\n';
        if(line->AnimKitID)animationIds.insert(line->AnimKitID);
        if(line->SpellVisualKitID)kits.insert(line->SpellVisualKitID);
    }
    for(auto v:visuals)if(selected.count(v->SpellID)){
        visualIds.insert(v->SpellVisualID);std::cout<<"VISUAL\t"<<v->SpellID<<'\t'<<v->SpellVisualID<<'\t'<<v->CasterPlayerConditionID<<'\t'<<v->ViewerPlayerConditionID<<'\n';
    }
    std::set<uint32> pending=visualIds,visited;
    while(!pending.empty()){
        uint32 id=*pending.begin();pending.erase(pending.begin());if(!id||!visited.insert(id).second)continue;
        auto v=visualData.LookupEntry(id);if(!v)return 10;
        for(int32 next:{v->CasterSpellVisualID,v->HostileSpellVisualID,v->LowViolenceSpellVisualID})if(next>0){visualIds.insert(next);if(!visited.count(next))pending.insert(next);}
    }
    auto eventInfo=SpellVisualEventLoadInfo::Instance();std::vector<DB2FieldMeta> eventFields(eventInfo->Fields,eventInfo->Fields+eventInfo->FieldCount);eventFields.push_back({true,FT_INT,"SpellVisualID"});
    DB2LoadInfo fullEvents(eventFields.data(),eventFields.size(),SpellVisualEventMeta::Instance(),HOTFIX_SEL_SPELL_VISUAL_EVENT);
    DB2Storage<EventRow> events("SpellVisualEvent.db2",&fullEvents);
    auto effectInfo=SpellVisualKitEffectLoadInfo::Instance();std::vector<DB2FieldMeta> effectFields(effectInfo->Fields,effectInfo->Fields+effectInfo->FieldCount);effectFields.push_back({true,FT_INT,"SpellVisualKitID"});
    DB2LoadInfo fullEffects(effectFields.data(),effectFields.size(),SpellVisualKitEffectMeta::Instance(),HOTFIX_SEL_SPELL_VISUAL_KIT_EFFECT);
    DB2Storage<EffectRow> kitEffects("SpellVisualKitEffect.db2",&fullEffects);
    if(sizeof(EventRow)!=fullEvents.Meta->GetRecordSize()||sizeof(EffectRow)!=fullEffects.Meta->GetRecordSize()||!events.Load(argv[1],0)||!kitEffects.Load(argv[1],0))return 11;
    for(auto e:events)if(visualIds.count(e->visual)){
        kits.insert(e->value.SpellVisualKitID);
        std::cout<<"EVENT\t"<<e->visual<<'\t'<<e->value.StartEvent<<'\t'<<e->value.StartMinOffsetMs<<'\t'<<e->value.EndEvent<<'\t'<<e->value.TargetType<<'\t'<<e->value.SpellVisualKitID<<'\n';
    }
    for(auto e:kitEffects)if(kits.count(e->kit)){
        std::cout<<"KIT\t"<<e->kit<<'\t'<<e->value.EffectType<<'\t'<<e->value.Effect<<'\n';
        if(e->value.EffectType==6){
            auto anim=animations.LookupEntry(e->value.Effect);if(!anim)return 12;
            animationIds.insert(anim->AnimKitID);
            std::cout<<"ANIMATION\t"<<anim->ID<<'\t'<<anim->InitialAnimID<<'\t'<<anim->LoopAnimID<<'\t'<<anim->AnimKitID<<'\n';
        }
    }
    for(uint32 id:animationIds)if(id){
        auto kit=animKits.LookupEntry(id);if(!kit)return 13;
        std::cout<<"ANIM_KIT\t"<<id<<'\t'<<kit->OneShotDuration<<'\t'<<kit->OneShotStopAnimKitID<<'\n';
        for(auto seg:segments)if(seg->ParentAnimKitID==id)std::cout<<"SEGMENT\t"<<id<<'\t'<<seg->ID<<'\t'<<seg->AnimID<<'\t'<<seg->AnimStartTime<<'\t'<<unsigned(seg->StartCondition)<<'\t'<<seg->StartConditionDelay<<'\t'<<unsigned(seg->EndCondition)<<'\t'<<seg->EndConditionDelay<<'\n';
    }
    DB2Storage<SoundKitNameEntry> soundNames("SoundKitName.db2",SoundKitNameLoadInfo::Instance());
    if(!soundNames.Load(argv[1],0))return 20;soundNames.LoadFromDB();
    for(auto effect:kitEffects)if(kits.count(effect->kit)&&effect->value.EffectType==5){
        auto sound=soundNames.LookupEntry(effect->value.Effect);
        std::cout<<"SOUND_NAME\t"<<effect->kit<<'\t'<<effect->value.Effect<<'\t'<<(sound?sound->Name->Str[0]:"MISSING")<<'\n';
    }
    std::cout<<"EOF\t"<<"living-lady"<<'\n';
    HotfixDatabase.Close(); MySQL::Library_End();
}
