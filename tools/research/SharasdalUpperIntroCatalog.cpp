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
    DB2Storage<ConversationLineEntry> lines("ConversationLine.db2",ConversationLineLoadInfo::Instance());
    DB2Storage<BroadcastTextEntry> texts("BroadcastText.db2",BroadcastTextLoadInfo::Instance());
    DB2Storage<CriteriaTreeEntry> trees("CriteriaTree.db2",CriteriaTreeLoadInfo::Instance());
    DB2Storage<CriteriaEntry> criteria("Criteria.db2",CriteriaLoadInfo::Instance());
    DB2Storage<SpellEffectEntry> effects("SpellEffect.db2",SpellEffectLoadInfo::Instance());
    DB2Storage<SoundKitEntry> kits("SoundKit.db2",SoundKitLoadInfo::Instance());
    DB2Storage<SoundKitEntryEntry> sounds("SoundKitEntry.db2",SoundKitEntryLoadInfo::Instance());
    DB2Storage<SpellXSpellVisualEntry> visuals("SpellXSpellVisual.db2",SpellXSpellVisualLoadInfo::Instance());
    if(!lines.Load(argv[1],0)||!texts.Load(argv[1],0)||!trees.Load(argv[1],0)||!criteria.Load(argv[1],0)||!effects.Load(argv[1],0)||!kits.Load(argv[1],0)||!sounds.Load(argv[1],0)||!visuals.Load(argv[1],0))return 5;
    lines.LoadFromDB();texts.LoadFromDB();trees.LoadFromDB();criteria.LoadFromDB();effects.LoadFromDB();kits.LoadFromDB();sounds.LoadFromDB();visuals.LoadFromDB();
    std::set<uint32> pending{3961},seen,wantedKits,resolved;
    while(!pending.empty())
    {
        uint32 id=*pending.begin();pending.erase(pending.begin());if(!seen.insert(id).second)continue;
        auto line=lines.LookupEntry(id);if(!line)return 6;
        auto text=texts.LookupEntry(line->BroadcastTextID);if(line->BroadcastTextID&&!text)return 7;
        std::cout<<"LINE\t"<<id<<'\t'<<line->BroadcastTextID<<'\t'<<line->AdditionalDuration<<'\t'<<line->NextConversationLineID<<'\n';
        if(text)std::cout<<"TEXT\t"<<text->ID<<'\t'<<text->Text->Str[0]<<'\t'<<text->Text1->Str[0]<<'\t'<<text->SoundEntriesID[0]<<'\t'<<text->SoundEntriesID[1]<<'\n';
        if(text)for(auto kit:text->SoundEntriesID)if(kit){if(!kits.LookupEntry(kit))return 10;wantedKits.insert(kit);}
        if(line->NextConversationLineID)pending.insert(line->NextConversationLineID);
    }
    for(uint32 id:{47131u,47378u})
    {
        auto tree=trees.LookupEntry(id);if(!tree)return 8;
        auto criterion=criteria.LookupEntry(tree->CriteriaID);if(!criterion)return 9;
        std::cout<<"TREE\t"<<id<<'\t'<<tree->Parent<<'\t'<<tree->Amount<<'\t'<<tree->CriteriaID<<'\t'<<unsigned(criterion->Type)<<'\t'<<criterion->Asset<<'\n';
    }
    uint32 producers=0;
    for(auto effect:effects)if(effect->Effect==219&&effect->EffectMiscValue[0]==1868)
    {
        ++producers;std::cout<<"CONVERSATION_EFFECT\t"<<effect->SpellID<<'\t'<<unsigned(effect->EffectIndex)<<'\t'<<unsigned(effect->DifficultyID)<<'\n';
    }
    std::cout<<"PRODUCERS\t"<<producers<<'\n';
    for(auto sound:sounds)if(wantedKits.count(sound->SoundKitID))
    {
        if(!sound->FileDataID||!sound->Frequency||sound->Volume<=0)return 11;
        resolved.insert(sound->SoundKitID);std::cout<<"AUDIO_FILE\t"<<sound->SoundKitID<<'\t'<<sound->FileDataID<<'\n';
    }
    if(resolved!=wantedKits)return 12;
    for(auto effect:effects)if(effect->SpellID==209312)
        std::cout<<"EFFECT\t"<<unsigned(effect->DifficultyID)<<'\t'<<unsigned(effect->EffectIndex)<<'\t'<<effect->Effect<<'\t'<<effect->EffectAura<<'\t'<<effect->EffectMiscValue[0]<<'\t'<<effect->EffectTriggerSpell<<'\t'<<effect->ImplicitTarget[0]<<'\t'<<effect->ImplicitTarget[1]<<'\n';
    uint32 visualCount=0;
    for(auto visual:visuals)if(visual->SpellID==209312){++visualCount;std::cout<<"VISUAL\t"<<visual->ID<<'\t'<<visual->SpellVisualID<<'\n';}
    std::cout<<"VISUAL_COUNT\t209312\t"<<visualCount<<"\nEOF\tupper-intro-audio-native\n";
    HotfixDatabase.Close();MySQL::Library_End();
}
