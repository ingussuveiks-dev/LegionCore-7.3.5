$ErrorActionPreference='Stop'
. "$PSScriptRoot/Test-TrialHelyaTaintRelease.ps1"
$out=Join-Path $repo 'build-extractors/runtime-test/trial-helya-grimelord-native'
foreach($pair in @(@('rot-tick','?Tick@spell_tov_fetid_rot_aura@@'),@('rot-spread','?HandleDamage@spell_fetid_rot@@'),@('grim-update','?UpdateAI@npc_grimelord@@'),@('grim-enter','?EnterCombat@npc_grimelord@@'),@('rot-apply','?ApplyFetidRot@@'),@('grim-reset','?Reset@npc_grimelord@@'))){
 $range=Bounds $pair[1];$argument='/RANGE:0x{0:X},0x{1:X}' -f $range[0],$range[1]
 (& $dump.FullName /DISASM:NOBYTES $argument $exe) | Set-Content "$out/$($pair[0])-disassembly.txt"
 if($LASTEXITCODE){throw 'Canonical Grimelord disassembly failed'}
}
$calls=@(Calls '?Tick@spell_tov_fetid_rot_aura@@')
foreach($name in @('?PreventDefaultAction@AuraScript@@','?ModStackAmount@AuraScript@@')){
 if($calls -notcontains (Resolve (Symbol $name))){throw "Canonical Fetid periodic lacks $name"}
}
if(@(Calls '?HandleDamage@spell_fetid_rot@@') -notcontains (Resolve (Symbol '?GetOriginalCaster@SpellScript@@'))){throw 'Canonical Fetid spread lost original caster'}
$update=Get-Content "$out/grim-update-disassembly.txt" -Raw
foreach($constant in @('3DB8h','2FA8h')){if($update -notmatch (',\s*'+$constant)){throw "Canonical Grimelord lacks Rot repeat $constant"}}
if($update -notmatch ',\s*5EECh'){throw 'Canonical Grimelord lacks24.3-second Nova repeat'}
if((Get-Content "$out/grim-enter-disassembly.txt" -Raw) -notmatch ',\s*1B58h'){throw 'Canonical Grimelord lacks seven-second Rot start'}
foreach($constant in @('2EE0h','4268h','36B0h')){if((Get-Content "$out/grim-enter-disassembly.txt" -Raw) -notmatch (',\s*'+$constant)){throw "Canonical Grimelord first ability timer lacks $constant"}}
$apply=Get-Content "$out/rot-apply-disassembly.txt" -Raw
foreach($constant in @('26B46h','3','4')){if($apply -notmatch (',\s*'+$constant+'\s*\r?\n')){throw "Canonical Rot application lacks Fission158534 or3/4 stacks: $constant"}}
if((Get-Content "$out/grim-enter-disassembly.txt" -Raw) -notmatch ',\s*2710h'){throw 'Canonical Fission lacks ten-second reference activation'}
foreach($body in @($update,(Get-Content "$out/grim-reset-disassembly.txt" -Raw))){if($body -notmatch ',\s*26B46h'){throw 'Canonical Fission cast/reset lacks spell158534'}}
[ordered]@{worldserver=(Get-FileHash $exe).Hash;scripts_library=(Get-FileHash $lib).Hash;prevent_default_and_stack_decay=$true;original_caster=$true;native_selector_timing=$true;fission_activation_application_reset=$true} | ConvertTo-Json | Set-Content "$out/grimelord-release-inputs.json"
'PASS: canonical Grimelord periodic/selector/original caster and Fission activation,3/4 application/reset are deployed.'
