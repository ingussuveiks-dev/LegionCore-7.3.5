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
