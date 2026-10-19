$ErrorActionPreference='Stop'
. "$PSScriptRoot/Test-TrialHelyaBreathRelease.ps1"
$calls=@(Calls '?OnCreate@at_bilewater_corrosion@@')
foreach($name in @('?GetSpellValue@AreaTrigger@@','?SetSphereScale@AreaTrigger@@','?UpdateOverrideScale@AreaTrigger@@')){
 if($calls -notcontains (Resolve (Symbol $name))){throw "Canonical Bilewater pool lacks $name"}
}
$range=Bounds '?OnCreate@at_bilewater_corrosion@@';$argument='/RANGE:0x{0:X},0x{1:X}' -f $range[0],$range[1]
$out=Join-Path $repo 'build-extractors/runtime-test/trial-helya-phase1-native'
(& $dump.FullName /DISASM:NOBYTES $argument $exe) | Set-Content "$out/pool-create-disassembly.txt"
if($LASTEXITCODE){throw 'Canonical Bilewater pool disassembly failed'}
if((Get-Content (Join-Path $repo 'build-extractors/runtime-test/trial-helya-phase3-native/debuff-cleanup-disassembly.txt') -Raw) -notmatch 'mov\s+edx,37A8Eh'){throw 'Canonical shared cleanup lacks Bilewater Redox227982'}
[ordered]@{worldserver=(Get-FileHash $exe).Hash;scripts_library=(Get-FileHash $lib).Hash;native_spell_value=$true;server_radius_and_client_scale=$true;redox_cleanup=$true} | ConvertTo-Json | Set-Content "$out/pool-release-inputs.json"
'PASS: canonical Bilewater creation reads native spell value, publishes server radius/client scale and retains Redox cleanup.'
