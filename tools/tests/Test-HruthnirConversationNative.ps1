param()
$ErrorActionPreference='Stop'
$repo=[IO.Path]::GetFullPath("$PSScriptRoot/../..")
$out="$repo/build-extractors/runtime-test/hruthnir-conversation-native/$(Get-Date -Format yyyyMMdd-HHmmss-fff)"
New-Item -ItemType Directory -Force $out|Out-Null
$source=Get-Content "$repo/src/server/scripts/Scenario/Artifacts/Warrior/EarthWarder/EarthWarder.cpp" -Raw
function Block([string]$text,[string]$start){$first=$text.IndexOf($start);if($first -lt 0){throw "Native Hruthnir block missing: $start"};$last=$text.IndexOf('{',$first)+1;$depth=1;while($depth){if($text[$last] -eq '{'){++$depth};if($text[$last] -eq '}'){--$depth};++$last};$text.Substring($first,$last-$first)}
foreach($entry in @(@('HruthnirCleanup','void RemoveMagnarConversation('),@('HruthnirReset','void Reset()'),@('HruthnirCancel','void CancelMagnarIntro('),@('HruthnirDeath','void JustDied('),@('HruthnirDamage','void DamageTaken('))){[IO.File]::WriteAllText("$out/$($entry[0]).inc",(Block $source $entry[1]))}
$first=$source.IndexOf('        EventMap events;');$last=$source.IndexOf('        void sGossipSelect(',$first)
if($first -lt 0 -or $last -le $first){throw 'Native Hruthnir lifecycle fields missing'}
[IO.File]::WriteAllText("$out/HruthnirFields.inc",$source.Substring($first,$last-$first))
$config=Get-Content "$repo/build-extractors/bin/Release/worldserver.conf" -Raw
$originalHash=(Get-FileHash "$repo/build-extractors/bin/Release/worldserver.conf").Hash
$config=[regex]::Replace($config,'(?m)^LogsDir\s*=.*$',('LogsDir = "'+$out.Replace('\','/')+'"'))
[IO.File]::WriteAllText("$out/worldserver.conf",$config)
$previous=$env:LEGION_DIALOGUE_CONFIG
$previousCompiler=$env:CL
try{
 $env:LEGION_DIALOGUE_CONFIG="$out/worldserver.conf"
 $env:CL="$previousCompiler /DNDEBUG /I`"$repo/src/server/scripts/Legion`" /I`"$out`""
 Push-Location "$repo/build-extractors/bin/Release"
 try{& "$repo/tools/research/Read-LegionLegendaryCatalog.ps1" -SourceFile ../tests/HruthnirConversationNative.cpp -ProbeName HruthnirConversationNative -OutputFile "$out/native.tsv" *> "$out/catalog.log";if(!$? -or $LASTEXITCODE){throw "Native497 execution failed: $out/catalog.log"}}finally{Pop-Location}
}finally{$env:LEGION_DIALOGUE_CONFIG=$previous;$env:CL=$previousCompiler}
if((Get-FileHash "$repo/build-extractors/bin/Release/worldserver.conf").Hash -ne $originalHash){throw 'Canonical configuration changed during native probe'}
if((Test-Path "$out/DBErrors.log") -and (Get-Item "$out/DBErrors.log").Length){throw 'Native497 probe DBErrors require review'}
$evidence=@(Get-Content "$out/native.tsv"|Where-Object {$_ -match '^PASS'})
if($evidence.Count -ne 1 -or !$evidence[0].StartsWith('PASS: actual native497 fails with missing96034')){throw "Native497 success evidence missing: $out/native.tsv"}
$evidence
