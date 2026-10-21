$ErrorActionPreference='Stop'
. "$PSScriptRoot/Test-RockawayRelease.ps1"
$out="$repo/build-extractors/runtime-test/ghost-return-release"
New-Item -ItemType Directory -Force $out | Out-Null
$relative=$release.Link.AdditionalDependencies.Split(';') | Where-Object {$_ -match 'game\\Release\\game.lib$'} | Select-Object -First 1
if(!$relative){throw 'Missing actual game final-link input'}
$gameLib=[IO.Path]::GetFullPath((Join-Path "$repo/build-extractors/src/server/worldserver" $relative))
foreach($path in @('Maps/Map.cpp','Maps/MapManager.cpp','Entities/EventObject/EventObject.cpp','Handlers/MovementHandler.cpp')){if((Get-Item "$repo/src/server/game/$path").LastWriteTimeUtc -gt (Get-Item $gameLib).LastWriteTimeUtc){throw "Game final-link input predates $path"}}
if((Get-Item $gameLib).LastWriteTimeUtc -gt (Get-Item $exe).LastWriteTimeUtc){throw 'Canonical executable predates actual game library'}
$bodies=@{}
foreach($name in @('CanPlayerEnter@MapManager','CanEnter@InstanceMap','Update@EventObject','HandleWorldPortAck@WorldSession')){
 $range=Bounds ('?'+$name+'@@');$argument='/RANGE:0x{0:X},0x{1:X}' -f $range[0],$range[1]
 $listing=@(& $dump.FullName /DISASM:NOBYTES $argument $exe)
 if($LASTEXITCODE){throw "Canonical disassembly failed: $name"}
 $listing | Set-Content "$out/$($name.Replace('@','-')).txt";$bodies[$name]=$listing
}
function NativeCalls($listing){foreach($line in $listing){$m=[regex]::Match($line,'call\s+([0-9A-Fa-f]{16})');if($m.Success){Resolve ([Convert]::ToInt64($m.Groups[1].Value,16))}}}
$resurrect=Resolve (Symbol '?ResurrectPlayer@Player@@');$bones=Resolve (Symbol '?SpawnCorpseBones@Player@@');$corpse=Resolve (Symbol '?GetCorpse@Player@@')
$preflight=@(NativeCalls $bodies['CanPlayerEnter@MapManager']);$ack=@(NativeCalls $bodies['HandleWorldPortAck@WorldSession'])
if($preflight -notcontains $corpse -or $preflight -contains $resurrect -or $preflight -contains $bones){throw 'Canonical admission no longer validates corpse, or still consumes/resurrects it'}
if($ack -notcontains $resurrect -or $ack -notcontains $bones){throw 'Canonical successful worldport lacks native resurrection/corpse conversion'}
$run=(Get-Content "$repo/build-extractors/runtime-test/trial-helya-control-native/latest-run.txt").Trim()
$abi=@(Get-Content "$run/native-lines.txt" | Where-Object {$_ -match '^GHOST_INSTANCE_ABI '} | Select-Object -Unique)
if($abi.Count -ne 1 -or $abi[0] -notmatch '^GHOST_INSTANCE_ABI object=(\d+) map=(\d+)$'){throw 'Run the current native different-instance fixture first'}
$objectInstance=([int]$Matches[1]).ToString('X');$mapInstance=([int]$Matches[2]).ToString('X')
$admission=$bodies['CanEnter@InstanceMap'] -join "`n"
# The selected map instance is compared with the actual corpse instance;
# equality continues admission, mismatch sends locked-to-different-instance12h.
$instanceGate=[regex]::Match($admission,"mov\s+(?<id>r\w+d),dword ptr \[[^\]]+\+0*${mapInstance}h\]\r?\n[^\r\n]*cmp\s+dword ptr \[[^\]]+\+0*${objectInstance}h\],\k<id>\r?\n[^\r\n]*je\s+(?<continue>[0-9A-Fa-f]{16})\r?\n[^\r\n]*xor\s+r9d,r9d\r?\n[^\r\n]*mov\s+r8d,12h")
if(!$instanceGate.Success -or @(NativeCalls $bodies['CanEnter@InstanceMap']) -notcontains $corpse){throw 'Canonical selected-instance corpse admission gate missing; review ABI/codegen if changed'}
$ackBody=$bodies['HandleWorldPortAck@WorldSession'] -join "`n"
$ackGate=[regex]::Match($ackBody,"mov\s+(?<id>\w+),dword ptr \[(?<player>\w+)\+0*${objectInstance}h\]\r?\n[^\r\n]*cmp\s+dword ptr \[(?<corpse>\w+)\+0*${objectInstance}h\],\k<id>\r?\n[^\r\n]*jne\s+(?<skip>[0-9A-Fa-f]{16})")
if(!$ackGate.Success){throw 'Canonical corpse/player instance comparison before resurrection missing'}
$resurrectLine=@($bodies['HandleWorldPortAck@WorldSession'] | Where-Object {$m=[regex]::Match($_,'call\s+([0-9A-Fa-f]{16})');$m.Success -and (Resolve ([Convert]::ToInt64($m.Groups[1].Value,16))) -eq $resurrect})
if($resurrectLine.Count -ne 1 -or [Convert]::ToInt64($ackGate.Groups['skip'].Value,16) -le [Convert]::ToInt64(([regex]::Match($resurrectLine[0],'^\s*([0-9A-Fa-f]{16}):')).Groups[1].Value,16)){throw 'Canonical instance mismatch does not skip the native resurrection call'}
$constructor=Resolve (Symbol '??0AnyPlayerInObjectRangeCheck@Trinity@@');$listing=$bodies['Update@EventObject']
$indexes=@(for($i=0;$i -lt $listing.Count;$i++){$m=[regex]::Match($listing[$i],'call\s+([0-9A-Fa-f]{16})');if($m.Success -and (Resolve ([Convert]::ToInt64($m.Groups[1].Value,16))) -eq $constructor){$i}})
if($indexes.Count -ne 1){throw 'Review canonical EventObject player-range admission'}
$region=$listing[($indexes[0]-18)..$indexes[0]] -join "`n"
# MSVC x64 passes reqAlive in r9b. Native EventObjectTemplate has
# WorldSafeLocID at8 and SpellID at4. Only a pure location portal passes false.
$gate=[regex]::Match($region,'cmp\s+dword ptr \[(?<template>\w+)\+8\],0\r?\n[^\r\n]*je\s+(?<alive>[0-9A-Fa-f]{16})\r?\n[^\r\n]*cmp\s+dword ptr \[\k<template>\+4\],0\r?\n[^\r\n]*jne\s+\k<alive>\r?\n[^\r\n]*xor\s+r9b,r9b\r?\n[^\r\n]*jmp\s+(?<ready>[0-9A-Fa-f]{16})\r?\n\s*\k<alive>:\s*mov\s+r9b,1\r?\n\s*\k<ready>:')
if(!$gate.Success){throw 'Canonical pure-portal ghost admission or living spell/mixed gate missing; review codegen/ABI if changed'}
[ordered]@{worldserver=(Get-FileHash $exe).Hash;game_library=(Get-FileHash $gameLib).Hash;corpse_preflight_without_resurrection=$true;selected_instance_rejects_foreign_corpse=$true;ack_requires_matching_instance=$true;successful_ack_resurrects=$true;native_pure_portal_reqAlive_false=$true;native_spell_and_mixed_reqAlive_true=$true} | ConvertTo-Json | Set-Content "$out/inputs.json"
'PASS: canonical Release selected-instance corpse admission and ACK instance match verified; preflight has no resurrection; same-instance ACK and pure-portal ghost admission retained.'
