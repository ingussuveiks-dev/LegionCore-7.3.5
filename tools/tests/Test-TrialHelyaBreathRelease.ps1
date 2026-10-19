$ErrorActionPreference='Stop'
. "$PSScriptRoot/Test-TrialHelyaFuryRelease.ps1"
$out=Join-Path $repo 'build-extractors/runtime-test/trial-helya-phase3-native';New-Item -ItemType Directory -Force $out | Out-Null
foreach($pair in @(@('absorb-remove','?OnRemove@spell_corrupted_axion_aura@@'),@('debuff-cleanup','?RemoveAllDebaffsOnPlayer@boss_helya_tov@@'))){
 $range=Bounds $pair[1];$argument='/RANGE:0x{0:X},0x{1:X}' -f $range[0],$range[1]
 $listing=(& $dump.FullName /DISASM:NOBYTES $argument $exe) -join "`n"
 if($LASTEXITCODE){throw 'Canonical Helya breath disassembly failed'}
 $listing | Set-Content "$out/$($pair[0])-disassembly.txt"
}
$body=Get-Content "$out/absorb-remove-disassembly.txt" -Raw
if($body -notmatch 'cmp\s+byte ptr \[rax\+10h\],4\s*\r?\n[^\r\n]*jne\s+'){throw 'Canonical Axion absorb removal lacks native EXPIRE-only gate; review if ABI changed'}
if(@(Calls '?OnRemove@spell_corrupted_axion_aura@@') -notcontains (Resolve (Symbol '?GetInstanceScript@WorldObject@@'))){throw 'Canonical Axion removal lacks live instance lookup'}
if((Get-Content "$out/debuff-cleanup-disassembly.txt" -Raw) -notmatch 'mov\s+edx,38C28h'){throw 'Canonical encounter cleanup lacks Dark Hatred232488'}
[ordered]@{worldserver=(Get-FileHash $exe).Hash;actual_scripts_library=(Get-FileHash $lib).Hash;axion_expire_gate=$true;axion_instance_lookup=$true;dark_hatred_cleanup=$true} | ConvertTo-Json | Set-Content "$out/breath-release-inputs.json"
'PASS: canonical Helya PE/thunks retain Axion expiry/instance gates and Dark Hatred cleanup.'
