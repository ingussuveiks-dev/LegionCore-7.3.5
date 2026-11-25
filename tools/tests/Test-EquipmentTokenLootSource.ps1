param([string]$EvidenceDirectory = "$PSScriptRoot/../../build-extractors/runtime-test/equipment-token-loot")

$ErrorActionPreference = 'Stop'
$repo = (Resolve-Path "$PSScriptRoot/../..").Path
$output = $ExecutionContext.SessionState.Path.GetUnresolvedProviderPathFromPSPath($EvidenceDirectory)

& "$PSScriptRoot/Invoke-WithRuntimeDiagnostics.ps1" -EvidenceDirectory $output -Test {
    $source = Get-Content "$repo/src/server/game/Spells/SpellEffects.cpp" -Raw
    $start = $source.IndexOf('    // Item-backed equipment tokens', $source.IndexOf('void Spell::EffectBonusLoot('))
    if ($start -lt 0) { throw 'Missing the production token loot dispatch' }
    $end = $source.IndexOf('    loot->AutoStoreItems();', $start)
    if ($end -lt 0) { throw 'Missing the production token loot dispatch boundary' }
    [IO.File]::WriteAllText("$output/TokenLootDispatch.inc", $source.Substring($start, $end - $start))
    $fixture = @'
#include <cassert>
#include <iostream>
#include <vector>

struct Item
{
    unsigned entry;
    unsigned GetEntry() const { return entry; }
};
struct Store
{
    unsigned id;
    unsigned entry;
    bool HaveLootFor(unsigned key) const { return entry && entry == key; }
};
struct Loot
{
    bool _isTokenLoot = true;
    bool _isLegendaryLoot = false;
    std::vector<unsigned> fills;
    unsigned missing = 0;

    bool FillLoot(unsigned id, Store const& store, int, bool)
    {
        fills.push_back(store.id);
        if (store.HaveLootFor(id))
            return true;
        ++missing;
        return _isLegendaryLoot;
    }
};

void Dispatch(Loot* loot, Item const* m_CastItem, unsigned lootId,
    Store LootTemplates_Spell, Store LootTemplates_Item)
{
    int player = 0;
#include "TokenLootDispatch.inc"
}

int main()
{
    for (auto pair : {std::pair{240412u, 147213u}, std::pair{240418u, 147220u}})
    {
        Item item{pair.second};
        Store noSpell{1, 0}, spell{1, pair.first}, noItem{2, 0}, givenItem{2, pair.second};
        Loot token;
        Dispatch(&token, &item, pair.first, noSpell, givenItem);
        assert(token.fills == std::vector<unsigned>{2} && token.missing == 0);

        Loot preferred;
        Dispatch(&preferred, &item, pair.first, spell, givenItem);
        assert(preferred.fills == std::vector<unsigned>{1} && preferred.missing == 0);

        Loot missing;
        Dispatch(&missing, &item, pair.first, noSpell, noItem);
        assert((missing.fills == std::vector<unsigned>{1, 2}) && missing.missing == 2);

        Loot noCastItem;
        Dispatch(&noCastItem, nullptr, pair.first, noSpell, givenItem);
        assert(noCastItem.fills == std::vector<unsigned>{1} && noCastItem.missing == 1);

        Loot legendary;
        legendary._isLegendaryLoot = true;
        Dispatch(&legendary, &item, pair.first, noSpell, givenItem);
        assert(legendary.fills == std::vector<unsigned>{1} && legendary.missing == 1);

        Loot otherEffect;
        otherEffect._isTokenLoot = false;
        Dispatch(&otherEffect, &item, pair.first, noSpell, givenItem);
        assert((otherEffect.fills == std::vector<unsigned>{1, 2}) && otherEffect.missing == 1);
    }
    std::cout << "PASS 12 equipment token loot dispatch cases\n";
}
'@
    [IO.File]::WriteAllText("$output/EquipmentTokenLoot.cpp", $fixture)
    $vs = & "${env:ProgramFiles(x86)}/Microsoft Visual Studio/Installer/vswhere.exe" -latest -products '*' -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
    if (!$vs) { throw 'Visual C++ compiler not found' }
    $commands = @"
@echo off
call "$vs/VC/Auxiliary/Build/vcvars64.bat" >nul
if errorlevel 1 exit /b 1
cd /d "$output"
cl /nologo /EHsc /std:c++17 /W4 EquipmentTokenLoot.cpp
if errorlevel 1 exit /b 1
EquipmentTokenLoot.exe
exit /b %errorlevel%
"@
    [IO.File]::WriteAllText("$output/run.cmd", $commands)
    & "$output/run.cmd"
    if ($LASTEXITCODE) { throw 'Equipment token loot dispatch failed' }
}
