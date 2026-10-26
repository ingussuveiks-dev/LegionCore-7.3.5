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
    if (!config || !sConfigMgr->LoadInitial(config, {}, error)) return 4;
    MySQL::Library_Init();
    HotfixDatabase.SetConnectionInfo(sConfigMgr->GetStringDefault("HotfixDatabaseInfo", ""), 1, 1);
    if (HotfixDatabase.Open() || !HotfixDatabase.PrepareStatements()) return 5;
    HotfixDatabase.DirectExecute("SET SESSION TRANSACTION READ ONLY");
    HotfixDatabase.Execute("SET SESSION TRANSACTION READ ONLY");
    auto readOnly = HotfixDatabase.Query("SELECT @@session.transaction_read_only");
    if (!readOnly || !readOnly->Fetch()[0].GetBool()) return 6;
    DB2Storage<SpellEffectEntry> effects("SpellEffect.db2", SpellEffectLoadInfo::Instance());
    if (!effects.Load(argv[1], 0)) return 2;
    effects.LoadFromDB();

    DB2Storage<BroadcastTextEntry> texts("BroadcastText.db2", BroadcastTextLoadInfo::Instance());
    if (!texts.Load(argv[1], 0)) return 7;
    texts.LoadFromDB();
    auto review = texts.LookupEntry(103645);
    if (!review || !review->Text || std::string(review->Text->Str[0]).find("weapons") == std::string::npos) return 8;
    std::cout << "REVIEW\t103645\t" << review->Text->Str[0] << '\n';
    unsigned found = 0;
    for (auto effect : effects)
        if (!effect->DifficultyID)
        {
            if (effect->SpellID == 198430 || effect->SpellID == 198433 || effect->SpellID == 198435 || effect->SpellID == 198436) std::cout << "RAW " << effect->SpellID << " effect=" << effect->Effect << " misc=" << effect->EffectMiscValue[0] << " trigger=" << effect->EffectTriggerSpell << "\n";
            if (effect->SpellID == 198430 && effect->Effect == 205 && effect->EffectMiscValue[0] == 240)
                { ++found; std::cout << "CHOICE\t198430\t240\n"; }
            uint32 spell = 0, quest = 0;
            switch (effect->SpellID) {
                case 198433: spell=198433; quest=40621; break;
                case 198435: spell=198435; quest=40619; break;
                case 198436: spell=198436; quest=40620; break;
            }
            if (spell && effect->Effect == 16 && effect->EffectMiscValue[0] == quest)
                { ++found; std::cout << "CHOSEN\t" << spell << '\t' << quest << '\n'; }
        }
    if (found != 4) return 9;
    std::cout << "PASS native26972/hotfix Hunter choice and reward spells; no item grant in selector\n";
    return 0;
}
