#include "DB2Store.h"
#include "DB2Structure.h"
#include "DB2LoadInfo.h"
#include "Config.h"
#include "DatabaseEnv.h"
#include "MySQLThreading.h"
#include <algorithm>
#include <cctype>
#include <cstdlib>
#include <iostream>
#include <string>
#include <vector>

int main(int argc, char** argv)
{
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
    HotfixDatabase.Execute("SET SESSION TRANSACTION READ ONLY");
    auto check = HotfixDatabase.Query("SELECT @@session.transaction_read_only");
    if (!check || !check->Fetch()[0].GetBool())
        return 4;
    DB2Storage<SpellEffectEntry> effects("SpellEffect.db2", SpellEffectLoadInfo::Instance());
    DB2Storage<SpellEntry> spells("Spell.db2", SpellLoadInfo::Instance());
    DB2Storage<SceneScriptTextEntry> scenes("SceneScriptText.db2", SceneScriptTextLoadInfo::Instance());
    if (!effects.Load(argv[1], 0) || !spells.Load(argv[1], 0) || !scenes.Load(argv[1], 0))
        return 5;
    effects.LoadFromDB();
    spells.LoadFromDB();
    uint32 effectCount = 0, producers = 0;
    for (auto effect : effects)
    {
        ++effectCount;
        if (effect->SpellID != 211502 && effect->SpellID != 185414 && effect->SpellID != 200181 &&
            effect->SpellID != 204891 && effect->SpellID != 215258 &&
            effect->EffectTriggerSpell != 211502 && effect->EffectTriggerSpell != 185414 &&
            effect->EffectMiscValue[0] != 106094 && effect->EffectMiscValue[1] != 106094)
            continue;
        ++producers;
        auto spell = spells.LookupEntry(effect->SpellID);
        std::cout << "EFFECT\t" << effect->SpellID << '\t' << unsigned(effect->DifficultyID) << '\t'
            << effect->EffectIndex << '\t' << effect->Effect << '\t' << effect->EffectAura << '\t'
            << effect->EffectMiscValue[0] << '\t' << effect->EffectMiscValue[1] << '\t' << effect->EffectTriggerSpell << '\t'
            << effect->ImplicitTarget[0] << '\t' << effect->ImplicitTarget[1] << '\t' << (spell ? spell->Name->Str[0] : "MISSING") << '\n';
    }
    uint32 sceneCount = 0, matches = 0;
    std::vector<std::string> const terms{"silverhand", "silver hand", "travard", "211502", "185414", "106094"};
    for (auto scene : scenes)
    {
        ++sceneCount;
        std::string name = scene->Name->Str[0], script = scene->Script->Str[0], lower = name + '\n' + script;
        std::transform(lower.begin(), lower.end(), lower.begin(), [](unsigned char c) { return char(std::tolower(c)); });
        bool found = false;
        for (auto const& term : terms)
            found = found || lower.find(term) != std::string::npos;
        if (!found)
            continue;
        ++matches;
        std::cout << "SCENE\t" << scene->ID << '\t' << name << "\nSCRIPT_BEGIN\t" << scene->ID << '\t' << script.size() << '\n'
            << script << "\nSCRIPT_END\t" << scene->ID << '\n';
    }
    std::cout << "COMPLETE\t" << effectCount << '\t' << producers << '\t' << sceneCount << '\t' << matches << '\n';
    HotfixDatabase.Close();
    MySQL::Library_End();
    return sceneCount == 5969 && effectCount > 10000 ? 0 : 6;
}
