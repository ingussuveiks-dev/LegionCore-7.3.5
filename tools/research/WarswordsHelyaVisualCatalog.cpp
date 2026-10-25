#include "DB2Store.h"
#include "DB2Structure.h"
#include "DB2LoadInfo.h"
#include "DB2Meta.h"
#include "Config.h"
#include "DatabaseEnv.h"
#include "MySQLThreading.h"
#include <cstdlib>
#include <iostream>
#include <set>
#include <vector>
#include <DbgHelp.h>
#pragma comment(lib, "Dbghelp.lib")

#ifndef ARTIFACT_VISUAL_ACTOR
#define ARTIFACT_VISUAL_ACTOR 66238
#endif
#ifndef ARTIFACT_VISUAL_SPELLS
#define ARTIFACT_VISUAL_SPELLS 195062u, 205424u
#endif

static LONG WINAPI ReportNativeFailure(EXCEPTION_POINTERS* exception)
{
    auto process = GetCurrentProcess();
    SymInitialize(process, nullptr, TRUE);
    CONTEXT context = *exception->ContextRecord;
    STACKFRAME64 frame{};
    frame.AddrPC.Offset = context.Rip; frame.AddrPC.Mode = AddrModeFlat;
    frame.AddrFrame.Offset = context.Rbp; frame.AddrFrame.Mode = AddrModeFlat;
    frame.AddrStack.Offset = context.Rsp; frame.AddrStack.Mode = AddrModeFlat;
    std::cerr << "Native visual catalogue exception " << std::hex << exception->ExceptionRecord->ExceptionCode << std::dec << '\n';
    for (uint32 depth = 0; depth < 24 && frame.AddrPC.Offset; ++depth)
    {
        char storage[sizeof(SYMBOL_INFO) + MAX_SYM_NAME]{};
        auto symbol = reinterpret_cast<SYMBOL_INFO*>(storage);
        symbol->SizeOfStruct = sizeof(SYMBOL_INFO); symbol->MaxNameLen = MAX_SYM_NAME;
        DWORD64 offset = 0;
        if (SymFromAddr(process, frame.AddrPC.Offset, &offset, symbol))
            std::cerr << symbol->Name << '+' << offset << '\n';
        if (!StackWalk64(IMAGE_FILE_MACHINE_AMD64, process, GetCurrentThread(), &frame, &context, nullptr, SymFunctionTableAccess64, SymGetModuleBase64, nullptr))
            break;
    }
    return EXCEPTION_CONTINUE_SEARCH;
}

#pragma pack(push, 1)
struct WarswordsVisualEventRecord { SpellVisualEventEntry event; int32 visual; };
struct WarswordsKitEffectRecord { SpellVisualKitEffectEntry effect; int32 kit; };
struct WarswordsAttachmentRecord { SpellVisualKitModelAttachEntry attachment; int32 kit; };
struct WarswordsEffectNameRecord
{
    int32 ID;
    float radius, speed, scale, minimumScale, maximumScale, alpha;
    uint32 flags;
    int32 generic, texture;
    uint8 type;
    int32 model;
    uint32 ribbon;
    int32 dissolve;
};
#pragma pack(pop)

int main(int argc, char** argv)
{
    SetUnhandledExceptionFilter(ReportNativeFailure);
    if (argc != 2)
        return 1;
    std::string error;
    auto config = std::getenv("LEGION_DIALOGUE_CONFIG");
    if (!config || !sConfigMgr->LoadInitial(config, {}, error))
        return 2;
    MySQL::Library_Init();
    HotfixDatabase.SetConnectionInfo(sConfigMgr->GetStringDefault("HotfixDatabaseInfo", ""), 1, 1);
    if (HotfixDatabase.Open() || !HotfixDatabase.PrepareStatements())
        return 3;
    HotfixDatabase.DirectExecute("SET SESSION TRANSACTION READ ONLY");
    auto check = HotfixDatabase.Query("SELECT @@session.transaction_read_only");
    if (!check || !check->Fetch()[0].GetBool())
        return 4;
    DB2Storage<SpellXSpellVisualEntry> links("SpellXSpellVisual.db2", SpellXSpellVisualLoadInfo::Instance());
    DB2Storage<SpellVisualEntry> visuals("SpellVisual.db2", SpellVisualLoadInfo::Instance());
    DB2Storage<SpellMiscEntry> misc("SpellMisc.db2", SpellMiscLoadInfo::Instance());
    DB2Storage<CreatureDisplayInfoEntry> displays("CreatureDisplayInfo.db2", CreatureDisplayInfoLoadInfo::Instance());
    DB2Storage<CreatureModelDataEntry> models("CreatureModelData.db2", CreatureModelDataLoadInfo::Instance());
    DB2Storage<SpellVisualKitEntry> kits("SpellVisualKit.db2", SpellVisualKitLoadInfo::Instance());
    DB2Storage<SpellVisualAnimEntry> animations("SpellVisualAnim.db2", SpellVisualAnimLoadInfo::Instance());
    DB2Storage<AnimKitEntry> animKits("AnimKit.db2", AnimKitLoadInfo::Instance());
    DB2Storage<AnimKitSegmentEntry> animSegments("AnimKitSegment.db2", AnimKitSegmentLoadInfo::Instance());
    DB2Storage<SpellProceduralEffectEntry> procedures("SpellProceduralEffect.db2", SpellProceduralEffectLoadInfo::Instance());
    DB2Storage<SoundKitEntry> sounds("SoundKit.db2", SoundKitLoadInfo::Instance());
    DB2Storage<SoundKitEntryEntry> soundFiles("SoundKitEntry.db2", SoundKitEntryLoadInfo::Instance());
    auto load = [&](auto& store, char const* name) { std::cerr << "Loading " << name << '\n'; return store.Load(argv[1], 0); };
    if (!load(links, "SpellXSpellVisual") || !load(visuals, "SpellVisual") || !load(misc, "SpellMisc") ||
        !load(displays, "CreatureDisplayInfo") || !load(models, "CreatureModelData") || !load(kits, "SpellVisualKit") ||
        !load(animations, "SpellVisualAnim") || !load(animKits, "AnimKit") || !load(animSegments, "AnimKitSegment") ||
        !load(procedures, "SpellProceduralEffect") || !load(sounds, "SoundKit") || !load(soundFiles, "SoundKitEntry"))
        return 5;
    std::cerr << "Loading Helya root hotfixes\n";
    links.LoadFromDB(); visuals.LoadFromDB(); misc.LoadFromDB();
    displays.LoadFromDB(); models.LoadFromDB(); kits.LoadFromDB();
    animations.LoadFromDB(); procedures.LoadFromDB(); sounds.LoadFromDB(); soundFiles.LoadFromDB();
    animKits.LoadFromDB(); animSegments.LoadFromDB();
    std::cerr << "Preparing typed client parent records\n";
    auto eventInfo = SpellVisualEventLoadInfo::Instance();
    std::vector<DB2FieldMeta> eventFields(eventInfo->Fields, eventInfo->Fields + eventInfo->FieldCount);
    eventFields.push_back({true, FT_INT, "SpellVisualID"});
    DB2LoadInfo fullEventInfo(eventFields.data(), eventFields.size(), SpellVisualEventMeta::Instance(), HOTFIX_SEL_SPELL_VISUAL_EVENT);
    DB2Storage<WarswordsVisualEventRecord> events("SpellVisualEvent.db2", &fullEventInfo);
    auto effectInfo = SpellVisualKitEffectLoadInfo::Instance();
    std::vector<DB2FieldMeta> effectFields(effectInfo->Fields, effectInfo->Fields + effectInfo->FieldCount);
    effectFields.push_back({true, FT_INT, "SpellVisualKitID"});
    DB2LoadInfo fullEffectInfo(effectFields.data(), effectFields.size(), SpellVisualKitEffectMeta::Instance(), HOTFIX_SEL_SPELL_VISUAL_KIT_EFFECT);
    DB2Storage<WarswordsKitEffectRecord> effects("SpellVisualKitEffect.db2", &fullEffectInfo);
    auto attachmentInfo = SpellVisualKitModelAttachLoadInfo::Instance();
    std::vector<DB2FieldMeta> attachmentFields(attachmentInfo->Fields, attachmentInfo->Fields + attachmentInfo->FieldCount);
    attachmentFields.push_back({true, FT_INT, "SpellVisualKitID"});
    DB2LoadInfo fullAttachmentInfo(attachmentFields.data(), attachmentFields.size(), SpellVisualKitModelAttachMeta::Instance(), HOTFIX_SEL_SPELL_VISUAL_KIT_MODEL_ATTACH);
    DB2Storage<WarswordsAttachmentRecord> attachments("SpellVisualKitModelAttach.db2", &fullAttachmentInfo);
    DB2FieldMeta nameFields[] = {
        {true, FT_INT, "ID"}, {false, FT_FLOAT, "EffectRadius"}, {false, FT_FLOAT, "BaseMissileSpeed"},
        {false, FT_FLOAT, "Scale"}, {false, FT_FLOAT, "MinAllowedScale"}, {false, FT_FLOAT, "MaxAllowedScale"},
        {false, FT_FLOAT, "Alpha"}, {false, FT_INT, "Flags"}, {true, FT_INT, "GenericID"},
        {true, FT_INT, "TextureFileDataID"}, {false, FT_BYTE, "Type"}, {true, FT_INT, "ModelFileDataID"},
        {false, FT_INT, "RibbonQualityID"}, {true, FT_INT, "DissolveEffectID"}
    };
    DB2LoadInfo nameInfo(nameFields, std::size(nameFields), SpellVisualEffectNameMeta::Instance(), HOTFIX_SEL_SPELL_VISUAL_EFFECT_NAME);
    DB2Storage<WarswordsEffectNameRecord> names("SpellVisualEffectName.db2", &nameInfo);
    if (sizeof(WarswordsVisualEventRecord) != fullEventInfo.Meta->GetRecordSize() ||
        sizeof(WarswordsKitEffectRecord) != fullEffectInfo.Meta->GetRecordSize() ||
        sizeof(WarswordsAttachmentRecord) != fullAttachmentInfo.Meta->GetRecordSize() ||
        sizeof(WarswordsEffectNameRecord) != nameInfo.Meta->GetRecordSize())
        return 6;
    std::cerr << "Loading typed client visual parent records\n";
    if (!events.Load(argv[1], 0) || !effects.Load(argv[1], 0) || !attachments.Load(argv[1], 0) || !names.Load(argv[1], 0))
        return 7;
    std::cout << "SOURCE\troot-tables-base-plus-hotfix\tclient-parent-records-base-only\n";
    std::set<int32> pendingVisuals, seenVisuals, pendingKits, seenKits;
    auto display = displays.LookupEntry(ARTIFACT_VISUAL_ACTOR);
    auto model = display ? models.LookupEntry(display->ModelID) : nullptr;
    if (!model)
        return 8;
    std::cout << "ACTOR\t" << ARTIFACT_VISUAL_ACTOR << '\t' << display->ModelID << '\t' << model->FileDataID << '\t'
        << display->StateSpellVisualKitID << '\t' << display->CreatureModelScale << '\t' << model->ModelScale << '\n';
    if (display->StateSpellVisualKitID)
        pendingKits.insert(display->StateSpellVisualKitID);
    for (uint32 spell : {ARTIFACT_VISUAL_SPELLS})
    {
        for (auto entry : misc)
            if (entry->SpellID == spell && !entry->DifficultyID)
                std::cout << "ATTRIBUTES\t" << spell << '\t' << entry->Attributes[0] << '\t' << entry->Attributes[1]
                    << '\t' << bool(entry->Attributes[1] & 0x44) << '\n';
        for (auto link : links)
            if (link->SpellID == spell)
            {
                std::cout << "LINK\t" << spell << '\t' << link->ID << '\t' << link->SpellVisualID << '\t'
                    << unsigned(link->DifficultyID) << '\t' << link->CasterPlayerConditionID << '\t'
                    << link->CasterUnitConditionID << '\t' << link->ViewerPlayerConditionID << '\t'
                    << link->ViewerUnitConditionID << '\t' << link->Probability << '\n';
                pendingVisuals.insert(link->SpellVisualID);
            }
    }
    while (!pendingVisuals.empty())
    {
        int32 identity = *pendingVisuals.begin(); pendingVisuals.erase(identity);
        if (!identity || !seenVisuals.insert(identity).second)
            continue;
        auto visual = visuals.LookupEntry(identity);
        if (!visual)
            return 9;
        std::cout << "VISUAL\t" << identity << '\t' << visual->Flags << '\t' << visual->CasterSpellVisualID << '\t'
            << visual->HostileSpellVisualID << '\t' << visual->LowViolenceSpellVisualID << '\t' << visual->SpellVisualMissileSetID << '\n';
        for (int32 alternate : {visual->CasterSpellVisualID, visual->HostileSpellVisualID, visual->LowViolenceSpellVisualID})
            if (alternate)
                pendingVisuals.insert(alternate);
        for (auto event : events)
            if (event->visual == identity)
            {
                std::cout << "EVENT\t" << identity << '\t' << event->event.ID << '\t' << event->event.StartEvent << '\t'
                    << event->event.EndEvent << '\t' << event->event.TargetType << '\t' << event->event.SpellVisualKitID << '\t'
                    << event->event.StartMinOffsetMs << '\t' << event->event.StartMaxOffsetMs << '\t'
                    << event->event.EndMinOffsetMs << '\t' << event->event.EndMaxOffsetMs << '\n';
                pendingKits.insert(event->event.SpellVisualKitID);
            }
    }
    while (!pendingKits.empty())
    {
        int32 identity = *pendingKits.begin(); pendingKits.erase(identity);
        if (!identity || !seenKits.insert(identity).second)
            continue;
        auto kit = kits.LookupEntry(identity);
        if (!kit)
            return 10;
        std::cout << "KIT\t" << identity << '\t' << kit->Flags << '\t' << kit->FallbackSpellVisualKitID << '\t'
            << kit->DelayMin << '\t' << kit->DelayMax << '\n';
        if (kit->FallbackSpellVisualKitID)
            pendingKits.insert(kit->FallbackSpellVisualKitID);
        for (auto effect : effects)
            if (effect->kit == identity)
            {
                std::cout << "KIT_EFFECT\t" << identity << '\t' << effect->effect.ID << '\t' << effect->effect.EffectType << '\t' << effect->effect.Effect << '\n';
                if (effect->effect.EffectType == 1)
                {
                    auto procedure = procedures.LookupEntry(effect->effect.Effect);
                    if (!procedure)
                        return 12;
                    std::cout << "PROCEDURE\t" << identity << '\t' << procedure->ID << '\t' << unsigned(procedure->Type);
                    for (float value : procedure->Value)
                        std::cout << '\t' << value;
                    std::cout << '\n';
                }
                if (effect->effect.EffectType == 6)
                {
                    auto animation = animations.LookupEntry(effect->effect.Effect);
                    if (!animation)
                        return 13;
                    std::cout << "ANIMATION\t" << identity << '\t' << animation->ID << '\t' << animation->InitialAnimID << '\t'
                        << animation->LoopAnimID << '\t' << animation->AnimKitID << '\n';
                    if (animation->AnimKitID)
                    {
                        auto animKit = animKits.LookupEntry(animation->AnimKitID);
                        if (!animKit)
                            return 16;
                        std::cout << "ANIMKIT\t" << animKit->ID << '\t' << animKit->OneShotDuration << '\t'
                            << animKit->OneShotStopAnimKitID << '\t' << animKit->LowDefAnimKitID << '\n';
                        for (auto segment : animSegments)
                            if (segment->ParentAnimKitID == animKit->ID)
                                std::cout << "ANIM_SEGMENT\t" << animKit->ID << '\t' << segment->ID << '\t' << segment->AnimID << '\t'
                                    << segment->AnimStartTime << '\t' << segment->EndConditionDelay << '\t' << unsigned(segment->EndCondition) << '\n';
                    }
                }
                if (effect->effect.EffectType == 5)
                {
                    auto sound = sounds.LookupEntry(effect->effect.Effect);
                    if (!sound)
                        return 14;
                    bool found = false;
                    for (auto file : soundFiles)
                        if (file->SoundKitID == sound->ID && file->FileDataID)
                        {
                            std::cout << "AUDIO\t" << identity << '\t' << sound->ID << '\t' << file->FileDataID << '\t' << unsigned(file->Frequency) << '\t' << file->Volume << '\n';
                            found = true;
                        }
                    if (!found)
                        return 15;
                }
                if (effect->effect.EffectType != 2)
                    continue;
                auto attachment = attachments.LookupEntry(effect->effect.Effect);
                auto name = attachment ? names.LookupEntry(attachment->attachment.SpellVisualEffectNameID) : nullptr;
                if (!name)
                    return 11;
                auto const& entry = attachment->attachment;
                std::cout << "ATTACHMENT\t" << identity << '\t' << entry.ID << '\t' << entry.SpellVisualEffectNameID << '\t'
                    << unsigned(entry.AttachmentID) << '\t' << entry.AnimKitID << '\t' << entry.StartAnimID << '\t'
                    << entry.AnimID << '\t' << entry.EndAnimID << '\t' << entry.Scale << '\t' << name->model << '\t'
                    << name->alpha << '\t' << unsigned(name->type) << '\n';
            }
    }
    HotfixDatabase.Close();
    MySQL::Library_End();
    return 0;
}
