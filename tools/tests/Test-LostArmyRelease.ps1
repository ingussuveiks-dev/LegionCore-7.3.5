$ErrorActionPreference='Stop'
# Reuse the canonical PE range/thunk reader and keep the existing quest paths
# checked against the same executable while validating the shared loot change.
. "$PSScriptRoot/Test-RockawayRelease.ps1"
[xml]$nativeProject=Get-Content "$repo/build-extractors/src/server/worldserver/worldserver.vcxproj"
$nativeRelease=$nativeProject.Project.ItemDefinitionGroup | Where-Object Condition -like '*Release|x64*' | Select-Object -First 1
$gameInput=$nativeRelease.Link.AdditionalDependencies.Split(';') | Where-Object {$_ -match 'game\\Release\\game.lib$'} | Select-Object -First 1
if(!$gameInput){throw 'Missing generated final-link game library'}
$gameInput=[IO.Path]::GetFullPath((Join-Path "$repo/build-extractors/src/server/worldserver" $gameInput))
foreach($path in @('Loot/LootMgr.cpp','Entities/Player/Player.cpp')){
 if((Get-Item "$repo/src/server/game/$path").LastWriteTimeUtc -gt (Get-Item $gameInput).LastWriteTimeUtc){throw "Final-link game library predates $path"}
}
if((Get-Item $gameInput).LastWriteTimeUtc -gt (Get-Item $exe).LastWriteTimeUtc){throw 'Canonical executable predates actual game library'}
$allowed=Resolve (Symbol '?AllowedForPlayer@Loot@@')
$visible=Resolve (Symbol '?IsLootItemVisibleTo@@')
if(@(Calls '?LootItemInSlot@Loot@@') -notcontains $allowed){throw 'Deployed slot lookup lacks current recipient admission'}
$viewCalls=@(Calls '?BuildLootResponse@Loot@@') | Where-Object {$_ -eq $visible}
$availableCalls=@(Calls '?hasItemFor@Loot@@') | Where-Object {$_ -eq $visible}
if($viewCalls.Count -ne 4 -or $availableCalls.Count -ne 3){throw "Deployed cached view/availability paths lack revalidation ($($viewCalls.Count)/$($availableCalls.Count))"}
$out=Join-Path $repo 'build-extractors/runtime-test/lost-army-native'
New-Item -ItemType Directory -Force $out | Out-Null
$range=Bounds '?AutoStoreLoot@Player@@QEAA_NEEIAEBVLootStore@@_NI@Z'
$argument='/RANGE:0x{0:X},0x{1:X}' -f $range[0],$range[1]
$listing=(& $dump.FullName /DISASM:NOBYTES $argument $exe) -join "`n"
if($LASTEXITCODE){throw 'Canonical AutoStore disassembly failed'}
$listing | Set-Content "$out/autostore-disassembly.txt"
$slotAddress=Symbol '?LootItemInSlot@Loot@@'
$slotBody=Resolve $slotAddress
$callPattern='call\s+([0-9A-Fa-f]+)\s*\r?\n(?:[^\r\n]*\r?\n){0,3}[^\r\n]*test\s+rax,rax\s*\r?\n[^\r\n]*je\s+'
$guarded=$false
foreach($site in [regex]::Matches($listing,$callPattern)){
 if((Resolve ([Convert]::ToInt64($site.Groups[1].Value,16))) -eq $slotBody){$guarded=$true}
}
if(!$guarded){throw 'Deployed AutoStore lacks immediate null-result branch after native slot lookup'}
[ordered]@{worldserver=(Get-FileHash $exe).Hash;actual_game_library=(Get-FileHash $gameInput).Hash;cached_view_calls=$viewCalls.Count;cached_availability_calls=$availableCalls.Count} | ConvertTo-Json | Set-Content "$out/release-inputs.json"
'PASS: canonical Release uses current native slot admission, all4 cached response paths, all3 availability paths and AutoStore null-result protection; actual final-link game/source freshness and hashes recorded.'
