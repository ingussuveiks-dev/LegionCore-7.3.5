param([switch]$Baseline)
$ErrorActionPreference='Stop';$repo=[IO.Path]::GetFullPath("$PSScriptRoot/../..")
$out="$repo/build-extractors/runtime-test/legion-challenge-completion-native/$(Get-Date -Format yyyyMMdd-HHmmss-fff)";New-Item -ItemType Directory -Force "$out/logs"|Out-Null
$config=Get-Content "$repo/build-extractors/bin/Release/worldserver.conf" -Raw
$config=[regex]::Replace($config,'(?m)^LogsDir\s*=.*$',('LogsDir = "'+$out.Replace('\','/')+'/logs"'));$config+="`nLogger.spells=2,Server`n";[IO.File]::WriteAllText("$out/worldserver.conf",$config)
$previous=$env:LEGION_DIALOGUE_CONFIG;$compiler=$env:CL;$link=$env:_LINK_
$inputs=@("$repo/src/server/game/Challenge/Challenge.cpp","$repo/src/server/game/Scenarios/Scenario.cpp","$repo/tools/tests/LegionChallengeCompletionNative.cpp","$repo/build-extractors/src/server/scripts/Release/scripts.lib","$repo/build-extractors/src/server/game/Release/game.lib")
$sealed=@(Get-FileHash -LiteralPath $inputs|Select-Object Path,Hash);$sealed|ConvertTo-Json|Set-Content "$out/inputs-before.json"
try{
 $env:LEGION_DIALOGUE_CONFIG="$out/worldserver.conf";$env:CL="$compiler /DNDEBUG /Zi /I`"$out`" /I`"$repo/src/server/scripts/Legion`" /I`"$repo/src/server/scripts/Legion/MawofSouls`""
 if($Baseline){$env:CL+=' /DCHALLENGE_COMPLETION_BASELINE'}
 $source=Get-Content "$repo/src/server/game/Challenge/Challenge.cpp" -Raw
 if(!$source){throw 'Challenge production source unavailable'};[IO.File]::WriteAllText("$out/ChallengeActual.inc",$source)
  foreach($pair in @(@('ScenarioActual.inc','src/server/game/Scenarios/Scenario.cpp'),@('ChallengeMgrActual.inc','src/server/game/Challenge/ChallengeMgr.cpp'),@('CharacterDatabaseActual.inc','src/server/database/Database/Implementation/CharacterDatabase.cpp'))){
  $body=if($Baseline){(& git -C $repo show ('3263742f7:'+$pair[1])) -join "`n"}else{Get-Content "$repo/$($pair[1])" -Raw};if(!$body){throw 'Native completion implementation missing'};[IO.File]::WriteAllText("$out/$($pair[0])",$body)
 }
 $env:_LINK_="$link /DEBUG /PDB:`"$out/native.pdb`" /MAP:`"$out/native.map`""
 Push-Location "$repo/build-extractors/bin/Release"
 try{& "$PSScriptRoot/Invoke-WithRuntimeDiagnostics.ps1" -EvidenceDirectory "$out/evidence" -RuntimeDirectories @($out,"$repo/build-extractors/runtime-test/legion-legendary-catalog") -Test {
  & "$repo/tools/research/Read-LegionLegendaryCatalog.ps1" -SourceFile ../tests/LegionChallengeCompletionNative.cpp -ProbeName LegionChallengeCompletionNative -OutputFile "$out/native.tsv" *> "$out/compiler.log"
  if(!$? -or $LASTEXITCODE){throw "Native challenge verification failed: $out/compiler.log"}
  Get-Content "$out/native.tsv"|Where-Object {$_ -match '^(PASS|ROUTE|COUNTERFACTUAL|CATALOG)'}
 }}finally{Pop-Location}
}finally{$env:LEGION_DIALOGUE_CONFIG=$previous;$env:CL=$compiler;$env:_LINK_=$link;foreach($input in $sealed){if((Get-FileHash -LiteralPath $input.Path).Hash -ne $input.Hash){throw 'Native challenge probe input changed while running'}}}
