$ErrorActionPreference='Stop'
. "$PSScriptRoot/Test-LostArmyTurnInRelease.ps1"
foreach($path in @('Loot/LootMgr.cpp','Loot/LootMgr.h','Entities/Unit/Unit.cpp','Entities/Player/Player.cpp')){
 if((Get-Item "$repo/src/server/game/$path").LastWriteTimeUtc -gt (Get-Item $gameInput).LastWriteTimeUtc){throw "Final-link game library predates $path"}
}
$out=Join-Path $repo 'build-extractors/runtime-test/raid-fragments-native'
New-Item -ItemType Directory -Force $out | Out-Null
$snapshot=[regex]::Match($map,'\s(\?FillLoot@Loot@@\S+WorldObject@@PEBV\S+)\s+[0-9A-Fa-f]{16}\s+f')
if(!$snapshot.Success -or @(Calls '?GeneratePersonalLoot@Unit@@') -notcontains (Resolve (Symbol $snapshot.Groups[1].Value))){throw 'Canonical raid generation lacks the eligibility snapshot overload'}
$range=Bounds '?AllowedForPlayer@LootItem@@';$argument='/RANGE:0x{0:X},0x{1:X}' -f $range[0],$range[1]
$admission=(& $dump.FullName /DISASM:NOBYTES $argument $exe) -join "`n"
if($LASTEXITCODE -or $admission -notmatch 'test\s+byte ptr \[[^\]]+\+65h\],1'){throw 'Canonical item admission lacks the restricted-looter flag branch'}
$admission | Set-Content "$out/item-admission-disassembly.txt"
if(@(Calls '?SendLoot@Player@@') -notcontains (Resolve (Symbol '?IsPlayerLootCooldown@Player@@'))){throw 'Canonical chest generation lacks cooldown admission'}
[ordered]@{worldserver=(Get-FileHash $exe).Hash;actual_game_library=(Get-FileHash $gameInput).Hash;raid_snapshot=$true;restricted_looter_admission=$true;chest_cooldown=$true} | ConvertTo-Json | Set-Content "$out/release-inputs.json"
'PASS: canonical Release raid recipient snapshot/admission and chest cooldown verified through actual PE/thunks and final-link freshness.'
