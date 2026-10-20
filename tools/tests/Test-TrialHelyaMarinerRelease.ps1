$ErrorActionPreference='Stop'
. "$PSScriptRoot/Test-TrialHelyaGrimelordRelease.ps1"
$out=Join-Path $repo 'build-extractors/runtime-test/trial-helya-mariner-native'
foreach($pair in @(@('mariner-update','?UpdateAI@npc_night_watch_mariner@@'),@('quarter-hit','?HandleHit@spell_tov_give_no_quarter@@'),@('darkness-tick','?Tick@spell_tov_darkness@@'),@('mariner-reset','?Reset@npc_night_watch_mariner@@'))){$range=Bounds $pair[1];$argument='/RANGE:0x{0:X},0x{1:X}' -f $range[0],$range[1];(& $dump.FullName /DISASM:NOBYTES $argument $exe) | Set-Content "$out/$($pair[0])-disassembly.txt";if($LASTEXITCODE){throw 'Mariner canonical disassembly failed'}}
$update=Get-Content "$out/mariner-update-disassembly.txt" -Raw
foreach($value in @('37D19h','2454h','17D4h')){if($update -notmatch (',\s*'+$value)){throw "Canonical Mariner selector/cadence missing $value"}}
if((Get-Content "$out/quarter-hit-disassembly.txt" -Raw) -notmatch ',\s*37D18h'){throw 'Canonical selector does not launch228632 missile'}
if(@(Calls '?Tick@spell_tov_darkness@@') -notcontains (Resolve (Symbol '?GetUnitOwner@AuraScript@@'))){throw 'Canonical Darkness lacks unit-owner periodic admission'}
if(@(Calls '?Reset@npc_night_watch_mariner@@') -notcontains (Resolve (Symbol '?SetPowerType@Unit@@'))){throw 'Canonical Mariner reset does not select primary energy'}
if($update -notmatch ',\s*37D0Bh'){throw 'Canonical Mariner event queue lacks Lantern228619'}
[ordered]@{worldserver=(Get-FileHash $exe).Hash;scripts_library=(Get-FileHash $lib).Hash;native_selector=$true;destination_missile=$true;primary_energy=$true;owner_periodic=$true;lantern_event=$true} | ConvertTo-Json | Set-Content "$out/mariner-release-inputs.json"
'PASS: canonical Mariner selector/missile, primary energy, owner-based Darkness tick and queued Lantern are deployed.'
