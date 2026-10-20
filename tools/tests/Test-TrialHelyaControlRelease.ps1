$ErrorActionPreference='Stop'
. "$PSScriptRoot/Test-TrialHelyaMistwatcherRelease.ps1"
$out=Join-Path $repo 'build-extractors/runtime-test/trial-helya-control-native'
New-Item -ItemType Directory -Force $out | Out-Null
if((Get-Item "$repo/src/server/game/Entities/Unit/Unit.cpp").LastWriteTimeUtc -gt (Get-Item $gameLib).LastWriteTimeUtc){throw 'Final-link game library predates charmed-Player damage fix'}
$range=Bounds '?DealDamage@Unit@@';$argument='/RANGE:0x{0:X},0x{1:X}' -f $range[0],$range[1]
$listing=@(& $dump.FullName /DISASM:NOBYTES $argument $exe)
if($LASTEXITCODE){throw 'Canonical DealDamage disassembly failed'}
$listing | Set-Content "$out/deal-damage-disassembly.txt"
$loot=Resolve (Symbol '?hasLootRecipient@Creature@@')
$indexes=@(for($i=0;$i -lt $listing.Count;$i++){
    $call=[regex]::Match($listing[$i],'call\s+([0-9A-Fa-f]{16})')
    if($call.Success -and (Resolve ([Convert]::ToInt64($call.Groups[1].Value,16))) -eq $loot){$i}
})
if($indexes.Count -ne 1){throw 'Review native loot-recipient call location'}
$index=$indexes[0];$region=$listing[([Math]::Max(0,$index-24))..($index+12)] -join "`n"
# This ABI emits ToCreature as a type3 test, conditional null pointer, then
# a null branch that skips loot admission. Fail for review if codegen changes.
$guard=[regex]::Match($region,'cmp\s+byte ptr \[rsi\+16h\],3\s*\r?\n[^\r\n]*cmovne\s+rbx,r8\s*\r?\n[^\r\n]*test\s+rbx,rbx\s*\r?\n[^\r\n]*je\s+([0-9A-Fa-f]{16})')
if(!$guard.Success -or $region -notmatch 'xor\s+r8d,r8d'){throw 'Canonical loot path lacks creature-only admission; review ABI if changed'}
$callAddress=[Convert]::ToInt64(([regex]::Match($listing[$index],'([0-9A-Fa-f]{16}):')).Groups[1].Value,16)
if([Convert]::ToInt64($guard.Groups[1].Value,16) -le $callAddress){throw 'Non-creature branch does not skip the native loot recipient call'}
$range=Bounds '?OnProc@spell_corrupted_mc@@';$argument='/RANGE:0x{0:X},0x{1:X}' -f $range[0],$range[1]
$tick=@(& $dump.FullName /DISASM:NOBYTES $argument $exe)
if($LASTEXITCODE){throw 'Canonical Corrupted periodic disassembly failed'}
$tick | Set-Content "$out/control-tick-disassembly.txt"
if(($tick -join "`n") -notmatch 'cvttss2si\s+ebx,dword ptr \[rbx\+1Ch\]'){throw 'Canonical Corrupted does not read the native aura amount; review ABI if changed'}
$remove=Resolve (Symbol '?Remove@AuraScript@@');$removedAt=-1
for($i=0;$i -lt $tick.Count;$i++){$call=[regex]::Match($tick[$i],'call\s+([0-9A-Fa-f]{16})');if($call.Success -and (Resolve ([Convert]::ToInt64($call.Groups[1].Value,16))) -eq $remove){$removedAt=$i;break}}
if($removedAt -lt 0){throw 'Canonical Corrupted lacks scoped aura removal'}
$exit=[regex]::Match($tick[$removedAt+1],'jmp\s+([0-9A-Fa-f]{16})')
$castLine=$tick | Where-Object {$_ -match 'mov\s+r8d,37DF6h'} | Select-Object -First 1
if(!$exit.Success -or !$castLine){throw 'Canonical release does not exit before the Infusion path'}
$castAddress=[Convert]::ToInt64(([regex]::Match($castLine,'([0-9A-Fa-f]{16}):')).Groups[1].Value,16)
if([Convert]::ToInt64($exit.Groups[1].Value,16) -le $castAddress){throw 'Canonical release continues into Infusion after removal'}
[ordered]@{worldserver=(Get-FileHash $exe).Hash;game_library=(Get-FileHash $gameLib).Hash;unit_source=(Get-FileHash "$repo/src/server/game/Entities/Unit/Unit.cpp").Hash;scripts_library=(Get-FileHash $lib).Hash;helya_source=(Get-FileHash "$repo/src/server/scripts/Legion/TrialOfValor/boss_tov_helya.cpp").Hash;creature_only_loot_admission=$true;native_break_amount=$true;release_skips_cast=$true} | ConvertTo-Json | Set-Content "$out/control-release-inputs.json"
'PASS: canonical DealDamage admits loot recipients only after the native creature type/null guard.'
'PASS: canonical Corrupted reads its native break percentage and exits after removal before starting Infusion.'
