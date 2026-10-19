$ErrorActionPreference='Stop'
. "$PSScriptRoot/Test-TrialRaidInstanceRelease.ps1"
foreach($path in @('DataStores/DBCEnums.h','Globals/ObjectMgr.cpp','Handlers/CalendarHandler.cpp')){
 if((Get-Item "$repo/src/server/game/$path").LastWriteTimeUtc -gt (Get-Item $gameInput).LastWriteTimeUtc){throw "Final-link game library predates $path"}
}
if((Get-Content "$repo/src/server/game/DataStores/DBCEnums.h" -Raw) -notmatch 'const uint8 MAX_BOUND = 4;'){throw 'Expected four native bind slots'}
# This optimized leaf has no x64 unwind entry. Decode its exact switch
# instructions and both PE-resident tables, rather than guessing a range up
# to an unrelated symbol or only searching for a literal return value.
$start=Resolve (Symbol '?GetboundTypeFromDifficulty@ObjectMgr@@');$at=Offset $start
function CheckBytes([int]$relative,[byte[]]$expected){for($i=0;$i -lt $expected.Length;$i++){if($binary[$at+$relative+$i] -ne $expected[$i]){throw 'Bind-selector leaf instruction layout changed; update the decoder'}}}
CheckBytes 0 @(0x0f,0xb6,0xc2,0x83,0xc0);CheckBytes 6 @(0x83,0xf8);CheckBytes 9 @(0x77);CheckBytes 11 @(0x48,0x8d,0x15)
CheckBytes 18 @(0x48,0x98,0x0f,0xb6,0x84,0x02);CheckBytes 28 @(0x8b,0x8c,0x82);CheckBytes 35 @(0x48,0x03,0xca,0xff,0xe1)
$base=$start+18+[BitConverter]::ToInt32($binary,$at+14);if($base -ne $imageBase){throw 'Unexpected selector table base'}
$byteTable=Offset ($base+[BitConverter]::ToUInt32($binary,$at+24));$jumpTable=Offset ($base+[BitConverter]::ToUInt32($binary,$at+31))
$delta=[int]$binary[$at+5];if($delta -gt 127){$delta-=256};$maximum=$binary[$at+8]
$branch=[int]$binary[$at+10];if($branch -gt 127){$branch-=256};$default=$start+11+$branch
$results=[ordered]@{}
foreach($case in @(@(1,0),@(2,1),@(3,0),@(4,0),@(5,1),@(6,1),@(7,3),@(14,0),@(15,1),@(16,2),@(17,3),@(23,2))){
 $index=$case[0]+$delta;$target=$default
 if($index -ge 0 -and $index -le $maximum){$slot=$binary[$byteTable+$index];if($slot -gt 3){throw 'Unexpected selector jump slot'};$target=$base+[BitConverter]::ToUInt32($binary,$jumpTable+4*$slot)}
 $ret=Offset $target
 if($binary[$ret] -eq 0xb0 -and $binary[$ret+2] -eq 0xc3){$actual=$binary[$ret+1]}
 elseif($binary[$ret] -eq 0x32 -and $binary[$ret+1] -eq 0xc0 -and $binary[$ret+2] -eq 0xc3){$actual=0}
 else{throw 'Unexpected bind-selector return instruction'}
 if($actual -ne $case[1]){throw "Canonical difficulty$($case[0]) maps to bind slot$actual"};$results[[string]$case[0]]=$actual
}
$out=Join-Path $repo 'build-extractors/runtime-test/trial-raid-reentry-native';New-Item -ItemType Directory -Force $out | Out-Null
[ordered]@{worldserver=(Get-FileHash $exe).Hash;actual_game_library=(Get-FileHash $gameInput).Hash;actual_scripts_library=(Get-FileHash $lib).Hash;decoded_canonical_bind_slots=$results;source_bind_array_slots=4} | ConvertTo-Json -Depth 5 | Set-Content "$out/release-inputs.json"
'PASS: canonical PE leaf/tables map both Raid Finder modes to independent slot3 and preserve Normal/Heroic/Mythic/legacy mappings.'
