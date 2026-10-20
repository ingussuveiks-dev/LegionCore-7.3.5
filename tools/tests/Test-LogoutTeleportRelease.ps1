$ErrorActionPreference='Stop'
. "$PSScriptRoot/Test-RockawayRelease.ps1"
$out="$repo/build-extractors/runtime-test/logout-teleport-release"
New-Item -ItemType Directory -Force $out | Out-Null
$relative=$release.Link.AdditionalDependencies.Split(';') | Where-Object {$_ -match 'game\\Release\\game.lib$'} | Select-Object -First 1
if(!$relative){throw 'Missing actual game final-link input'}
$gameLib=[IO.Path]::GetFullPath((Join-Path "$repo/build-extractors/src/server/worldserver" $relative))
foreach($path in @('Entities/Player/Player.cpp','Server/WorldSession.cpp')){if((Get-Item "$repo/src/server/game/$path").LastWriteTimeUtc -gt (Get-Item $gameLib).LastWriteTimeUtc){throw "Final-link game library predates $path"}}
if((Get-Item $gameLib).LastWriteTimeUtc -gt (Get-Item $exe).LastWriteTimeUtc){throw 'Canonical executable predates its actual game library'}
foreach($name in @('SaveToDB@Player','LogoutPlayer@WorldSession')){
 $range=Bounds ('?'+$name+'@@');$argument='/RANGE:0x{0:X},0x{1:X}' -f $range[0],$range[1]
 $listing=@(& $dump.FullName /DISASM:NOBYTES $argument $exe)
 if($LASTEXITCODE){throw "Canonical disassembly failed: $name"}
 $listing | Set-Content "$out/$($name.Replace('@','-')).txt"
}
$run=(Get-Content "$repo/build-extractors/runtime-test/trial-helya-control-native/latest-run.txt").Trim()
$abiRows=@(Get-Content "$run/native-lines.txt" | Where-Object {$_ -match '^LOGOUT_ABI '} | Select-Object -Unique)
if($abiRows.Count -ne 1){throw 'Run the current native logout fixture before Release verification'}
$abi=[regex]::Match($abiRows[0],'^LOGOUT_ABI far=(\d+) session=(\d+) logout=(\d+)$')
if(!$abi.Success){throw 'Native logout ABI evidence changed'}
$far=([int]$abi.Groups[1].Value).ToString('X');$session=([int]$abi.Groups[2].Value).ToString('X');$logout=([int]$abi.Groups[3].Value).ToString('X')
$save=Get-Content "$out/SaveToDB-Player.txt" -Raw
# The live-transfer branch must defer; the same pending far transfer during
# logout must bypass that return. Check the two jumps share the actual body.
$gate=[regex]::Match($save,"(?m)cmp\s+byte ptr \[(?<player>r\w+)\+0*${far}h\],(?<zero>r\w+b)\r?\n[^\r\n]*je\s+(?<body>[0-9A-Fa-f]{16})\r?\n[^\r\n]*mov\s+(?<session>r\w+),qword ptr \[\k<player>\+0*${session}h\]\r?\n[^\r\n]*cmp\s+byte ptr \[\k<session>\+0*${logout}h\],\k<zero>\r?\n[^\r\n]*jne\s+\k<body>\r?\n[^\r\n]*or\s+dword ptr \[[^\]]+\],1\r?\n[^\r\n]*jmp\s+(?<exit>[0-9A-Fa-f]{16})\r?\n\s*\k<body>:")
if(!$gate.Success -or [Convert]::ToInt64($gate.Groups['exit'].Value,16) -le [Convert]::ToInt64($gate.Groups['body'].Value,16)){throw 'Canonical SaveToDB lacks the native far-transfer/logout admission; review ABI/codegen if changed'}
$zero=$gate.Groups['zero'].Value -replace 'b$','d'
if($save -notmatch "xor\s+${zero},${zero}"){throw 'Save admission comparison register is not zero-initialized'}
$listing=@(Get-Content "$out/LogoutPlayer-WorldSession.txt");$code=$listing -join "`n"
if($code -match "mov\s+byte ptr \[\w+\+0*${far}h\],0\b"){throw 'Logout still clears the far destination before persistence'}
if($code -notmatch "mov\s+byte ptr \[\w+\+0*${far}h\],1\b"){throw 'Logout fallback does not retain destination-save admission'}
$validator=Resolve (Symbol '?IsValidMapCoord@MapManager@@SA_NAEBVWorldLocation@@@Z')
$validatorIndexes=@(for($i=0;$i -lt $listing.Count;$i++){$m=[regex]::Match($listing[$i],'call\s+([0-9A-Fa-f]{16})');if($m.Success -and (Resolve ([Convert]::ToInt64($m.Groups[1].Value,16))) -eq $validator){$i}})
if($validatorIndexes.Count -ne 1){throw 'Canonical logout does not validate its pending WorldLocation'}
$index=$validatorIndexes[0];$around=$listing[($index-3)..($index+3)] -join "`n"
$validation=[regex]::Match($around,"(?m)cmp\s+byte ptr \[[^\]]+\+0*${far}h\],0\r?\n[^\r\n]*je\s+(?<keep>[0-9A-Fa-f]{16})[\s\S]*test\s+al,al\r?\n[^\r\n]*jne\s+\k<keep>")
if(!$validation.Success){throw 'Valid pending location does not skip the homebind fallback'}
$saveAddress=Resolve (Symbol '?SaveToDB@Player@@')
$saveCalls=@($listing | Where-Object {$m=[regex]::Match($_,'call\s+([0-9A-Fa-f]{16})');$m.Success -and (Resolve ([Convert]::ToInt64($m.Groups[1].Value,16))) -eq $saveAddress})
if($saveCalls.Count -ne 1){throw 'Logout does not call the verified native SaveToDB implementation'}
$zoneAddress=Resolve (Symbol '?GetZoneId@MapManager@@')
$zoneCalls=@(Get-Content "$out/SaveToDB-Player.txt" | Where-Object {$m=[regex]::Match($_,'call\s+([0-9A-Fa-f]{16})');$m.Success -and (Resolve ([Convert]::ToInt64($m.Groups[1].Value,16))) -eq $zoneAddress})
if($zoneCalls.Count -ne 1){throw 'Canonical pending save lacks native destination-zone resolution'}
[ordered]@{worldserver=(Get-FileHash $exe).Hash;game_library=(Get-FileHash $gameLib).Hash;native_abi=$abiRows[0];live_save_defers=$true;logout_saves_destination=$true;valid_destination_skips_homebind=$true;fallback_retains_far_flag=$true;native_save_dispatch=$true;native_destination_zone=$true} | ConvertTo-Json | Set-Content "$out/inputs.json"
'PASS: canonical Release native SaveToDB far/logout gate, valid-destination fallback skip, retained far flag, native destination zone and actual logout save dispatch.'
