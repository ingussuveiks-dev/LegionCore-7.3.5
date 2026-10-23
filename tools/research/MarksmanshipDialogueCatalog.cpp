#include "DB2Store.h"
#include "DB2Structure.h"
#include "DB2LoadInfo.h"
#include "Config.h"
#include "DatabaseEnv.h"
#include "MySQLThreading.h"
#include <cstdlib>
#include <iostream>
#include <set>

int main(int argc, char** argv)
{
    if (argc != 2)
        return 1;
    char const* config = std::getenv("LEGION_DIALOGUE_CONFIG");
    std::string error;
    if (!config || !sConfigMgr->LoadInitial(config, {}, error))
        return 6;
    MySQL::Library_Init();
    HotfixDatabase.SetConnectionInfo(sConfigMgr->GetStringDefault("HotfixDatabaseInfo", ""), 1, 1);
    if (HotfixDatabase.Open() || !HotfixDatabase.PrepareStatements())
        return 7;
    HotfixDatabase.DirectExecute("SET SESSION TRANSACTION READ ONLY");
    HotfixDatabase.Execute("SET SESSION TRANSACTION READ ONLY");
    auto readOnly = HotfixDatabase.Query("SELECT @@session.transaction_read_only");
    if (!readOnly || !readOnly->Fetch()[0].GetBool())
        return 8;
    DB2Storage<BroadcastTextEntry> texts("BroadcastText.db2", BroadcastTextLoadInfo::Instance());
    DB2Storage<SoundKitEntry> kits("SoundKit.db2", SoundKitLoadInfo::Instance());
    DB2Storage<SoundKitEntryEntry> sounds("SoundKitEntry.db2", SoundKitEntryLoadInfo::Instance());
    DB2Storage<ConversationLineEntry> lines("ConversationLine.db2", ConversationLineLoadInfo::Instance());
    DB2Storage<CreatureDisplayInfoEntry> displays("CreatureDisplayInfo.db2", CreatureDisplayInfoLoadInfo::Instance());
    DB2Storage<SpellEffectEntry> effects("SpellEffect.db2", SpellEffectLoadInfo::Instance());
    DB2Storage<SpellMiscEntry> misc("SpellMisc.db2", SpellMiscLoadInfo::Instance());
    DB2Storage<SpellCastTimesEntry> castTimes("SpellCastTimes.db2", SpellCastTimesLoadInfo::Instance());
    if (!texts.Load(argv[1], 0) || !kits.Load(argv[1], 0) || !sounds.Load(argv[1], 0) ||
        !lines.Load(argv[1], 0) || !displays.Load(argv[1], 0) || !effects.Load(argv[1], 0) ||
        !misc.Load(argv[1], 0) || !castTimes.Load(argv[1], 0))
        return 2;
    for (uint32 identity : {103289u, 103242u, 103441u, 106998u, 107282u, 103501u, 107281u, 103558u, 103652u})
        std::cout << "BASE_BROADCAST\t" << identity << '\t' << bool(texts.LookupEntry(identity)) << '\n';
    texts.LoadFromDB();
    displays.LoadFromDB();
    effects.LoadFromDB();
    misc.LoadFromDB();
    castTimes.LoadFromDB();
    uint32 storyCasts = 0;
    for (auto entry : misc)
        if (entry->SpellID == 200832 && !entry->DifficultyID)
        {
            auto cast = castTimes.LookupEntry(entry->CastingTimeIndex);
            if ((entry->CastingTimeIndex && !cast) || (cast && cast->Base))
                return 15;
            ++storyCasts;
            std::cout << "STORY_CAST\t" << entry->SpellID << '\t' << entry->CastingTimeIndex << '\t' << (cast ? cast->Base : 0) << '\t' << entry->Attributes[0] << '\n';
        }
    if (storyCasts != 1)
        return 16;
    for (auto effect : effects)
        if (effect->SpellID == 130491)
            std::cout << "POSE_EFFECT\t" << effect->SpellID << '\t' << effect->EffectIndex << '\t'
                << effect->Effect << '\t' << effect->EffectAura << '\n';
    uint32 conversationEffects = 0;
    for (auto effect : effects)
        if (effect->SpellID == 200832 || effect->SpellID == 200848)
        {
            if (effect->Effect != 219 || effect->EffectAura || effect->EffectIndex || effect->DifficultyID ||
                effect->EffectMiscValue[0] != (effect->SpellID == 200832 ? 1072 : 1073) ||
                effect->ImplicitTarget[0] != 18 || effect->ImplicitTarget[1])
                return 13;
            ++conversationEffects;
            std::cout << "STORY_EFFECT\t" << effect->SpellID << '\t' << effect->Effect << '\t'
                << effect->EffectMiscValue[0] << '\t' << effect->ImplicitTarget[0] << '\n';
        }
    if (conversationEffects != 2)
        return 14;
    auto line = lines.LookupEntry(3566);
    auto display = displays.LookupEntry(65514);
    if (!line || !display)
        return 3;
    std::cout << "CONVERSATION_LINE\t" << line->ID << '\t' << line->BroadcastTextID << '\t'
        << line->SpellVisualKitID << '\t' << line->AnimKitID << '\t' << line->AdditionalDuration << '\n';
    std::set<uint32> broadcastIds{103289,103242,103441,106998,107282,103501,107281,103558,103652};
    for (uint32 identity : {2737u, 2464u, 3421u, 3183u, 2632u, 2633u, 3364u,
                            2625u, 2630u, 2658u, 2659u, 3427u, 2631u})
    {
        auto combatLine = lines.LookupEntry(identity);
        if (!combatLine)
            return 12;
        if (combatLine->BroadcastTextID)
            broadcastIds.insert(combatLine->BroadcastTextID);
        std::cout << "CONVERSATION_LINE\t" << combatLine->ID << '\t' << combatLine->BroadcastTextID << '\t'
            << combatLine->SpellVisualKitID << '\t' << combatLine->AnimKitID << '\t' << combatLine->AdditionalDuration << '\n';
    }
    for (uint32 identity : {28222u, 65514u, 18345u, 4426u, 67031u})
    {
        auto actorDisplay = displays.LookupEntry(identity);
        if (!actorDisplay)
            return 9;
        std::cout << "DISPLAY\t" << actorDisplay->ID << '\t' << actorDisplay->ModelID << '\t'
            << uint32(actorDisplay->Gender) << '\n';
    }
    std::set<uint32> soundKits;
    for (uint32 identity : broadcastIds)
    {
        auto text = texts.LookupEntry(identity);
        if (!text)
            return 4;
        std::cout << "BROADCAST\t" << identity << '\t' << text->SoundEntriesID[0] << '\t' << text->SoundEntriesID[1]
            << '\t' << text->Text->Str[0] << '\t' << text->Text1->Str[0] << '\n';
        for (uint32 sound : text->SoundEntriesID)
            if (sound)
            {
                if (!kits.LookupEntry(sound))
                    return 5;
                soundKits.insert(sound);
            }
    }
    std::set<uint32> resolvedKits;
    for (auto sound : sounds)
        if (soundKits.count(sound->SoundKitID))
        {
            if (!sound->FileDataID || !sound->Frequency || sound->Volume <= 0)
                return 10;
            resolvedKits.insert(sound->SoundKitID);
            std::cout << "AUDIO_FILE\t" << sound->SoundKitID << '\t' << sound->FileDataID << '\n';
        }
    if (resolvedKits != soundKits)
        return 11;
    HotfixDatabase.Close();
}
