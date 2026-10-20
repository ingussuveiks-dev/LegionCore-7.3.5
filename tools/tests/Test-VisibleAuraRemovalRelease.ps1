$ErrorActionPreference='Stop'
. "$PSScriptRoot/Test-TrialHelyaMistwatcherRelease.ps1"
$out=Join-Path $repo 'build-extractors/runtime-test/visible-aura-removal'
New-Item -ItemType Directory -Force $out | Out-Null
if((Get-Item "$repo/src/server/game/Entities/Unit/Unit.cpp").LastWriteTimeUtc -gt (Get-Item $gameLib).LastWriteTimeUtc){throw 'Final-link game library predates visible-aura lifetime fix'}
$method='?_UnapplyAura@Unit@@QEAAXAEAV'
$calls=@(Calls $method)
$remove=Resolve (Symbol '?RemoveVisibleAura@Unit@@')
$linked=Resolve (Symbol '?HandleAuraSpecificMods@Aura@@')
if([Array]::LastIndexOf($calls,$remove) -le [Array]::LastIndexOf($calls,$linked) -or $calls -notcontains $linked){throw 'Canonical unapply lacks final detach after linked aura cleanup'}
foreach($pair in @(@('visible-remove','?RemoveVisibleAura@Unit@@'),@('unapply-final-detach',$method))){
    $range=Bounds $pair[1];$argument='/RANGE:0x{0:X},0x{1:X}' -f $range[0],$range[1]
    & $dump.FullName /DISASM:NOBYTES $argument $exe | Set-Content "$out/$($pair[0])-disassembly.txt"
    if($LASTEXITCODE){throw 'Canonical visible-aura disassembly failed'}
}
[ordered]@{worldserver=(Get-FileHash $exe).Hash;game_library=(Get-FileHash $gameLib).Hash;unit_source=(Get-FileHash "$repo/src/server/game/Entities/Unit/Unit.cpp").Hash;detach_after_linked_cleanup=$true} | ConvertTo-Json | Set-Content "$out/visible-aura-release-inputs.json"
'PASS: canonical aura unapply detaches the application after linked cleanup and before releasing it.'
