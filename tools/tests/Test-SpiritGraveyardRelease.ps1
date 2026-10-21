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

$queryOut="$repo/build-extractors/runtime-test/spirit-query-release"
New-Item -ItemType Directory -Force $queryOut | Out-Null
if(@($lines | Where-Object {$_ -match '^PASS: spirit query login '}).Count -ne 392){throw 'Run all392 native deferred login-query cases first'}
foreach($mode in 0..6){if(@($lines | Where-Object {$_ -match "^PASS: spirit query login .* mode=$mode map="}).Count -ne 56){throw "Missing deferred-query mode $mode"}}
if($inputs.'tools/tests/TrialSpiritQueryNative.inc' -ne (Get-FileHash "$repo/tools/tests/TrialSpiritQueryNative.inc").Hash){throw 'Deferred-query fixture changed after native run'}
$queryAbi=@($lines | Where-Object {$_ -match '^SPIRIT_QUERY_ABI '} | Select-Object -Unique)
if($queryAbi.Count -ne 1){throw 'Missing unique native query admission ABI'}
$queryFields=[regex]::Match($queryAbi[0],'^SPIRIT_QUERY_ABI loading=(\d+) player=(\d+)$')
if(!$queryFields.Success){throw 'Native query admission ABI changed'}
$loadingOffset=([int]$queryFields.Groups[1].Value).ToString('X')
$playerOffset=([int]$queryFields.Groups[2].Value).ToString('X')
$range=Bounds '?_Do_call@?$_Func_impl_no_alloc@V<lambda_1>@?1??HandleContinuePlayerLogin@WorldSession@@';$argument='/RANGE:0x{0:X},0x{1:X}' -f $range[0],$range[1]
$queryLines=@(& $dump.FullName /DISASM:NOBYTES $argument $exe)
if($LASTEXITCODE){throw 'Canonical deferred-query callback disassembly failed'}
$queryText=$queryLines -join "`n";$queryLines | Set-Content "$queryOut/query-callback.txt"
$queryGate=[regex]::Match($queryText,"(?im)movzx\s+eax,byte ptr \[r\w+\+10h\]\r?\n[^\r\n]*cmp\s+byte ptr \[r\w+\+0*${serialOffset}h\],al\r?\n[^\r\n]*jne\s+(?<reject>[0-9a-f]{16})")
if(!$queryGate.Success){throw 'Canonical query callback lacks captured request serial rejection'}
$tailJump=[regex]::Match($queryText,'(?im)jmp\s+([0-9a-f]{16})')
if(!$tailJump.Success -or (Resolve ([Convert]::ToInt64($tailJump.Groups[1].Value,16))) -ne (Resolve (Symbol '?HandlePlayerLogin@WorldSession@@'))){throw 'Canonical current-query callback no longer dispatches native login handler'}
$reject=[regex]::Match($queryText,'(?im)^\s*'+$queryGate.Groups['reject'].Value+':(?<body>[\s\S]*)')
if(!$reject.Success -or $reject.Groups['body'].Value -notmatch '\bret\b' -or $reject.Groups['body'].Value -match '\b(call|jmp)\s+'){throw 'Stale query rejection does not return without entering login'}

$admissionGate=[regex]::Match($handleText,"(?im)lea\s+rcx,\[(?<session>r\w+)\+0*${loadingOffset}h\]\r?\n[^\r\n]*call\s+(?<empty>[0-9a-f]{16})\r?\n[^\r\n]*test\s+al,al\r?\n[^\r\n]*jne\s+(?<reject>[0-9a-f]{16})\r?\n[^\r\n]*cmp\s+qword ptr \[\k<session>\+0*${playerOffset}h\],0\r?\n[^\r\n]*jne\s+\k<reject>\r?\n[^\r\n]*lea\s+rdx,[^\r\n]*\r?\n[^\r\n]*lea\s+rcx,\[\k<session>\+0*${loadingOffset}h\]\r?\n[^\r\n]*call\s+(?<guid>[0-9a-f]{16})\r?\n[^\r\n]*test\s+al,al\r?\n[^\r\n]*jne\s+\k<reject>")
if(!$admissionGate.Success -or (Resolve ([Convert]::ToInt64($admissionGate.Groups['empty'].Value,16))) -ne (Resolve (Symbol '?IsEmpty@ObjectGuid@@')) -or (Resolve ([Convert]::ToInt64($admissionGate.Groups['guid'].Value,16))) -ne (Resolve (Symbol '??9ObjectGuid@@'))){throw 'Canonical handler lacks loading/current-Player/expected-GUID guards'}
$construct=$handleText.Substring($admissionGate.Index+$admissionGate.Length,1500)
$constructCalls=@([regex]::Matches($construct,'call\s+([0-9a-fA-F]{16})') | ForEach-Object {Resolve ([Convert]::ToInt64($_.Groups[1].Value,16))})
if($constructCalls.Count -lt 3 -or $constructCalls[1] -ne (Resolve (Symbol '??0Player@@')) -or $constructCalls[2] -ne (Resolve (Symbol '?LoadFromDB@Player@@'))){throw 'Native admission guards no longer precede Player construction and LoadFromDB'}
$range=Bounds '?HandlePlayerLoginOpcode@WorldSession@@';$argument='/RANGE:0x{0:X},0x{1:X}' -f $range[0],$range[1]
$requestLines=@(& $dump.FullName /DISASM:NOBYTES $argument $exe)
if($LASTEXITCODE){throw 'Canonical login request disassembly failed'}
$requestText=$requestLines -join "`n";$requestLines | Set-Content "$queryOut/login-request.txt"
if(@([regex]::Matches($requestText,"inc\s+byte ptr \[r\w+\+0*${serialOffset}h\]")).Count -ne 1){throw 'Accepted login request must advance its independent serial once'}
$admissionGate.Value | Set-Content "$queryOut/preconstruction-gate.txt"
[ordered]@{worldserver=(Get-FileHash $exe).Hash;game_library=(Get-FileHash $gameLib).Hash;native_inputs=(Get-FileHash "$run/native-inputs.json").Hash;cases=392;cases_per_mode=56;serial_offset=[int]$sessionFields.Groups[1].Value;loading_guid_offset=[int]$queryFields.Groups[1].Value;current_player_offset=[int]$queryFields.Groups[2].Value;stale_query_returns_before_handler=$true;admission_before_construction=$true} | ConvertTo-Json | Set-Content "$queryOut/inputs.json"
'PASS: canonical login SQL callback rejects an older request serial, and native handler checks loading/current Player/GUID before construction;392 deferred-query cases match final-link inputs.'

$linkOut="$repo/build-extractors/runtime-test/spirit-instance-link-release"
New-Item -ItemType Directory -Force $linkOut | Out-Null
if(@($lines | Where-Object {$_ -match '^PASS: spirit instance link '}).Count -ne 392){throw 'Run all392 native instance-link cases first'}
foreach($mode in 0..6){if(@($lines | Where-Object {$_ -match "^PASS: spirit instance link .* mode=$mode map="}).Count -ne 56){throw "Missing native instance-link mode $mode"}}
foreach($path in @('tools/tests/TrialSpiritInstanceLinkNative.inc','src/server/game/World/World.cpp','src/server/game/Server/WorldSocket.cpp','src/server/game/../shared/Networking/Socket.h')){
    if($inputs.$path -ne (Get-FileHash "$repo/$path").Hash){throw "Instance-link input changed after native run: $path"}
}
$linkAbi=@($lines | Where-Object {$_ -match '^SPIRIT_LINK_ABI exit=\d+$'} | Select-Object -Unique)
if($linkAbi.Count -ne 1){throw 'Missing unique native teardown ABI'}
$exitOffset=([int]$linkAbi[0].Split('=')[1]).ToString('X')
$range=Bounds '?ProcessLinkInstanceSocket@World@@';$argument='/RANGE:0x{0:X},0x{1:X}' -f $range[0],$range[1]
$linkLines=@(& $dump.FullName /DISASM:NOBYTES $argument $exe)
if($LASTEXITCODE){throw 'Canonical instance-link disassembly failed'}
$linkText=$linkLines -join "`n";$linkLines | Set-Content "$linkOut/ProcessLinkInstanceSocket.txt"
$exitGate=[regex]::Match($linkText,"(?im)cmp\s+byte ptr \[r\w+\+0*${exitOffset}h\],0\r?\n[^\r\n]*jne\s+(?<reject>[0-9a-f]{16})")
if(!$exitGate.Success){throw 'Canonical instance link lacks native session teardown rejection'}
$linkCalls=@([regex]::Matches($linkText,'call\s+([0-9a-fA-F]{16})') | ForEach-Object {[pscustomobject]@{Index=$_.Index;Target=(Resolve ([Convert]::ToInt64($_.Groups[1].Value,16)))}})
$associate=@($linkCalls | Where-Object Target -eq (Resolve (Symbol '?SetWorldSession@WorldSocket@@')))
$continue=@($linkCalls | Where-Object Target -eq (Resolve (Symbol '?HandleContinuePlayerLogin@WorldSession@@')))
if($associate.Count -ne 1 -or $continue.Count -ne 1 -or $exitGate.Index -ge $associate[0].Index -or $associate[0].Index -ge $continue[0].Index){throw 'Session teardown rejection must precede socket association and native login SQL dispatch'}
$rejection=[regex]::Match($linkText,'(?im)^\s*'+$exitGate.Groups['reject'].Value+':(?<body>[\s\S]*)')
if(!$rejection.Success){throw 'Missing native teardown rejection target'}
$rejectCalls=@([regex]::Matches($rejection.Groups['body'].Value,'call\s+([0-9a-fA-F]{16})') | ForEach-Object {Resolve ([Convert]::ToInt64($_.Groups[1].Value,16))})
if(!$rejectCalls.Count -or $rejectCalls[0] -ne (Resolve (Symbol '?SendAuthResponseError@WorldSocket@@')) -or $rejectCalls -contains (Resolve (Symbol '?SetWorldSession@WorldSocket@@')) -or $rejectCalls -contains (Resolve (Symbol '?HandleContinuePlayerLogin@WorldSession@@'))){throw 'Native teardown branch does not reject only the incoming socket'}
$exitGate.Value | Set-Content "$linkOut/teardown-gate.txt"
[ordered]@{worldserver=(Get-FileHash $exe).Hash;game_library=(Get-FileHash $gameLib).Hash;native_inputs=(Get-FileHash "$run/native-inputs.json").Hash;cases=392;cases_per_mode=56;teardown_offset=[int]$linkAbi[0].Split('=')[1];rejection_before_socket_association_and_sql=$true} | ConvertTo-Json | Set-Content "$linkOut/inputs.json"
'PASS: canonical instance-link admission rejects a session already closing before socket association or login SQL;392 real loopback link/retry/cancel/EOF cases match final-link inputs.'

$cancelOut="$repo/build-extractors/runtime-test/spirit-linked-cancel-release"
New-Item -ItemType Directory -Force $cancelOut | Out-Null
if(@($lines | Where-Object {$_ -match '^PASS: spirit linked cancel '}).Count -ne 392 -or @($lines | Where-Object {$_ -match '^PASS: native socket owner release kind=[01]$'}).Count -ne 2){throw 'Run392 linked-cancellation cases and both native last-owner probes first'}
foreach($mode in 0..6){if(@($lines | Where-Object {$_ -match "^PASS: spirit linked cancel .* mode=$mode map="}).Count -ne 56){throw "Missing linked-cancellation mode $mode"}}
foreach($path in @('tools/tests/TrialSpiritLinkedCancelNative.inc','tools/tests/TrialSpiritQueryNative.inc','src/server/game/Handlers/CharacterHandler.cpp','src/server/game/Server/WorldSocket.cpp','src/server/game/Server/WorldSession.cpp')){
    if($inputs.$path -ne (Get-FileHash "$repo/$path").Hash){throw "Linked-cancellation input changed after native run: $path"}
}
$cancelAbi=@($lines | Where-Object {$_ -match '^SPIRIT_CANCEL_ABI '} | Select-Object -Unique)
if($cancelAbi.Count -ne 1){throw 'Missing unique native cancellation ABI'}
$cancelFields=[regex]::Match($cancelAbi[0],'^SPIRIT_CANCEL_ABI instance=(\d+) authed=(\d+) account=(\d+)$')
if(!$cancelFields.Success){throw 'Native cancellation ABI changed'}
$instanceOffset=([int]$cancelFields.Groups[1].Value).ToString('X');$instanceOwnerOffset=([int]$cancelFields.Groups[1].Value+8).ToString('X')
$authOffset=([int]$cancelFields.Groups[2].Value).ToString('X');$accountOffset=([int]$cancelFields.Groups[3].Value).ToString('X')
$cancelBodies=@{}
foreach($entry in @(@('?AbortLogin@WorldSession@@','AbortLogin'),@('?LogoutPlayer@WorldSession@@','LogoutPlayer'),@('?SetWorldSession@WorldSocket@@','SetWorldSession'),@('?OnClose@WorldSocket@@','OnClose'))){
    $range=Bounds $entry[0];$argument='/RANGE:0x{0:X},0x{1:X}' -f $range[0],$range[1]
    $body=@(& $dump.FullName /DISASM:NOBYTES $argument $exe);if($LASTEXITCODE){throw "Canonical cancellation disassembly failed: $($entry[1])"}
    $body | Set-Content "$cancelOut/$($entry[1]).txt";$cancelBodies[$entry[1]]=$body -join "`n"
}
function CancelCalls([string]$body){@([regex]::Matches($body,'call\s+([0-9a-fA-F]{16})') | ForEach-Object {[pscustomobject]@{Index=$_.Index;Target=(Resolve ([Convert]::ToInt64($_.Groups[1].Value,16)))}})}
$detach=Resolve (Symbol '?SetWorldSession@WorldSocket@@');$abortSocket=Resolve (Symbol '?AbortSocket@?$Socket@VWorldSocket@@');$closeSocket=Resolve (Symbol '?CloseSocket@?$Socket@VWorldSocket@@')
foreach($entry in @(@('AbortLogin',$abortSocket),@('LogoutPlayer',$closeSocket))){
    $calls=@(CancelCalls $cancelBodies[$entry[0]]);$detachCalls=@($calls | Where-Object Target -eq $detach);$closeCalls=@($calls | Where-Object Target -eq $entry[1])
    if($detachCalls.Count -ne 1 -or $closeCalls.Count -ne 1 -or $detachCalls[0].Index -ge $closeCalls[0].Index){throw "Canonical $($entry[0]) does not detach before closing its instance"}
    $before=$cancelBodies[$entry[0]].Substring([Math]::Max(0,$detachCalls[0].Index-350),[Math]::Min(350,$detachCalls[0].Index))
    if($before -notmatch 'xorps\s+xmm0,xmm0[\s\S]*movdqu\s+xmmword ptr \[rsp\+[0-9a-f]+h\],xmm0[\s\S]*lea\s+rdx,\[rsp\+[0-9a-f]+h\]'){throw "Canonical $($entry[0]) no longer passes an empty session owner"}
}
if($cancelBodies.AbortLogin -notmatch "xor\s+eax,eax\r?\n[^\r\n]*mov\s+qword ptr \[(?<session>r\w+)\+0*${instanceOffset}h\],rax\r?\n[^\r\n]*mov\s+qword ptr \[\k<session>\+0*${instanceOwnerOffset}h\],rax"){throw 'Canonical cancellation does not clear the instance slot before retirement'}
if($cancelBodies.SetWorldSession -notmatch "xor\s+eax,eax\r?\n[^\r\n]*mov\s+dword ptr \[r\w+\+0*${accountOffset}h\],eax" -or $cancelBodies.SetWorldSession -notmatch "setne\s+al\r?\n[^\r\n]*mov\s+byte ptr \[r\w+\+0*${authOffset}h\],al"){throw 'Canonical socket detach does not clear nullable account/authentication state'}
$unlock=Resolve (Symbol '_Mtx_unlock')
foreach($name in @('SetWorldSession','OnClose')){
    $unlocks=@(CancelCalls $cancelBodies[$name] | Where-Object Target -eq $unlock);$decrement=[regex]::Match($cancelBodies[$name],'lock xadd\s+dword ptr')
    if($unlocks.Count -ne 1 -or !$decrement.Success -or $unlocks[0].Index -ge $decrement.Index){throw "Canonical $name releases its last session owner before unlocking"}
}
[ordered]@{worldserver=(Get-FileHash $exe).Hash;game_library=(Get-FileHash $gameLib).Hash;native_inputs=(Get-FileHash "$run/native-inputs.json").Hash;cases=392;owner_probes=2;instance_offset=[int]$cancelFields.Groups[1].Value;auth_offset=[int]$cancelFields.Groups[2].Value;account_offset=[int]$cancelFields.Groups[3].Value;detach_before_abort_and_logout=$true;last_owner_release_after_unlock=$true} | ConvertTo-Json | Set-Content "$cancelOut/inputs.json"
'PASS: canonical AbortLogin/logout detach their instance before closure, nullable socket ownership clears auth/account state and final owners release after unlocking;392 native linked-cancellation/relogin cases and2 ownership probes match final-link inputs.'

$earlyOut="$repo/build-extractors/runtime-test/spirit-early-loading-release"
New-Item -ItemType Directory -Force $earlyOut | Out-Null
if(@($lines | Where-Object {$_ -match '^PASS: spirit early loading '}).Count -ne 392){throw 'Run all392 native early-loading cases first'}
foreach($mode in 0..6){if(@($lines | Where-Object {$_ -match "^PASS: spirit early loading .* mode=$mode map="}).Count -ne 56){throw "Missing native early-loading mode $mode"}}
foreach($path in @('tools/tests/TrialSpiritEarlyLoadingNative.inc','tools/tests/TrialSpiritQueryNative.inc','src/server/game/Handlers/CharacterHandler.cpp','src/server/game/Server/Packets/CharacterPackets.cpp')){
    if($inputs.$path -ne (Get-FileHash "$repo/$path").Hash){throw "Early-loading input changed after native run: $path"}
}
$range=Bounds '?HandleLoadScreenOpcode@WorldSession@@';$argument='/RANGE:0x{0:X},0x{1:X}' -f $range[0],$range[1]
$earlyLines=@(& $dump.FullName /DISASM:NOBYTES $argument $exe);if($LASTEXITCODE){throw 'Canonical loading-notification disassembly failed'}
$earlyText=$earlyLines -join "`n";$earlyLines | Set-Content "$earlyOut/HandleLoadScreenOpcode.txt"
$earlyGate=[regex]::Match($earlyText,"(?im)mov\s+(?<player>r\w+),qword ptr \[r\w+\+0*${playerOffset}h\][\s\S]{0,250}?test\s+\k<player>,\k<player>\r?\n[^\r\n]*je\s+(?<reject>[0-9a-f]{16})")
if(!$earlyGate.Success){throw 'Canonical loading notification lacks early null-Player rejection'}
$nullReturn=[regex]::Match($earlyText,'(?im)^\s*'+$earlyGate.Groups['reject'].Value+':(?<body>[\s\S]*?\bret\b)')
if(!$nullReturn.Success -or $nullReturn.Groups['body'].Value -match '\b(call|jmp)\s+|\bmov\s+(byte|word|dword|qword|xmmword) ptr \['){throw 'Null-Player notification path does not return without calls or state writes'}
$earlyOps=@([regex]::Matches($earlyText,'(?:call|jmp)\s+([0-9a-fA-F]{16})') | ForEach-Object {[pscustomobject]@{Index=$_.Index;Target=(Resolve ([Convert]::ToInt64($_.Groups[1].Value,16)))}})
$expectedEarly=@('?NotifyMapObjectArrivalLoading@Player@@','?SendInitialPacketsAfterAddToMap@Player@@','?NotifyMapObjectArrivalLoaded@Player@@','?Clear@ObjectGuid@@')
if($earlyOps.Count -ne 4){throw 'Review canonical loading notification dispatch'}
for($i=0;$i -lt 4;++$i){if($earlyOps[$i].Target -ne (Resolve (Symbol $expectedEarly[$i])) -or $earlyOps[$i].Index -le $earlyGate.Index){throw 'Canonical loading notification lost its guarded Player/arrival/loading-state operations'}}
if($earlyText -notmatch "lea\s+rcx,\[r\w+\+0*${loadingOffset}h\]"){throw 'Canonical valid loading completion no longer addresses the pending login GUID'}
$logoutLoadingClear=[regex]::Match($cancelBodies.LogoutPlayer,"(?im)lea\s+rcx,\[r\w+\+0*${loadingOffset}h\]\r?\n[^\r\n]*call\s+([0-9a-f]{16})")
if(!$logoutLoadingClear.Success -or (Resolve ([Convert]::ToInt64($logoutLoadingClear.Groups[1].Value,16))) -ne (Resolve (Symbol '?Clear@ObjectGuid@@'))){throw 'Canonical logout does not clear its own pending login GUID'}
[ordered]@{worldserver=(Get-FileHash $exe).Hash;game_library=(Get-FileHash $gameLib).Hash;native_inputs=(Get-FileHash "$run/native-inputs.json").Hash;cases=392;cases_per_mode=56;player_offset=[int]$queryFields.Groups[2].Value;pending_guid_offset=[int]$queryFields.Groups[1].Value;null_player_returns_without_state_changes=$true;constructed_player_notification_flow_retained=$true;logout_clears_pending_request=$true} | ConvertTo-Json | Set-Content "$earlyOut/inputs.json"
'PASS: canonical loading notification returns without state changes before Player creation and retains constructed-Player arrival/completion handling;392 native early-notification/login/retry cases match final-link inputs.'
