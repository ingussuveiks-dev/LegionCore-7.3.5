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
if(@($lines | Where-Object {$_ -match '^PASS: spirit far transfer/save/logout '}).Count -ne 48 -or @($lines | Where-Object {$_ -match '^PASS: spirit far saved-field/prepared-aura relog '}).Count -ne 32){throw 'Run all48 cross-continent spirit cases and32 relog components first'}
if(@($lines | Where-Object {$_ -match '^PASS: spirit failure recovery '}).Count -ne 56){throw 'Run all56 spirit refusal/transport-loss recovery cases first'}
if(@($lines | Where-Object {$_ -match '^PASS: spirit session login '}).Count -ne 280){throw 'Run all280 complete session login/callback/notification cases first'}
foreach($mode in 0..4){if(@($lines | Where-Object {$_ -match "^PASS: spirit session login .* mode=$mode map="}).Count -ne 56){throw "Missing complete session-login mode $mode"}}
foreach($path in @('tools/tests/TrialSpiritSessionNative.inc','src/server/game/Handlers/CharacterHandler.cpp','src/server/game/Server/WorldSession.h','src/server/game/Server/WorldSession.cpp','src/server/game/Server/Packets/CharacterPackets.cpp')){
    if($inputs.$path -ne (Get-FileHash "$repo/$path").Hash){throw "Session login input changed after native run: $path"}
}
foreach($path in @('TrialSpiritFailureNative.inc','TrialHelyaControlNative.cpp')){
    if($inputs."tools/tests/$path" -ne (Get-FileHash "$repo/tools/tests/$path").Hash){throw "Spirit failure fixture changed after native run: $path"}
}
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
[ordered]@{worldserver=(Get-FileHash $exe).Hash;game_library=(Get-FileHash $gameLib).Hash;native_inputs=(Get-FileHash "$run/native-inputs.json").Hash;near_offset=[int]($abiNear[0].Split('=')[1]);far_offset=[int]$abi.Groups[1].Value;cases=24;relog_components=16;far_cases=48;far_relog_components=32;failure_cases=56;session_login_cases=280;near_and_far_resolve_destination_zone=$true} | ConvertTo-Json | Set-Content "$out/inputs.json"
'PASS: canonical Release resolves saved destination zone for native near OR far transfers;24 near/48 cross-continent cases,56 refusal/transport-loss cases and16/32 relog components match final-link inputs.'

$loginOut="$repo/build-extractors/runtime-test/spirit-login-release"
New-Item -ItemType Directory -Force $loginOut | Out-Null
if(@($lines | Where-Object {$_ -match '^PASS: spirit aggregate login '}).Count -ne 168){throw 'Run all168 aggregate LoadFromDB/offline recovery cases first'}
if($inputs.'tools/tests/TrialSpiritLoginNative.inc' -ne (Get-FileHash "$repo/tools/tests/TrialSpiritLoginNative.inc").Hash){throw 'Aggregate login fixture changed after native run'}
$loginAbi=@($lines | Where-Object {$_ -match '^SPIRIT_LOGIN_ABI '} | Select-Object -Unique)
if($loginAbi.Count -ne 1){throw 'Missing unique native login ABI'}
$loginFields=[regex]::Match($loginAbi[0],'^SPIRIT_LOGIN_ABI stats=(\d+) stamina=(\d+) mask=(\d+)$')
if(!$loginFields.Success){throw 'Native login ABI changed'}
$statsOffset=([int]$loginFields.Groups[1].Value).ToString('X')
$stamina=[int]$loginFields.Groups[2].Value
$clear=([uint32]::MaxValue -bxor [uint32]$loginFields.Groups[3].Value).ToString('X')
$range=Bounds '?LoadFromDB@Player@@';$argument='/RANGE:0x{0:X},0x{1:X}' -f $range[0],$range[1]
$loadLines=@(& $dump.FullName /DISASM:NOBYTES $argument $exe)
if($LASTEXITCODE){throw 'Canonical login disassembly failed'}
$loadText=$loadLines -join "`n";$loadLines | Set-Content "$loginOut/LoadFromDB-Player.txt"
$healthGate=[regex]::Match($loadText,"(?im)mov\s+rax,qword ptr \[(?<player>r\w+)\]\r?\n[^\r\n]*mov\s+edx,$stamina\r?\n[^\r\n]*mov\s+rcx,\k<player>\r?\n[^\r\n]*call\s+qword ptr \[rax\+0*(?<slot>[0-9a-f]+)h\]\r?\n[^\r\n]*and\s+dword ptr \[\k<player>\+0*${statsOffset}h\],0*${clear}h")
if(!$healthGate.Success){throw 'Canonical login lacks synchronous stamina resolution and consumed deferred stamina bit'}
$vtable=[regex]::Match($map,'(?m)\?\?_7Player@@6BUnit@@@\s+([0-9a-fA-F]{16})')
if(!$vtable.Success){throw 'Missing native Player Unit vtable'}
$vtableAddress=[Convert]::ToInt64($vtable.Groups[1].Value,16)
$slot=[Convert]::ToInt32($healthGate.Groups['slot'].Value,16)
$target=Resolve ([BitConverter]::ToInt64($binary,(Offset ($vtableAddress+$slot))))
if($target -ne (Resolve (Symbol '?UpdateStats@Player@@'))){throw 'Canonical login stamina call does not dispatch native Player::UpdateStats'}
$before=$loadText.Substring([Math]::Max(0,$healthGate.Index-250),[Math]::Min(250,$healthGate.Index))
$virtualCalls=[regex]::Matches($before,'call\s+qword ptr \[rax\+([0-9a-fA-F]+)h\]')
if(!$virtualCalls.Count){throw 'Missing preceding native stat scheduling call'}
$priorSlot=[Convert]::ToInt32($virtualCalls[$virtualCalls.Count-1].Groups[1].Value,16)
if((Resolve ([BitConverter]::ToInt64($binary,(Offset ($vtableAddress+$priorSlot))))) -ne (Resolve (Symbol '?UpdateAllStats@Player@@'))){throw 'Stamina resolution does not follow native UpdateAllStats'}
$after=$loadText.Substring($healthGate.Index+$healthGate.Length,[Math]::Min(1500,$loadText.Length-$healthGate.Index-$healthGate.Length))
$healthCalls=@([regex]::Matches($after,'call\s+([0-9a-fA-F]{16})') | ForEach-Object {Resolve ([Convert]::ToInt64($_.Groups[1].Value,16))})
$maxHealth=Resolve (Symbol '?GetMaxHealth@Unit@@QEBA_KXZ')
$setHealth=Resolve (Symbol '?SetHealth@Unit@@')
if($healthCalls.Count -lt 4 -or $healthCalls[1] -ne $maxHealth -or $healthCalls[2] -ne $maxHealth -or $healthCalls[3] -ne $setHealth -or $after -notmatch 'cmp\s+rbx,rax[\s\S]*jbe\s+'){throw 'Canonical saved-health maximum clamp no longer follows stamina resolution'}
($healthGate.Value+"`n"+$after) | Set-Content "$loginOut/saved-health-gate.txt"
[ordered]@{worldserver=(Get-FileHash $exe).Hash;game_library=(Get-FileHash $gameLib).Hash;native_inputs=(Get-FileHash "$run/native-inputs.json").Hash;aggregate_cases=168;stats_offset=[int]$loginFields.Groups[1].Value;stamina=$stamina;stamina_mask=[uint32]$loginFields.Groups[3].Value;virtual_update_stats_slot=$slot;saved_health_clamp=$true} | ConvertTo-Json | Set-Content "$loginOut/inputs.json"
'PASS: canonical LoadFromDB resolves native stamina and clears its deferred bit before the saved-health clamp;168 full login/offline recovery cases match final-link inputs.'

$sessionOut="$repo/build-extractors/runtime-test/spirit-session-release"
New-Item -ItemType Directory -Force $sessionOut | Out-Null
$sessionAbi=@($lines | Where-Object {$_ -match '^SPIRIT_SESSION_ABI '} | Select-Object -Unique)
if($sessionAbi.Count -ne 1){throw 'Missing unique session completion ABI'}
$sessionFields=[regex]::Match($sessionAbi[0],'^SPIRIT_SESSION_ABI serial=(\d+) flood=(\d+)$')
if(!$sessionFields.Success -or $sessionFields.Groups[1].Value -eq $sessionFields.Groups[2].Value){throw 'Login completion serial must be separate from the flood counter'}
$serialOffset=([int]$sessionFields.Groups[1].Value).ToString('X')
$range=Bounds '??R<lambda_1>@?1??HandlePlayerLogin@WorldSession@@';$argument='/RANGE:0x{0:X},0x{1:X}' -f $range[0],$range[1]
$callbackLines=@(& $dump.FullName /DISASM:NOBYTES $argument $exe)
if($LASTEXITCODE){throw 'Canonical login callback disassembly failed'}
$callbackText=$callbackLines -join "`n";$callbackLines | Set-Content "$sessionOut/login-callback.txt"
$completionGate=[regex]::Match($callbackText,"(?im)test\s+(?<player>r\w+),\k<player>\r?\n[^\r\n]*je\s+(?<reject>[0-9a-f]{16})\r?\n[^\r\n]*movzx\s+eax,byte ptr \[(?<capture>r\w+)\+8\]\r?\n[^\r\n]*cmp\s+byte ptr \[r\w+\+0*${serialOffset}h\],al\r?\n[^\r\n]*jne\s+\k<reject>")
if(!$completionGate.Success){throw 'Canonical login completion lacks null/current-serial admission before initialization'}
$post=$callbackText.Substring($completionGate.Index+$completionGate.Length,[Math]::Min(900,$callbackText.Length-$completionGate.Index-$completionGate.Length))
$postCalls=@([regex]::Matches($post,'call\s+([0-9a-fA-F]{16})') | ForEach-Object {Resolve ([Convert]::ToInt64($_.Groups[1].Value,16))})
if($postCalls.Count -lt 3 -or $postCalls[0] -ne (Resolve (Symbol '?GetGuidValue@Object@@')) -or $postCalls[1] -ne (Resolve (Symbol '??9ObjectGuid@@')) -or $postCalls[2] -ne (Resolve (Symbol '?SendInitialPacketsBeforeAddToMap@Player@@')) -or $post -notmatch ("test\s+al,al[\s\S]*jne\s+"+$completionGate.Groups['reject'].Value)){throw 'Canonical callback does not reject a different current Player GUID before initial packets'}
$range=Bounds '?HandlePlayerLogin@WorldSession@@';$argument='/RANGE:0x{0:X},0x{1:X}' -f $range[0],$range[1]
$handleLines=@(& $dump.FullName /DISASM:NOBYTES $argument $exe)
if($LASTEXITCODE){throw 'Canonical session login disassembly failed'}
$handleText=$handleLines -join "`n";$handleLines | Set-Content "$sessionOut/HandlePlayerLogin.txt"
if($handleText -notmatch "inc\s+byte ptr \[r\w+\+0*${serialOffset}h\][\s\S]*movzx\s+eax,byte ptr \[r\w+\+0*${serialOffset}h\]"){throw 'Canonical login does not establish its completion serial'}
($completionGate.Value+"`n"+$post) | Set-Content "$sessionOut/completion-gate.txt"
[ordered]@{worldserver=(Get-FileHash $exe).Hash;game_library=(Get-FileHash $gameLib).Hash;native_inputs=(Get-FileHash "$run/native-inputs.json").Hash;cases=280;replacement_cases=56;completion_serial_offset=[int]$sessionFields.Groups[1].Value;flood_counter_offset=[int]$sessionFields.Groups[2].Value} | ConvertTo-Json | Set-Content "$sessionOut/inputs.json"
'PASS: canonical delayed login callback checks its own completion serial and current GUID before initial packets;280 session cases include56 same-GUID replacement logins.'
