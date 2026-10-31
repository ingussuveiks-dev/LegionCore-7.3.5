#include "DB2Store.h"
#include "DB2Structure.h"
#include "DB2LoadInfo.h"
#include "Config.h"
#include "DatabaseEnv.h"
#include "MySQLThreading.h"
#include <cstdlib>
#include <iostream>

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
    if (!spells.Load(argv[1],0) || !effects.Load(argv[1],0)) return 5;
    spells.LoadFromDB();effects.LoadFromDB();
    for (uint32 id : {209748u,209764u,209755u,209757u,20508u})
    {
        auto spell=spells.LookupEntry(id);
        if (!spell) return 6;
        std::cout << "SPELL\t" << id << '\t' << spell->Name->Str[0] << '\n';
        for(auto effect : effects) if(effect->SpellID==id)
            std::cout << "EFFECT\t" << id << '\t' << unsigned(effect->DifficultyID) << '\t' << unsigned(effect->EffectIndex) << '\t' << effect->Effect << '\t' << effect->EffectAura << '\t' << effect->EffectMiscValue[0] << '\t' << effect->EffectTriggerSpell << '\t' << effect->ImplicitTarget[0] << '\t' << effect->ImplicitTarget[1] << '\n';
    }
    HotfixDatabase.Close(); MySQL::Library_End();
}
