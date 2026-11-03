param([switch]$OriginalGarona,[switch]$ShadowGaronaRepair)
if($OriginalGarona -and $ShadowGaronaRepair){throw 'Original counterfactual and repaired shadow must be separate'}
$ErrorActionPreference='Stop';$repo=[IO.Path]::GetFullPath("$PSScriptRoot/../..")
$out="$repo/build-extractors/runtime-test/althea-conversation-native/$(Get-Date -Format yyyyMMdd-HHmmss-fff)"
New-Item -ItemType Directory -Force "$out/logs"|Out-Null
function Block([string]$s,[string]$start){$a=$s.IndexOf($start);if($a -lt 0){throw 'Actual Althea class missing'};$b=$s.IndexOf('{',$a)+1;$depth=1;while($depth){$depth+=([int]($s[$b] -eq '{'))-([int]($s[$b] -eq '}'));++$b};$s.Substring($a,$b-$a)}
[IO.File]::WriteAllText("$out/AltheaActor.inc",(Block (Get-Content "$repo/src/server/scripts/Scenario/Artifacts/Rogue/Kingslayers/Kingslayers.cpp" -Raw) 'class AltheaDeathConversation final : public Conversation')+';')
$config=Get-Content "$repo/build-extractors/bin/Release/worldserver.conf" -Raw;$originalHash=(Get-FileHash "$repo/build-extractors/bin/Release/worldserver.conf").Hash
$config=[regex]::Replace($config,'(?m)^LogsDir\s*=.*$',('LogsDir = "'+$out.Replace('\','/')+'/logs"'));[IO.File]::WriteAllText("$out/worldserver.conf",$config)
$previousOriginal=$env:GARONA_OUTRO_ORIGINAL;$previousShadow=$env:GARONA_OUTRO_SHADOW;
$previous=$env:LEGION_DIALOGUE_CONFIG;$previousCompiler=$env:CL;$previousLink=$env:_LINK_
try{
 $env:GARONA_OUTRO_ORIGINAL=if($OriginalGarona){'1'}else{$null};$env:GARONA_OUTRO_SHADOW=if($ShadowGaronaRepair){'1'}else{$null}
 $env:LEGION_DIALOGUE_CONFIG="$out/worldserver.conf";$env:CL="$previousCompiler /DNDEBUG /Zi /I`"$out`" /I`"$repo/src/server/scripts/Legion`""
 $env:_LINK_="$previousLink /DEBUG /PDB:`"$out/native.pdb`" /MAP:`"$out/native.map`""
 Push-Location "$repo/build-extractors/bin/Release"
 try{
  & "$PSScriptRoot/Invoke-WithRuntimeDiagnostics.ps1" -EvidenceDirectory "$out/evidence" -RuntimeDirectories @($out,"$repo/build-extractors/runtime-test/legion-legendary-catalog") -Test {
   & "$repo/tools/research/Read-LegionLegendaryCatalog.ps1" -SourceFile ../tests/AltheaConversationNative.cpp -ProbeName AltheaConversationNative -OutputFile "$out/native.tsv" *> "$out/catalog.log"
   if(!$? -or $LASTEXITCODE){throw "Native Althea/Garona conversation probe failed: $out/catalog.log"}
  }
 }finally{Pop-Location}
}finally{$env:GARONA_OUTRO_ORIGINAL=$previousOriginal;$env:GARONA_OUTRO_SHADOW=$previousShadow;$env:LEGION_DIALOGUE_CONFIG=$previous;$env:CL=$previousCompiler;$env:_LINK_=$previousLink}
if((Get-FileHash "$repo/build-extractors/bin/Release/worldserver.conf").Hash -ne $originalHash){throw 'Canonical configuration changed'}
if((Test-Path "$out/logs/DBErrors.log") -and (Get-Item "$out/logs/DBErrors.log").Length){throw 'Native DBErrors require investigation'}
Get-Content "$out/native.tsv"|Where-Object {$_ -match '^(EXPECTED_OPEN|COUNTERFACTUAL|PASS)'}
