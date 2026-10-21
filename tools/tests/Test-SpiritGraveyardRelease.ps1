$ErrorActionPreference='Stop'
. "$PSScriptRoot/Test-LogoutTeleportRelease.ps1"
$out="$repo/build-extractors/runtime-test/spirit-graveyard-release"
New-Item -ItemType Directory -Force $out | Out-Null
$run=(Get-Content "$repo/build-extractors/runtime-test/trial-helya-control-native/latest-run.txt").Trim()
$inputs=Get-Content "$run/native-inputs.json" -Raw | ConvertFrom-Json
if($inputs.'canonical-worldserver.exe' -ne (Get-FileHash $exe).Hash -or $inputs.'actual-final-link-game.lib' -ne (Get-FileHash $gameLib).Hash -or $inputs.'src/server/game/Entities/Player/Player.cpp' -ne (Get-FileHash "$repo/src/server/game/Entities/Player/Player.cpp").Hash){throw 'Native spirit graveyard run and canonical source/library/executable differ'}
$lines=Get-Content "$run/native-lines.txt"
$abiNear=@($lines | Where-Object {$_ -match '^SPIRIT_GRAVE_ABI near=\d+$'} | Select-Object -Unique)
if($abiNear.Count -ne 1 -or @($lines | Where-Object {$_ -match '^PASS: spirit graveyard transfer/save/logout '}).Count -ne 24 -or @($lines | Where-Object {$_ -match '^PASS: spirit graveyard logout saved-field/prepared-aura relog '}).Count -ne 16){throw 'Run all24 spirit transfer cases and16 relog components first'}
$near=([int]($abiNear[0].Split('=')[1])).ToString('X')
$far=([int]$abi.Groups[1].Value).ToString('X')
$listing=@(Get-Content "$repo/build-extractors/runtime-test/logout-teleport-release/SaveToDB-Player.txt")
$zone=Resolve (Symbol '?GetZoneId@MapManager@@')
$indexes=@(for($i=0;$i -lt $listing.Count;$i++){$m=[regex]::Match($listing[$i],'call\s+([0-9A-Fa-f]{16})');if($m.Success -and (Resolve ([Convert]::ToInt64($m.Groups[1].Value,16))) -eq $zone){$i}})
if($indexes.Count -ne 1){throw 'Missing unique native destination-zone resolver'}
$i=$indexes[0];$around=$listing[($i-28)..($i+5)] -join "`n"
$gate=[regex]::Match($around,"cmp\s+byte ptr \[(?<player>r\w+)\+0*${near}h\],(?<zero>r\w+b)\r?\n[^\r\n]*jne\s+(?<resolve>[0-9A-Fa-f]{16})\r?\n[^\r\n]*cmp\s+byte ptr \[\k<player>\+0*${far}h\],\k<zero>\r?\n[^\r\n]*je\s+(?<skip>[0-9A-Fa-f]{16})\r?\n\s*\k<resolve>:")
if(!$gate.Success){throw 'Review canonical near-or-far destination-zone code generation'}
$callAddress=[Convert]::ToInt64(([regex]::Match($listing[$i],'^\s*([0-9A-Fa-f]{16}):')).Groups[1].Value,16)
if([Convert]::ToInt64($gate.Groups['resolve'].Value,16) -ge $callAddress -or [Convert]::ToInt64($gate.Groups['skip'].Value,16) -le $callAddress){throw 'Near/far gate does not enclose native zone resolution'}
$zeroReg=$gate.Groups['zero'].Value -replace 'b$','d'
if(($listing -join "`n") -notmatch "xor\s+${zeroReg},${zeroReg}"){throw 'Teleport comparison register is not zero-initialized'}
$around | Set-Content "$out/near-far-zone-gate.txt"
[ordered]@{worldserver=(Get-FileHash $exe).Hash;game_library=(Get-FileHash $gameLib).Hash;native_inputs=(Get-FileHash "$run/native-inputs.json").Hash;near_offset=[int]($abiNear[0].Split('=')[1]);far_offset=[int]$abi.Groups[1].Value;cases=24;relog_components=16;near_and_far_resolve_destination_zone=$true} | ConvertTo-Json | Set-Content "$out/inputs.json"
'PASS: canonical Release resolves saved destination zone for native near OR far transfers;24 spirit graveyard cases/16 relog components match final-link inputs.'
