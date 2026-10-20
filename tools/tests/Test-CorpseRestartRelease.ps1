$ErrorActionPreference='Stop'
. "$PSScriptRoot/Test-RockawayRelease.ps1"
$out="$repo/build-extractors/runtime-test/corpse-restart-release"
New-Item -ItemType Directory -Force $out | Out-Null
$relative=$release.Link.AdditionalDependencies.Split(';') | Where-Object {$_ -match 'game\\Release\\game.lib$'} | Select-Object -First 1
if(!$relative){throw 'Missing actual game final-link input'}
$gameLib=[IO.Path]::GetFullPath((Join-Path "$repo/build-extractors/src/server/worldserver" $relative))
if((Get-Item "$repo/src/server/game/Globals/ObjectMgr.cpp").LastWriteTimeUtc -gt (Get-Item $gameLib).LastWriteTimeUtc -or (Get-Item $gameLib).LastWriteTimeUtc -gt (Get-Item $exe).LastWriteTimeUtc){throw 'Canonical loader or actual final-link input is stale'}
$run=(Get-Content "$repo/build-extractors/runtime-test/corpse-restart-native/latest-run.txt").Trim()
$inputs=Get-Content "$run/native-inputs.json" -Raw | ConvertFrom-Json
if($inputs.'canonical-worldserver.exe' -ne (Get-FileHash $exe).Hash -or $inputs.'actual-final-link-game.lib' -ne (Get-FileHash $gameLib).Hash){throw 'Cold native fixture and deployed executable/library inputs differ'}
$rows=Get-Content "$run/native-lines.txt"
$abi=@($rows | Where-Object {$_ -match '^CORPSE_ABI field=\d+$'} | Select-Object -Unique)
$pids=@($rows | Where-Object {$_ -match '^CORPSE_PROCESS '})
if($abi.Count -ne 1 -or $pids.Count -ne 2 -or $pids[0] -notmatch 'phase=write pid=(\d+)$'){throw 'Run the two-process native fixture first'}
$writer=$Matches[1]
if($pids[1] -notmatch 'phase=read pid=(\d+)$' -or $Matches[1] -eq $writer){throw 'Cold reader did not use a separate process'}
$field=[int]($abi[0].Split('=')[1]);$offset=($field*15).ToString('X')
$range=Bounds '?LoadCorpses@ObjectMgr@@';$argument='/RANGE:0x{0:X},0x{1:X}' -f $range[0],$range[1]
$listing=@(& $dump.FullName /DISASM:NOBYTES $argument $exe)
if($LASTEXITCODE){throw 'Canonical corpse loader disassembly failed'}
$listing | Set-Content "$out/LoadCorpses-ObjectMgr.txt"
$get=Resolve (Symbol '?GetUInt64@Field@@');$load=Resolve (Symbol '?LoadCorpseFromDB@Corpse@@')
$indexes=@(for($i=0;$i -lt $listing.Count;$i++){$m=[regex]::Match($listing[$i],'call\s+([0-9A-Fa-f]{16})');if($m.Success -and (Resolve ([Convert]::ToInt64($m.Groups[1].Value,16))) -eq $get){$i}})
if($indexes.Count -ne 1){throw 'Review canonical corpse GUID reader'}
$input=$listing[($indexes[0]-1)]
if($input -notmatch "lea\s+rcx,\[rax\+0*${offset}h\]"){throw 'Canonical corpse GUID does not read field15 (corpseGuid)'}
$calls=@(foreach($line in $listing){$m=[regex]::Match($line,'call\s+([0-9A-Fa-f]{16})');if($m.Success){Resolve ([Convert]::ToInt64($m.Groups[1].Value,16))}})
if($calls -notcontains $load){throw 'Canonical startup does not call native corpse hydration'}
# MSVC keeps the uint64 read result in r15 and passes it as the second
# hydration argument (rdx); reject a codegen change instead of guessing.
$body=$listing -join "`n"
if($listing[$indexes[0]+1] -notmatch 'mov\s+r15,rax' -or $body -notmatch 'mov\s+r8,rsi\r?\n[^\r\n]*mov\s+rdx,r15\r?\n[^\r\n]*mov\s+rcx,rax\r?\n[^\r\n]*call'){throw 'Review canonical corpse GUID flow into hydration'}
[ordered]@{worldserver=(Get-FileHash $exe).Hash;game_library=(Get-FileHash $gameLib).Hash;native_inputs=(Get-FileHash "$run/native-inputs.json").Hash;field_size=$field;corpse_guid_column=15;separate_writer_reader_processes=$true} | ConvertTo-Json | Set-Content "$out/inputs.json"
'PASS: canonical Release startup reads uint64 corpseGuid from field15, calls native hydration, and matches the independently restarted native fixture inputs.'
