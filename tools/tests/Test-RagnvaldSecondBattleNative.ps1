param()
$ErrorActionPreference='Stop';$repo=[IO.Path]::GetFullPath("$PSScriptRoot/../..")
$out="$repo/build-extractors/runtime-test/ragnvald-second-battle-native/$(Get-Date -Format yyyyMMdd-HHmmss-fff)"
New-Item -ItemType Directory -Force "$out/logs"|Out-Null
function Block([string]$s,[string]$start){$a=$s.IndexOf($start);if($a -lt 0){throw "Missing $start"};$b=$s.IndexOf('{',$a)+1;$depth=1;while($depth -and $b -lt $s.Length){$depth+=([int]($s[$b] -eq '{'))-([int]($s[$b] -eq '}'));++$b};if($depth){throw 'Unclosed source'};$s.Substring($a,$b-$a)}
$header=Get-Content "$repo/src/server/scripts/Scenario/Artifacts/Warrior/Intro/ragnvald_second_battle.h" -Raw
[IO.File]::WriteAllText("$out/RagnvaldDiscard.inc",'struct NativeRagnvaldDiscard {'+(Block $header 'static void Discard(')+'};')
$config=Get-Content "$repo/build-extractors/bin/Release/worldserver.conf" -Raw;$originalHash=(Get-FileHash "$repo/build-extractors/bin/Release/worldserver.conf").Hash
$config=[regex]::Replace($config,'(?m)^LogsDir\s*=.*$',('LogsDir = "'+$out.Replace('\','/')+'/logs"'));[IO.File]::WriteAllText("$out/worldserver.conf",$config)
$previous=$env:LEGION_DIALOGUE_CONFIG;$previousCompiler=$env:CL
try{
 $env:LEGION_DIALOGUE_CONFIG="$out/worldserver.conf";$env:CL="$previousCompiler /DNDEBUG /DRAGNVALD_SECOND_BATTLE_NATIVE /I`"$out`" /I`"$repo/src/server/scripts/Legion`""
 Push-Location "$repo/build-extractors/bin/Release"
 try{
  & "$PSScriptRoot/Invoke-WithRuntimeDiagnostics.ps1" -EvidenceDirectory "$out/evidence" -RuntimeDirectories @($out,"$repo/build-extractors/runtime-test/legion-legendary-catalog") -Test {
   & "$repo/tools/research/Read-LegionLegendaryCatalog.ps1" -SourceFile ../tests/WarriorOpeningConversationNative.cpp -ProbeName RagnvaldSecondBattleNative -OutputFile "$out/native.tsv" *> "$out/catalog.log"
   if(!$? -or $LASTEXITCODE){throw "Native1412 execution failed: $out/catalog.log"}
  }
 }finally{Pop-Location}
}finally{$env:LEGION_DIALOGUE_CONFIG=$previous;$env:CL=$previousCompiler}
if((Get-FileHash "$repo/build-extractors/bin/Release/worldserver.conf").Hash -ne $originalHash){throw 'Canonical configuration changed'}
if((Test-Path "$out/logs/DBErrors.log") -and (Get-Item "$out/logs/DBErrors.log").Length){throw 'Native DBErrors require investigation'}
Get-Content "$out/native.tsv"|Where-Object {$_ -match '^PASS'}
