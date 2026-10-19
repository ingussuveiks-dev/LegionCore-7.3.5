$ErrorActionPreference='Stop'
. "$PSScriptRoot/Test-TrialHelyaPoolRelease.ps1"
$calls=@(Calls '?OnRemove@spell_taint_of_the_sea@@')
foreach($name in @('?GetInstanceScript@WorldObject@@','?CastSpell@Unit@@QEAA?AW4SpellCastResult@@PEAV1@I_N','?CastSpell@Unit@@QEAA?AW4SpellCastResult@@MMMI_N')){
 if($calls -notcontains (Resolve (Symbol $name))){throw "Canonical Taint removal lacks $name"}
}
$out=Join-Path $repo 'build-extractors/runtime-test/trial-helya-taint-native'
$range=Bounds '?OnRemove@spell_taint_of_the_sea@@';$argument='/RANGE:0x{0:X},0x{1:X}' -f $range[0],$range[1]
(& $dump.FullName /DISASM:NOBYTES $argument $exe) | Set-Content "$out/taint-remove-disassembly.txt"
if($LASTEXITCODE){throw 'Canonical Taint removal disassembly failed'}
$return='?_Do_call@?$_Func_impl_no_alloc@V<lambda_1>@?1???R<lambda_2>@?3??StartEvents@boss_helya_tov@@QEAAXE@Z@QEBAXXZ@X$$V@std@@EEAAXXZ'
foreach($pair in @(@('taint-update','?UpdateAI@boss_helya_tov@@'),@('taint-start','?StartEvents@boss_helya_tov@@'),@('taint-return',$return))){
 $range=Bounds $pair[1];$argument='/RANGE:0x{0:X},0x{1:X}' -f $range[0],$range[1]
 (& $dump.FullName /DISASM:NOBYTES $argument $exe) | Set-Content "$out/$($pair[0])-disassembly.txt"
 if($LASTEXITCODE){throw 'Canonical Taint scheduling disassembly failed'}
}
$update=Get-Content "$out/taint-update-disassembly.txt" -Raw
$arrival=Get-Content "$out/taint-return-disassembly.txt" -Raw
$start=Get-Content "$out/taint-start-disassembly.txt" -Raw
foreach($delay in @('3C8Ch','4C2Ch','5528h')){if($start -notmatch (',\s*'+$delay)){throw "Canonical first-phase Taint start lacks $delay; inspect compiler output if changed"}}
foreach($delay in @('4268h','2F44h')){if($update -notmatch (',\s*'+$delay)){throw "Canonical first-phase Taint repeat lacks $delay; inspect compiler output if changed"}}
if($update -notmatch 'mov\s+ecx,4E20h' -or $update -notmatch 'mov\s+r8d,6D60h'){throw 'Canonical Taint lacks phase-three20/28-second cadence; inspect compiler output if changed'}
if($arrival -notmatch 'mov\s+edx,37CC2h' -or $arrival -notmatch 'mov\s+r8d,5FB4h' -or $arrival -notmatch 'mov\s+esi,1\s*\r?\n' -or $arrival -notmatch 'cmp\s+byte ptr \[rcx\+0000000000000188h\],3'){throw 'Canonical delayed return lacks phase gate,228546 marker or1ms/24.5s Taint timer; inspect ABI/compiler if changed'}
foreach($name in @('?CancelEvent@EventMap@@','?ScheduleEvent@EventMap@@QEAAXIIGG@Z')){
 if(@(Calls $return) -notcontains (Resolve (Symbol $name))){throw "Canonical return lacks native scheduling call $name"}
}
[ordered]@{worldserver=(Get-FileHash $exe).Hash;scripts_library=(Get-FileHash $lib).Hash;instance_gate=$true;native_raid_burst_and_fixed_destination=$true;phase_three_first_and_repeat_schedule=$true;phase_one_difficulty_first_and_repeat=$true} | ConvertTo-Json | Set-Content "$out/taint-release-inputs.json"
'PASS: canonical Taint removal emits raid burst/fixed-position Essence; bounded return/update bodies retain phase-three scheduling.'
