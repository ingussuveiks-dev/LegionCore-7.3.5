$ErrorActionPreference='Stop';$env:PYTHONUTF8='1'
$repo=[IO.Path]::GetFullPath("$PSScriptRoot/../..")
$out="$repo/.codex/five-ten-20261113/native-$(Get-Date -Format yyyyMMdd-HHmmss-fff)";[void][IO.Directory]::CreateDirectory("$out/logs")
$config=Get-Content "$repo/build-extractors/bin/Release/worldserver.conf" -Raw
$config=[regex]::Replace($config,'(?m)^LogsDir\s*=.*$',('LogsDir = "'+$out.Replace('\','/')+'/logs"'));[IO.File]::WriteAllText("$out/worldserver.conf",$config)
$previous=$env:CL;$oldConfig=$env:LEGION_DIALOGUE_CONFIG
try{$env:CL="$previous /DNDEBUG /I`"$repo/src/server/scripts/Legion`" /I`"$out`"";$env:LEGION_DIALOGUE_CONFIG="$out/worldserver.conf"
 & "$PSScriptRoot/Invoke-WithRuntimeDiagnostics.ps1" -EvidenceDirectory "$out/evidence" -RuntimeDirectories @($out,"$repo/build-extractors/runtime-test/legion-legendary-catalog") -Test {
  & "$PSScriptRoot/Prepare-PetBattleOwnedNative.ps1" -Out $out
  & C:/Users/Admin/.cache/codex-runtimes/codex-primary-runtime/dependencies/python/python.exe "$PSScriptRoot/prepare_five_ten_20261113.py" $out fixed
  if($LASTEXITCODE){throw 'Whole native filter extraction failed'}
  try{& "$repo/tools/research/Read-LegionLegendaryCatalog.ps1" -SourceFile '../tests/FiveTen20261113FiltersNative.cpp' -ProbeName FiveTen20261113FiltersNative -OutputFile "$out/native.tsv" *> "$out/build.log"}catch{Get-Content "$out/build.log" -Tail 12;throw}
  Get-Content "$out/native.tsv" -Tail 1
 }
}finally{$env:CL=$previous;$env:LEGION_DIALOGUE_CONFIG=$oldConfig}
$global:LASTEXITCODE=0
