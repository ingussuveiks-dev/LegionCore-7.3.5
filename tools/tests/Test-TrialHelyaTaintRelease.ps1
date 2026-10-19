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
[ordered]@{worldserver=(Get-FileHash $exe).Hash;scripts_library=(Get-FileHash $lib).Hash;instance_gate=$true;native_raid_burst_and_fixed_destination=$true} | ConvertTo-Json | Set-Content "$out/taint-release-inputs.json"
'PASS: canonical Taint removal checks encounter state and emits raid burst plus fixed-position Essence.'
