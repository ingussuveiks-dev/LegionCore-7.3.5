$ErrorActionPreference='Stop'
. "$PSScriptRoot/Test-RockawayRelease.ps1"
if((Get-Item "$repo/src/server/scripts/Legion/TrialOfValor/boss_tov_helya.cpp").LastWriteTimeUtc -gt (Get-Item $lib).LastWriteTimeUtc){throw 'Final-link scripts library predates Helya Fury lifecycle repair'}
$interrupt=Resolve (Symbol '?InterruptNonMeleeSpells@Unit@@')
foreach($method in @('EnterEvadeMode','JustDied')){
 if(@(Calls ('?'+$method+'@boss_helya_tov@@')) -notcontains $interrupt){throw "Canonical Helya $method lacks delayed-spell cleanup"}
}
$out=Join-Path $repo 'build-extractors/runtime-test/trial-helya-native';New-Item -ItemType Directory -Force $out | Out-Null
foreach($pair in @(@('finish','?SpellFinishCast@boss_helya_tov@@'),@('aura-remove','?OnRemove@spell_fury_of_the_maw@@'))){
 $range=Bounds $pair[1];$argument='/RANGE:0x{0:X},0x{1:X}' -f $range[0],$range[1]
 $listing=(& $dump.FullName /DISASM:NOBYTES $argument $exe) -join "`n"
 if($LASTEXITCODE){throw 'Canonical Helya disassembly failed'}
 $listing | Set-Content "$out/$($pair[0])-disassembly.txt"
}
# RescheduleEvent is inlined here: the native CancelEvent calls must only
# precede the two boat branches and cleaning, never either skeleton wave.
$cancel=Resolve (Symbol '?CancelEvent@EventMap@@');$schedule=Resolve (Symbol '?ScheduleEvent@EventMap@@QEAAXIIGG@Z')
$calls=@(Calls '?SpellFinishCast@boss_helya_tov@@')
$count=@($calls | Where-Object {$_ -eq $cancel}).Count
if($count -ne 3 -or @($calls | Where-Object {$_ -eq $schedule}).Count -ne 7){throw 'Canonical Fury finish lost independent skeleton schedules or retains their cancellations'}
$removal=Get-Content "$out/aura-remove-disassembly.txt" -Raw
if($removal -notmatch 'cmp\s+byte ptr \[rax\+10h\],4\s*\r?\n[^\r\n]*jne\s+') {throw 'Canonical Fury aura removal lacks native EXPIRE-only branch; review disassembly if ABI changed'}
[ordered]@{worldserver=(Get-FileHash $exe).Hash;actual_scripts_library=(Get-FileHash $lib).Hash;delayed_spell_cleanup_in_evade_and_death=$true;fury_finish_cancel_calls=$count;fury_finish_schedule_calls=7;aura_removal_expire_gate=$true} | ConvertTo-Json | Set-Content "$out/fury-release-inputs.json"
'PASS: canonical Helya PE/thunks retain delayed-spell cleanup and independent skeleton-wave scheduling.'
