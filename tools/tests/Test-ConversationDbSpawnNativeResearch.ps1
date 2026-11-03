param()
$ErrorActionPreference='Stop';$repo=[IO.Path]::GetFullPath("$PSScriptRoot/../..")
$out="$repo/build-extractors/runtime-test/conversation-dbspawn-research/$(Get-Date -Format yyyyMMdd-HHmmss-fff)"
New-Item -ItemType Directory -Force "$out/logs"|Out-Null
$config=Get-Content "$repo/build-extractors/bin/Release/worldserver.conf" -Raw;$originalHash=(Get-FileHash "$repo/build-extractors/bin/Release/worldserver.conf").Hash
$config=[regex]::Replace($config,'(?m)^LogsDir\s*=.*$',('LogsDir = "'+$out.Replace('\','/')+'/logs"'));[IO.File]::WriteAllText("$out/worldserver.conf",$config)
function Block([string]$s,[string]$start){$a=$s.IndexOf($start);if($a -lt 0){throw 'Missing native grid helper'};$b=$s.IndexOf('{',$a)+1;$depth=1;while($depth -and $b -lt $s.Length){$depth+=([int]($s[$b] -eq '{'))-([int]($s[$b] -eq '}'));++$b};if($depth){throw 'Unclosed grid helper'};$s.Substring($a,$b-$a)}
$gridSource="$repo/src/server/game/Grids/ObjectGridLoader.cpp"
[IO.File]::WriteAllText("$out/NativeConversationGrid.inc",'template<class T> '+(Block (Get-Content $gridSource -Raw) 'void AddObjectHelper('))
$inputs=@($gridSource,"$PSScriptRoot/ConversationDbSpawnNativeResearch.cpp","$out/NativeConversationGrid.inc","$repo/src/server/game/Entities/Conversation/Conversation.cpp","$repo/build-extractors/src/server/game/Release/game.lib")
$hashes=@{};foreach($inputPath in $inputs){$hashes[$inputPath]=(Get-FileHash -LiteralPath $inputPath).Hash}
$previous=$env:LEGION_DIALOGUE_CONFIG;$previousCompiler=$env:CL
try{
 $env:LEGION_DIALOGUE_CONFIG="$out/worldserver.conf";$env:CL="$previousCompiler /DNDEBUG /I`"$out`" /I`"$repo/src/server/scripts/Legion`""
 Push-Location "$repo/build-extractors/bin/Release"
 try{
  & "$PSScriptRoot/Invoke-WithRuntimeDiagnostics.ps1" -EvidenceDirectory "$out/evidence" -RuntimeDirectories @($out,"$repo/build-extractors/runtime-test/legion-legendary-catalog") -Test {
   & "$repo/tools/research/Read-LegionLegendaryCatalog.ps1" -SourceFile ../tests/ConversationDbSpawnNativeResearch.cpp -ProbeName ConversationDbSpawnNativeResearch -OutputFile "$out/native.tsv" *> "$out/catalog.log"
   if(!$? -or $LASTEXITCODE){throw "Native DB-spawn research failed: $out/catalog.log"}
  }
 }finally{Pop-Location}
}finally{$env:LEGION_DIALOGUE_CONFIG=$previous;$env:CL=$previousCompiler}
if((Get-FileHash "$repo/build-extractors/bin/Release/worldserver.conf").Hash -ne $originalHash){throw 'Canonical configuration changed'}
if((Test-Path "$out/logs/DBErrors.log") -and (Get-Item "$out/logs/DBErrors.log").Length){throw 'Native DBErrors require investigation'}
foreach($inputPath in $inputs){if((Get-FileHash -LiteralPath $inputPath).Hash -ne $hashes[$inputPath]){throw 'Native research input changed during execution'}}
$hashes|ConvertTo-Json|Set-Content "$out/native-inputs.json"
$lines=@(Get-Content "$out/native.tsv")
if(@($lines|Where-Object {$_ -like 'EXPECTED_BROKEN6 *'}).Count -ne 1 -or @($lines|Where-Object {$_ -like 'COUNTERFACTUAL_DBSPAWN*'}).Count -ne 6){throw 'Native six-context completion marker missing; partial observations cannot certify the probe'}
$lines|Where-Object {$_ -match '^(EXPECTED_BROKEN|COUNTERFACTUAL)'}
