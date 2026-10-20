$ErrorActionPreference='Stop'
# Shared PE reader resolves incremental-link thunks and bounds methods using
# their native x64 unwind entries, including chained fragments.
. "$PSScriptRoot/Test-RockawayRelease.ps1"
$out=Join-Path $repo 'build-extractors/runtime-test/trial-helya-mistwatcher-native'
New-Item -ItemType Directory -Force $out | Out-Null
$relative=$release.Link.AdditionalDependencies.Split(';') | Where-Object {$_ -match 'game\\Release\\game.lib$'} | Select-Object -First 1
if(!$relative){throw 'Missing actual game final-link input'}
$gameLib=[IO.Path]::GetFullPath((Join-Path "$repo/build-extractors/src/server/worldserver" $relative))
if((Get-Item "$repo/src/server/game/Spells/Spell.cpp").LastWriteTimeUtc -gt (Get-Item $gameLib).LastWriteTimeUtc -or (Get-Item $gameLib).LastWriteTimeUtc -gt (Get-Item $exe).LastWriteTimeUtc){throw 'Canonical executable/game library predates Spell.cpp'}
$range=Bounds '?CheckCast@Spell@@'
$argument='/RANGE:0x{0:X},0x{1:X}' -f $range[0],$range[1]
$listing=@(& $dump.FullName /DISASM:NOBYTES $argument $exe)
if($LASTEXITCODE){throw 'Canonical CheckCast disassembly failed'}
$listing | Set-Content "$out/spell-check-cast-disassembly.txt"
$school=Resolve (Symbol '?HasSchoolMaskCooldown@Creature@@')
$indexes=@(for($i=0;$i -lt $listing.Count;$i++){
    $call=[regex]::Match($listing[$i],'call\s+([0-9A-Fa-f]{16})')
    if($call.Success -and (Resolve ([Convert]::ToInt64($call.Groups[1].Value,16))) -eq $school){$i}
})
if($indexes.Count -ne 1){throw 'Review native school-lockout call location'}
$index=$indexes[0]
$region=$listing[([Math]::Max(0,$index-18))..([Math]::Min($listing.Count-1,$index+6))] -join "`n"
$gate=[regex]::Match($region,'test\s+byte ptr \[[^\]]+\],2\s*\r?\n\s*([0-9A-Fa-f]{16}): jne\s+([0-9A-Fa-f]{16})')
if(!$gate.Success){throw 'Canonical school check lacks triggered cooldown exemption; review ABI if changed'}
$callAddress=[Convert]::ToInt64(([regex]::Match($listing[$index],'([0-9A-Fa-f]{16}):')).Groups[1].Value,16)
if([Convert]::ToInt64($gate.Groups[2].Value,16) -le $callAddress -or $region -notmatch 'mov\s+esi,58h'){throw 'Triggered flag does not skip the ordinary school-lockout rejection'}
[ordered]@{worldserver=(Get-FileHash $exe).Hash;game_library=(Get-FileHash $gameLib).Hash;spell_source=(Get-FileHash "$repo/src/server/game/Spells/Spell.cpp").Hash;triggered_cooldown_gate=$true;ordinary_school_rejection=$true} | ConvertTo-Json | Set-Content "$out/mistwatcher-release-inputs.json"
'PASS: canonical CheckCast bypasses creature school lockout only with the triggered cooldown exemption; ordinary rejection remains.'
