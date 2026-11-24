param([switch]$VerifyRuntime)

$ErrorActionPreference = 'Stop'
$root = (Resolve-Path "$PSScriptRoot/../..").Path
$configs = @("$root/src/server/worldserver/worldserver.conf.dist")
if ($VerifyRuntime) { $configs += "$root/build-extractors/bin/Release/worldserver.conf" }

foreach ($path in $configs) {
    $content = Get-Content -LiteralPath $path -Raw
    foreach ($setting in @{ 'Arena.ArenaSeason.ID' = 25; 'PvP.LevelEnable' = 0; 'PvP.ActiveSeason' = 7 }.GetEnumerator()) {
        $matches = [regex]::Matches($content, '(?im)^\s*' + [regex]::Escape($setting.Key) + '\s*=\s*(\d+)\s*$')
        if ($matches.Count -ne 1 -or [int]$matches[0].Groups[1].Value -ne $setting.Value) {
            throw "Unexpected $($setting.Key) in $path"
        }
    }
}

$source = Get-Content -LiteralPath "$root/src/server/game/World/World.cpp" -Raw
if ($source -notmatch 'GetBoolDefault\("PvP.LevelEnable", false\)' -or
    $source -notmatch '(?s)GetIntDefault\("PvP.ActiveSeason", 7\).*?pvpRewardSeason < 0 \|\| pvpRewardSeason > 7.*?pvpRewardSeason = 7;.*?CONFIG_PVP_ACTIVE_SEASON\] = pvpRewardSeason;') {
    throw 'Missing consistent defaults or bounds check'
}

'Legion Season 7 client ID, end-of-expansion reward path and legacy season bounds passed.'
