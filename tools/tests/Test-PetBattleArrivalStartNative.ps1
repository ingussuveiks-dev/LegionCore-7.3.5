param()
$ErrorActionPreference='Stop';$repo=[IO.Path]::GetFullPath("$PSScriptRoot/../..")
$out="$repo/.codex/pet-arrival-start-2026-11-13/native-$(Get-Date -Format yyyyMMdd-HHmmss-fff)";[void][IO.Directory]::CreateDirectory("$out/logs")
$config=Get-Content "$repo/build-extractors/bin/Release/worldserver.conf" -Raw
$config=[regex]::Replace($config,'(?m)^LogsDir\s*=.*$',('LogsDir = "'+$out.Replace('\','/')+'/logs"'))
[IO.File]::WriteAllText("$out/worldserver.conf",$config)
$previous=$env:CL;$previousConfig=$env:LEGION_DIALOGUE_CONFIG
try {
 $env:CL="$previous /DNDEBUG /I`"$repo/src/server/scripts/Legion`" /I`"$out`"";$env:LEGION_DIALOGUE_CONFIG="$out/worldserver.conf"
 & "$PSScriptRoot/Invoke-WithRuntimeDiagnostics.ps1" -EvidenceDirectory "$out/evidence" -RuntimeDirectories @($out,"$repo/build-extractors/runtime-test/legion-legendary-catalog") -Test {
  & "$PSScriptRoot/Prepare-PetBattleOwnedNative.ps1" -Out $out
  $source=Get-Content "$repo/src/server/scripts/World/PetBattle/PetBattlePlayerScript.cpp" -Raw;$a=$source.IndexOf('class PlayerScriptPetBattle :');$b=$source.IndexOf('void AddSC_PetBattlePlayerScript()',$a)
  if($a -lt 0 -or $b -le $a){throw 'Production arrival class missing'}
  [IO.File]::WriteAllText("$out/PetBattleArrivalScript.inc",$source.Substring($a,$b-$a))
  try { & "$repo/tools/research/Read-LegionLegendaryCatalog.ps1" -SourceFile '../tests/PetBattleArrivalStartNative.cpp' -ProbeName 'PetBattleArrivalStartNative' -OutputFile "$out/native.tsv" *> "$out/build.log" }
  catch {Get-Content "$out/build.log" -Tail 18;throw}
  if($LASTEXITCODE -or !(Get-Content "$out/native.tsv" -Raw).Contains('PASS actual native arrival cases=6;')){throw 'Actual native arrival sequence did not complete'}
  Get-Content "$out/native.tsv" -Tail 1
 }
} finally {$env:CL=$previous;$env:LEGION_DIALOGUE_CONFIG=$previousConfig}
$global:LASTEXITCODE=0
