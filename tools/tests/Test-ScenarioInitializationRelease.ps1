$ErrorActionPreference='Stop'
. "$PSScriptRoot/Test-RockawayRelease.ps1"
$out="$repo/build-extractors/runtime-test/scenario-initialization-release"
New-Item -ItemType Directory -Force $out | Out-Null
$relative=$release.Link.AdditionalDependencies.Split(';') | Where-Object {$_ -match 'game\\Release\\game.lib$'} | Select-Object -First 1
if(!$relative){throw 'Missing actual final-link game library'}
$gameLib=[IO.Path]::GetFullPath((Join-Path "$repo/build-extractors/src/server/worldserver" $relative))
$run=(Get-Content "$repo/build-extractors/runtime-test/trial-helya-control-native/latest-run.txt").Trim()
$inputs=Get-Content "$run/native-inputs.json" -Raw | ConvertFrom-Json
foreach($entry in @(@('canonical-worldserver.exe',$exe),@('actual-final-link-game.lib',$gameLib),@('src/server/game/Scenarios/Scenario.cpp',"$repo/src/server/game/Scenarios/Scenario.cpp"),@('tools/tests/TrialScenarioInitializationNative.inc',"$PSScriptRoot/TrialScenarioInitializationNative.inc"),@('tools/tests/Test-ScenarioInitializationRelease.ps1',$PSCommandPath))){if($inputs.($entry[0]) -ne (Get-FileHash $entry[1]).Hash){throw "Scenario native input changed: $($entry[0])"}}
$lines=Get-Content "$run/native-lines.txt"
$cases=@($lines | Where-Object {$_ -match '^PASS: native scenario initialization '})
if($cases.Count -ne 48){throw 'Run all48 native scenario initialization cases first'}
foreach($mode in 0..2){if(@($cases | Where-Object {$_ -match " mode=$mode scenario="}).Count -ne 16){throw "Missing scenario arrival/re-entry mode $mode"}}
foreach($id in 786,1189){if(@($cases | Where-Object {$_ -match " scenario=$id$"}).Count -ne 24){throw "Missing faction scenario $id"}}
$abi=@($lines | Where-Object {$_ -match '^SCENARIO_INITIALIZATION_ABI '} | Select-Object -Unique)
if($abi.Count -ne 1 -or $abi[0] -notmatch '^SCENARIO_INITIALIZATION_ABI node_date=(\d+) packet_date=(\d+)$'){throw 'Missing native scenario timestamp ABI'}
$nodeDate=[int]$Matches[1];$packetDate=[int]$Matches[2]
$range=Bounds '?SendStepUpdate@Scenario@@';$argument='/RANGE:0x{0:X},0x{1:X}' -f $range[0],$range[1]
$listing=@(& $dump.FullName /DISASM:NOBYTES $argument $exe);if($LASTEXITCODE){throw 'Scenario snapshot disassembly failed'}
$text=$listing -join "`n"
# The MSVC unordered_map iterator points at its list node; derive the value
# offset from the emitted criterion-ID lookup before applying the native ABI.
$id=[regex]::Match($text,'(?i)mov\s+eax,dword ptr \[(?<node>r\w+)\+(?<offset>[0-9a-f]+)h\]\r?\n[^\r\n]*cmp\s+eax,dword ptr \[[0-9a-f]+h\]')
if(!$id.Success){throw 'Review scenario map-node lookup code generation'}
$source=([Convert]::ToInt32($id.Groups['offset'].Value,16)+$nodeDate).ToString('X')
$copy=[regex]::Match($text,"(?i)mov\s+(?<date>r\w+),qword ptr \[$($id.Groups['node'].Value)\+0*${source}h\]\r?\n[^\r\n]*mov\s+qword ptr \[rsp\+(?<dest>[0-9a-f]+)h\],\k<date>\r?\n[^\r\n]*call\s+(?<clock>[0-9a-f]{16})")
if(!$copy.Success -or (Resolve ([Convert]::ToInt64($copy.Groups['clock'].Value,16))) -ne (Resolve (Symbol '?GetGameTime@GameTime@@'))){throw 'Release does not copy the absolute date before calculating elapsed time'}
$base=([Convert]::ToInt32($copy.Groups['dest'].Value,16)-$packetDate).ToString('X')
if($text -notmatch "lea\s+r8,\[rsp\+0*${base}h\]"){throw 'Date copy does not address the actual progress-vector element'}
$tail=$text.Substring($copy.Index+$copy.Length,550)
if([regex]::Matches($tail,'sub\s+eax,edi').Count -ne 2 -or $tail -notmatch 'mov\s+dword ptr \[rsp\+60h\],eax' -or $tail -notmatch 'mov\s+dword ptr \[rsp\+64h\],eax'){throw 'Review retained elapsed-time fields in canonical snapshot'}
$listing | Set-Content "$out/SendStepUpdate-Scenario.txt"
[ordered]@{worldserver=(Get-FileHash $exe).Hash;game_library=(Get-FileHash $gameLib).Hash;native_inputs=(Get-FileHash "$run/native-inputs.json").Hash;cases=$cases.Count;node_date_offset=$nodeDate;packet_date_offset=$packetDate;absolute_date_copy=$true;elapsed_fields_preserved=$true} | ConvertTo-Json | Set-Content "$out/inputs.json"
'PASS: canonical Scenario snapshot copies the native absolute timestamp and preserves elapsed-time fields;48 actual serialized arrival/re-entry cases cover both faction scenarios.'

$relogOut="$repo/build-extractors/runtime-test/scenario-relog-release"
New-Item -ItemType Directory -Force $relogOut | Out-Null
foreach($path in @('src/server/game/Entities/Player/PlayerScenario.cpp','tools/tests/TrialScenarioRelogNative.inc','tools/tests/ScenarioResumeTest.cpp','tools/tests/Test-ScenarioResume.ps1')){if($inputs.$path -ne (Get-FileHash "$repo/$path").Hash){throw "Scenario relog input changed: $path"}}
$relogs=@($lines | Where-Object {$_ -match '^PASS: native scenario relog '})
if($relogs.Count -ne 64){throw 'Run all64 live/old-boot scenario relog cases first'}
foreach($id in 786,1189){foreach($early in 0..1){foreach($restart in 0..1){if(@($relogs | Where-Object {$_ -match " scenario=$id early=$early restart=$restart instance="}).Count -ne 8){throw "Missing faction/arrival/boot relog combination $id/$early/$restart"}}}}
foreach($name in @('?PrepareScenarioLogin@Player@@','?RecordScenarioEntrance@Player@@')){if(@(Calls $name) -notcontains (Resolve (Symbol '?ResumeDungeon@'))){throw "Canonical $name bypasses the shared route resolver"}}
$routeCalls=@(Calls '?ResumeDungeon@')
foreach($name in @('?GetLFGDungeon@LFGMgr@lfg@@QEAAPEBULFGDungeonData@2@GG@Z','?GetDefaultMapDifficulty@DB2Manager@@','?GetLFGDungeon@LFGMgr@lfg@@QEAAPEBULFGDungeonData@2@IW4Difficulty@@I@Z','?GetScenarioOnMap@ObjectMgr@@')){if($routeCalls -notcontains (Resolve (Symbol $name))){throw "Canonical resume route lacks $name"}}
$range=Bounds '?ResumeDungeon@';$argument='/RANGE:0x{0:X},0x{1:X}' -f $range[0],$range[1]
$routeListing=@(& $dump.FullName /DISASM:NOBYTES $argument $exe);if($LASTEXITCODE){throw 'Resume route disassembly failed'}
$routeText=$routeListing -join "`n"
$abi=@($lines | Where-Object {$_ -match '^SCENARIO_RELOG_ABI '} | Select-Object -Unique)
if($abi.Count -ne 1 -or $abi[0] -notmatch '^SCENARIO_RELOG_ABI team=(\d+) class=(\d+) difficulty=(\d+) dungeon=(\d+)$'){throw 'Missing native scenario-route ABI'}
$team=([int]$Matches[1]).ToString('X');$class=([int]$Matches[2]).ToString('X');$difficulty=([int]$Matches[3]).ToString('X');$dungeon=([int]$Matches[4]).ToString('X')
foreach($offset in @($team,$class,$difficulty,$dungeon)){if($routeText -notmatch "dword ptr \[r\w+\+0*${offset}h?\]"){throw "Canonical route lacks native condition field $offset"}}
$gate=[regex]::Match($routeText,'(?i)test\s+rax,rax\r?\n[^\r\n]*je\s+(?<reject>[0-9a-f]{16})\r?\n[^\r\n]*cmp\s+dword ptr \[rax\],r15d\r?\n[^\r\n]*jne\s+\k<reject>')
if(!$gate.Success -or [regex]::Matches($routeText,("(?i)jne\s+"+$gate.Groups['reject'].Value)).Count -ne 4 -or $routeText -notmatch 'cmove\s+rax,rsi'){throw 'Review native exact scenario, faction/class/difficulty rejection and dungeon conditional return'}
$routeListing | Set-Content "$relogOut/ResumeDungeon-verified.txt"
[ordered]@{worldserver=(Get-FileHash $exe).Hash;game_library=(Get-FileHash $gameLib).Hash;native_inputs=(Get-FileHash "$run/native-inputs.json").Hash;cases=$relogs.Count;live=32;old_boot=32;native_route_abi=$abi[0];caller_and_route_calls=$true;condition_rejection=$true} | ConvertTo-Json | Set-Content "$relogOut/inputs.json"
'PASS: canonical scenario login/legacy entrance use the guarded native faction/class route;64 complete SQL logout/login cases preserve live progress or restart old-boot checkpoints at the entrance.'

$reconnectOut="$repo/build-extractors/runtime-test/scenario-reconnect-release"
New-Item -ItemType Directory -Force $reconnectOut | Out-Null
if($inputs.'tools/tests/TrialScenarioReconnectNative.inc' -ne (Get-FileHash "$PSScriptRoot/TrialScenarioReconnectNative.inc").Hash){throw 'Scenario reconnect fixture changed after native execution'}
foreach($path in @('Maps/Map.cpp','Maps/MapInstanced.cpp','Server/WorldSession.cpp','Server/WorldSocket.cpp','World/World.cpp')){if($inputs."src/server/game/$path" -ne (Get-FileHash "$repo/src/server/game/$path").Hash){throw "Scenario reconnect source changed: $path"}}
$reconnects=@($lines | Where-Object {$_ -match '^PASS: native scenario reconnect '})
if($reconnects.Count -ne 32){throw 'Run all32 native EOF/fresh-session reconnect cases first'}
foreach($id in 786,1189){foreach($unload in 0..1){foreach($channel in 0..1){if(@($reconnects | Where-Object {$_ -match " scenario=$id unload=$unload instance_eof=$channel instance="}).Count -ne 4){throw "Missing reconnect faction/instance/channel combination $id/$unload/$channel"}}}}
foreach($name in @('?UpdateSessions@Map@@','?UpdateSessions@World@@')){$calls=@(Calls $name);foreach($target in @('?Update@WorldSession@@','?LogoutPlayer@WorldSession@@')){if($calls -notcontains (Resolve (Symbol $target))){throw "Canonical $name lacks $target"}}}
if(@(Calls '?DestroyInstance@MapInstanced@@') -notcontains (Resolve (Symbol '?RemoveScenario@ScenarioMgr@@'))){throw 'Canonical instance destruction does not remove its Scenario'}
foreach($name in @('?UpdateSessions@Map@@','?UpdateSessions@World@@','?DestroyInstance@MapInstanced@@')){$range=Bounds $name;$argument='/RANGE:0x{0:X},0x{1:X}' -f $range[0],$range[1];$body=@(& $dump.FullName /DISASM:NOBYTES $argument $exe);if($LASTEXITCODE){throw "Reconnect disassembly failed: $name"};$body | Set-Content "$reconnectOut/$($name.Trim('?').Replace('@','-')).txt"}
[ordered]@{worldserver=(Get-FileHash $exe).Hash;game_library=(Get-FileHash $gameLib).Hash;native_inputs=(Get-FileHash "$run/native-inputs.json").Hash;cases=$reconnects.Count;live=16;destroyed_instance=16;native_map_and_world_cleanup=$true;native_scenario_destruction=$true} | ConvertTo-Json | Set-Content "$reconnectOut/inputs.json"
'PASS: canonical map/world session cycles retain native update/logout and instance destruction removes Scenario;32 real TCP EOF/fresh-session reconnect cases cover live and destroyed instances for both factions.'
