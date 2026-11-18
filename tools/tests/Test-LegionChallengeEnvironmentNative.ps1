# Baseline also requires the original game.lib before the Sanguine repair.
# Its expected broken assertions deliberately reject a repaired core.
param([switch]$Baseline)
$ErrorActionPreference='Stop'
$repo=[IO.Path]::GetFullPath("$PSScriptRoot/../..")
$out="$repo/build-extractors/runtime-test/legion-challenge-environment-native/$(Get-Date -Format yyyyMMdd-HHmmss-fff)"
New-Item -ItemType Directory -Force "$out/logs"|Out-Null
$config=Get-Content "$repo/build-extractors/bin/Release/worldserver.conf" -Raw
$config=[regex]::Replace($config,'(?m)^LogsDir\s*=.*$',('LogsDir = "'+$out.Replace('\','/')+'/logs"'))
$config+="`nLogger.spells=2,Server`n"
[IO.File]::WriteAllText("$out/worldserver.conf",$config)
$previous=$env:LEGION_DIALOGUE_CONFIG;$compiler=$env:CL;$link=$env:_LINK_
$inputs=@("$repo/src/server/game/Entities/AreaTrigger/AreaTrigger.cpp","$repo/src/server/game/Entities/Object/Object.cpp","$repo/tools/tests/LegionChallengeDeathAffixesNative.cpp","$repo/src/server/scripts/World/challenge_scripts.cpp","$repo/tools/tests/LegionChallengeEnvironmentNative.cpp","$repo/tools/tests/LegionChallengeLifecycleNative.cpp","$repo/build-extractors/src/server/scripts/Release/scripts.lib","$repo/build-extractors/src/server/game/Release/game.lib")
$sealed=@(Get-FileHash -LiteralPath $inputs|Select-Object Path,Hash)
$sealed|ConvertTo-Json|Set-Content "$out/inputs-before.json"
try{
 $env:LEGION_DIALOGUE_CONFIG="$out/worldserver.conf"
 $env:CL="$compiler /DNDEBUG /Zi /I`"$out`" /I`"$repo/src/server/scripts/Legion`" /I`"$repo/src/server/scripts/Legion/MawofSouls`""
 if($Baseline){
  $env:CL+=' /DCHALLENGE_DEATH_BASELINE'
  $source=(& git -C $repo show '535731809:src/server/scripts/World/challenge_scripts.cpp') -join "`n"
  if(!$source){throw 'Pinned environmental-affix implementation unavailable'}
  [IO.File]::WriteAllText("$out/ChallengeDeathActual.inc",$source)
 }
 $env:_LINK_="$link /DEBUG /PDB:`"$out/native.pdb`" /MAP:`"$out/native.map`""
 Push-Location "$repo/build-extractors/bin/Release"
 try{
  & "$PSScriptRoot/Invoke-WithRuntimeDiagnostics.ps1" -EvidenceDirectory "$out/evidence" -RuntimeDirectories @($out,"$repo/build-extractors/runtime-test/legion-legendary-catalog") -Test {
   & "$repo/tools/research/Read-LegionLegendaryCatalog.ps1" -SourceFile ../tests/LegionChallengeEnvironmentNative.cpp -ProbeName LegionChallengeEnvironmentNative -OutputFile "$out/native.tsv" *> "$out/compiler.log"
   if(!$? -or $LASTEXITCODE){throw "Native environmental-affix verification failed: $out/compiler.log"}
   Get-Content "$out/native.tsv"|Where-Object {$_ -match '^(META|PASS|DATA|ENV|INSPECT|COUNTERFACTUAL)'}
  }
 }finally{Pop-Location}
}finally{
 $env:LEGION_DIALOGUE_CONFIG=$previous;$env:CL=$compiler;$env:_LINK_=$link
 foreach($input in $sealed){if((Get-FileHash -LiteralPath $input.Path).Hash -ne $input.Hash){throw 'Native environmental-affix input changed while running'}}
}
