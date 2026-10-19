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
$hit=@(Calls '?SpellHit@boss_helya_tov@@')
foreach($name in @('?GetNearPoint2D@WorldObject@@QEBAXAEAM0MM_N@Z','?GetHeight@WorldObject@@QEBAMMMM_NM@Z')){
 if($hit -notcontains (Resolve (Symbol $name))){throw "Canonical Helya breath lacks projectile geometry call $name"}
}
if($hit -contains (Resolve (Symbol '?GetNearPosition@WorldObject@@'))){throw 'Canonical Helya breath retains walking-position fallback'}
if($hit -contains (Resolve (Symbol '?GetFirstCollisionPosition@WorldObject@@'))){throw 'Canonical Bilewater breath retains walking collision/range contraction'}
$range=Bounds '?SpellHit@boss_helya_tov@@';$argument='/RANGE:0x{0:X},0x{1:X}' -f $range[0],$range[1]
(& $dump.FullName /DISASM:NOBYTES $argument $exe) | Set-Content "$out/breath-hit-disassembly.txt"
if($LASTEXITCODE){throw 'Canonical Helya geometry disassembly failed'}
[ordered]@{worldserver=(Get-FileHash $exe).Hash;actual_scripts_library=(Get-FileHash $lib).Hash;axion_expire_gate=$true;axion_instance_lookup=$true;dark_hatred_cleanup=$true;projectile_xy_and_native_surface_calls=$true;walking_fallback_absent=$true} | ConvertTo-Json | Set-Content "$out/breath-release-inputs.json"
'PASS: canonical Helya PE/thunks retain Axion expiry/instance gates, Dark Hatred cleanup and projectile surface placement.'
