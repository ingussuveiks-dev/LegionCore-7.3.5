#include "DB2Store.h"
#include "DB2Structure.h"
#include "DB2LoadInfo.h"
#include "Config.h"
#include "DatabaseEnv.h"
#include "MySQLThreading.h"
#include <cstdlib>
#include <iostream>
#include <set>
#include <string>

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
    DB2Storage<CriteriaTreeEntry> trees("CriteriaTree.db2", CriteriaTreeLoadInfo::Instance());
    DB2Storage<CriteriaEntry> criteria("Criteria.db2", CriteriaLoadInfo::Instance());
    DB2Storage<ConversationLineEntry> lines("ConversationLine.db2", ConversationLineLoadInfo::Instance());
    DB2Storage<BroadcastTextEntry> texts("BroadcastText.db2", BroadcastTextLoadInfo::Instance());
    DB2Storage<SpellEffectEntry> effects("SpellEffect.db2", SpellEffectLoadInfo::Instance());
    DB2Storage<MapDifficultyEntry> difficulties("MapDifficulty.db2", MapDifficultyLoadInfo::Instance());
    if (!trees.Load(argv[1], 0) || !criteria.Load(argv[1], 0) || !lines.Load(argv[1], 0) || !texts.Load(argv[1], 0) || !effects.Load(argv[1], 0) || !difficulties.Load(argv[1], 0)) return 5;
    trees.LoadFromDB(); criteria.LoadFromDB(); lines.LoadFromDB(); texts.LoadFromDB(); effects.LoadFromDB(); difficulties.LoadFromDB();
    for (auto difficulty : difficulties)
        if (difficulty->MapID == 1600)
            std::cout << "DIFFICULTY\t" << difficulty->ID << '\t' << unsigned(difficulty->DifficultyID) << '\t' << unsigned(difficulty->MaxPlayers) << '\n';
    for (uint32 id : {47126u, 47127u, 47330u, 47132u, 47133u, 47380u})
    {
        auto tree = trees.LookupEntry(id);
        if (!tree) return 6;
        std::cout << "TREE\t" << tree->ID << '\t' << tree->Parent << '\t' << unsigned(tree->Operator) << '\t' << tree->Amount << '\t' << tree->CriteriaID << '\n';
        if (auto criterion = criteria.LookupEntry(tree->CriteriaID))
            std::cout << "CRITERION\t" << criterion->ID << '\t' << unsigned(criterion->Type) << '\t' << criterion->Asset << '\n';
    }
    std::set<uint32> wanted;
    for (auto text : texts)
    {
        std::string value=std::string(text->Text->Str[0])+" "+text->Text1->Str[0];
        if (value.find("Zithreen") != std::string::npos || value.find("Take up the Scepter") != std::string::npos ||
            value.find("tell you NOTHING") != std::string::npos || value.find("done talking here") != std::string::npos ||
            value.find("Agraa") != std::string::npos || value == "Dead now. ")
        {
            wanted.insert(text->ID);
            std::cout << "TEXT\t" << text->ID << '\t' << text->Text->Str[0] << '\t' << text->Text1->Str[0] << '\t' << text->SoundEntriesID[0] << '\t' << text->SoundEntriesID[1] << '\n';
        }
    }
    for (auto line : lines)
        if (wanted.count(line->BroadcastTextID))
            std::cout << "LINE\t" << line->ID << '\t' << line->BroadcastTextID << '\t' << line->AdditionalDuration << '\t' << line->NextConversationLineID << '\n';
    HotfixDatabase.Close(); MySQL::Library_End();
}
